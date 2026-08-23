#include "YenkaDesktopPawn.h"
#include "YenkaHandAvatar.h"
#include "YenkaVR/Physics/YenkaBlock.h"
#include "YenkaVR/UI/YenkaScenarioMenu.h"
#include "YenkaVR/Environment/YenkaEnvironmentManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/GameViewportClient.h"

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
	GrabHandRotationOffset = FRotator(90.0f, -90.0f, 0.0f);

	PokeHandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
	PokeHandRotationOffset = FRotator(0.0f, -90.0f, 0.0f);

	GrabStandbySeparation = 1.0f;
	PokeStandbySeparation = 1.0f;
	ActiveGesturePreview = EHandPoseMode::OpenHand;
	bForceGesturePreview = false;
	bIsPhalanxEditMode = false;
	SelectedFinger = 2;  // Index Finger by default
	SelectedPhalanx = 1; // Proximal Phalanx by default
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

	LoadCustomGesturesFromDisk();
	LoadCustomTransformsFromDisk();
}

void AYenkaDesktopPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
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
		PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyQPressed);
		PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyMPressed);
		PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AYenkaDesktopPawn::OnTogglePokeMode);

		// Phalanx Edit Mode Toggle & Custom Gestures / Position Transforms
		PlayerInputComponent->BindKey(EKeys::AnyKey, IE_Pressed, this, &AYenkaDesktopPawn::OnAnyKeyPressed);
		PlayerInputComponent->BindKey(EKeys::F4, IE_Pressed, this, &AYenkaDesktopPawn::TogglePhalanxEditMode);
		PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AYenkaDesktopPawn::OnResetSelectedPhalanx);
		PlayerInputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AYenkaDesktopPawn::StartNamingGesture);
		PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyEnterPressed);
		PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyEscapePressed);
		PlayerInputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyBackspacePressed);
		PlayerInputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyLeftBracketPressed);
		PlayerInputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyRightBracketPressed);
		PlayerInputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyLeftBracketPressed);
		PlayerInputComponent->BindKey(EKeys::F7, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyRightBracketPressed);
		PlayerInputComponent->BindKey(EKeys::F8, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyF8Pressed);
		PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyF9Pressed);
		PlayerInputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyF10Pressed);
		PlayerInputComponent->BindKey(EKeys::Semicolon, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyF9Pressed);
		PlayerInputComponent->BindKey(EKeys::Quote, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyF10Pressed);
		PlayerInputComponent->BindKey(EKeys::Delete, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyDeletePressed);

		// Gesture Switching Keys
		PlayerInputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AYenkaDesktopPawn::SetGesturePush);
		PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &AYenkaDesktopPawn::SetGesturePush);
		PlayerInputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AYenkaDesktopPawn::SetGestureGrab);
		PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AYenkaDesktopPawn::SetGestureGrab);
		PlayerInputComponent->BindKey(EKeys::F3, IE_Pressed, this, &AYenkaDesktopPawn::SetGestureOpen);
		PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyVPressed);
		PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AYenkaDesktopPawn::CycleGesture);

		// Hand Calibration & Phalanx Hotkeys (3 DOF: Pitch, Yaw, Roll)
		PlayerInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxYawMinus);
		PlayerInputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxYawPlus);
		PlayerInputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxPitchMinus);
		PlayerInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxPitchPlus);
		PlayerInputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxRollMinus);
		PlayerInputComponent->BindKey(EKeys::NumPadNine, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxRollPlus);
		PlayerInputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotateYaw90);
		PlayerInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotateRoll90);
		PlayerInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &AYenkaDesktopPawn::QuickRotatePitch90);
		PlayerInputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &AYenkaDesktopPawn::ResetHandCalibration);
		PlayerInputComponent->BindKey(EKeys::Add, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZPlus);
		PlayerInputComponent->BindKey(EKeys::Subtract, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZMinus);
		PlayerInputComponent->BindKey(EKeys::Multiply, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXPlus);
		PlayerInputComponent->BindKey(EKeys::Divide, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXMinus);
		PlayerInputComponent->BindKey(EKeys::Decimal, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYMinus);

		// Arrow and Navigation Key Bindings for Position & Phalanx
		PlayerInputComponent->BindKey(EKeys::Up, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxPitchMinus);
		PlayerInputComponent->BindKey(EKeys::Down, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxPitchPlus);
		PlayerInputComponent->BindKey(EKeys::Left, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxYawMinus);
		PlayerInputComponent->BindKey(EKeys::Right, IE_Pressed, this, &AYenkaDesktopPawn::OnPhalanxYawPlus);
		PlayerInputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZPlus);
		PlayerInputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZMinus);

		// Alternative Letter Key Bindings
		PlayerInputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYawPlus);
		PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyHPressed);
		PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibPitchPlus);
		PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyBPressed);
		PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyXPressed);
		PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyZPressed);
		PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyCPressed);
		PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibXPlus);
		PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &AYenkaDesktopPawn::TogglePhalanxEditMode);
		PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYMinus);
		PlayerInputComponent->BindKey(EKeys::L, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibYPlus);
		PlayerInputComponent->BindKey(EKeys::U, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZPlus);
		PlayerInputComponent->BindKey(EKeys::O, IE_Pressed, this, &AYenkaDesktopPawn::OnCalibZMinus);
		PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AYenkaDesktopPawn::ResetHandCalibration);

		// Scenario Theme / Finger Selection Hotkeys (1 to 7, 0)
		PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario1);
		PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario2);
		PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario3);
		PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario4);
		PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario5);
		PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario6);
		PlayerInputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AYenkaDesktopPawn::OnSelectScenario7);
		PlayerInputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AYenkaDesktopPawn::OnKeyVPressed);

		PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AYenkaDesktopPawn::OnMouseX);
		PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AYenkaDesktopPawn::OnMouseY);
		PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &AYenkaDesktopPawn::OnMouseWheel);
		PlayerInputComponent->BindAxisKey(EKeys::W, this, &AYenkaDesktopPawn::MoveForward);
		PlayerInputComponent->BindAxisKey(EKeys::S, this, &AYenkaDesktopPawn::MoveForward);
		PlayerInputComponent->BindAxisKey(EKeys::D, this, &AYenkaDesktopPawn::MoveRight);
		PlayerInputComponent->BindAxisKey(EKeys::A, this, &AYenkaDesktopPawn::MoveRight);
	}
}

void AYenkaDesktopPawn::TogglePhalanxEditMode()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	bIsPhalanxEditMode = !bIsPhalanxEditMode;
}

void AYenkaDesktopPawn::SelectFinger(int32 FingerIdx)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	SelectedFinger = FingerIdx;
}

void AYenkaDesktopPawn::SelectPhalanx(int32 PhalanxIdx)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	SelectedPhalanx = PhalanxIdx;
}

void AYenkaDesktopPawn::OnKeyZPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		SelectedPhalanx = 1; // Proximal (Base)
	}
	else
	{
		QuickRotateRoll90();
	}
}

void AYenkaDesktopPawn::OnKeyXPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		SelectedPhalanx = 2; // Intermediate (Middle)
	}
	else
	{
		QuickRotateYaw90();
	}
}

void AYenkaDesktopPawn::OnKeyCPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		SelectedPhalanx = 3; // Distal (Tip)
	}
}

void AYenkaDesktopPawn::OnKeyVPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		SelectedPhalanx = 0; // All Phalanges
	}
	else
	{
		SetGestureOpen();
	}
}

void AYenkaDesktopPawn::OnKeyQPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		OnPhalanxRollMinus();
	}
}

void AYenkaDesktopPawn::OnPhalanxPitchPlus()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		if (SelectedFinger == 7 && VirtualHand)
		{
			VirtualHand->AddHandAxialRotation(+5.0f);
		}
		else if (SelectedFinger == 6)
		{
			AdjustHandPitch(+5.0f);
		}
		else if (VirtualHand)
		{
			VirtualHand->SetPhalanxPitch(SelectedFinger, SelectedPhalanx, +5.0f);
		}
	}
	else
	{
		OnCalibXMinus();
	}
}

void AYenkaDesktopPawn::OnPhalanxPitchMinus()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		if (SelectedFinger == 7 && VirtualHand)
		{
			VirtualHand->AddHandAxialRotation(-5.0f);
		}
		else if (SelectedFinger == 6)
		{
			AdjustHandPitch(-5.0f);
		}
		else if (VirtualHand)
		{
			VirtualHand->SetPhalanxPitch(SelectedFinger, SelectedPhalanx, -5.0f);
		}
	}
	else
	{
		OnCalibXPlus();
	}
}

void AYenkaDesktopPawn::OnPhalanxYawPlus()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		if (SelectedFinger == 7 && VirtualHand)
		{
			VirtualHand->AddHandAxialRotation(+5.0f);
		}
		else if (SelectedFinger == 6)
		{
			AdjustHandYaw(+5.0f);
		}
		else if (VirtualHand)
		{
			VirtualHand->SetPhalanxYaw(SelectedFinger, SelectedPhalanx, +5.0f);
		}
	}
	else
	{
		OnCalibYPlus();
	}
}

void AYenkaDesktopPawn::OnPhalanxYawMinus()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		if (SelectedFinger == 7 && VirtualHand)
		{
			VirtualHand->AddHandAxialRotation(-5.0f);
		}
		else if (SelectedFinger == 6)
		{
			AdjustHandYaw(-5.0f);
		}
		else if (VirtualHand)
		{
			VirtualHand->SetPhalanxYaw(SelectedFinger, SelectedPhalanx, -5.0f);
		}
	}
	else
	{
		OnCalibYMinus();
	}
}

