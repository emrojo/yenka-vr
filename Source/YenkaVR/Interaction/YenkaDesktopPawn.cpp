#include "YenkaDesktopPawn.h"
#include "YenkaHandAvatar.h"
#include "YenkaVR/Physics/YenkaBlock.h"
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
	LockedFloorZ = 85.0f;
	GrabDistance = 65.0f;
	LastHitLocation = FVector::ZeroVector;
	LastHitNormal = FVector::UpVector;
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
		SetActorLocation(FVector(0.0f, 0.0f, 85.0f));
	}
	CameraBoom->TargetArmLength = 65.0f;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && IsLocallyControlled())
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
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
		PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AYenkaDesktopPawn::OnMouseX);
		PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AYenkaDesktopPawn::OnMouseY);
		PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &AYenkaDesktopPawn::OnMouseWheel);
		PlayerInputComponent->BindAxisKey(EKeys::W, this, &AYenkaDesktopPawn::MoveForward);
		PlayerInputComponent->BindAxisKey(EKeys::S, this, &AYenkaDesktopPawn::MoveForward);
		PlayerInputComponent->BindAxisKey(EKeys::D, this, &AYenkaDesktopPawn::MoveRight);
		PlayerInputComponent->BindAxisKey(EKeys::A, this, &AYenkaDesktopPawn::MoveRight);
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
	bIsPushingBlock = true;
	if (HoveredBlock)
	{
		PerformLongitudinalPush(HoveredBlock, LastHitNormal);
	}
}

void AYenkaDesktopPawn::OnPokeKeyReleased()
{
	bIsPushingBlock = false;
	bIsLockedPerpendicular = false;
}

void AYenkaDesktopPawn::OnTogglePokeMode()
{
	bIsPokeModeActive = !bIsPokeModeActive;
	if (!bIsPokeModeActive)
	{
		bIsLockedPerpendicular = false;
	}
}

void AYenkaDesktopPawn::PerformLongitudinalPush(AYenkaBlock* Block, const FVector& DirectionNormal)
{
	if (!Block || !Block->BlockMesh) return;

	// Calculate longitudinal axis of the block
	FVector ForwardVec = Block->GetActorForwardVector();
	FVector RightVec = Block->GetActorRightVector();

	float DotForward = FVector::DotProduct(-DirectionNormal, ForwardVec);
	float DotRight = FVector::DotProduct(-DirectionNormal, RightVec);

	FVector PushAxis;
	if (FMath::Abs(DotForward) >= FMath::Abs(DotRight))
	{
		PushAxis = ForwardVec * FMath::Sign(DotForward);
	}
	else
	{
		PushAxis = RightVec * FMath::Sign(DotRight);
	}

	Block->SetPhysicsActive(true);
	Block->BlockMesh->WakeRigidBody();
	Block->BlockMesh->AddImpulse(PushAxis * 0.05f, NAME_None, true);
}

FRotator AYenkaDesktopPawn::GetHorizontalFacingRotation(const FVector& TargetLocation) const
{
	AActor* TowerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass());
	FVector TowerCenter = TowerActor ? TowerActor->GetActorLocation() : FVector(0.0f, 0.0f, 85.0f);
	FVector FacingDir = (TowerCenter - TargetLocation);
	FacingDir.Z = 0.0f; // strictly on XY horizontal plane

	if (FacingDir.IsNearlyZero(0.1f))
	{
		FacingDir = FollowCamera ? FollowCamera->GetForwardVector() : FVector::ForwardVector;
		FacingDir.Z = 0.0f;
	}

	FRotator FacingRot = FacingDir.GetSafeNormal().Rotation();
	FacingRot.Pitch = 0.0f; // strictly horizontal
	FacingRot.Roll = 0.0f;  // strictly level
	return FacingRot;
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

