#include "YenkaDesktopPawn.h"
#include "YenkaHandAvatar.h"
#include "YenkaVR/Physics/YenkaBlock.h"
#include "YenkaVR/UI/YenkaScenarioMenu.h"
#include "YenkaVR/Environment/YenkaEnvironmentManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "PhysicsEngine/PhysicsHandleComponent.h"

AYenkaDesktopPawn::AYenkaDesktopPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	RootComponent = CameraBoom;
	CameraBoom->TargetArmLength = 65.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	HandAvatarClass = AYenkaHandAvatar::StaticClass();
	HoveredBlock = nullptr;
	GrabbedBlock = nullptr;
	bIsOrbitingCamera = false;
	bIsPokeModeActive = false;
	bIsPushingBlock = false;
	bIsLockedPerpendicular = false;
	LockedRadialDirection = FVector::ForwardVector;
	LockedFloorZ = 90.0f;
	GrabDistance = 65.0f;
	LastHitLocation = FVector::ZeroVector;
	LastHitNormal = FVector::UpVector;
	LastPrimaryClickTime = -10.0f;

	// Hand Calibration defaults (Applied to ALL gestures: Push, Grab, OpenHand)
	GrabHandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
	GrabHandRotationOffset = FRotator(0.0f, -90.0f, 0.0f);

	PokeHandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
	PokeHandRotationOffset = FRotator(0.0f, -90.0f, 0.0f);

	GrabStandbySeparation = 1.0f;
	PokeStandbySeparation = 1.0f;
	ActiveGesturePreview = EHandPoseMode::OpenHand;
	bForceGesturePreview = false;
}

#include "YenkaVR/Physics/YenkaTowerManager.h"

void AYenkaDesktopPawn::BeginPlay()
{
	Super::BeginPlay();

	// Center camera target on the Yenka Tower
	AActor* TowerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass());
	if (TowerActor)
	{
		SetActorLocation(TowerActor->GetActorLocation() + FVector(0.0f, 0.0f, 15.0f));
	}
	else
	{
		SetActorLocation(FVector(0.0f, 0.0f, 105.0f));
	}
	CameraBoom->TargetArmLength = 65.0f;
	CameraBoom->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && IsLocallyControlled())
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		PC->SetControlRotation(FRotator(-20.0f, 0.0f, 0.0f));
	}

	if (IsLocallyControlled() && HandAvatarClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		VirtualHand = GetWorld()->SpawnActor<AYenkaHandAvatar>(HandAvatarClass, FVector(0.0f, 0.0f, -1000.0f), FRotator::ZeroRotator, SpawnParams);
		if (VirtualHand)
		{
			VirtualHand->SetActorHiddenInGame(true);
		}
	}
}

void AYenkaDesktopPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		HandleMouseTrace();
		UpdatePersistentCalibrationHUD();
	}
}

void AYenkaDesktopPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (PlayerInputComponent)
	{
		// Direct Key Bindings
		PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AYenkaDesktopPawn::OnPrimaryClickPressed);
		PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AYenkaDesktopPawn::OnPrimaryClickReleased);
		PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AYenkaDesktopPawn::OnSecondaryClickPressed);
		PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AYenkaDesktopPawn::OnSecondaryClickReleased);
		PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AYenkaDesktopPawn::OnPokeKeyPressed);
		PlayerInputComponent->BindKey(EKeys::E, IE_Released, this, &AYenkaDesktopPawn::OnPokeKeyReleased);
		PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AYenkaDesktopPawn::OnTogglePokeMode);
		PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &AYenkaDesktopPawn::OnToggleScenarioMenu);

		// Gesture Switching Keys
		PlayerInputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AYenkaDesktopPawn::SetGesturePush);
		PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &AYenkaDesktopPawn::SetGesturePush);
		PlayerInputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AYenkaDesktopPawn::SetGestureGrab);
		PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AYenkaDesktopPawn::SetGestureGrab);
		PlayerInputComponent->BindKey(EKeys::F3, IE_Pressed, this, &AYenkaDesktopPawn::SetGestureOpen);
		PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &AYenkaDesktopPawn::SetGestureOpen);
		PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AYenkaDesktopPawn::CycleGesture);

		// Hand Calibration Hotkeys (Live in-game tuning via NumPad, Arrows and Letters)
		PlayerInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYawMinus);
		PlayerInputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYawPlus);
		PlayerInputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibPitchPlus);
		PlayerInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibPitchMinus);
		PlayerInputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibRollMinus);
		PlayerInputComponent->BindKey(EKeys::NumPadNine, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibRollPlus);
		PlayerInputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotateYaw90);
		PlayerInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotateRoll90);
		PlayerInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotatePitch90);
		PlayerInputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &AYenkaDesktopPawn::ResetHandCalibration);
		PlayerInputComponent->BindKey(EKeys::Add, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZPlus);
		PlayerInputComponent->BindKey(EKeys::Subtract, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZMinus);
		PlayerInputComponent->BindKey(EKeys::Multiply, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXPlus);
		PlayerInputComponent->BindKey(EKeys::Divide, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXMinus);
		PlayerInputComponent->BindKey(EKeys::Decimal, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYMinus);

		// Arrow and Navigation Key Bindings for Position
		PlayerInputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXPlus);
		PlayerInputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXMinus);
		PlayerInputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYMinus);
		PlayerInputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYPlus);
		PlayerInputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZPlus);
		PlayerInputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZMinus);

		// Alternative Letter Key Bindings
		PlayerInputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYawPlus);
		PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYawMinus);
		PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibPitchPlus);
		PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibPitchMinus);
		PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibRollMinus);
		PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibRollPlus);
		PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotateYaw90);
		PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotateRoll90);
		PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXPlus);
		PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXMinus);
		PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYMinus);
		PlayerInputComponent->BindKey(EKeys::L, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYPlus);
		PlayerInputComponent->BindKey(EKeys::U, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZPlus);
		PlayerInputComponent->BindKey(EKeys::O, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZMinus);
		PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AYenkaDesktopPawn::ResetHandCalibration);

		// Scenario Theme Hotkeys (1 to 7)
		PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario1);
		PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario2);
		PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario3);
		PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario4);
		PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario5);
		PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario6);
		PlayerInputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario7);

		PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AYenkaDesktopPawn::OnMouseX);
		PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AYenkaDesktopPawn::OnMouseY);
		PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &AYenkaDesktopPawn::OnMouseWheel);
		PlayerInputComponent->BindAxisKey(EKeys::W, this, &AYenkaDesktopPawn::MoveForward);
		PlayerInputComponent->BindAxisKey(EKeys::S, this, &AYenkaDesktopPawn::MoveForward);
		PlayerInputComponent->BindAxisKey(EKeys::D, this, &AYenkaDesktopPawn::MoveRight);
		PlayerInputComponent->BindAxisKey(EKeys::A, this, &AYenkaDesktopPawn::MoveRight);
	}
}