void AYenkaDesktopPawn::OnPhalanxRollPlus()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		if (SelectedFinger == 7 && VirtualHand)
		{
			VirtualHand->AddHandAxialRotation(+5.0f);
		}
		else if (SelectedFinger == 6)
		{
			AdjustHandRoll(+5.0f);
		}
		else if (VirtualHand)
		{
			VirtualHand->SetPhalanxRoll(SelectedFinger, SelectedPhalanx, +5.0f);
		}
	}
	else
	{
		OnCalibRollPlus();
	}
}

void AYenkaDesktopPawn::OnPhalanxRollMinus()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		if (SelectedFinger == 7 && VirtualHand)
		{
			VirtualHand->AddHandAxialRotation(-5.0f);
		}
		else if (SelectedFinger == 6)
		{
			AdjustHandRoll(-5.0f);
		}
		else if (VirtualHand)
		{
			VirtualHand->SetPhalanxRoll(SelectedFinger, SelectedPhalanx, -5.0f);
		}
	}
	else
	{
		OnCalibRollMinus();
	}
}

void AYenkaDesktopPawn::OnResetSelectedPhalanx()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		if (SelectedFinger == 7 && VirtualHand)
		{
			VirtualHand->SetHandAxialRotation(0.0f);
		}
		else if (SelectedFinger == 6)
		{
			PokeHandRotationOffset = FRotator(0.0f, -90.0f, 0.0f);
			GrabHandRotationOffset = FRotator(90.0f, -90.0f, 0.0f);
		}
		else if (VirtualHand)
		{
			VirtualHand->ResetPhalanx(SelectedFinger, SelectedPhalanx);
		}
	}
}

void AYenkaDesktopPawn::StartNamingGesture()
{
	bIsNamingCustomGesture = true;
	bIsNamingCustomTransform = false;
	CurrentTypedGestureName.Empty();
}

void AYenkaDesktopPawn::ConfirmSaveGesture()
{
	if (!bIsNamingCustomGesture && CurrentTypedGestureName.IsEmpty()) return;

	FString FinalName = CurrentTypedGestureName.TrimStartAndEnd();
	if (FinalName.IsEmpty())
	{
		FinalName = FString::Printf(TEXT("Gesto_%d"), CustomGesturesList.Num() + 1);
	}

	SaveHandGesture(FinalName);
	bIsNamingCustomGesture = false;
	CurrentTypedGestureName.Empty();
}

void AYenkaDesktopPawn::CancelNamingGesture()
{
	bIsNamingCustomGesture = false;
	CurrentTypedGestureName.Empty();
}

void AYenkaDesktopPawn::StartNamingTransform()
{
	bIsNamingCustomTransform = true;
	bIsNamingCustomGesture = false;
	CurrentTypedTransformName.Empty();
}

void AYenkaDesktopPawn::ConfirmSaveTransform()
{
	if (!bIsNamingCustomTransform && CurrentTypedTransformName.IsEmpty()) return;

	FString FinalName = CurrentTypedTransformName.TrimStartAndEnd();
	if (FinalName.IsEmpty())
	{
		FinalName = FString::Printf(TEXT("Posicion_%d"), CustomTransformsList.Num() + 1);
	}

	SaveHandTransform(FinalName);
	bIsNamingCustomTransform = false;
	CurrentTypedTransformName.Empty();
}

void AYenkaDesktopPawn::CancelNamingTransform()
{
	bIsNamingCustomTransform = false;
	CurrentTypedTransformName.Empty();
}

void AYenkaDesktopPawn::OnKeyF8Pressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	StartNamingTransform();
}

void AYenkaDesktopPawn::OnKeyF9Pressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	CyclePrevCustomTransform();
}

void AYenkaDesktopPawn::OnKeyF10Pressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	CycleNextCustomTransform();
}

void AYenkaDesktopPawn::OnKeyEnterPressed()
{
	if (bIsNamingCustomGesture)
	{
		ConfirmSaveGesture();
	}
	else if (bIsNamingCustomTransform)
	{
		ConfirmSaveTransform();
	}
	else if (bIsPhalanxEditMode)
	{
		StartNamingGesture();
	}
	else
	{
		StartNamingTransform();
	}
}

void AYenkaDesktopPawn::OnKeyEscapePressed()
{
	if (bIsNamingCustomGesture)
	{
		CancelNamingGesture();
	}
	else if (bIsNamingCustomTransform)
	{
		CancelNamingTransform();
	}
	else if (bIsPhalanxEditMode)
	{
		TogglePhalanxEditMode();
	}
}

void AYenkaDesktopPawn::OnKeyBackspacePressed()
{
	if (bIsNamingCustomGesture && CurrentTypedGestureName.Len() > 0)
	{
		CurrentTypedGestureName.RemoveAt(CurrentTypedGestureName.Len() - 1);
	}
	else if (bIsNamingCustomTransform && CurrentTypedTransformName.Len() > 0)
	{
		CurrentTypedTransformName.RemoveAt(CurrentTypedTransformName.Len() - 1);
	}
}

void AYenkaDesktopPawn::OnKeyLeftBracketPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	CyclePrevCustomGesture();
}

void AYenkaDesktopPawn::OnKeyRightBracketPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	CycleNextCustomGesture();
}

void AYenkaDesktopPawn::OnKeyDeletePressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode && CustomGesturesList.IsValidIndex(ActiveCustomGestureIndex))
	{
		FString DeletedName = CustomGesturesList[ActiveCustomGestureIndex].GestureName;
		DeleteHandGesture(DeletedName);
	}
}

void AYenkaDesktopPawn::SaveHandGesture(const FString& Name)
{
	if (!VirtualHand) return;

	const bool bInPoke = (bIsPokeModeActive || bIsPushingBlock || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
	const FVector ActivePos = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;
	const FRotator ActiveRot = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;

	FCustomHandGesture NewGesture = VirtualHand->ExportCurrentGesture(Name, ActivePos, ActiveRot);

	// Check if a gesture with this name already exists -> overwrite it
	int32 ExistingIndex = CustomGesturesList.IndexOfByPredicate([&Name](const FCustomHandGesture& G) {
		return G.GestureName.Equals(Name, ESearchCase::IgnoreCase);
	});

	if (ExistingIndex != INDEX_NONE)
	{
		CustomGesturesList[ExistingIndex] = NewGesture;
		ActiveCustomGestureIndex = ExistingIndex;
	}
	else
	{
		ActiveCustomGestureIndex = CustomGesturesList.Add(NewGesture);
	}

	SaveCustomGesturesToDisk();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9999, 4.0f, FColor::Green,
			FString::Printf(TEXT("✅ GESTO GUARDADO: \"%s\" (Total: %d)"), *Name, CustomGesturesList.Num()));
	}
}

void AYenkaDesktopPawn::LoadHandGesture(const FString& Name)
{
	int32 FoundIndex = CustomGesturesList.IndexOfByPredicate([&Name](const FCustomHandGesture& G) {
		return G.GestureName.Equals(Name, ESearchCase::IgnoreCase);
	});

	if (FoundIndex != INDEX_NONE)
	{
		LoadCustomGestureByIndex(FoundIndex);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9998, 4.0f, FColor::Red,
			FString::Printf(TEXT("❌ No se encontró el gesto: \"%s\""), *Name));
	}
}

void AYenkaDesktopPawn::LoadCustomGestureByIndex(int32 Index)
{
	if (!CustomGesturesList.IsValidIndex(Index) || !VirtualHand) return;

	ActiveCustomGestureIndex = Index;
	const FCustomHandGesture& Gesture = CustomGesturesList[Index];

	VirtualHand->ApplyCustomGesture(Gesture);

	// If loading LightPullGesture, bind and apply LightPullPositioning-1 transform!
	if (Gesture.GestureName.Contains(TEXT("LightPull"), ESearchCase::IgnoreCase))
	{
		int32 TransformIdx = CustomTransformsList.IndexOfByPredicate([](const FCustomHandTransform& T) {
			return T.TransformName.Equals(TEXT("LightPullPositioning-1"), ESearchCase::IgnoreCase);
		});
		if (TransformIdx != INDEX_NONE)
		{
			GrabHandLocationOffset = CustomTransformsList[TransformIdx].LocationOffset;
			GrabHandRotationOffset = CustomTransformsList[TransformIdx].RotationOffset;
			ActiveCustomTransformIndex = TransformIdx;
		}
		else
		{
			GrabHandLocationOffset = Gesture.HandLocationOffset;
			GrabHandRotationOffset = Gesture.HandRotationOffset;
		}
	}
	else if (Gesture.GestureName.Contains(TEXT("Point"), ESearchCase::IgnoreCase))
	{
		int32 TransformIdx = CustomTransformsList.IndexOfByPredicate([](const FCustomHandTransform& T) {
			return T.TransformName.Equals(TEXT("PointPositioning-1"), ESearchCase::IgnoreCase);
		});
		if (TransformIdx != INDEX_NONE)
		{
			PokeHandLocationOffset = CustomTransformsList[TransformIdx].LocationOffset;
			PokeHandRotationOffset = CustomTransformsList[TransformIdx].RotationOffset;
			ActiveCustomTransformIndex = TransformIdx;
		}
		else
		{
			PokeHandLocationOffset = Gesture.HandLocationOffset;
			PokeHandRotationOffset = Gesture.HandRotationOffset;
		}
	}
	else
	{
		PokeHandLocationOffset = Gesture.HandLocationOffset;
		PokeHandRotationOffset = Gesture.HandRotationOffset;
		GrabHandLocationOffset = Gesture.HandLocationOffset;
		GrabHandRotationOffset = Gesture.HandRotationOffset;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9997, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("📂 GESTO CARGADO [%d/%d]: \"%s\""), Index + 1, CustomGesturesList.Num(), *Gesture.GestureName));
	}
}

