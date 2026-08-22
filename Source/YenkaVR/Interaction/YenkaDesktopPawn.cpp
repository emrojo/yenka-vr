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
	LockedPushBlock = nullptr;
}

void AYenkaDesktopPawn::OnTogglePokeMode()
{
	bIsPokeModeActive = !bIsPokeModeActive;
	if (!bIsPokeModeActive)
	{
		bIsLockedPerpendicular = false;
		LockedPushBlock = nullptr;
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

FVector AYenkaDesktopPawn::GetBlockStandOffLocation(const AYenkaBlock* Block, const FVector& ViewOrigin, FVector& OutApproachNormal, float Clearance, const FVector& LocalFingertipOffset) const
{
	if (!Block)
	{
		OutApproachNormal = FVector::ForwardVector;
		return FVector::ZeroVector;
	}

	FVector BlockCenter = Block->GetActorLocation();
	FVector ForwardVec = Block->GetActorForwardVector(); // 7.5cm length (+- 3.75cm)
	FVector RightVec = Block->GetActorRightVector();     // 2.5cm width (+- 1.25cm)

	// Block face candidate positions (exact outer surface of the Jenga piece)
	FVector EndPosPos = BlockCenter + (ForwardVec * 3.75f);
	FVector EndNegPos = BlockCenter - (ForwardVec * 3.75f);
	FVector SidePosPos = BlockCenter + (RightVec * 1.25f);
	FVector SideNegPos = BlockCenter - (RightVec * 1.25f);

	// Find the face closest to the camera/cursor ray origin
	float DistEndPos = FVector::DistSquared(EndPosPos, ViewOrigin);
	float DistEndNeg = FVector::DistSquared(EndNegPos, ViewOrigin);
	float DistSidePos = FVector::DistSquared(SidePosPos, ViewOrigin);
	float DistSideNeg = FVector::DistSquared(SideNegPos, ViewOrigin);

	FVector ChosenFacePos = EndPosPos;
	OutApproachNormal = ForwardVec;
	float MinDist = DistEndPos;

	if (DistEndNeg < MinDist)
	{
		MinDist = DistEndNeg;
		ChosenFacePos = EndNegPos;
		OutApproachNormal = -ForwardVec;
	}
	if (DistSidePos < MinDist)
	{
		MinDist = DistSidePos;
		ChosenFacePos = SidePosPos;
		OutApproachNormal = RightVec;
	}
	if (DistSideNeg < MinDist)
	{
		MinDist = DistSideNeg;
		ChosenFacePos = SideNegPos;
		OutApproachNormal = -RightVec;
	}

	// Target location for the fingertip in world space (at distance Clearance from block face)
	FVector TargetFingertipWorld = ChosenFacePos + (OutApproachNormal * Clearance);

	// The hand faces the block along -OutApproachNormal
	FRotator HandRot = (-OutApproachNormal).Rotation();
	HandRot.Pitch = 0.0f;
	HandRot.Roll = 0.0f;

	// Position HandRoot such that HandRoot + HandRot.RotateVector(LocalFingertipOffset) == TargetFingertipWorld
	// This centers the finger perfectly on the block face (both in width and height)
	return TargetFingertipWorld - HandRot.RotateVector(LocalFingertipOffset);
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

		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams);
		if (bHit)
		{
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

			AYenkaBlock* ActivePushBlock = (bIsPokeModeActive || bIsPushingBlock) ? (LockedPushBlock ? LockedPushBlock : HoveredBlock) : nullptr;
			if ((bIsPokeModeActive || bIsPushingBlock) && HoveredBlock && !LockedPushBlock)
			{
				LockedPushBlock = HoveredBlock;
				ActivePushBlock = LockedPushBlock;
			}

			if (ActivePushBlock)
			{
				// --- PUSH / POKE MODE ---
				VirtualHand->SetHandPoseMode(EHandPoseMode::FingerPoke);
				FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset(); // (6.0, -1.5, 0.0)

				FVector ApproachNormal;
				FVector DummyStandby = GetBlockStandOffLocation(ActivePushBlock, WorldLocation, ApproachNormal, PUSH_STANDBY_SEPARATION, LocalOffset);

				FRotator HandRot = (-ApproachNormal).Rotation();
				HandRot.Pitch = 0.0f;
				HandRot.Roll = 0.0f;

				LockedRadialDirection = ApproachNormal;
				LockedFloorZ = DummyStandby.Z + PUSH_VERTICAL_OFFSET;
				bIsLockedPerpendicular = true;

				// Find block face location in world space
				FVector BlockCenter = ActivePushBlock->GetActorLocation();
				FVector ForwardVec = ActivePushBlock->GetActorForwardVector();
				FVector RightVec = ActivePushBlock->GetActorRightVector();
				FVector EndPosPos = BlockCenter + (ForwardVec * 3.75f);
				FVector EndNegPos = BlockCenter - (ForwardVec * 3.75f);
				FVector SidePosPos = BlockCenter + (RightVec * 1.25f);
				FVector SideNegPos = BlockCenter - (RightVec * 1.25f);

				FVector ChosenFacePos = EndPosPos;
				float MinDist = FVector::DistSquared(EndPosPos, WorldLocation);
				if (FVector::DistSquared(EndNegPos, WorldLocation) < MinDist) { MinDist = FVector::DistSquared(EndNegPos, WorldLocation); ChosenFacePos = EndNegPos; }
				if (FVector::DistSquared(SidePosPos, WorldLocation) < MinDist) { MinDist = FVector::DistSquared(SidePosPos, WorldLocation); ChosenFacePos = SidePosPos; }
				if (FVector::DistSquared(SideNegPos, WorldLocation) < MinDist) { ChosenFacePos = SideNegPos; }

				FVector TargetFaceCenter = ChosenFacePos;
				TargetFaceCenter.Z += PUSH_VERTICAL_OFFSET;

				// Project mouse cursor onto horizontal plane at TargetFaceCenter.Z
				float t = (FMath::Abs(WorldDirection.Z) > 0.001f) ? (TargetFaceCenter.Z - WorldLocation.Z) / WorldDirection.Z : 50.0f;
				FVector MousePlanePos = WorldLocation + (WorldDirection * t);

				// Distance from the block face along the approach normal (positive = outside tower, negative = pushed into block)
				float DistFromFace = FVector::DotProduct(MousePlanePos - TargetFaceCenter, ApproachNormal);

				if (bIsPushingBlock)
				{
					// Direct click push: place fingertip at -1.5cm into block face and push
					FVector PushHandPos = GetBlockStandOffLocation(ActivePushBlock, WorldLocation, ApproachNormal, -1.5f, LocalOffset);
					PushHandPos.Z += PUSH_VERTICAL_OFFSET;
					VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), PushHandPos), 0.0f);
					PerformLongitudinalPush(ActivePushBlock, ApproachNormal);
				}
				else
				{
					// Mouse-driven advance: smoothly moves fingertip between 8.0cm standby and -2.0cm pushed into piece
					float ClampedClearance = FMath::Clamp(DistFromFace, -2.0f, PUSH_STANDBY_SEPARATION);
					FVector HandPos = GetBlockStandOffLocation(ActivePushBlock, WorldLocation, ApproachNormal, ClampedClearance, LocalOffset);
					HandPos.Z += PUSH_VERTICAL_OFFSET;
					VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), HandPos), 0.0f);

					if (ClampedClearance <= 0.0f)
					{
						// Fingertip reached contact: push the block forward
						PerformLongitudinalPush(ActivePushBlock, ApproachNormal);
					}
				}
			}
			else if (bHit)
			{
				const float DistToTowerXY = FVector::Dist2D(HitResult.ImpactPoint, TowerCenter);
				const bool bInProximity = (DistToTowerXY <= ProximityRadius);

				if (bInProximity && HoveredBlock)
				{
					// --- GRAB / INSPECTION STAND-BY ---
					bIsLockedPerpendicular = false;
					VirtualHand->SetHandPoseMode(EHandPoseMode::OpenHand);
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset();
					FVector ApproachNormal;
					FVector StandbyPos = GetBlockStandOffLocation(HoveredBlock, WorldLocation, ApproachNormal, GRAB_STANDBY_SEPARATION, LocalOffset);

					FRotator HandRot = (-ApproachNormal).Rotation();
					HandRot.Pitch = 0.0f;
					HandRot.Roll = 0.0f;

					VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), StandbyPos), 0.0f);
				}
				else if (bInProximity)
				{
					// --- PROXIMITY TO TABLE (No Block Hovered) ---
					bIsLockedPerpendicular = false;
					FVector Diff = HitResult.ImpactPoint - TowerCenter;
					float Angle = FMath::Atan2(Diff.Y, Diff.X);
					float R = FMath::Max(FVector2D(Diff.X, Diff.Y).Size(), INSPECTION_SAFE_RADIUS);
					FVector SafeHandPos = TowerCenter + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Diff.Z);
					FRotator InspectRot = GetHorizontalFacingRotation(SafeHandPos);
					VirtualHand->SetHandPoseMode(EHandPoseMode::OpenHand);
					VirtualHand->SetTargetHandTransform(FTransform(InspectRot.Quaternion(), SafeHandPos), 0.0f);
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
	AYenkaBlock* TargetPush = LockedPushBlock ? LockedPushBlock : HoveredBlock;
	if (bIsPokeModeActive && TargetPush)
	{
		// In poke mode, clicking directly pushes the block along the locked perpendicular axis
		bIsPushingBlock = true;
		PerformLongitudinalPush(TargetPush, LockedRadialDirection);
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