void AYenkaDesktopPawn::SelectScenarioTheme(int32 ThemeIndex)
{
	AYenkaScenarioMenu* Menu = Cast<AYenkaScenarioMenu>(UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaScenarioMenu::StaticClass()));
	if (Menu)
	{
		Menu->SelectThemeByIndex(ThemeIndex);
	}
	else
	{
		AYenkaEnvironmentManager* EnvMgr = Cast<AYenkaEnvironmentManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaEnvironmentManager::StaticClass()));
		if (EnvMgr)
		{
			EnvMgr->ApplyEnvironmentTheme(static_cast<EYenkaEnvironmentTheme>(ThemeIndex));
		}
	}
}

void AYenkaDesktopPawn::OnToggleScenarioMenu()
{
	AYenkaScenarioMenu* Menu = Cast<AYenkaScenarioMenu>(UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaScenarioMenu::StaticClass()));
	if (Menu)
	{
		Menu->ToggleMenuVisibility();
	}
}

void AYenkaDesktopPawn::AdjustHandOffsetX(float Delta)
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandLocationOffset.X += Delta;
	}
	else
	{
		GrabHandLocationOffset.X += Delta;
	}
}

void AYenkaDesktopPawn::AdjustHandOffsetY(float Delta)
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandLocationOffset.Y += Delta;
	}
	else
	{
		GrabHandLocationOffset.Y += Delta;
	}
}

void AYenkaDesktopPawn::AdjustHandOffsetZ(float Delta)
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandLocationOffset.Z += Delta;
	}
	else
	{
		GrabHandLocationOffset.Z += Delta;
	}
}

void AYenkaDesktopPawn::AdjustHandPitch(float Delta)
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandRotationOffset.Pitch = FRotator::NormalizeAxis(PokeHandRotationOffset.Pitch + Delta);
	}
	else
	{
		GrabHandRotationOffset.Pitch = FRotator::NormalizeAxis(GrabHandRotationOffset.Pitch + Delta);
	}
}

void AYenkaDesktopPawn::AdjustHandYaw(float Delta)
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandRotationOffset.Yaw = FRotator::NormalizeAxis(PokeHandRotationOffset.Yaw + Delta);
	}
	else
	{
		GrabHandRotationOffset.Yaw = FRotator::NormalizeAxis(GrabHandRotationOffset.Yaw + Delta);
	}
}

void AYenkaDesktopPawn::AdjustHandRoll(float Delta)
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandRotationOffset.Roll = FRotator::NormalizeAxis(PokeHandRotationOffset.Roll + Delta);
	}
	else
	{
		GrabHandRotationOffset.Roll = FRotator::NormalizeAxis(GrabHandRotationOffset.Roll + Delta);
	}
}

void AYenkaDesktopPawn::QuickRotateYaw90()
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandRotationOffset.Yaw = FRotator::NormalizeAxis(PokeHandRotationOffset.Yaw + 90.0f);
	}
	else
	{
		GrabHandRotationOffset.Yaw = FRotator::NormalizeAxis(GrabHandRotationOffset.Yaw + 90.0f);
	}
}