void AYenkaDesktopPawn::LoadCustomGestureByName(const FString& Name)
{
	LoadHandGesture(Name);
}

void AYenkaDesktopPawn::CycleNextCustomGesture()
{
	LoadCustomGesturesFromDisk();
	if (CustomGesturesList.Num() == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9995, 2.5f, FColor::Orange, TEXT("⚠️ No hay gestos guardados. Pulsa F4 y luego F5 para crear uno."));
		}
		return;
	}
	int32 NextIdx = (ActiveCustomGestureIndex + 1) % CustomGesturesList.Num();
	LoadCustomGestureByIndex(NextIdx);
}

void AYenkaDesktopPawn::CyclePrevCustomGesture()
{
	LoadCustomGesturesFromDisk();
	if (CustomGesturesList.Num() == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9995, 2.5f, FColor::Orange, TEXT("⚠️ No hay gestos guardados. Pulsa F4 y luego F5 para crear uno."));
		}
		return;
	}
	int32 PrevIdx = (ActiveCustomGestureIndex - 1 + CustomGesturesList.Num()) % CustomGesturesList.Num();
	LoadCustomGestureByIndex(PrevIdx);
}

void AYenkaDesktopPawn::ReloadHandGestures()
{
	LoadCustomGesturesFromDisk();
	if (ActiveCustomGestureIndex >= 0 && CustomGesturesList.IsValidIndex(ActiveCustomGestureIndex))
	{
		LoadCustomGestureByIndex(ActiveCustomGestureIndex);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9992, 3.5f, FColor::Green,
			FString::Printf(TEXT("🔄 GESTOS RECARGADOS DEL JSON (Total: %d)"), CustomGesturesList.Num()));
	}
}

void AYenkaDesktopPawn::DeleteHandGesture(const FString& Name)
{
	int32 FoundIndex = CustomGesturesList.IndexOfByPredicate([&Name](const FCustomHandGesture& G) {
		return G.GestureName.Equals(Name, ESearchCase::IgnoreCase);
	});

	if (FoundIndex != INDEX_NONE)
	{
		CustomGesturesList.RemoveAt(FoundIndex);
		SaveCustomGesturesToDisk();
		if (ActiveCustomGestureIndex >= CustomGesturesList.Num())
		{
			ActiveCustomGestureIndex = CustomGesturesList.Num() - 1;
		}
		if (ActiveCustomGestureIndex >= 0)
		{
			LoadCustomGestureByIndex(ActiveCustomGestureIndex);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9996, 3.0f, FColor::Yellow,
				FString::Printf(TEXT("🗑️ Gesto \"%s\" eliminado. Restantes: %d"), *Name, CustomGesturesList.Num()));
		}
	}
}

void AYenkaDesktopPawn::ListHandGestures()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::White, FString::Printf(TEXT("=== BIBLIOTECA DE GESTOS (%d guardados) ==="), CustomGesturesList.Num()));
		for (int32 i = 0; i < CustomGesturesList.Num(); ++i)
		{
			GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Cyan, FString::Printf(TEXT(" [%d] %s"), i + 1, *CustomGesturesList[i].GestureName));
		}
	}
}

bool AYenkaDesktopPawn::SaveCustomTransformsToDisk()
{
	FCustomTransformLibrary Library;
	Library.Transforms = CustomTransformsList;

	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(Library, JsonString, 0, 0))
	{
		FString FilePath = FPaths::ProjectSavedDir() / TEXT("HandTransforms/CustomTransforms.json");
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
		return FFileHelper::SaveStringToFile(JsonString, *FilePath);
	}
	return false;
}

bool AYenkaDesktopPawn::LoadCustomTransformsFromDisk()
{
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("HandTransforms/CustomTransforms.json");
	if (FPaths::FileExists(FilePath))
	{
		FString JsonString;
		if (FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			FCustomTransformLibrary Library;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Library, 0, 0))
			{
				CustomTransformsList = Library.Transforms;
			}
		}
	}

	// Ensure default presets are present if list was empty
	int32 LightPullPosIdx = CustomTransformsList.IndexOfByPredicate([](const FCustomHandTransform& T) {
		return T.TransformName.Equals(TEXT("LightPullPositioning-1"), ESearchCase::IgnoreCase);
	});

	if (LightPullPosIdx == INDEX_NONE)
	{
		FCustomHandTransform LightPullPos;
		LightPullPos.TransformName = TEXT("LightPullPositioning-1");
		LightPullPos.LocationOffset = FVector(-5.50f, 8.50f, -0.50f);
		LightPullPos.RotationOffset = FRotator(90.0f, -90.0f, 0.0f);
		CustomTransformsList.Insert(LightPullPos, 0);
		SaveCustomTransformsToDisk();
	}

	int32 PointPosIdx = CustomTransformsList.IndexOfByPredicate([](const FCustomHandTransform& T) {
		return T.TransformName.Equals(TEXT("PointPositioning-1"), ESearchCase::IgnoreCase);
	});

	if (PointPosIdx == INDEX_NONE)
	{
		FCustomHandTransform PointPos;
		PointPos.TransformName = TEXT("PointPositioning-1");
		PointPos.LocationOffset = FVector(-5.50f, 8.50f, -0.50f);
		PointPos.RotationOffset = FRotator(0.0f, -90.0f, 0.0f);
		CustomTransformsList.Add(PointPos);
		SaveCustomTransformsToDisk();
	}

	return CustomTransformsList.Num() > 0;
}

void AYenkaDesktopPawn::SaveHandTransform(const FString& Name)
{
	const bool bInPoke = (bIsPokeModeActive || bIsPushingBlock || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
	const FVector ActivePos = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;
	const FRotator ActiveRot = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;

	FCustomHandTransform NewTransform;
	NewTransform.TransformName = Name.IsEmpty() ? TEXT("CustomTransform") : Name;
	NewTransform.LocationOffset = ActivePos;
	NewTransform.RotationOffset = ActiveRot;

	int32 ExistingIndex = CustomTransformsList.IndexOfByPredicate([&Name](const FCustomHandTransform& T) {
		return T.TransformName.Equals(Name, ESearchCase::IgnoreCase);
	});

	if (ExistingIndex != INDEX_NONE)
	{
		CustomTransformsList[ExistingIndex] = NewTransform;
		ActiveCustomTransformIndex = ExistingIndex;
	}
	else
	{
		ActiveCustomTransformIndex = CustomTransformsList.Add(NewTransform);
	}

	SaveCustomTransformsToDisk();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9991, 4.0f, FColor::Green,
			FString::Printf(TEXT("✅ TRANSFORMACIÓN GUARDADA: \"%s\" [Pos: (%.1f, %.1f, %.1f) | Rot: (P:%.0f, Y:%.0f, R:%.0f)] (Total: %d)"),
				*Name, ActivePos.X, ActivePos.Y, ActivePos.Z, ActiveRot.Pitch, ActiveRot.Yaw, ActiveRot.Roll, CustomTransformsList.Num()));
	}
}

void AYenkaDesktopPawn::LoadHandTransform(const FString& Name)
{
	int32 FoundIndex = CustomTransformsList.IndexOfByPredicate([&Name](const FCustomHandTransform& T) {
		return T.TransformName.Equals(Name, ESearchCase::IgnoreCase);
	});

	if (FoundIndex != INDEX_NONE)
	{
		LoadCustomTransformByIndex(FoundIndex);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9990, 4.0f, FColor::Red,
			FString::Printf(TEXT("❌ No se encontró la transformación: \"%s\""), *Name));
	}
}

void AYenkaDesktopPawn::LoadCustomTransformByIndex(int32 Index)
{
	if (!CustomTransformsList.IsValidIndex(Index)) return;

	ActiveCustomTransformIndex = Index;
	const FCustomHandTransform& Transform = CustomTransformsList[Index];

	const bool bInPoke = (bIsPokeModeActive || bIsPushingBlock || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
	if (bInPoke)
	{
		PokeHandLocationOffset = Transform.LocationOffset;
		PokeHandRotationOffset = Transform.RotationOffset;
	}
	else
	{
		GrabHandLocationOffset = Transform.LocationOffset;
		GrabHandRotationOffset = Transform.RotationOffset;
	}

	if (VirtualHand)
	{
		VirtualHand->UpdateFingerPoses(0.0f);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9989, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("📍 TRANSFORMACIÓN CARGADA [%d/%d]: \"%s\" [Pos: (%.1f, %.1f, %.1f) | Rot: (P:%.0f, Y:%.0f, R:%.0f)]"),
				Index + 1, CustomTransformsList.Num(), *Transform.TransformName,
				Transform.LocationOffset.X, Transform.LocationOffset.Y, Transform.LocationOffset.Z,
				Transform.RotationOffset.Pitch, Transform.RotationOffset.Yaw, Transform.RotationOffset.Roll));
	}
}

void AYenkaDesktopPawn::LoadCustomTransformByName(const FString& Name)
{
	LoadHandTransform(Name);
}

void AYenkaDesktopPawn::CycleNextCustomTransform()
{
	LoadCustomTransformsFromDisk();
	if (CustomTransformsList.Num() == 0) return;
	int32 NextIdx = (ActiveCustomTransformIndex + 1) % CustomTransformsList.Num();
	LoadCustomTransformByIndex(NextIdx);
}

void AYenkaDesktopPawn::CyclePrevCustomTransform()
{
	LoadCustomTransformsFromDisk();
	if (CustomTransformsList.Num() == 0) return;
	int32 PrevIdx = (ActiveCustomTransformIndex - 1 + CustomTransformsList.Num()) % CustomTransformsList.Num();
	LoadCustomTransformByIndex(PrevIdx);
}