void AYenkaDesktopPawn::HandleMouseTrace()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	AActor* TowerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass());
	FVector TowerCenter = TowerActor ? TowerActor->GetActorLocation() : FVector(0.0f, 0.0f, 85.0f);
	const float ProximityRadius = TOWER_BASE_RADIUS + PROXIMITY_THRESHOLD; // 3.75 + 22.0 = 25.75 cm

	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		// 1. If currently dragging a block with Physics Handle
		if (GrabbedBlock && VirtualHand && VirtualHand->PhysicsHandle)
		{
			FVector DragTargetLocation = WorldLocation + (WorldDirection * GrabDistance);
			VirtualHand->PhysicsHandle->SetTargetLocation(DragTargetLocation);
			FRotator HandRot = GetHorizontalFacingRotation(DragTargetLocation);
			FTransform HandTarget(HandRot.Quaternion(), DragTargetLocation);
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

		if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			AYenkaBlock* HitBlock = Cast<AYenkaBlock>(HitResult.GetActor());
			HoveredBlock = HitBlock;
			LastHitLocation = HitResult.ImpactPoint;
			LastHitNormal = HitResult.ImpactNormal;

			if (VirtualHand)
			{
				VirtualHand->SetActorHiddenInGame(false);

				const float DistToTowerXY = FVector::Dist2D(HitResult.ImpactPoint, TowerCenter);
				const bool bInProximity = (DistToTowerXY <= ProximityRadius);

				if (bInProximity)
				{
					// --- PROXIMITY ZONE: Strict Horizontal Orientation (Pitch = 0, Roll = 0) ---

					if (bIsPokeModeActive || bIsPushingBlock)
					{
						// --- PUSH / POKE MODE: Lock mouse movement strictly perpendicular to tower ---
						if (!bIsLockedPerpendicular || HoveredBlock)
						{
							FVector RadialDiff = HitResult.ImpactPoint - TowerCenter;
							RadialDiff.Z = 0.0f;
							if (!RadialDiff.IsNearlyZero(0.1f))
							{
								LockedRadialDirection = RadialDiff.GetSafeNormal();
							}
							else
							{
								LockedRadialDirection = (-FollowCamera->GetForwardVector()).GetSafeNormal2D();
							}
							LockedFloorZ = HitResult.ImpactPoint.Z;
							bIsLockedPerpendicular = true;
						}

						// Project mouse cursor along the locked perpendicular line
						float t = (FMath::Abs(WorldDirection.Z) > 0.001f) ? (LockedFloorZ - WorldLocation.Z) / WorldDirection.Z : 50.0f;
						FVector MousePlanePos = WorldLocation + (WorldDirection * t);
						float RadialDist = FVector::DotProduct(MousePlanePos - TowerCenter, LockedRadialDirection);
						RadialDist = FMath::Clamp(RadialDist, TOWER_BASE_RADIUS + 0.5f, ProximityRadius);

						FVector HandPos = TowerCenter + (LockedRadialDirection * RadialDist);
						HandPos.Z = LockedFloorZ;

						FRotator PokeRot = (-LockedRadialDirection).Rotation();
						PokeRot.Pitch = 0.0f;
						PokeRot.Roll = 0.0f;

						VirtualHand->SetHandPoseMode(EHandPoseMode::FingerPoke);
						VirtualHand->SetTargetHandTransform(FTransform(PokeRot.Quaternion(), HandPos), 0.0f);

						if (bIsPushingBlock && HoveredBlock)
						{
							PerformLongitudinalPush(HoveredBlock, LockedRadialDirection);
						}
					}
					else
					{
						// --- INSPECTION MODE: Allow only movement around the perimeter, cannot approach tower ---
						bIsLockedPerpendicular = false;

						FVector Diff = HitResult.ImpactPoint - TowerCenter;
						float Angle = FMath::Atan2(Diff.Y, Diff.X);
						float R = FMath::Max(FVector2D(Diff.X, Diff.Y).Size(), INSPECTION_SAFE_RADIUS);

						FVector SafeHandPos = TowerCenter + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Diff.Z);

						FRotator InspectRot = GetHorizontalFacingRotation(SafeHandPos);
						VirtualHand->SetHandPoseMode(EHandPoseMode::OpenHand);
						VirtualHand->SetTargetHandTransform(FTransform(InspectRot.Quaternion(), SafeHandPos), 0.0f);
					}
				}
				else
				{
					// --- OUTSIDE PROXIMITY: Free 3D movement ---
					bIsLockedPerpendicular = false;
					VirtualHand->SetHandPoseMode(EHandPoseMode::OpenHand);
					FTransform HandTarget(FRotationMatrix::MakeFromX(WorldDirection).ToQuat(), HitResult.ImpactPoint);
					VirtualHand->SetTargetHandTransform(HandTarget, 0.0f);
				}
			}
		}
		else
		{
			HoveredBlock = nullptr;
			bIsLockedPerpendicular = false;
			if (VirtualHand)
			{
				VirtualHand->SetActorHiddenInGame(true);
			}
		}
	}
}

void AYenkaDesktopPawn::OnPrimaryClickPressed()
{
	if (bIsPokeModeActive && HoveredBlock)
	{
		// In poke mode, clicking directly pushes the block along the locked perpendicular axis
		bIsPushingBlock = true;
		PerformLongitudinalPush(HoveredBlock, LockedRadialDirection);
		return;
	}

	if (HoveredBlock && VirtualHand && VirtualHand->PhysicsHandle)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			FVector WorldLocation, WorldDirection;
			if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
			{
				GrabDistance = FMath::Clamp((LastHitLocation - WorldLocation).Size(), 20.0f, 250.0f);
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
			LastHitLocation,
			GrabbedBlock->GetActorRotation()
		);
		FRotator HandRot = GetHorizontalFacingRotation(LastHitLocation);
		FTransform HandTarget(HandRot.Quaternion(), LastHitLocation);
		VirtualHand->SetTargetHandTransform(HandTarget, 1.0f);
	}
}

void AYenkaDesktopPawn::OnPrimaryClickReleased()
{
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