void AYenkaDesktopPawn::QuickRotateRoll90()
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandRotationOffset.Roll = FRotator::NormalizeAxis(PokeHandRotationOffset.Roll + 90.0f);
	}
	else
	{
		GrabHandRotationOffset.Roll = FRotator::NormalizeAxis(GrabHandRotationOffset.Roll + 90.0f);
	}
}

void AYenkaDesktopPawn::QuickRotatePitch90()
{
	if (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
	{
		PokeHandRotationOffset.Pitch = FRotator::NormalizeAxis(PokeHandRotationOffset.Pitch + 90.0f);
	}
	else
	{
		GrabHandRotationOffset.Pitch = FRotator::NormalizeAxis(GrabHandRotationOffset.Pitch + 90.0f);
	}
}

void AYenkaDesktopPawn::ResetHandCalibration()
{
	GrabHandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
	GrabHandRotationOffset = FRotator(0.0f, -90.0f, 0.0f);
	PokeHandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
	PokeHandRotationOffset = FRotator(0.0f, -90.0f, 0.0f);
	bForceGesturePreview = false;
	ActiveGesturePreview = EHandPoseMode::OpenHand;
}

void AYenkaDesktopPawn::SetGesturePush()
{
	ActiveGesturePreview = EHandPoseMode::FingerPoke;
	bForceGesturePreview = true;
	bIsPokeModeActive = true;
	if (VirtualHand) VirtualHand->SetHandPoseMode(EHandPoseMode::FingerPoke);
}

void AYenkaDesktopPawn::SetGestureGrab()
{
	ActiveGesturePreview = EHandPoseMode::GrabPinch;
	bForceGesturePreview = true;
	bIsPokeModeActive = false;
	if (VirtualHand) VirtualHand->SetHandPoseMode(EHandPoseMode::GrabPinch);
}

void AYenkaDesktopPawn::SetGestureOpen()
{
	ActiveGesturePreview = EHandPoseMode::OpenHand;
	bForceGesturePreview = true;
	bIsPokeModeActive = false;
	if (VirtualHand) VirtualHand->SetHandPoseMode(EHandPoseMode::OpenHand);
}

void AYenkaDesktopPawn::CycleGesture()
{
	bForceGesturePreview = true;
	if (ActiveGesturePreview == EHandPoseMode::OpenHand)
	{
		ActiveGesturePreview = EHandPoseMode::GrabPinch;
		bIsPokeModeActive = false;
	}
	else if (ActiveGesturePreview == EHandPoseMode::GrabPinch)
	{
		ActiveGesturePreview = EHandPoseMode::FingerPoke;
		bIsPokeModeActive = true;
	}
	else
	{
		ActiveGesturePreview = EHandPoseMode::OpenHand;
		bIsPokeModeActive = false;
	}

	if (VirtualHand)
	{
		VirtualHand->SetHandPoseMode(ActiveGesturePreview);
	}
}

void AYenkaDesktopPawn::UpdatePersistentCalibrationHUD()
{
	if (GEngine && IsLocallyControlled())
	{
		const bool bInPoke = (bIsPokeModeActive || bIsPushingBlock || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
		const FVector ActivePos = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;
		const FRotator ActiveRot = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;

		FString GestureName = TEXT("🖐️ LIBRE / INSPECCIÓN (OpenHand)");
		if (bForceGesturePreview)
		{
			if (ActiveGesturePreview == EHandPoseMode::FingerPoke) GestureName = TEXT("👉 EMPUJAR FORZADO (FingerPoke)");
			else if (ActiveGesturePreview == EHandPoseMode::GrabPinch) GestureName = TEXT("🤏 AGARRAR FORZADO (GrabPinch)");
			else GestureName = TEXT("🖐️ MANO LIBRE FORZADA (OpenHand)");
		}
		else if (bIsPokeModeActive || bIsPushingBlock)
		{
			GestureName = TEXT("👉 EMPUJAR ACTIVO (FingerPoke)");
		}
		else if (GrabbedBlock)
		{
			GestureName = TEXT("🤏 AGARRANDO PIEZA (GrabPinch)");
		}

		FString Line1 = FString::Printf(TEXT("=== 🎛️ CALIBRACIÓN DE MANO YENKA (TIEMPO REAL) ==="));
		FString Line2 = FString::Printf(TEXT("📍 POSICIÓN [%s]:  [ X: %+.2f cm | Y: %+.2f cm | Z: %+.2f cm ]"),
			bInPoke ? TEXT("EMPUJAR") : TEXT("AGARRAR/LIBRE"), ActivePos.X, ActivePos.Y, ActivePos.Z);
		FString Line3 = FString::Printf(TEXT("🔄 ROTACIÓN [%s]:  [ Yaw: %+.0f° | Pitch: %+.0f° | Roll: %+.0f° ]"),
			bInPoke ? TEXT("EMPUJAR") : TEXT("AGARRAR/LIBRE"), ActiveRot.Yaw, ActiveRot.Pitch, ActiveRot.Roll);
		FString Line4 = FString::Printf(TEXT("✋ GESTO:     [ %s ]  (Teclas: F1/P Empujar, F2/G Agarrar, F3/V Libre, Tab Ciclar)"),
			*GestureName);
		FString Line5 = FString::Printf(TEXT("⌨️ POSICIÓN:  I/K o Flechas Arriba/Abajo(X) | J/L o Flechas Izq/Der(Y) | U/O o RePág/AvPág(Z) | NumPad / * + - ."));
		FString Line6 = FString::Printf(TEXT("⌨️ ROTACIÓN:  NumPad 4/6 o Y/H(Yaw) | 8/2 o T/G(Pitch) | 7/9 o B/N(Roll) | Num 5 o X(+90° Yaw) | Num 1 o Z(+90° Roll) | Num 0 o R(Reset)"));

		GEngine->AddOnScreenDebugMessage(1001, 0.20f, FColor(255, 215, 0), Line1);
		GEngine->AddOnScreenDebugMessage(1002, 0.20f, FColor(0, 240, 255), Line2);
		GEngine->AddOnScreenDebugMessage(1003, 0.20f, FColor(50, 255, 120), Line3);
		GEngine->AddOnScreenDebugMessage(1004, 0.20f, FColor(255, 140, 0), Line4);
		GEngine->AddOnScreenDebugMessage(1005, 0.20f, FColor(220, 220, 220), Line5);
		GEngine->AddOnScreenDebugMessage(1006, 0.20f, FColor(180, 200, 255), Line6);
	}
}

void AYenkaDesktopPawn::OnSecondaryClickPressed()
{
	bIsOrbitingCamera = true;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = false;
	}
}

void AYenkaDesktopPawn::OnSecondaryClickReleased()
{
	bIsOrbitingCamera = false;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = true;
	}
}