void AYenkaDesktopPawn::ReloadHandTransforms()
{
	LoadCustomTransformsFromDisk();
	if (ActiveCustomTransformIndex >= 0 && CustomTransformsList.IsValidIndex(ActiveCustomTransformIndex))
	{
		LoadCustomTransformByIndex(ActiveCustomTransformIndex);
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9991, 3.5f, FColor::Green,
			FString::Printf(TEXT("🔄 TRANSFORMACIONES RECARGADAS DEL JSON (Total: %d)"), CustomTransformsList.Num()));
	}
}

void AYenkaDesktopPawn::ListHandTransforms()
{
	LoadCustomTransformsFromDisk();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::White, FString::Printf(TEXT("=== BIBLIOTECA DE TRANSFORMACIONES (%d guardadas) ==="), CustomTransformsList.Num()));
		for (int32 i = 0; i < CustomTransformsList.Num(); ++i)
		{
			const FCustomHandTransform& T = CustomTransformsList[i];
			GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Cyan,
				FString::Printf(TEXT(" [%d] %s -> Pos: [%.1f, %.1f, %.1f] | Rot: [P:%.0f, Y:%.0f, R:%.0f]"),
					i + 1, *T.TransformName,
					T.LocationOffset.X, T.LocationOffset.Y, T.LocationOffset.Z,
					T.RotationOffset.Pitch, T.RotationOffset.Yaw, T.RotationOffset.Roll));
		}
	}
}

void AYenkaDesktopPawn::OnKeyHPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	CycleHandModel();
}

void AYenkaDesktopPawn::CycleHandModel()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (VirtualHand)
	{
		VirtualHand->CycleHandModel();
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9992, 3.5f, FColor(100, 255, 255),
				FString::Printf(TEXT("🎨 MODELO DE MANO: %s"), *VirtualHand->GetHandModelDisplayName()));
		}
	}
}

void AYenkaDesktopPawn::SetHandModel(const FString& ModelName)
{
	if (!VirtualHand) return;

	FString Lower = ModelName.ToLower();
	if (Lower.Contains(TEXT("quinn")) && Lower.Contains(TEXT("alt")))
	{
		VirtualHand->SetHandModelType(EHandModelType::QuinnAlt);
	}
	else if (Lower.Contains(TEXT("quinn")))
	{
		VirtualHand->SetHandModelType(EHandModelType::QuinnXR);
	}
	else if (Lower.Contains(TEXT("manny")) && Lower.Contains(TEXT("alt")))
	{
		VirtualHand->SetHandModelType(EHandModelType::MannyAlt);
	}
	else if (Lower.Contains(TEXT("manny")))
	{
		VirtualHand->SetHandModelType(EHandModelType::MannyXR);
	}
	else if (Lower.Contains(TEXT("skin")) || Lower.Contains(TEXT("piel")) || Lower.Contains(TEXT("humana")))
	{
		VirtualHand->SetHandModelType(EHandModelType::HumanSkin);
	}
	else if (Lower.Contains(TEXT("holo")) || Lower.Contains(TEXT("neon")))
	{
		VirtualHand->SetHandModelType(EHandModelType::HologramNeon);
	}
	else if (Lower.Contains(TEXT("black")) || Lower.Contains(TEXT("negro")) || Lower.Contains(TEXT("stealth")))
	{
		VirtualHand->SetHandModelType(EHandModelType::StealthBlack);
	}
	else if (Lower.Contains(TEXT("gold")) || Lower.Contains(TEXT("oro")) || Lower.Contains(TEXT("chrome")))
	{
		VirtualHand->SetHandModelType(EHandModelType::GoldenChrome);
	}
	else
	{
		VirtualHand->CycleHandModel();
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9992, 3.5f, FColor(100, 255, 255),
			FString::Printf(TEXT("🎨 MODELO DE MANO: %s"), *VirtualHand->GetHandModelDisplayName()));
	}
}

void AYenkaDesktopPawn::ListHandModels()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::White, TEXT("=== MODELOS Y SKINS DE MANO DISPONIBLES ==="));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [1] MannyXR (Robótico / Futurista por defecto)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [2] QuinnXR (Estilizado / Esbelto)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [3] MannyAlt (Variante Carbono Oscura)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [4] QuinnAlt (Variante Clara)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [5] HumanSkin (Piel Humana Natural)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [6] HologramNeon (Holograma Neón Translúcido)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [7] StealthBlack (Negro Mate)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Cyan, TEXT(" [8] GoldenChrome (Oro Metálico)"));
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow, TEXT("👉 Pulsa la tecla [H] en cualquier momento para ciclar entre ellos."));
	}
}

bool AYenkaDesktopPawn::SaveCustomGesturesToDisk()
{
	FCustomGestureLibrary Library;
	Library.Gestures = CustomGesturesList;

	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(Library, JsonString, 0, 0))
	{
		FString FilePath = FPaths::ProjectSavedDir() / TEXT("HandGestures/CustomGestures.json");
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
		return FFileHelper::SaveStringToFile(JsonString, *FilePath);
	}
	return false;
}

bool AYenkaDesktopPawn::LoadCustomGesturesFromDisk()
{
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("HandGestures/CustomGestures.json");
	if (FPaths::FileExists(FilePath))
	{
		FString JsonString;
		if (FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			FCustomGestureLibrary Library;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Library, 0, 0))
			{
				CustomGesturesList = Library.Gestures;
			}
		}
	}

	// Ensure default PointGesture is present in the library
	int32 PointIdx = CustomGesturesList.IndexOfByPredicate([](const FCustomHandGesture& G) {
		return G.GestureName.Equals(TEXT("PointGesture"), ESearchCase::IgnoreCase);
	});

	if (PointIdx == INDEX_NONE)
	{
		FCustomHandGesture PointGesture;
		PointGesture.GestureName = TEXT("PointGesture");
		PointGesture.Thumb.Proximal = FPhalanxData{ -40.0f, -50.0f, 0.0f };
		PointGesture.Thumb.Intermediate = FPhalanxData{ -15.0f, -20.0f, -1.0f };
		PointGesture.Thumb.Distal = FPhalanxData{ 5.0f, 25.0f, 0.0f };

		PointGesture.Index.Proximal = FPhalanxData{ 0.0f, 0.0f, -5.0f };
		PointGesture.Index.Intermediate = FPhalanxData{ 0.0f, 0.0f, 15.0f };
		PointGesture.Index.Distal = FPhalanxData{ 0.0f, 0.0f, 15.0f };

		PointGesture.Middle.Proximal = FPhalanxData{ 0.0f, 0.0f, -20.0f };
		PointGesture.Middle.Intermediate = FPhalanxData{ 0.0f, 0.0f, -95.0f };
		PointGesture.Middle.Distal = FPhalanxData{ 0.0f, 0.0f, -60.0f };

		PointGesture.Ring.Proximal = FPhalanxData{ 0.0f, 0.0f, 20.0f };
		PointGesture.Ring.Intermediate = FPhalanxData{ 0.0f, 0.0f, 75.0f };
		PointGesture.Ring.Distal = FPhalanxData{ 0.0f, 0.0f, -80.0f };

		PointGesture.Pinky.Proximal = FPhalanxData{ 0.0f, 0.0f, -20.0f };
		PointGesture.Pinky.Intermediate = FPhalanxData{ 0.0f, 0.0f, -80.0f };
		PointGesture.Pinky.Distal = FPhalanxData{ 0.0f, 0.0f, -95.0f };

		PointGesture.HandLocationOffset = PokeHandLocationOffset;
		PointGesture.HandRotationOffset = PokeHandRotationOffset;
		PointGesture.HandAxialRotation = 0.0f;

		CustomGesturesList.Add(PointGesture);
		SaveCustomGesturesToDisk();
	}

	// Ensure default LightPullGesture is present in the library
	int32 PullIdx = CustomGesturesList.IndexOfByPredicate([](const FCustomHandGesture& G) {
		return G.GestureName.Equals(TEXT("LightPullGesture"), ESearchCase::IgnoreCase);
	});

	if (PullIdx == INDEX_NONE)
	{
		FCustomHandGesture LightPull;
		LightPull.GestureName = TEXT("LightPullGesture");
		LightPull.Thumb.Proximal = FPhalanxData{ -45.0f, -85.0f, 20.0f };
		LightPull.Thumb.Intermediate = FPhalanxData{ 0.0f, 0.0f, 15.0f };
		LightPull.Thumb.Distal = FPhalanxData{ 0.0f, 0.0f, -10.0f };

		LightPull.Index.Proximal = FPhalanxData{ 0.0f, 0.0f, 0.0f };
		LightPull.Index.Intermediate = FPhalanxData{ 0.0f, 0.0f, -30.0f };
		LightPull.Index.Distal = FPhalanxData{ 0.0f, 0.0f, -15.0f };

		LightPull.Middle.Proximal = FPhalanxData{ 0.0f, 0.0f, 10.0f };
		LightPull.Middle.Intermediate = FPhalanxData{ 0.0f, 0.0f, 0.0f };
		LightPull.Middle.Distal = FPhalanxData{ 0.0f, 0.0f, 10.0f };

		LightPull.Ring.Proximal = FPhalanxData{ 0.0f, 0.0f, 15.0f };
		LightPull.Ring.Intermediate = FPhalanxData{ 0.0f, 0.0f, 0.0f };
		LightPull.Ring.Distal = FPhalanxData{ 0.0f, 0.0f, 0.0f };

		LightPull.Pinky.Proximal = FPhalanxData{ 0.0f, 0.0f, 25.0f };
		LightPull.Pinky.Intermediate = FPhalanxData{ 0.0f, 0.0f, 15.0f };
		LightPull.Pinky.Distal = FPhalanxData{ 0.0f, 0.0f, 0.0f };

		LightPull.HandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
		LightPull.HandRotationOffset = FRotator(90.0f, -90.0f, 0.0f);
		LightPull.HandAxialRotation = 0.0f;

		CustomGesturesList.Add(LightPull);
		SaveCustomGesturesToDisk();
	}

	return CustomGesturesList.Num() > 0;
}

