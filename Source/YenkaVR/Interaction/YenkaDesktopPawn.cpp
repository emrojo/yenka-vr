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
	bIsPokeModeActive = true;
	CurrentPushAdvance = 0.0f;
}

void AYenkaDesktopPawn::OnPokeKeyReleased()
{
	bIsPokeModeActive = false;
	bIsPushingBlock = false;
	bIsLockedPerpendicular = false;
	LockedPushBlock = nullptr;
	CurrentPushAdvance = 0.0f;
}

void AYenkaDesktopPawn::OnTogglePokeMode()
{
	bIsPokeModeActive = !bIsPokeModeActive;
	CurrentPushAdvance = 0.0f;
	if (!bIsPokeModeActive)
	{
		bIsPushingBlock = false;
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
	FVector PushVel = PushAxis * 4.0f;
	PushVel.Z = 0.0f;
	Block->BlockMesh->SetPhysicsLinearVelocity(PushVel);
	Block->BlockMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
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
	else if (bIsPokeModeActive && FMath::Abs(Val) > 0.001f)
	{
		// Moving mouse forward (+Val) advances finger towards block and pushes; moving backward (-Val) retracts
		CurrentPushAdvance = FMath::Clamp(CurrentPushAdvance + (Val * 0.25f), 0.0f, PUSH_STANDBY_SEPARATION + 0.5f);

		if (Val > 0.01f && CurrentPushAdvance >= PUSH_STANDBY_SEPARATION)
		{
			AYenkaBlock* ActivePushBlock = LockedPushBlock ? LockedPushBlock : HoveredBlock;
			if (ActivePushBlock && ActivePushBlock->BlockMesh)
			{
				ActivePushBlock->BlockMesh->WakeRigidBody();
				FVector PushVel = PushLongitudinalAxis * (FMath::Clamp(Val * 12.0f, 2.0f, 6.0f));
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

FVector AYenkaDesktopPawn::GetBlockChosenFacePos(const AYenkaBlock* Block, const FVector& ApproachNormal) const
{
	if (!Block) return FVector::ZeroVector;

	FVector BlockCenter = Block->GetActorLocation();
	FVector ForwardVec = Block->GetActorForwardVector(); // 7.5cm length (+- 3.75cm)
	FVector RightVec = Block->GetActorRightVector();     // 2.5cm width (+- 1.25cm)

	FVector EndPosPos = BlockCenter + (ForwardVec * 3.75f);
	FVector EndNegPos = BlockCenter - (ForwardVec * 3.75f);
	FVector SidePosPos = BlockCenter + (RightVec * 1.25f);
	FVector SideNegPos = BlockCenter - (RightVec * 1.25f);

	float DotEndPos = FVector::DotProduct(ApproachNormal, ForwardVec);
	float DotEndNeg = FVector::DotProduct(ApproachNormal, -ForwardVec);
	float DotSidePos = FVector::DotProduct(ApproachNormal, RightVec);
	float DotSideNeg = FVector::DotProduct(ApproachNormal, -RightVec);

	if (DotEndPos >= DotEndNeg && DotEndPos >= DotSidePos && DotEndPos >= DotSideNeg) return EndPosPos;
	if (DotEndNeg >= DotSidePos && DotEndNeg >= DotSideNeg) return EndNegPos;
	if (DotSidePos >= DotSideNeg) return SidePosPos;
	return SideNegPos;
}

bool AYenkaDesktopPawn::IsBlockProtruding(const AYenkaBlock* Block, FVector& OutProtrudingEdgePos, FVector& OutProtrudingNormal) const
{
	if (!Block) return false;

	AActor* TowerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass());
	FVector TowerCenter = TowerActor ? TowerActor->GetActorLocation() : FVector(0.0f, 0.0f, 85.0f);
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

			if (bIsPokeModeActive || bIsPushingBlock)
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
					FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset(); // (6.0, -1.5, 0.0)

					FVector ApproachNormal;
					GetBlockStandOffLocation(ActivePushBlock, WorldLocation, ApproachNormal, PUSH_STANDBY_SEPARATION, LocalOffset);

					// Calculate longitudinal push axis (facing inward towards tower)
					FVector ForwardVec = ActivePushBlock->GetActorForwardVector();
					FVector RightVec = ActivePushBlock->GetActorRightVector();
					float DotFwd = FVector::DotProduct(-ApproachNormal, ForwardVec);
					float DotRt = FVector::DotProduct(-ApproachNormal, RightVec);
					PushLongitudinalAxis = (FMath::Abs(DotFwd) >= FMath::Abs(DotRt)) ? (ForwardVec * FMath::Sign(DotFwd)) : (RightVec * FMath::Sign(DotRt));
					PushApproachNormal = ApproachNormal;

					FRotator HandRot = (-ApproachNormal).Rotation();
					HandRot.Pitch = 0.0f;
					HandRot.Roll = 0.0f;

					LockedRadialDirection = ApproachNormal;
					bIsLockedPerpendicular = true;

					// Find block face location in world space
					FVector CurrentFacePos = GetBlockChosenFacePos(ActivePushBlock, ApproachNormal);

					if (bIsPushingBlock && ActivePushBlock->BlockMesh)
					{
						CurrentPushAdvance = PUSH_STANDBY_SEPARATION;
						ActivePushBlock->BlockMesh->WakeRigidBody();
						FVector PushVel = PushLongitudinalAxis * 4.0f;
						PushVel.Z = 0.0f;
						ActivePushBlock->BlockMesh->SetPhysicsLinearVelocity(PushVel);
						ActivePushBlock->BlockMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
					}

					float EffectiveAdvance = CurrentPushAdvance;

					if (EffectiveAdvance < PUSH_STANDBY_SEPARATION)
					{
						// In the air (Standby to Contact): Hand moves freely towards the block surface
						float Clearance = PUSH_STANDBY_SEPARATION - EffectiveAdvance; // 3cm -> 0cm
						FVector TargetFingertipWorld = CurrentFacePos + (ApproachNormal * Clearance);
						FVector HandPos = TargetFingertipWorld - HandRot.RotateVector(LocalOffset);
						VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), HandPos), 0.0f);
					}
					else
					{
						// In Contact & Pushing: Fingertip stays EXACTLY ON THE SURFACE (Clearance = 0.0cm)!
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
					FVector ProtrudingPos, ProtrudingNorm;
					bool bIsProtruding = IsBlockProtruding(HoveredBlock, ProtrudingPos, ProtrudingNorm);

					if (bIsProtruding)
					{
						// Block is protruding: position hand at protruding edge ready to grab
						bIsLockedPerpendicular = false;
						VirtualHand->SetHandPoseMode(EHandPoseMode::GrabPinch);
						FVector LocalOffset = VirtualHand->GetExtendedFingertipLocalOffset();
						FVector StandbyPos = ProtrudingPos + (ProtrudingNorm * GRAB_STANDBY_SEPARATION);

						FRotator HandRot = (-ProtrudingNorm).Rotation();
						HandRot.Pitch = 0.0f;
						HandRot.Roll = 0.0f;

						VirtualHand->SetTargetHandTransform(FTransform(HandRot.Quaternion(), StandbyPos), 0.0f);
					}
					else
					{
						// Block is flush/not protruding: hand hovers outside in inspection mode
						bIsLockedPerpendicular = false;
						VirtualHand->SetHandPoseMode(EHandPoseMode::OpenHand);
						FVector Diff = HitResult.ImpactPoint - TowerCenter;
						float Angle = FMath::Atan2(Diff.Y, Diff.X);
						float R = FMath::Max(FVector2D(Diff.X, Diff.Y).Size(), INSPECTION_SAFE_RADIUS);
						FVector SafeHandPos = TowerCenter + FVector(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Diff.Z);
						FRotator InspectRot = GetHorizontalFacingRotation(SafeHandPos);
						VirtualHand->SetTargetHandTransform(FTransform(InspectRot.Quaternion(), SafeHandPos), 0.0f);
					}
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
		// In poke mode, clicking smoothly advances to contact and pushes along the locked axis
		bIsPushingBlock = true;
		if (!LockedPushBlock)
		{
			LockedPushBlock = TargetPush;
			PushBlockInitialLocation = LockedPushBlock->GetActorLocation();
			CurrentPushDisplacement = 0.0f;
		}
		CurrentPushAdvance = PUSH_STANDBY_SEPARATION;
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

		APlayerController* PC = Cast<APlayerController>(GetController());
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
		FRotator HandRot = GetHorizontalFacingRotation(ProtrudingPos);
		FTransform HandTarget(HandRot.Quaternion(), ProtrudingPos);
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