void AYenkaDesktopPawn::OnPokeKeyPressed()
{
	bIsPokeModeActive = true;
}

void AYenkaDesktopPawn::OnPokeKeyReleased()
{
	if (!bIsPushingBlock)
	{
		bIsPokeModeActive = false;
	}
}

void AYenkaDesktopPawn::OnTogglePokeMode()
{
	bIsPokeModeActive = !bIsPokeModeActive;
}

void AYenkaDesktopPawn::PerformLongitudinalPush(AYenkaBlock* Block, const FVector& DirectionNormal)
{
	if (!Block || !Block->BlockMesh) return;

	Block->BlockMesh->WakeRigidBody();
	FVector ForwardVec = Block->GetActorForwardVector();
	float Dot = FVector::DotProduct(DirectionNormal, ForwardVec);
	FVector PushDir = (Dot >= 0.0f) ? ForwardVec : -ForwardVec;

	FVector PushVel = PushDir * 18.0f;
	PushVel.Z = 0.0f;
	Block->BlockMesh->SetPhysicsLinearVelocity(PushVel);
}

FRotator AYenkaDesktopPawn::GetHorizontalFacingRotation(const FVector& TargetLocation) const
{
	FVector CamLoc = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
	FVector Dir = (TargetLocation - CamLoc).GetSafeNormal2D();
	return Dir.Rotation();
}

void AYenkaDesktopPawn::OnMouseX(float Val)
{
	if (bIsOrbitingCamera && FMath::Abs(Val) > 0.001f)
	{
		AddControllerYawInput(Val * 2.5f);
	}
}

void AYenkaDesktopPawn::OnMouseY(float Val)
{
	if (bIsOrbitingCamera && FMath::Abs(Val) > 0.001f)
	{
		AddControllerPitchInput(-Val * 2.5f);
	}
	else if (bIsPokeModeActive && FMath::Abs(Val) > 0.001f)
	{
		// Moving mouse forward (+Val) advances finger towards block and pushes; moving backward (-Val) retracts
		CurrentPushAdvance = FMath::Clamp(CurrentPushAdvance + (Val * 0.15f), 0.0f, PokeStandbySeparation + 0.5f);

		if (Val > 0.01f && CurrentPushAdvance >= PokeStandbySeparation)
		{
			AYenkaBlock* ActivePushBlock = LockedPushBlock ? LockedPushBlock : HoveredBlock;
			if (ActivePushBlock && ActivePushBlock->BlockMesh)
			{
				ActivePushBlock->BlockMesh->WakeRigidBody();
				float TargetSpeed = FMath::Clamp(Val * 24.0f, 7.5f, 24.0f);
				FVector PushVel = PushLongitudinalAxis * TargetSpeed;
				PushVel.Z = 0.0f;
				ActivePushBlock->BlockMesh->SetPhysicsLinearVelocity(PushVel);
				ActivePushBlock->BlockMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}
		}
	}
}

void AYenkaDesktopPawn::OnMouseWheel(float Val)
{
	if (CameraBoom && FMath::Abs(Val) > 0.001f)
	{
		CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - (Val * 8.0f), 30.0f, 250.0f);
	}
}