void AYenkaDesktopPawn::OnAnyKeyPressed(FKey Key)
{
	if (bIsNamingCustomGesture)
	{
		if (Key == EKeys::Enter)
		{
			ConfirmSaveGesture();
			return;
		}
		if (Key == EKeys::Escape)
		{
			CancelNamingGesture();
			return;
		}
		if (Key == EKeys::BackSpace)
		{
			if (CurrentTypedGestureName.Len() > 0)
			{
				CurrentTypedGestureName.RemoveAt(CurrentTypedGestureName.Len() - 1);
			}
			return;
		}
		if (Key == EKeys::SpaceBar)
		{
			if (CurrentTypedGestureName.Len() < 24)
			{
				CurrentTypedGestureName.AppendChar(TEXT('_'));
			}
			return;
		}

		FString KeyStr = Key.GetFName().ToString();
		if (KeyStr.Len() == 1 && FChar::IsAlnum(KeyStr[0]))
		{
			if (CurrentTypedGestureName.Len() < 24)
			{
				CurrentTypedGestureName.Append(KeyStr);
			}
		}
		else if (Key == EKeys::Zero || Key == EKeys::NumPadZero) CurrentTypedGestureName.AppendChar(TEXT('0'));
		else if (Key == EKeys::One || Key == EKeys::NumPadOne) CurrentTypedGestureName.AppendChar(TEXT('1'));
		else if (Key == EKeys::Two || Key == EKeys::NumPadTwo) CurrentTypedGestureName.AppendChar(TEXT('2'));
		else if (Key == EKeys::Three || Key == EKeys::NumPadThree) CurrentTypedGestureName.AppendChar(TEXT('3'));
		else if (Key == EKeys::Four || Key == EKeys::NumPadFour) CurrentTypedGestureName.AppendChar(TEXT('4'));
		else if (Key == EKeys::Five || Key == EKeys::NumPadFive) CurrentTypedGestureName.AppendChar(TEXT('5'));
		else if (Key == EKeys::Six || Key == EKeys::NumPadSix) CurrentTypedGestureName.AppendChar(TEXT('6'));
		else if (Key == EKeys::Seven || Key == EKeys::NumPadSeven) CurrentTypedGestureName.AppendChar(TEXT('7'));
		else if (Key == EKeys::Eight || Key == EKeys::NumPadEight) CurrentTypedGestureName.AppendChar(TEXT('8'));
		else if (Key == EKeys::Nine || Key == EKeys::NumPadNine) CurrentTypedGestureName.AppendChar(TEXT('9'));
		else if (Key == EKeys::Underscore || Key == EKeys::Hyphen) CurrentTypedGestureName.AppendChar(TEXT('_'));
		return;
	}

	if (bIsNamingCustomTransform)
	{
		if (Key == EKeys::Enter)
		{
			ConfirmSaveTransform();
			return;
		}
		if (Key == EKeys::Escape)
		{
			CancelNamingTransform();
			return;
		}
		if (Key == EKeys::BackSpace)
		{
			if (CurrentTypedTransformName.Len() > 0)
			{
				CurrentTypedTransformName.RemoveAt(CurrentTypedTransformName.Len() - 1);
			}
			return;
		}
		if (Key == EKeys::SpaceBar)
		{
			if (CurrentTypedTransformName.Len() < 24)
			{
				CurrentTypedTransformName.AppendChar(TEXT('_'));
			}
			return;
		}

		FString KeyStr = Key.GetFName().ToString();
		if (KeyStr.Len() == 1 && FChar::IsAlnum(KeyStr[0]))
		{
			if (CurrentTypedTransformName.Len() < 24)
			{
				CurrentTypedTransformName.Append(KeyStr);
			}
		}
		else if (Key == EKeys::Zero || Key == EKeys::NumPadZero) CurrentTypedTransformName.AppendChar(TEXT('0'));
		else if (Key == EKeys::One || Key == EKeys::NumPadOne) CurrentTypedTransformName.AppendChar(TEXT('1'));
		else if (Key == EKeys::Two || Key == EKeys::NumPadTwo) CurrentTypedTransformName.AppendChar(TEXT('2'));
		else if (Key == EKeys::Three || Key == EKeys::NumPadThree) CurrentTypedTransformName.AppendChar(TEXT('3'));
		else if (Key == EKeys::Four || Key == EKeys::NumPadFour) CurrentTypedTransformName.AppendChar(TEXT('4'));
		else if (Key == EKeys::Five || Key == EKeys::NumPadFive) CurrentTypedTransformName.AppendChar(TEXT('5'));
		else if (Key == EKeys::Six || Key == EKeys::NumPadSix) CurrentTypedTransformName.AppendChar(TEXT('6'));
		else if (Key == EKeys::Seven || Key == EKeys::NumPadSeven) CurrentTypedTransformName.AppendChar(TEXT('7'));
		else if (Key == EKeys::Eight || Key == EKeys::NumPadEight) CurrentTypedTransformName.AppendChar(TEXT('8'));
		else if (Key == EKeys::Nine || Key == EKeys::NumPadNine) CurrentTypedTransformName.AppendChar(TEXT('9'));
		else if (Key == EKeys::Underscore || Key == EKeys::Hyphen) CurrentTypedTransformName.AppendChar(TEXT('_'));
		return;
	}
}

void AYenkaDesktopPawn::SelectScenarioTheme(int32 ThemeIndex)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;

	if (bIsPhalanxEditMode)
	{
		// In Phalanx Edit Mode: 1-5 fingers, 6: wrist, 7: wrist-to-index axial rotation, 0: all!
		if (ThemeIndex >= 0 && ThemeIndex <= 4)
		{
			SelectedFinger = ThemeIndex + 1; // 1: Thumb, 2: Index, 3: Middle, 4: Ring, 5: Pinky
			return;
		}
		else if (ThemeIndex == 5) // Key 6
		{
			SelectedFinger = 6; // Wrist Rotation (Muñeca)
			return;
		}
		else if (ThemeIndex == 6) // Key 7
		{
			SelectedFinger = 7; // Wrist-to-Index Axial Rotation
			return;
		}
		else if (ThemeIndex >= 7) // Key 0 or other
		{
			SelectedFinger = 0; // All fingers
			return;
		}
	}

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
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	AYenkaScenarioMenu* Menu = Cast<AYenkaScenarioMenu>(UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaScenarioMenu::StaticClass()));
	if (Menu)
	{
		Menu->ToggleMenuVisibility();
	}
}

void AYenkaDesktopPawn::OnKeyMPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		SelectedFinger = 6; // Select Muñeca (Wrist / Whole Hand Rotation Mode)
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9994, 2.5f, FColor(255, 220, 50), TEXT("🖐️ SELECCIONADA: MUÑECA (Rotación de la Mano Completa)"));
		}
	}
	else
	{
		OnToggleScenarioMenu();
	}
}

void AYenkaDesktopPawn::OnKeyBPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		SelectedFinger = 7; // Select Eje Muñeca-Índice
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9993, 2.5f, FColor(50, 255, 255), TEXT("🎯 SELECCIONADO: EJE MUÑECA-ÍNDICE (Giro Axial de la Mano Completa)"));
		}
	}
	else
	{
		OnCalibRollMinus();
	}
}

void AYenkaDesktopPawn::AdjustHandOffsetX(float Delta)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	PokeHandLocationOffset.X += Delta;
	GrabHandLocationOffset.X += Delta;
}

void AYenkaDesktopPawn::AdjustHandOffsetY(float Delta)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	PokeHandLocationOffset.Y += Delta;
	GrabHandLocationOffset.Y += Delta;
}

void AYenkaDesktopPawn::AdjustHandOffsetZ(float Delta)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	PokeHandLocationOffset.Z += Delta;
	GrabHandLocationOffset.Z += Delta;
}

void AYenkaDesktopPawn::AdjustHandPitch(float Delta)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	PokeHandRotationOffset.Pitch = FRotator::NormalizeAxis(PokeHandRotationOffset.Pitch + Delta);
	GrabHandRotationOffset.Pitch = FRotator::NormalizeAxis(GrabHandRotationOffset.Pitch + Delta);
}

void AYenkaDesktopPawn::AdjustHandYaw(float Delta)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	PokeHandRotationOffset.Yaw = FRotator::NormalizeAxis(PokeHandRotationOffset.Yaw + Delta);
	GrabHandRotationOffset.Yaw = FRotator::NormalizeAxis(GrabHandRotationOffset.Yaw + Delta);
}

void AYenkaDesktopPawn::AdjustHandRoll(float Delta)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	PokeHandRotationOffset.Roll = FRotator::NormalizeAxis(PokeHandRotationOffset.Roll + Delta);
	GrabHandRotationOffset.Roll = FRotator::NormalizeAxis(GrabHandRotationOffset.Roll + Delta);
}

void AYenkaDesktopPawn::QuickRotateYaw90()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
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
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
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
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
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
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	GrabHandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
	GrabHandRotationOffset = FRotator(90.0f, -90.0f, 0.0f);
	PokeHandLocationOffset = FVector(-5.50f, 8.50f, -0.50f);
	PokeHandRotationOffset = FRotator(0.0f, -90.0f, 0.0f);
	bForceGesturePreview = false;
	ActiveGesturePreview = EHandPoseMode::OpenHand;
	if (VirtualHand)
	{
		VirtualHand->ResetAllPhalanges();
	}
}

void AYenkaDesktopPawn::SetGesturePush()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	ActiveGesturePreview = EHandPoseMode::FingerPoke;
	bForceGesturePreview = true;
	bIsPokeModeActive = true;

	// Use PointPositioning-1 transform when using PointGesture / Push
	int32 TransformIdx = CustomTransformsList.IndexOfByPredicate([](const FCustomHandTransform& T) {
		return T.TransformName.Equals(TEXT("PointPositioning-1"), ESearchCase::IgnoreCase);
	});
	if (TransformIdx != INDEX_NONE)
	{
		PokeHandLocationOffset = CustomTransformsList[TransformIdx].LocationOffset;
		PokeHandRotationOffset = CustomTransformsList[TransformIdx].RotationOffset;
		ActiveCustomTransformIndex = TransformIdx;
	}

	if (VirtualHand)
	{
		VirtualHand->LoadPresetPose(EHandPoseMode::FingerPoke);
		VirtualHand->SetHandPoseMode(EHandPoseMode::FingerPoke);
	}
}

void AYenkaDesktopPawn::SetGestureGrab()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	ActiveGesturePreview = EHandPoseMode::GrabPinch;
	bForceGesturePreview = true;
	bIsPokeModeActive = false;

	// Use LightPullPositioning-1 transform when using LightPull / Grab
	int32 TransformIdx = CustomTransformsList.IndexOfByPredicate([](const FCustomHandTransform& T) {
		return T.TransformName.Equals(TEXT("LightPullPositioning-1"), ESearchCase::IgnoreCase);
	});
	if (TransformIdx != INDEX_NONE)
	{
		GrabHandLocationOffset = CustomTransformsList[TransformIdx].LocationOffset;
		GrabHandRotationOffset = CustomTransformsList[TransformIdx].RotationOffset;
		ActiveCustomTransformIndex = TransformIdx;
	}

	if (VirtualHand)
	{
		VirtualHand->LoadPresetPose(EHandPoseMode::GrabPinch);
		VirtualHand->SetHandPoseMode(EHandPoseMode::GrabPinch);
	}
}

void AYenkaDesktopPawn::SetGestureOpen()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	ActiveGesturePreview = EHandPoseMode::OpenHand;
	bForceGesturePreview = true;
	bIsPokeModeActive = false;
	if (VirtualHand)
	{
		VirtualHand->LoadPresetPose(EHandPoseMode::OpenHand);
		VirtualHand->SetHandPoseMode(EHandPoseMode::OpenHand);
	}
}

void AYenkaDesktopPawn::CycleGesture()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	bForceGesturePreview = true;
	if (ActiveGesturePreview == EHandPoseMode::OpenHand)
	{
		SetGestureGrab();
	}
	else if (ActiveGesturePreview == EHandPoseMode::GrabPinch)
	{
		SetGesturePush();
	}
	else
	{
		SetGestureOpen();
	}
}