void AYenkaDesktopPawn::MoveForward(float Val)
{
	if (FMath::Abs(Val) > 0.01f)
	{
		FVector Forward = FollowCamera ? FollowCamera->GetForwardVector() : GetActorForwardVector();
		Forward.Z = 0.0f;
		AddActorWorldOffset(Forward.GetSafeNormal() * (Val * 2.0f));
	}
}

void AYenkaDesktopPawn::MoveRight(float Val)
{
	if (FMath::Abs(Val) > 0.01f)
	{
		FVector Right = FollowCamera ? FollowCamera->GetRightVector() : GetActorRightVector();
		Right.Z = 0.0f;
		AddActorWorldOffset(Right.GetSafeNormal() * (Val * 2.0f));
	}
}

FVector AYenkaDesktopPawn::GetBlockStandOffLocation(const AYenkaBlock* Block, const FVector& ViewOrigin, FVector& OutApproachNormal, float Clearance, const FVector& LocalFingertipOffset) const
{
	if (!Block) return ViewOrigin;

	FVector BlockCenter = Block->GetActorLocation();
	FVector ForwardVec = Block->GetActorForwardVector(); // 7.5cm length (+- 3.75cm)

	FVector EndPosPos = BlockCenter + (ForwardVec * 3.75f);
	FVector EndNegPos = BlockCenter - (ForwardVec * 3.75f);

	float DistToPos = FVector::DistSquared(ViewOrigin, EndPosPos);
	float DistToNeg = FVector::DistSquared(ViewOrigin, EndNegPos);

	FVector ChosenFacePos;
	if (DistToPos < DistToNeg)
	{
		OutApproachNormal = ForwardVec;
		ChosenFacePos = EndPosPos;
	}
	else
	{
		OutApproachNormal = -ForwardVec;
		ChosenFacePos = EndNegPos;
	}

	return ChosenFacePos + (OutApproachNormal * Clearance);
}

FVector AYenkaDesktopPawn::GetBlockChosenFacePos(const AYenkaBlock* Block, const FVector& ApproachNormal) const
{
	if (!Block) return FVector::ZeroVector;

	FVector BlockCenter = Block->GetActorLocation();
	FVector ForwardVec = Block->GetActorForwardVector();

	float Dot = FVector::DotProduct(ApproachNormal, ForwardVec);
	FVector FaceNormal = (Dot >= 0.0f) ? ForwardVec : -ForwardVec;

	return BlockCenter + (FaceNormal * 3.75f);
}

bool AYenkaDesktopPawn::IsBlockProtruding(const AYenkaBlock* Block, FVector& OutProtrudingEdgePos, FVector& OutProtrudingNormal) const
{
	if (!Block) return false;

	AActor* TowerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass());
	FVector TowerCenter = TowerActor ? TowerActor->GetActorLocation() : FVector(0.0f, 0.0f, 90.0f);
	FVector BlockCenter = Block->GetActorLocation();

	FVector ForwardVec = Block->GetActorForwardVector(); // 7.5cm length (+- 3.75cm)

	// The two longitudinal end faces of the block
	FVector EndPos = BlockCenter + (ForwardVec * 3.75f);
	FVector EndNeg = BlockCenter - (ForwardVec * 3.75f);

	// Distance of the ends from the tower center on the XY plane
	float DistEndPosToTower = FVector::Dist2D(EndPos, TowerCenter);
	float DistEndNegToTower = FVector::Dist2D(EndNeg, TowerCenter);

	// Flush tower outer boundary is ~3.75cm from center.
	// A block is considered protruding if its end extends at least 0.4cm (4mm) past the flush tower boundary.
	const float PROTRUSION_THRESHOLD = 4.15f; // 3.75cm + 0.4cm

	if (DistEndPosToTower > PROTRUSION_THRESHOLD && DistEndPosToTower >= DistEndNegToTower)
	{
		OutProtrudingEdgePos = EndPos;
		OutProtrudingNormal = ForwardVec;
		return true;
	}
	else if (DistEndNegToTower > PROTRUSION_THRESHOLD)
	{
		OutProtrudingEdgePos = EndNeg;
		OutProtrudingNormal = -ForwardVec;
		return true;
	}

	return false;
}