void AYenkaDesktopPawn::UpdatePersistentCalibrationHUD()
{
	if (GEngine && IsLocallyControlled())
	{
		if (bIsNamingCustomGesture)
		{
			FString Line1 = TEXT("╔══════════════════════════════════════════════════════════════════════════════╗");
			FString Line2 = TEXT("║ 💾 GUARDAR NUEVO GESTO: Teclea el nombre y pulsa ENTER (ESC para Cancelar)   ║");
			FString Line3 = FString::Printf(TEXT("║ 📝 NOMBRE DEL GESTO: [ %s_ ]                                                 ║"), *CurrentTypedGestureName);
			FString Line4 = FString::Printf(TEXT("║ 📊 Gestos Guardados Actuales en Biblioteca: %d                                ║"), CustomGesturesList.Num());
			FString Line5 = TEXT("╚══════════════════════════════════════════════════════════════════════════════╝");

			GEngine->AddOnScreenDebugMessage(1001, 0.20f, FColor(50, 255, 50), Line1);
			GEngine->AddOnScreenDebugMessage(1002, 0.20f, FColor(255, 255, 50), Line2);
			GEngine->AddOnScreenDebugMessage(1003, 0.20f, FColor(50, 255, 255), Line3);
			GEngine->AddOnScreenDebugMessage(1004, 0.20f, FColor(200, 200, 255), Line4);
			GEngine->AddOnScreenDebugMessage(1005, 0.20f, FColor(50, 255, 50), Line5);
			return;
		}

		if (bIsNamingCustomTransform)
		{
			const bool bInPoke = (bIsPokeModeActive || bIsPushingBlock || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
			const FVector ActivePos = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;
			const FRotator ActiveRot = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;

			FString Line1 = TEXT("╔══════════════════════════════════════════════════════════════════════════════╗");
			FString Line2 = TEXT("║ 💾 GUARDAR TRANSFORMACIÓN DE POSICIÓN: Teclea el nombre y ENTER (ESC Cancela) ║");
			FString Line3 = FString::Printf(TEXT("║ 📝 NOMBRE: [ %s_ ]                                                           ║"), *CurrentTypedTransformName);
			FString Line4 = FString::Printf(TEXT("║ 📍 POSICIÓN [%s]: X: %+.2f cm | Y: %+.2f cm | Z: %+.2f cm                     ║"),
				bInPoke ? TEXT("EMPUJAR") : TEXT("AGARRAR"), ActivePos.X, ActivePos.Y, ActivePos.Z);
			FString Line5 = FString::Printf(TEXT("║ 🔄 ROTACIÓN [%s]: Pitch: %+.0f° | Yaw: %+.0f° | Roll: %+.0f°                  ║"),
				bInPoke ? TEXT("EMPUJAR") : TEXT("AGARRAR"), ActiveRot.Pitch, ActiveRot.Yaw, ActiveRot.Roll);
			FString Line6 = FString::Printf(TEXT("║ 📊 Presets de Transformación Guardados en Disco: %d                          ║"), CustomTransformsList.Num());
			FString Line7 = TEXT("╚══════════════════════════════════════════════════════════════════════════════╝");

			GEngine->AddOnScreenDebugMessage(1001, 0.20f, FColor(50, 255, 50), Line1);
			GEngine->AddOnScreenDebugMessage(1002, 0.20f, FColor(255, 255, 50), Line2);
			GEngine->AddOnScreenDebugMessage(1003, 0.20f, FColor(50, 255, 255), Line3);
			GEngine->AddOnScreenDebugMessage(1004, 0.20f, FColor(120, 255, 180), Line4);
			GEngine->AddOnScreenDebugMessage(1005, 0.20f, FColor(120, 255, 120), Line5);
			GEngine->AddOnScreenDebugMessage(1006, 0.20f, FColor(200, 200, 255), Line6);
			GEngine->AddOnScreenDebugMessage(1007, 0.20f, FColor(50, 255, 50), Line7);
			return;
		}

		if (bIsPhalanxEditMode && VirtualHand)
		{
			// --- PHALANX, WRIST & PERPENDICULAR INDEX BASE AXIS EDIT MODE HUD ---
			static const TCHAR* FingerNames[] = {
				TEXT("Todos los Dedos"),
				TEXT("Pulgar"),
				TEXT("Índice"),
				TEXT("Medio"),
				TEXT("Anular"),
				TEXT("Meñique"),
				TEXT("🖐️ MUÑECA (Rotación Espacial de Muñeca)"),
				TEXT("🎯 EJE BASE ÍNDICE (Rotación Perpendicular Horizontal)")
			};
			static const TCHAR* PhalanxNames[] = { TEXT("Todas las Falanges"), TEXT("Proximal (Base)"), TEXT("Media (Centro)"), TEXT("Distal (Punta)") };

			const TCHAR* ActiveFingerName = (SelectedFinger >= 0 && SelectedFinger <= 7) ? FingerNames[SelectedFinger] : TEXT("Desconocido");
			const TCHAR* ActivePhalanxName = (SelectedFinger >= 6) ? TEXT("Orientación de la Mano") : ((SelectedPhalanx >= 0 && SelectedPhalanx <= 3) ? PhalanxNames[SelectedPhalanx] : TEXT("Desconocido"));

			if (SelectedFinger == 6)
			{
				FString Line1 = FString::Printf(TEXT("=== 🖐️ MODO EDICIÓN: ROTACIÓN DE MUÑECA [Pulsa F4 o K para Salir] ==="));
				FString Line2 = FString::Printf(TEXT("🎯 SELECCIÓN: [ %s ] (Teclas: 1-5 Dedos | 6/M: Muñeca | 7/B: Eje Base Índice | 0: Todos)"), ActiveFingerName);
				FString Line3 = FString::Printf(TEXT("⌨️ 3 EJES MUÑECA: [ Arriba/Abajo: Pitch +-5° | Izq/Der: Yaw +-5° | Q/E: Roll +-5° | Espacio: Reset Muñeca | F5: Guardar ]"));
				FString Line4 = FString::Printf(TEXT("📊 ROTACIÓN ACTUAL DE MUÑECA: [ Pitch: %+.0f° | Yaw: %+.0f° | Roll: %+.0f° ]"),
					PokeHandRotationOffset.Pitch, PokeHandRotationOffset.Yaw, PokeHandRotationOffset.Roll);
				FString Line5 = FString::Printf(TEXT("📊 POSICIÓN ESPACIAL: [ X: %+.1f cm | Y: %+.1f cm | Z: %+.1f cm ]"),
					PokeHandLocationOffset.X, PokeHandLocationOffset.Y, PokeHandLocationOffset.Z);
				FString Line6 = FString::Printf(TEXT("🏷️ GESTO: [ %s ] | 📂 GUARDADOS (%d): [ y ] para Ciclar | Supr: Borrar"),
					*VirtualHand->GetDetectedGestureDescription(), CustomGesturesList.Num());

				GEngine->AddOnScreenDebugMessage(1001, 0.20f, FColor(255, 100, 255), Line1);
				GEngine->AddOnScreenDebugMessage(1002, 0.20f, FColor(255, 220, 50), Line2);
				GEngine->AddOnScreenDebugMessage(1003, 0.20f, FColor(50, 255, 255), Line3);
				GEngine->AddOnScreenDebugMessage(1004, 0.20f, FColor(120, 255, 120), Line4);
				GEngine->AddOnScreenDebugMessage(1005, 0.20f, FColor(120, 255, 180), Line5);
				GEngine->AddOnScreenDebugMessage(1006, 0.20f, FColor(255, 160, 50), Line6);
				return;
			}
			else if (SelectedFinger == 7)
			{
				FString Line1 = FString::Printf(TEXT("=== 🎯 MODO EDICIÓN: ROTACIÓN PERPENDICULAR (PIVOTE EN BASE DEL ÍNDICE) [Pulsa F4 para Salir] ==="));
				FString Line2 = FString::Printf(TEXT("🎯 SELECCIÓN: [ %s ] (Teclas: 1-5 Dedos | 6/M: Muñeca | 7/B: Eje Base Índice | 0: Todos)"), ActiveFingerName);
				FString Line3 = FString::Printf(TEXT("⌨️ CONTROLES DE GIRO: [ Flechas Izq/Der o Arriba/Abajo o Q/E: Girar Mano +-5° | Espacio: Reset ]"));
				FString Line4 = FString::Printf(TEXT("📊 ROTACIÓN PERPENDICULAR HORIZONTAL: [ Ángulo de Giro: %+.0f° ]"),
					VirtualHand->GetHandAxialRotation());
				FString Line5 = FString::Printf(TEXT("📌 EJE DE GIRO: Eje perpendicular en el plano horizontal pasando por el inicio de la falange base del índice"));
				FString Line6 = FString::Printf(TEXT("🏷️ GESTO: [ %s ] | 📂 GUARDADOS (%d): [ y ] para Ciclar | F5: Guardar"),
					*VirtualHand->GetDetectedGestureDescription(), CustomGesturesList.Num());

				GEngine->AddOnScreenDebugMessage(1001, 0.20f, FColor(255, 100, 255), Line1);
				GEngine->AddOnScreenDebugMessage(1002, 0.20f, FColor(50, 255, 255), Line2);
				GEngine->AddOnScreenDebugMessage(1003, 0.20f, FColor(255, 255, 50), Line3);
				GEngine->AddOnScreenDebugMessage(1004, 0.20f, FColor(120, 255, 120), Line4);
				GEngine->AddOnScreenDebugMessage(1005, 0.20f, FColor(200, 200, 255), Line5);
				GEngine->AddOnScreenDebugMessage(1006, 0.20f, FColor(255, 160, 50), Line6);
				return;
			}

			FFingerPhalanges Thumb = VirtualHand->ThumbPhalanges;
			FFingerPhalanges Index = VirtualHand->IndexPhalanges;
			FFingerPhalanges Middle = VirtualHand->MiddlePhalanges;
			FFingerPhalanges Ring = VirtualHand->RingPhalanges;
			FFingerPhalanges Pinky = VirtualHand->PinkyPhalanges;

			FString Line1 = FString::Printf(TEXT("=== 🖐️ MODO EDICIÓN DE FALANGES (3 EJES: PITCH, YAW, ROLL) [Pulsa F4 o K para Salir] ==="));
			FString Line2 = FString::Printf(TEXT("🎯 SELECCIÓN: [ DEDO (%d): %s | FALANGE (%d): %s ]  (Teclas: 1-5 Dedo, 6/M: Muñeca, 7/B: Eje Muñeca-Índice, 0: Todos | Z/X/C/V: Falange)"),
				SelectedFinger, ActiveFingerName, SelectedPhalanx, ActivePhalanxName);
			FString Line3 = FString::Printf(TEXT("⌨️ 3 EJES: [ Arriba/Abajo: Pitch | Izq/Der: Yaw | Q/E: Roll | Espacio: Reset | F5 o ENTER: Guardar Gesto ]"));
			FString Line4 = FString::Printf(TEXT("📊 PULGAR: [P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°] | ÍNDICE: [P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°]"),
				Thumb.Proximal.Pitch, Thumb.Proximal.Yaw, Thumb.Proximal.Roll, Thumb.Intermediate.Pitch, Thumb.Intermediate.Yaw, Thumb.Intermediate.Roll, Thumb.Distal.Pitch, Thumb.Distal.Yaw, Thumb.Distal.Roll,
				Index.Proximal.Pitch, Index.Proximal.Yaw, Index.Proximal.Roll, Index.Intermediate.Pitch, Index.Intermediate.Yaw, Index.Intermediate.Roll, Index.Distal.Pitch, Index.Distal.Yaw, Index.Distal.Roll);
			FString Line5 = FString::Printf(TEXT("📊 MEDIO: [P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°] | ANULAR: [P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°] | MEÑIQUE: [P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°, P:%+.0f° Y:%+.0f° R:%+.0f°]"),
				Middle.Proximal.Pitch, Middle.Proximal.Yaw, Middle.Proximal.Roll, Middle.Intermediate.Pitch, Middle.Intermediate.Yaw, Middle.Intermediate.Roll, Middle.Distal.Pitch, Middle.Distal.Yaw, Middle.Distal.Roll,
				Ring.Proximal.Pitch, Ring.Proximal.Yaw, Ring.Proximal.Roll, Ring.Intermediate.Pitch, Ring.Intermediate.Yaw, Ring.Intermediate.Roll, Ring.Distal.Pitch, Ring.Distal.Yaw, Ring.Distal.Roll,
				Pinky.Proximal.Pitch, Pinky.Proximal.Yaw, Pinky.Proximal.Roll, Pinky.Intermediate.Pitch, Pinky.Intermediate.Yaw, Pinky.Intermediate.Roll, Pinky.Distal.Pitch, Pinky.Distal.Yaw, Pinky.Distal.Roll);
			FString ModelName = VirtualHand ? VirtualHand->GetHandModelDisplayName() : TEXT("Manny XR");
			FString Line6 = FString::Printf(TEXT("🎨 MODELO: [ %s ] (Pulsa H para Cambiar) | 📂 GUARDADOS (%d): [ y ] | Supr: Borrar"),
				*ModelName, CustomGesturesList.Num());

			GEngine->AddOnScreenDebugMessage(1001, 0.20f, FColor(255, 100, 255), Line1);
			GEngine->AddOnScreenDebugMessage(1002, 0.20f, FColor(255, 220, 50), Line2);
			GEngine->AddOnScreenDebugMessage(1003, 0.20f, FColor(50, 255, 255), Line3);
			GEngine->AddOnScreenDebugMessage(1004, 0.20f, FColor(120, 255, 120), Line4);
			GEngine->AddOnScreenDebugMessage(1005, 0.20f, FColor(120, 255, 180), Line5);
			GEngine->AddOnScreenDebugMessage(1006, 0.20f, FColor(255, 160, 50), Line6);
			return;
		}

		const bool bInPoke = (bIsPokeModeActive || bIsPushingBlock || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
		const FVector ActivePos = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;
		const FRotator ActiveRot = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;

		FString TransformPresetName = (ActiveCustomTransformIndex >= 0 && CustomTransformsList.IsValidIndex(ActiveCustomTransformIndex))
			? FString::Printf(TEXT("[%d/%d] %s"), ActiveCustomTransformIndex + 1, CustomTransformsList.Num(), *CustomTransformsList[ActiveCustomTransformIndex].TransformName)
			: TEXT("Personalizada");

		FString GestureName = VirtualHand ? VirtualHand->GetDetectedGestureDescription() : TEXT("🖐️ LIBRE / INSPECCIÓN (OpenHand)");
		if (ActiveCustomGestureIndex >= 0 && CustomGesturesList.IsValidIndex(ActiveCustomGestureIndex))
		{
			GestureName = FString::Printf(TEXT("🎨 [%d/%d] %s"), ActiveCustomGestureIndex + 1, CustomGesturesList.Num(), *CustomGesturesList[ActiveCustomGestureIndex].GestureName);
		}
		else if (bForceGesturePreview)
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

		FString ModelName = VirtualHand ? VirtualHand->GetHandModelDisplayName() : TEXT("Manny XR");
		FString Line1 = FString::Printf(TEXT("=== 🎛️ CALIBRACIÓN DE MANO YENKA (TIEMPO REAL) ==="));
		FString Line2 = FString::Printf(TEXT("📍 POSICIÓN [%s]:  [ X: %+.2f cm | Y: %+.2f cm | Z: %+.2f cm ] (Preset: %s)"),
			bInPoke ? TEXT("EMPUJAR") : TEXT("AGARRAR/LIBRE"), ActivePos.X, ActivePos.Y, ActivePos.Z, *TransformPresetName);
		FString Line3 = FString::Printf(TEXT("🔄 ROTACIÓN [%s]:  [ Yaw: %+.0f° | Pitch: %+.0f° | Roll: %+.0f° ] (F8/Enter: Guardar Posición | F9/F10: Ciclar)"),
			bInPoke ? TEXT("EMPUJAR") : TEXT("AGARRAR/LIBRE"), ActiveRot.Yaw, ActiveRot.Pitch, ActiveRot.Roll);
		FString Line4 = FString::Printf(TEXT("✋ GESTO:     [ %s ]  (Teclas: F1 Empujar, F2 Agarrar, F3 Libre, Tab Ciclar)"),
			*GestureName);
		FString Line5 = FString::Printf(TEXT("🎨 MODELO:    [ %s ]  (Pulsa [H] para Cambiar Modelo o Skin)"),
			*ModelName);
		FString Line6 = FString::Printf(TEXT("⌨️ FALANGES:  ⭐ Pulsa [F4] o [K] para entrar al MODO EDICIÓN DE FALANGES (Control Dedo a Dedo)"));

		GEngine->AddOnScreenDebugMessage(1001, 0.20f, FColor(255, 215, 0), Line1);
		GEngine->AddOnScreenDebugMessage(1002, 0.20f, FColor(0, 240, 255), Line2);
		GEngine->AddOnScreenDebugMessage(1003, 0.20f, FColor(50, 255, 120), Line3);
		GEngine->AddOnScreenDebugMessage(1004, 0.20f, FColor(255, 140, 0), Line4);
		GEngine->AddOnScreenDebugMessage(1005, 0.20f, FColor(100, 255, 255), Line5);
		GEngine->AddOnScreenDebugMessage(1006, 0.20f, FColor(255, 130, 255), Line6);
	}
}

void AYenkaDesktopPawn::OnSecondaryClickPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	bIsOrbitingCamera = true;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = false;
	}
}