void AYenkaDesktopPawn::HandleMouseTrace()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	AActor* TowerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass());
	FVector TowerCenter = TowerActor ? TowerActor->GetActorLocation() : FVector(0.0f, 0.0f, 90.0f);
	const float ProximityRadius = TOWER_BASE_RADIUS + PROXIMITY_THRESHOLD; // 3.75 + 22.0 = 25.75 cm

	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		// 1. If currently dragging a block with Physics Handle
		if (GrabbedBlock && VirtualHand && VirtualHand->PhysicsHandle)
		{
			FVector DragTargetLocation = WorldLocation + (WorldDirection * GrabDistance);
			VirtualHand->PhysicsHandle->SetTargetLocation(DragTargetLocation);
			FRotator HandRot = GetHorizontalFacingRotation(DragTargetLocation) + GrabHandRotationOffset;
			FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + GrabHandLocationOffset;
			FVector HandPos = DragTargetLocation - HandRot.RotateVector(LocalOffset);
			FTransform HandTarget(HandRot.Quaternion(), HandPos);
			VirtualHand->SetHandPoseMode(EHandPoseMode::GrabPinch);
			VirtualHand->SetTargetHandTransform(HandTarget, 1.0f);
			return;
		}

		// 2. Normal cursor hovering trace
		FHitResult HitResult;
		FVector TraceEnd = WorldLocation + (WorldDirection * 500.0f);
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		if (VirtualHand) QueryParams.AddIgnoredActor(VirtualHand);

		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams);
		if (bHit)
		{
			if (AYenkaScenarioMenu* ScenarioMenu = Cast<AYenkaScenarioMenu>(HitResult.GetActor()))
			{
				ScenarioMenu->ProcessRayHit(HitResult, false);
			}

			AYenkaBlock* HitBlock = Cast<AYenkaBlock>(HitResult.GetActor());
			HoveredBlock = HitBlock;
			LastHitLocation = HitResult.ImpactPoint;
			LastHitNormal = HitResult.ImpactNormal;
		}
		else if (!LockedPushBlock)
		{
			HoveredBlock = nullptr;
		}

		if (VirtualHand)
		{
			VirtualHand->SetActorHiddenInGame(false);

			if (bIsPokeModeActive || bIsPushingBlock || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke))
			{
				if (HoveredBlock && !LockedPushBlock)
				{
					LockedPushBlock = HoveredBlock;
					PushBlockInitialLocation = LockedPushBlock->GetActorLocation();
					CurrentPushDisplacement = 0.0f;
				}

				AYenkaBlock* ActivePushBlock = LockedPushBlock ? LockedPushBlock : HoveredBlock;

				if (ActivePushBlock)
				{
					// --- PUSH / POKE MODE ---
					VirtualHand->SetHandPoseMode(EHandPoseMode::FingerPoke);
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + PokeHandLocationOffset;

					FVector ApproachNormal;
					GetBlockStandOffLocation(ActivePushBlock, WorldLocation, ApproachNormal, PokeStandbySeparation, LocalOffset);

					// Calculate longitudinal push axis (facing inward towards tower)
					FVector ForwardVec = ActivePushBlock->GetActorForwardVector();
					FVector RightVec = ActivePushBlock->GetActorRightVector();
					float DotFwd = FVector::DotProduct(-ApproachNormal, ForwardVec);
					float DotRt = FVector::DotProduct(-ApproachNormal, RightVec);
					PushLongitudinalAxis = (FMath::Abs(DotFwd) >= FMath::Abs(DotRt)) ? (ForwardVec * FMath::Sign(DotFwd)) : (RightVec * FMath::Sign(DotRt));
					PushApproachNormal = ApproachNormal;

					FRotator BaseRot = (-ApproachNormal).Rotation();
					BaseRot.Pitch = 0.0f;
					BaseRot.Roll = 0.0f;
					FRotator HandRot = BaseRot + PokeHandRotationOffset;

					LockedRadialDirection = ApproachNormal;
					bIsLockedPerpendicular = true;

					// Find block face location in world space
					FVector CurrentFacePos = GetBlockChosenFacePos(ActivePushBlock, ApproachNormal);

					if (bIsPushingBlock && ActivePushBlock->BlockMesh)
					{
						CurrentPushAdvance = PokeStandbySeparation;
						ActivePushBlock->BlockMesh->WakeRigidBody();
						// High-power active push velocity while mouse button is held down (18.0 cm/s)
						FVector PushVel = PushLongitudinalAxis * 18.0f;
						PushVel.Z = 0.0f;
						ActivePushBlock->BlockMesh->SetPhysicsLinearVelocity(PushVel);
						ActivePushBlock->BlockMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
					}

					float EffectiveAdvance = CurrentPushAdvance;

					if (EffectiveAdvance < PokeStandbySeparation)
					{
						// In the air (Standby to Contact): Hand moves freely towards the block surface
						float Clearance = PokeStandbySeparation - EffectiveAdvance;
						FVector TargetFingertipWorld = CurrentFacePos + (ApproachNormal * Clearance);
						FVector HandPos = TargetFingertipWorld - HandRot.RotateVector(LocalOffset);
						VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), HandPos), 0.0f);
					}
					else
					{
						// In Contact & Pushing: Fingertip stays EXACTLY ON THE SURFACE
						FVector UpdatedFacePos = GetBlockChosenFacePos(ActivePushBlock, ApproachNormal);
						FVector HandPos = UpdatedFacePos - HandRot.RotateVector(LocalOffset);
						VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), HandPos), 0.0f);
					}
				}
			}
			else if (bHit)
			{
				const float DistToTowerXY = FVector::Dist2D(HitResult.ImpactPoint, TowerCenter);
				const bool bInProximity = (DistToTowerXY <= ProximityRadius);

				if (bInProximity && HoveredBlock)
				{
					const float BlockZ = HoveredBlock->GetActorLocation().Z;
					FVector ProtrudingPos, ProtrudingNorm;
					bool bIsProtruding = IsBlockProtruding(HoveredBlock, ProtrudingPos, ProtrudingNorm);

					if (bIsProtruding)
					{
						// Block is protruding: position pinch fingers at standby separation from protruding edge
						bIsLockedPerpendicular = false;
						VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : EHandPoseMode::GrabPinch);
						FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + GrabHandLocationOffset;
						FVector StandbyFingertipPos = ProtrudingPos + (ProtrudingNorm * GrabStandbySeparation);
						StandbyFingertipPos.Z = BlockZ;

						FRotator BaseRot = (-ProtrudingNorm).Rotation();
						BaseRot.Pitch = 0.0f;
						BaseRot.Roll = 0.0f;
						FRotator HandRot = BaseRot + GrabHandRotationOffset;

						FVector SafeHandPos = StandbyFingertipPos - HandRot.RotateVector(LocalOffset);
						VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), SafeHandPos), 0.0f);
					}
					else
					{
						// Block is flush/not protruding: hand hovers outside in inspection mode (fingertips strictly outside tower at block height)
						bIsLockedPerpendicular = false;
						VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : EHandPoseMode::OpenHand);
						FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + GrabHandLocationOffset;

						FVector Diff = HitResult.ImpactPoint - TowerCenter;
						float Angle = FMath::Atan2(Diff.Y, Diff.X);
						float SafeFingertipRadius = FMath::Max(FVector2D(Diff.X, Diff.Y).Size(), TOWER_BASE_RADIUS + 1.0f);
						FVector TargetFingertipPos = FVector(TowerCenter.X + FMath::Cos(Angle) * SafeFingertipRadius, TowerCenter.Y + FMath::Sin(Angle) * SafeFingertipRadius, BlockZ);
						FRotator BaseRot = GetHorizontalFacingRotation(TargetFingertipPos);
						BaseRot.Pitch = 0.0f;
						BaseRot.Roll = 0.0f;
						FRotator InspectRot = BaseRot + GrabHandRotationOffset;

						FVector SafeHandPos = TargetFingertipPos - InspectRot.RotateVector(LocalOffset);
						VirtualHand->SetTargetHandTransform(FTransform(InspectRot.Quaternion(), SafeHandPos), 0.0f);
					}
				}
				else if (bInProximity)
				{
					// --- PROXIMITY TO TABLE (No Block Hovered) ---
					bIsLockedPerpendicular = false;
					VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : EHandPoseMode::OpenHand);
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + GrabHandLocationOffset;

					FVector Diff = HitResult.ImpactPoint - TowerCenter;
					float Angle = FMath::Atan2(Diff.Y, Diff.X);
					float SafeFingertipRadius = FMath::Max(FVector2D(Diff.X, Diff.Y).Size(), TOWER_BASE_RADIUS + 1.0f);
					FVector TargetFingertipPos = FVector(TowerCenter.X + FMath::Cos(Angle) * SafeFingertipRadius, TowerCenter.Y + FMath::Sin(Angle) * SafeFingertipRadius, HitResult.ImpactPoint.Z);
					FRotator BaseRot = GetHorizontalFacingRotation(TargetFingertipPos);
					BaseRot.Pitch = 0.0f;
					BaseRot.Roll = 0.0f;
					FRotator InspectRot = BaseRot + GrabHandRotationOffset;

					FVector SafeHandPos = TargetFingertipPos - InspectRot.RotateVector(LocalOffset);
					VirtualHand->SetTargetHandTransform(FTransform(InspectRot.Quaternion(), SafeHandPos), 0.0f);
				}
				else
				{
					// --- OUTSIDE PROXIMITY: Free 3D movement ---
					bIsLockedPerpendicular = false;
					VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : EHandPoseMode::OpenHand);
					FTransform HandTarget(FRotationMatrix::MakeFromX(WorldDirection).ToQuat(), HitResult.ImpactPoint);
					VirtualHand->SetTargetHandTransform(HandTarget, 0.0f);
				}
			}
			else
			{
				bIsLockedPerpendicular = false;
				VirtualHand->SetActorHiddenInGame(true);
			}
		}
	}
}