void AYenkaDesktopPawn::OnSecondaryClickReleased()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	bIsOrbitingCamera = false;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = true;
	}
}

void AYenkaDesktopPawn::OnPokeKeyPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode)
	{
		OnPhalanxRollPlus();
		return;
	}
	bIsPokeModeActive = true;
}

void AYenkaDesktopPawn::OnPokeKeyReleased()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode) return;
	if (!bIsPushingBlock)
	{
		bIsPokeModeActive = false;
	}
}

void AYenkaDesktopPawn::OnTogglePokeMode()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsPhalanxEditMode) return;
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
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (bIsOrbitingCamera && FMath::Abs(Val) > 0.001f)
	{
		AddControllerYawInput(Val * 2.5f);
	}
}

void AYenkaDesktopPawn::OnMouseY(float Val)
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
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
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (CameraBoom && FMath::Abs(Val) > 0.001f)
	{
		CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - (Val * 8.0f), 30.0f, 250.0f);
	}
}

void AYenkaDesktopPawn::MoveForward(float Val)
{
	if (bIsPhalanxEditMode || bIsNamingCustomGesture || bIsNamingCustomTransform) return;
	if (FMath::Abs(Val) > 0.01f)
	{
		FVector Forward = FollowCamera ? FollowCamera->GetForwardVector() : GetActorForwardVector();
		Forward.Z = 0.0f;
		AddActorWorldOffset(Forward.GetSafeNormal() * (Val * 2.0f));
	}
}

void AYenkaDesktopPawn::MoveRight(float Val)
{
	if (bIsPhalanxEditMode || bIsNamingCustomGesture || bIsNamingCustomTransform) return;
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
	if (bIsPhalanxEditMode)
	{
		if (VirtualHand)
		{
			VirtualHand->SetActorHiddenInGame(false);

			// Position the hand in front of the player's camera for clear editing & observation
			FVector CamLoc = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
			FRotator CamRot = FollowCamera ? FollowCamera->GetComponentRotation() : GetActorRotation();
			FVector CamFwd = CamRot.Vector();
			FVector CamRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
			FVector CamUp = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Z);

			FVector EditAnchorPos = CamLoc + (CamFwd * 50.0f) + (CamRight * 10.0f) - (CamUp * 6.0f);

			// Base rotation faces the camera with wrist pointing back
			FRotator BaseRot = (-CamFwd).Rotation();
			BaseRot.Pitch = 0.0f;
			BaseRot.Roll = 0.0f;

			const bool bInPoke = (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
			const FRotator ActiveRotOffset = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;
			const FVector ActiveLocOffset = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;

			FRotator HandRot = BaseRot + ActiveRotOffset;
			FVector FinalHandPos = EditAnchorPos - HandRot.RotateVector(ActiveLocOffset);

			VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), FinalHandPos), 0.0f);
		}
		return;
	}

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
				else
				{
					// Poke mode active without a hovered block: hover freely in front of cursor with Poke offsets
					bIsLockedPerpendicular = false;
					VirtualHand->SetHandPoseMode(EHandPoseMode::FingerPoke);
					FVector TargetPos = bHit ? HitResult.ImpactPoint : (WorldLocation + (WorldDirection * 60.0f));
					FRotator BaseRot = GetHorizontalFacingRotation(TargetPos);
					BaseRot.Pitch = 0.0f;
					BaseRot.Roll = 0.0f;
					FRotator HandRot = BaseRot + PokeHandRotationOffset;
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + PokeHandLocationOffset;
					FVector SafeHandPos = TargetPos - HandRot.RotateVector(LocalOffset);
					VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), SafeHandPos), 0.0f);
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
						const bool bInPoke = (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
						const FRotator ActiveRotOffset = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;
						const FVector ActiveLocOffset = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;

						VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : (bInPoke ? EHandPoseMode::FingerPoke : EHandPoseMode::OpenHand));
						FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + ActiveLocOffset;

						FVector Diff = HitResult.ImpactPoint - TowerCenter;
						float Angle = FMath::Atan2(Diff.Y, Diff.X);
						float SafeFingertipRadius = FMath::Max(FVector2D(Diff.X, Diff.Y).Size(), TOWER_BASE_RADIUS + 1.0f);
						FVector TargetFingertipPos = FVector(TowerCenter.X + FMath::Cos(Angle) * SafeFingertipRadius, TowerCenter.Y + FMath::Sin(Angle) * SafeFingertipRadius, BlockZ);
						FRotator BaseRot = GetHorizontalFacingRotation(TargetFingertipPos);
						BaseRot.Pitch = 0.0f;
						BaseRot.Roll = 0.0f;
						FRotator InspectRot = BaseRot + ActiveRotOffset;

						FVector SafeHandPos = TargetFingertipPos - InspectRot.RotateVector(LocalOffset);
						VirtualHand->SetTargetHandTransform(FTransform(InspectRot.Quaternion(), SafeHandPos), 0.0f);
					}
				}
				else if (bInProximity)
				{
					// --- PROXIMITY TO TABLE (No Block Hovered) ---
					bIsLockedPerpendicular = false;
					const bool bInPoke = (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
					const FRotator ActiveRotOffset = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;
					const FVector ActiveLocOffset = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;

					VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : (bInPoke ? EHandPoseMode::FingerPoke : EHandPoseMode::OpenHand));
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + ActiveLocOffset;

					FVector Diff = HitResult.ImpactPoint - TowerCenter;
					float Angle = FMath::Atan2(Diff.Y, Diff.X);
					float SafeFingertipRadius = FMath::Max(FVector2D(Diff.X, Diff.Y).Size(), TOWER_BASE_RADIUS + 1.0f);
					FVector TargetFingertipPos = FVector(TowerCenter.X + FMath::Cos(Angle) * SafeFingertipRadius, TowerCenter.Y + FMath::Sin(Angle) * SafeFingertipRadius, HitResult.ImpactPoint.Z);
					FRotator BaseRot = GetHorizontalFacingRotation(TargetFingertipPos);
					BaseRot.Pitch = 0.0f;
					BaseRot.Roll = 0.0f;
					FRotator InspectRot = BaseRot + ActiveRotOffset;

					FVector SafeHandPos = TargetFingertipPos - InspectRot.RotateVector(LocalOffset);
					VirtualHand->SetTargetHandTransform(FTransform(InspectRot.Quaternion(), SafeHandPos), 0.0f);
				}
				else
				{
					// --- OUTSIDE PROXIMITY: Free 3D movement ---
					bIsLockedPerpendicular = false;
					const bool bInPoke = (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
					const FRotator ActiveRotOffset = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;
					const FVector ActiveLocOffset = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;

					VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : (bInPoke ? EHandPoseMode::FingerPoke : EHandPoseMode::OpenHand));
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + ActiveLocOffset;

					FRotator BaseRot = GetHorizontalFacingRotation(HitResult.ImpactPoint);
					BaseRot.Pitch = 0.0f;
					BaseRot.Roll = 0.0f;
					FRotator HandRot = BaseRot + ActiveRotOffset;

					FVector SafeHandPos = HitResult.ImpactPoint - HandRot.RotateVector(LocalOffset);
					VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), SafeHandPos), 0.0f);
				}
			}
			else
			{
				if (bForceGesturePreview || bIsPokeModeActive)
				{
					bIsLockedPerpendicular = false;
					const bool bInPoke = (bIsPokeModeActive || (bForceGesturePreview && ActiveGesturePreview == EHandPoseMode::FingerPoke));
					const FRotator ActiveRotOffset = bInPoke ? PokeHandRotationOffset : GrabHandRotationOffset;
					const FVector ActiveLocOffset = bInPoke ? PokeHandLocationOffset : GrabHandLocationOffset;

					FVector FreeTargetPos = WorldLocation + (WorldDirection * 60.0f);
					FRotator BaseRot = (-WorldDirection).Rotation();
					BaseRot.Pitch = 0.0f;
					BaseRot.Roll = 0.0f;
					FRotator HandRot = BaseRot + ActiveRotOffset;
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset() + ActiveLocOffset;
					FVector SafeHandPos = FreeTargetPos - HandRot.RotateVector(LocalOffset);

					VirtualHand->SetHandPoseMode(bForceGesturePreview ? ActiveGesturePreview : (bInPoke ? EHandPoseMode::FingerPoke : EHandPoseMode::OpenHand));
					VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), SafeHandPos), 0.0f);
				}
				else
				{
					bIsLockedPerpendicular = false;
					VirtualHand->SetActorHiddenInGame(true);
				}
			}
		}
	}
}

void AYenkaDesktopPawn::OnPrimaryClickPressed()
{
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
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
	if (bIsNamingCustomGesture || bIsNamingCustomTransform) return;
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