void AYenkaDesktopPawn::OnPrimaryClickPressed()
{
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	float TimeSinceLastClick = CurrentTime - LastPrimaryClickTime;
	const float DoubleClickMaxInterval = 0.35f;
	bool bIsDoubleClick = (TimeSinceLastClick > 0.03f && TimeSinceLastClick <= DoubleClickMaxInterval);
	LastPrimaryClickTime = CurrentTime;

	// Check if clicking on the 3D Scenario Menu
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		FVector WorldLocation, WorldDirection;
		if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		{
			FHitResult MenuHit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			if (VirtualHand) QueryParams.AddIgnoredActor(VirtualHand);
			if (GetWorld()->LineTraceSingleByChannel(MenuHit, WorldLocation, WorldLocation + (WorldDirection * 500.0f), ECC_Visibility, QueryParams))
			{
				if (AYenkaScenarioMenu* ScenarioMenu = Cast<AYenkaScenarioMenu>(MenuHit.GetActor()))
				{
					ScenarioMenu->ProcessRayHit(MenuHit, true);
					return;
				}
			}
		}
	}

	AYenkaBlock* TargetPush = LockedPushBlock ? LockedPushBlock : HoveredBlock;
	if (bIsPokeModeActive && TargetPush)
	{
		// In poke mode, clicking advances to contact and actively pushes along the locked axis
		bIsPushingBlock = true;
		if (!LockedPushBlock)
		{
			LockedPushBlock = TargetPush;
			PushBlockInitialLocation = LockedPushBlock->GetActorLocation();
			CurrentPushDisplacement = 0.0f;
		}
		CurrentPushAdvance = PUSH_STANDBY_SEPARATION;

		if (TargetPush->BlockMesh)
		{
			TargetPush->BlockMesh->WakeRigidBody();

			if (bIsDoubleClick)
			{
				// Double-click strike ("golpe"): 50% stronger than standard push (1.5 * 18.0 = 27.0 cm/s + instantaneous impulse)
				const float StrikeVelocity = 27.0f;
				FVector StrikeVel = PushLongitudinalAxis * StrikeVelocity;
				StrikeVel.Z = 0.0f;
				TargetPush->BlockMesh->SetPhysicsLinearVelocity(StrikeVel);
				TargetPush->BlockMesh->AddImpulse(PushLongitudinalAxis * (StrikeVelocity * TargetPush->BlockMesh->GetMass() * 1.5f), NAME_None, false);
				TargetPush->BlockMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}
			else
			{
				// Standard push: 18.0 cm/s
				FVector PushVel = PushLongitudinalAxis * 18.0f;
				PushVel.Z = 0.0f;
				TargetPush->BlockMesh->SetPhysicsLinearVelocity(PushVel);
				TargetPush->BlockMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}
		}
		return;
	}

	// In Grab Mode: Only grab if the hovered block is PROTRUDING from the tower!
	if (HoveredBlock && VirtualHand && VirtualHand->PhysicsHandle)
	{
		FVector ProtrudingPos, ProtrudingNorm;
		if (!IsBlockProtruding(HoveredBlock, ProtrudingPos, ProtrudingNorm))
		{
			// Piece is flush/not protruding: DO NOTHING!
			return;
		}

		if (PC)
		{
			FVector WorldLocation, WorldDirection;
			if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
			{
				GrabDistance = FMath::Clamp((ProtrudingPos - WorldLocation).Size(), 20.0f, 250.0f);
			}
		}

		GrabbedBlock = HoveredBlock;
		if (GrabbedBlock && GrabbedBlock->BlockMesh)
		{
			GrabbedBlock->BlockMesh->WakeRigidBody();
		}

		VirtualHand->SetHandPoseMode(EHandPoseMode::GrabPinch);
		VirtualHand->PhysicsHandle->GrabComponentAtLocationWithRotation(
			GrabbedBlock->BlockMesh,
			NAME_None,
			ProtrudingPos,
			GrabbedBlock->GetActorRotation()
		);
		ProtrudingPos.Z = GrabbedBlock->GetActorLocation().Z;
		FRotator BaseRot = (-ProtrudingNorm).Rotation();
		BaseRot.Pitch = 0.0f;
		BaseRot.Roll = 0.0f;
		FRotator HandRot = BaseRot + GrabHandRotationOffset;
		FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + GrabHandLocationOffset;
		FVector HandPos = ProtrudingPos - HandRot.RotateVector(LocalOffset);
		FTransform HandTarget(HandRot.Quaternion(), HandPos);
		VirtualHand->SetTargetHandTransform(HandTarget, 1.0f);
	}
}

void AYenkaDesktopPawn::OnPrimaryClickReleased()
{
	if (bIsPushingBlock)
	{
		AYenkaBlock* ActivePushBlock = LockedPushBlock ? LockedPushBlock : HoveredBlock;
		if (ActivePushBlock && ActivePushBlock->BlockMesh)
		{
			ActivePushBlock->BlockMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		}
	}
	bIsPushingBlock = false;

	if (GrabbedBlock)
	{
		if (VirtualHand && VirtualHand->PhysicsHandle)
		{
			VirtualHand->PhysicsHandle->ReleaseComponent();
		}
		GrabbedBlock = nullptr;
		if (VirtualHand)
		{
			VirtualHand->SetHandPoseMode(bIsPokeModeActive ? EHandPoseMode::FingerPoke : EHandPoseMode::OpenHand);
			FRotator HandRot = GetHorizontalFacingRotation(LastHitLocation);
			FTransform HandTarget(HandRot.Quaternion(), LastHitLocation);
			VirtualHand->SetTargetHandTransform(HandTarget, 0.0f);
		}
	}
}
