#include "YenkaVRPawn.h"
#include "YenkaHandAvatar.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AYenkaVRPawn::AYenkaVRPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	RootComponent = VROrigin;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(VROrigin);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VROrigin);
	LeftController->SetTrackingMotionSource(FName("Left"));

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VROrigin);
	RightController->SetTrackingMotionSource(FName("Right"));

	TeleportSpline = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportSpline"));
	TeleportSpline->SetupAttachment(VROrigin);
	TeleportSpline->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TeleportTargetRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportTargetRing"));
	TeleportTargetRing->SetupAttachment(VROrigin);
	TeleportTargetRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TeleportTargetRing->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderAsset.Succeeded())
	{
		SplineCylinderMesh = CylinderAsset.Object;
		TeleportTargetRing->SetStaticMesh(CylinderAsset.Object);
		TeleportTargetRing->SetRelativeScale3D(FVector(0.40f, 0.40f, 0.005f)); // Disc ring on ground (40cm radius)
	}

	HandAvatarClass = AYenkaHandAvatar::StaticClass();

	// Teleport defaults
	TeleportLaunchSpeed = 900.0f; // 9 m/s launch velocity
	MaxTeleportDistance = 1200.0f; // 12 m max reach
	TeleportArcRadius = 1.2f; // 1.2cm beam radius
	ValidTeleportColor = FLinearColor(0.0f, 0.85f, 1.0f, 1.0f); // Bright luminous cyan
	InvalidTeleportColor = FLinearColor(1.0f, 0.15f, 0.05f, 1.0f); // Bright neon red
	SnapTurnAngle = 45.0f;

	bIsTeleportAiming = false;
	bTeleportUsingLeftHand = true;
	bIsTeleportTargetValid = false;
	TeleportTargetLocation = FVector::ZeroVector;
	TeleportTargetNormal = FVector::UpVector;

	bSnapTurnAxisReset = true;
	SnapTurnCooldownTimer = 0.0f;

	bIsLeftGrabbingSpace = false;
	bIsRightGrabbingSpace = false;
	InitialPinchDistance = 1.0f;
}

void AYenkaVRPawn::BeginPlay()
{
	Super::BeginPlay();

	// Position VR player standing directly in front of the Yenka table facing +X towards the tower
	SetActorLocationAndRotation(FVector(-70.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	if (VROrigin)
	{
		VROrigin->SetWorldLocationAndRotation(FVector(-70.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	}

	// Create dynamic material instances for the teleport arc and ground target marker
	UMaterialInterface* BaseMat = TeleportTargetRing ? TeleportTargetRing->GetMaterial(0) : nullptr;
	if (BaseMat)
	{
		ArcMaterialInstance = UMaterialInstanceDynamic::Create(BaseMat, this);
		RingMaterialInstance = UMaterialInstanceDynamic::Create(BaseMat, this);

		if (ArcMaterialInstance)
		{
			ArcMaterialInstance->SetVectorParameterValue(TEXT("Color"), ValidTeleportColor);
			ArcMaterialInstance->SetScalarParameterValue(TEXT("Roughness"), 0.2f);
			ArcMaterialInstance->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
		}

		if (RingMaterialInstance)
		{
			RingMaterialInstance->SetVectorParameterValue(TEXT("Color"), ValidTeleportColor);
			RingMaterialInstance->SetScalarParameterValue(TEXT("Roughness"), 0.2f);
			RingMaterialInstance->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			TeleportTargetRing->SetMaterial(0, RingMaterialInstance);
		}
	}

	TeleportTargetRing->SetVisibility(false);

	if (IsLocallyControlled() && HandAvatarClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		LeftHandAvatar = GetWorld()->SpawnActor<AYenkaHandAvatar>(HandAvatarClass, GetActorTransform(), SpawnParams);
		if (LeftHandAvatar)
		{
			LeftHandAvatar->SetIsLeftHand(true);
			LeftHandAvatar->AttachToComponent(LeftController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}

		RightHandAvatar = GetWorld()->SpawnActor<AYenkaHandAvatar>(HandAvatarClass, GetActorTransform(), SpawnParams);
		if (RightHandAvatar)
		{
			RightHandAvatar->SetIsLeftHand(false);
			RightHandAvatar->AttachToComponent(RightController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}
}

#include "YenkaVR/UI/YenkaScenarioMenu.h"
#include "YenkaVR/Environment/YenkaEnvironmentManager.h"

void AYenkaVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SnapTurnCooldownTimer > 0.0f)
	{
		SnapTurnCooldownTimer -= DeltaTime;
	}

	if (IsLocallyControlled())
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			// 1. Direct Hardware Key Polling for Left Hand (Thumbstick Y / Trigger / Grip / Face Buttons)
			float LeftY = PC->GetInputAnalogKeyState(EKeys::OculusTouch_Left_Thumbstick_Y);
			if (FMath::IsNearlyZero(LeftY)) LeftY = PC->GetInputAnalogKeyState(EKeys::MixedReality_Left_Thumbstick_Y);
			if (FMath::IsNearlyZero(LeftY)) LeftY = PC->GetInputAnalogKeyState(EKeys::ValveIndex_Left_Thumbstick_Y);
			if (FMath::IsNearlyZero(LeftY)) LeftY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
			if (FMath::IsNearlyZero(LeftY)) LeftY = PC->GetInputAxisValue(TEXT("TeleportAimLeftY"));

			// 2. Direct Hardware Key Polling for Right Hand (Thumbstick Y)
			float RightY = PC->GetInputAnalogKeyState(EKeys::OculusTouch_Right_Thumbstick_Y);
			if (FMath::IsNearlyZero(RightY)) RightY = PC->GetInputAnalogKeyState(EKeys::MixedReality_Right_Thumbstick_Y);
			if (FMath::IsNearlyZero(RightY)) RightY = PC->GetInputAnalogKeyState(EKeys::ValveIndex_Right_Thumbstick_Y);
			if (FMath::IsNearlyZero(RightY)) RightY = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY);
			if (FMath::IsNearlyZero(RightY)) RightY = PC->GetInputAxisValue(TEXT("TeleportAimRightY"));

			// 3. Triggers & Grips
			const float LeftTrigger = FMath::Max(PC->GetInputAnalogKeyState(EKeys::OculusTouch_Left_Trigger_Axis), PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis));
			const float RightTrigger = FMath::Max(PC->GetInputAnalogKeyState(EKeys::OculusTouch_Right_Trigger_Axis), PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis));
			const float LeftGrip = PC->GetInputAnalogKeyState(EKeys::OculusTouch_Left_Grip_Axis);
			const float RightGrip = PC->GetInputAnalogKeyState(EKeys::OculusTouch_Right_Grip_Axis);

			// 4. Buttons & Keyboard
			const bool bLeftBtn = PC->IsInputKeyDown(EKeys::OculusTouch_Left_X_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Left_Y_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Left_Trigger_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Left_Grip_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Left_Thumbstick_Click)
				|| PC->IsInputKeyDown(EKeys::Gamepad_LeftShoulder)
				|| PC->IsInputKeyDown(EKeys::Gamepad_LeftThumbstick)
				|| PC->IsInputKeyDown(EKeys::T)
				|| PC->IsInputKeyDown(EKeys::SpaceBar);

			const bool bRightBtn = PC->IsInputKeyDown(EKeys::OculusTouch_Right_A_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Right_B_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Right_Trigger_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Right_Grip_Click)
				|| PC->IsInputKeyDown(EKeys::OculusTouch_Right_Thumbstick_Click)
				|| PC->IsInputKeyDown(EKeys::Gamepad_RightShoulder)
				|| PC->IsInputKeyDown(EKeys::Gamepad_RightThumbstick);

			// Raycast from active motion controller to interact with the 3D Scenario Menu
			UMotionControllerComponent* ActiveMC = RightController ? RightController : LeftController;
			if (ActiveMC)
			{
				FHitResult MenuHit;
				FVector TraceStart = ActiveMC->GetComponentLocation();
				FVector TraceEnd = TraceStart + (ActiveMC->GetForwardVector() * 300.0f);
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);
				if (LeftHandAvatar) QueryParams.AddIgnoredActor(LeftHandAvatar);
				if (RightHandAvatar) QueryParams.AddIgnoredActor(RightHandAvatar);

				if (GetWorld()->LineTraceSingleByChannel(MenuHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
				{
					if (AYenkaScenarioMenu* ScenarioMenu = Cast<AYenkaScenarioMenu>(MenuHit.GetActor()))
					{
						const bool bTriggerClick = (RightTrigger > 0.65f) || (LeftTrigger > 0.65f)
							|| PC->IsInputKeyDown(EKeys::OculusTouch_Right_Trigger_Click)
							|| PC->IsInputKeyDown(EKeys::OculusTouch_Left_Trigger_Click);

						ScenarioMenu->ProcessRayHit(MenuHit, bTriggerClick);
					}
				}
			}

			// Left hand teleport activation
			if (LeftY > 0.40f || LeftTrigger > 0.40f || LeftGrip > 0.40f || bLeftBtn)
			{
				if (!bIsTeleportAiming)
				{
					StartTeleportTrace(true);
				}
			}
			// Right hand teleport activation
			else if (RightY > 0.40f || RightTrigger > 0.40f || RightGrip > 0.40f || bRightBtn)
			{
				if (!bIsTeleportAiming)
				{
					StartTeleportTrace(false);
				}
			}
			// Releasing trigger/thumbstick executes teleport
			else if (bIsTeleportAiming)
			{
				ExecuteTeleport();
			}

			// Snap Turn polling (Right Thumbstick X / Gamepad Right X / Keys Q & E)
			float SnapTurnX = PC->GetInputAnalogKeyState(EKeys::OculusTouch_Right_Thumbstick_X);
			if (FMath::IsNearlyZero(SnapTurnX)) SnapTurnX = PC->GetInputAnalogKeyState(EKeys::MixedReality_Right_Thumbstick_X);
			if (FMath::IsNearlyZero(SnapTurnX)) SnapTurnX = PC->GetInputAnalogKeyState(EKeys::ValveIndex_Right_Thumbstick_X);
			if (FMath::IsNearlyZero(SnapTurnX)) SnapTurnX = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);
			if (FMath::IsNearlyZero(SnapTurnX)) SnapTurnX = PC->GetInputAxisValue(TEXT("SnapTurnX"));
			if (PC->IsInputKeyDown(EKeys::E)) SnapTurnX = 1.0f;
			if (PC->IsInputKeyDown(EKeys::Q)) SnapTurnX = -1.0f;

			if (!bIsTeleportAiming && FMath::Abs(SnapTurnX) > 0.50f)
			{
				ExecuteSnapTurn(SnapTurnX);
			}
			else if (FMath::Abs(SnapTurnX) < 0.30f)
			{
				bSnapTurnAxisReset = true;
			}
		}

		if (bIsTeleportAiming)
		{
			UpdateTeleportTrace();
		}

		UpdateSpectatorGestures();
		UpdateExpressiveHandTracking(DeltaTime);
		UpdateGravityGloves(DeltaTime);
		UpdateFlyingBlocks(DeltaTime);
	}
}

void AYenkaVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent) return;

	// Teleport Aim & Execute bindings (Left Thumbstick Y / Gamepad Y)
	PlayerInputComponent->BindAxis(TEXT("OculusTouch_Left_Thumbstick_Y"), this, &AYenkaVRPawn::OnLeftThumbstickY);
	PlayerInputComponent->BindAxis(TEXT("MixedReality_Left_Thumbstick_Y"), this, &AYenkaVRPawn::OnLeftThumbstickY);
	PlayerInputComponent->BindAxis(TEXT("ValveIndex_Left_Thumbstick_Y"), this, &AYenkaVRPawn::OnLeftThumbstickY);
	PlayerInputComponent->BindAxis(TEXT("Gamepad_LeftY"), this, &AYenkaVRPawn::OnLeftThumbstickY);

	// Right hand teleport alternate binding
	PlayerInputComponent->BindAxis(TEXT("OculusTouch_Right_Thumbstick_Y"), this, &AYenkaVRPawn::OnRightThumbstickY);
	PlayerInputComponent->BindAxis(TEXT("MixedReality_Right_Thumbstick_Y"), this, &AYenkaVRPawn::OnRightThumbstickY);
	PlayerInputComponent->BindAxis(TEXT("ValveIndex_Right_Thumbstick_Y"), this, &AYenkaVRPawn::OnRightThumbstickY);

	// Snap Turn bindings (Right Thumbstick X / Gamepad Right X)
	PlayerInputComponent->BindAxis(TEXT("OculusTouch_Right_Thumbstick_X"), this, &AYenkaVRPawn::OnRightThumbstickX);
	PlayerInputComponent->BindAxis(TEXT("MixedReality_Right_Thumbstick_X"), this, &AYenkaVRPawn::OnRightThumbstickX);
	PlayerInputComponent->BindAxis(TEXT("ValveIndex_Right_Thumbstick_X"), this, &AYenkaVRPawn::OnRightThumbstickX);
	PlayerInputComponent->BindAxis(TEXT("Gamepad_RightX"), this, &AYenkaVRPawn::OnRightThumbstickX);

	PlayerInputComponent->BindAxis(TEXT("OculusTouch_Left_Thumbstick_X"), this, &AYenkaVRPawn::OnLeftThumbstickX);
	PlayerInputComponent->BindAxis(TEXT("Gamepad_LeftX"), this, &AYenkaVRPawn::OnLeftThumbstickX);
}

void AYenkaVRPawn::OnLeftThumbstickY(float Value)
{
	if (Value > 0.55f)
	{
		// Forward tilt on left thumbstick activates teleport aiming with Left Controller
		if (!bIsTeleportAiming)
		{
			StartTeleportTrace(true);
		}
	}
	else if (Value < 0.20f && bIsTeleportAiming && bTeleportUsingLeftHand)
	{
		// Releasing thumbstick executes the teleportation
		ExecuteTeleport();
	}
}

void AYenkaVRPawn::OnRightThumbstickY(float Value)
{
	if (Value > 0.55f)
	{
		// Forward tilt on right thumbstick activates teleport aiming with Right Controller
		if (!bIsTeleportAiming)
		{
			StartTeleportTrace(false);
		}
	}
	else if (Value < 0.20f && bIsTeleportAiming && !bTeleportUsingLeftHand)
	{
		// Releasing thumbstick executes the teleportation
		ExecuteTeleport();
	}
}

void AYenkaVRPawn::OnRightThumbstickX(float Value)
{
	if (!bIsTeleportAiming)
	{
		ExecuteSnapTurn(Value);
	}
}

void AYenkaVRPawn::OnLeftThumbstickX(float Value)
{
	if (!bIsTeleportAiming)
	{
		ExecuteSnapTurn(Value);
	}
}

void AYenkaVRPawn::StartTeleportTrace(bool bIsLeft)
{
	bIsTeleportAiming = true;
	bTeleportUsingLeftHand = bIsLeft;
	bIsTeleportTargetValid = false;
	UpdateTeleportTrace();
}

void AYenkaVRPawn::UpdateTeleportTrace()
{
	UMotionControllerComponent* ActiveMC = bTeleportUsingLeftHand ? LeftController : RightController;
	if (!ActiveMC) return;

	const FVector StartPos = ActiveMC->GetComponentLocation();
	const FVector LaunchDir = ActiveMC->GetForwardVector();
	const FVector LaunchVelocity = LaunchDir * TeleportLaunchSpeed;

	FPredictProjectilePathParams PathParams;
	PathParams.StartLocation = StartPos;
	PathParams.LaunchVelocity = LaunchVelocity;
	PathParams.bTraceWithCollision = true;
	PathParams.ProjectileRadius = TeleportArcRadius;
	PathParams.MaxSimTime = 2.0f;
	PathParams.bTraceWithChannel = true;
	PathParams.TraceChannel = ECC_Visibility;
	PathParams.SimFrequency = 25.0f;
	PathParams.ActorsToIgnore.Add(this);
	if (LeftHandAvatar) PathParams.ActorsToIgnore.Add(LeftHandAvatar);
	if (RightHandAvatar) PathParams.ActorsToIgnore.Add(RightHandAvatar);

	FPredictProjectilePathResult PathResult;
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if (TeleportSpline)
	{
		TeleportSpline->ClearSplinePoints(false);
		for (const FPredictProjectilePathPointData& PointData : PathResult.PathData)
		{
			TeleportSpline->AddSplinePoint(PointData.Location, ESplineCoordinateSpace::World, false);
		}
		TeleportSpline->UpdateSpline();
	}

	if (bHit && PathResult.HitResult.bBlockingHit)
	{
		TeleportTargetLocation = PathResult.HitResult.ImpactPoint;
		TeleportTargetNormal = PathResult.HitResult.ImpactNormal;

		// Surface is valid if horizontal floor (Normal Z >= 0.65) and not hitting the elevated table top (Z <= 70cm)
		const bool bIsFloorSlope = TeleportTargetNormal.Z >= 0.65f;
		const bool bIsNotTable = TeleportTargetLocation.Z <= 70.0f; // Table is at Z = 90cm
		const float HorizontalDist = FVector::Dist2D(StartPos, TeleportTargetLocation);

		bIsTeleportTargetValid = bIsFloorSlope && bIsNotTable && (HorizontalDist <= MaxTeleportDistance);
	}
	else
	{
		bIsTeleportTargetValid = false;
		if (PathResult.PathData.Num() > 0)
		{
			TeleportTargetLocation = PathResult.PathData.Last().Location;
			TeleportTargetNormal = FVector::UpVector;
		}
	}

	// Update Target Ring on floor
	if (TeleportTargetRing)
	{
		TeleportTargetRing->SetVisibility(true);
		TeleportTargetRing->SetWorldLocation(TeleportTargetLocation + TeleportTargetNormal * 0.5f);
		TeleportTargetRing->SetWorldRotation(TeleportTargetNormal.Rotation() + FRotator(-90.0f, 0.0f, 0.0f));
	}

	// Update Dynamic Materials Color
	const FLinearColor CurrentColor = bIsTeleportTargetValid ? ValidTeleportColor : InvalidTeleportColor;
	if (ArcMaterialInstance)
	{
		ArcMaterialInstance->SetVectorParameterValue(TEXT("Color"), CurrentColor);
	}
	if (RingMaterialInstance)
	{
		RingMaterialInstance->SetVectorParameterValue(TEXT("Color"), CurrentColor);
	}

	BuildSplineMeshes();
}

void AYenkaVRPawn::BuildSplineMeshes()
{
	if (!TeleportSpline || !SplineCylinderMesh) return;

	const int32 NumPoints = TeleportSpline->GetNumberOfSplinePoints();
	const int32 NumSegments = FMath::Max(0, NumPoints - 1);

	// Ensure pool size matches segment count
	while (SplineMeshPool.Num() < NumSegments)
	{
		USplineMeshComponent* NewSplineMesh = NewObject<USplineMeshComponent>(this);
		NewSplineMesh->SetStaticMesh(SplineCylinderMesh);
		NewSplineMesh->SetMobility(EComponentMobility::Movable);
		NewSplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewSplineMesh->SetCastShadow(false);
		NewSplineMesh->SetupAttachment(VROrigin);
		NewSplineMesh->RegisterComponent();
		SplineMeshPool.Add(NewSplineMesh);
	}

	// Configure each segment along the parabolic curve
	for (int32 i = 0; i < SplineMeshPool.Num(); ++i)
	{
		USplineMeshComponent* SegmentMesh = SplineMeshPool[i];
		if (!SegmentMesh) continue;

		if (i < NumSegments)
		{
			SegmentMesh->SetVisibility(true);
			if (ArcMaterialInstance)
			{
				SegmentMesh->SetMaterial(0, ArcMaterialInstance);
			}

			const FVector StartPos = TeleportSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
			const FVector StartTangent = TeleportSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
			const FVector EndPos = TeleportSpline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);
			const FVector EndTangent = TeleportSpline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);

			SegmentMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, true);
			SegmentMesh->SetStartScale(FVector2D(0.015f, 0.015f));
			SegmentMesh->SetEndScale(FVector2D(0.015f, 0.015f));
		}
		else
		{
			SegmentMesh->SetVisibility(false);
		}
	}
}

void AYenkaVRPawn::ClearSplineMeshes()
{
	for (USplineMeshComponent* SegmentMesh : SplineMeshPool)
	{
		if (SegmentMesh)
		{
			SegmentMesh->SetVisibility(false);
		}
	}
	if (TeleportSpline)
	{
		TeleportSpline->ClearSplinePoints(true);
	}
}

void AYenkaVRPawn::ExecuteTeleport()
{
	if (bIsTeleportAiming && bIsTeleportTargetValid)
	{
		// Offset HMD position relative to VROrigin so eyes land precisely on the destination
		const FVector CameraRelative = CameraComponent ? CameraComponent->GetRelativeLocation() : FVector::ZeroVector;
		const FVector TargetOrigin = TeleportTargetLocation - FVector(CameraRelative.X, CameraRelative.Y, 0.0f);

		if (VROrigin)
		{
			VROrigin->SetWorldLocation(TargetOrigin);
		}
	}

	CancelTeleport();
}

void AYenkaVRPawn::CancelTeleport()
{
	bIsTeleportAiming = false;
	bIsTeleportTargetValid = false;

	if (TeleportTargetRing)
	{
		TeleportTargetRing->SetVisibility(false);
	}

	ClearSplineMeshes();
}

void AYenkaVRPawn::ExecuteSnapTurn(float Direction)
{
	if (FMath::Abs(Direction) < 0.60f)
	{
		bSnapTurnAxisReset = true;
		return;
	}

	if (!bSnapTurnAxisReset || SnapTurnCooldownTimer > 0.0f || !VROrigin || !CameraComponent)
	{
		return;
	}

	bSnapTurnAxisReset = false;
	SnapTurnCooldownTimer = 0.25f; // 250ms debouncing interval

	const float TurnSign = Direction > 0.0f ? 1.0f : -1.0f;
	const float DeltaYaw = TurnSign * SnapTurnAngle;

	// Pivot rotation around the player's physical eye position in world space
	const FVector CameraWorldPos = CameraComponent->GetComponentLocation();
	const FRotator CurrentRot = VROrigin->GetComponentRotation();
	const FRotator NewRot = CurrentRot + FRotator(0.0f, DeltaYaw, 0.0f);

	VROrigin->SetWorldRotation(NewRot);

	const FVector NewCameraWorldPos = CameraComponent->GetComponentLocation();
	const FVector PositionalCorrection = CameraWorldPos - NewCameraWorldPos;
	VROrigin->AddWorldOffset(FVector(PositionalCorrection.X, PositionalCorrection.Y, 0.0f));
}

void AYenkaVRPawn::UpdateSpectatorGestures()
{
	// Bimanual Pinch-to-Zoom logic
	if (bIsLeftGrabbingSpace && bIsRightGrabbingSpace && LeftController && RightController)
	{
		float CurrentDistance = FVector::Dist(LeftController->GetComponentLocation(), RightController->GetComponentLocation());
		if (InitialPinchDistance > 0.01f)
		{
			float ScaleFactor = CurrentDistance / InitialPinchDistance;
			FVector CurrentScale = VROrigin->GetComponentScale();
			VROrigin->SetWorldScale3D(CurrentScale * ScaleFactor);
		}
		InitialPinchDistance = CurrentDistance;
	}
}

#include "YenkaVR/Physics/YenkaBlock.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

void AYenkaVRPawn::UpdateExpressiveHandTracking(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// --- 1. LEFT HAND CAPACITIVE & SKELETAL SYNTHESIS ---
	if (LeftHandAvatar)
	{
		const float LeftTrigger = FMath::Max(PC->GetInputAnalogKeyState(EKeys::OculusTouch_Left_Trigger_Axis), PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis));
		const float LeftGrip = FMath::Max(PC->GetInputAnalogKeyState(EKeys::OculusTouch_Left_Grip_Axis), PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis));

		const bool bLeftIndexTouch = PC->IsInputKeyDown(EKeys::OculusTouch_Left_Trigger_Touch) || (LeftTrigger > 0.05f);
		const bool bLeftThumbTouch = PC->IsInputKeyDown(EKeys::OculusTouch_Left_Thumbstick_Touch) 
			|| PC->IsInputKeyDown(EKeys::OculusTouch_Left_X_Touch) 
			|| PC->IsInputKeyDown(EKeys::OculusTouch_Left_Y_Touch);

		// Continuous curl mapping for all 5 fingers
		const float ThumbCurl = bLeftThumbTouch ? (0.2f + 0.8f * LeftTrigger) : 0.0f;
		const float IndexCurl = LeftTrigger;
		const float MiddleCurl = LeftGrip;
		const float RingCurl = FMath::Clamp(LeftGrip * 1.05f, 0.0f, 1.0f);
		const float PinkyCurl = FMath::Clamp(LeftGrip * 1.10f, 0.0f, 1.0f);

		LeftHandAvatar->UpdateContinuousFingerCurls(ThumbCurl, IndexCurl, MiddleCurl, RingCurl, PinkyCurl, bLeftThumbTouch, bLeftIndexTouch);
	}

	// --- 2. RIGHT HAND CAPACITIVE & SKELETAL SYNTHESIS ---
	if (RightHandAvatar)
	{
		const float RightTrigger = FMath::Max(PC->GetInputAnalogKeyState(EKeys::OculusTouch_Right_Trigger_Axis), PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis));
		const float RightGrip = FMath::Max(PC->GetInputAnalogKeyState(EKeys::OculusTouch_Right_Grip_Axis), PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis));

		const bool bRightIndexTouch = PC->IsInputKeyDown(EKeys::OculusTouch_Right_Trigger_Touch) || (RightTrigger > 0.05f);
		const bool bRightThumbTouch = PC->IsInputKeyDown(EKeys::OculusTouch_Right_Thumbstick_Touch) 
			|| PC->IsInputKeyDown(EKeys::OculusTouch_Right_A_Touch) 
			|| PC->IsInputKeyDown(EKeys::OculusTouch_Right_B_Touch);

		// Continuous curl mapping for all 5 fingers
		const float ThumbCurl = bRightThumbTouch ? (0.2f + 0.8f * RightTrigger) : 0.0f;
		const float IndexCurl = RightTrigger;
		const float MiddleCurl = RightGrip;
		const float RingCurl = FMath::Clamp(RightGrip * 1.05f, 0.0f, 1.0f);
		const float PinkyCurl = FMath::Clamp(RightGrip * 1.10f, 0.0f, 1.0f);

		RightHandAvatar->UpdateContinuousFingerCurls(ThumbCurl, IndexCurl, MiddleCurl, RingCurl, PinkyCurl, bRightThumbTouch, bRightIndexTouch);
	}
}

void AYenkaVRPawn::UpdateGravityGloves(float DeltaTime)
{
	if (!bEnableGravityGloves) return;

	auto ProcessGravityHand = [this, DeltaTime](bool bIsLeft, UMotionControllerComponent* MotionController, AYenkaHandAvatar* Avatar, AActor*& TargetedBlock, FVector& PrevLoc, FRotator& PrevRot)
	{
		if (!MotionController || !Avatar) return;

		const FVector HandLoc = MotionController->GetComponentLocation();
		const FRotator HandRot = MotionController->GetComponentRotation();
		const FVector ForwardDir = MotionController->GetForwardVector();

		// 1. Raycast cone towards Yenka blocks
		FHitResult Hit;
		FVector TraceStart = HandLoc;
		FVector TraceEnd = TraceStart + (ForwardDir * GravityGloveLockDistance);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (LeftHandAvatar) Params.AddIgnoredActor(LeftHandAvatar);
		if (RightHandAvatar) Params.AddIgnoredActor(RightHandAvatar);

		AActor* FoundBlock = nullptr;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			if (Hit.GetActor() && Hit.GetActor()->IsA(AYenkaBlock::StaticClass()))
			{
				FoundBlock = Hit.GetActor();
			}
		}

		TargetedBlock = FoundBlock;

		// 2. Wrist Flick Detector (Angular velocity / rapid tilt back towards player)
		if (TargetedBlock && DeltaTime > 0.0001f)
		{
			FVector Velocity = (HandLoc - PrevLoc) / DeltaTime;
			FRotator DeltaRot = (HandRot - PrevRot);
			float PitchSpeed = DeltaRot.Pitch / DeltaTime;

			// Quick upward/backward flick of the wrist while aiming at block
			if (PitchSpeed > 140.0f || FVector::DotProduct(Velocity, -ForwardDir) > 80.0f)
			{
				InitiateGravityPull(bIsLeft, TargetedBlock);
			}
		}

		PrevLoc = HandLoc;
		PrevRot = HandRot;
	};

	ProcessGravityHand(true, LeftController, LeftHandAvatar, LeftTargetedBlock, LeftPrevControllerLoc, LeftPrevControllerRot);
	ProcessGravityHand(false, RightController, RightHandAvatar, RightTargetedBlock, RightPrevControllerLoc, RightPrevControllerRot);
}

void AYenkaVRPawn::InitiateGravityPull(bool bIsLeftHand, AActor* TargetBlock)
{
	if (!TargetBlock) return;

	if (bIsLeftHand)
	{
		LeftFlyingBlock = TargetBlock;
		LeftFlyingFlightTime = 0.0f;
	}
	else
	{
		RightFlyingBlock = TargetBlock;
		RightFlyingFlightTime = 0.0f;
	}

	AYenkaBlock* Block = Cast<AYenkaBlock>(TargetBlock);
	if (Block && Block->BlockMesh)
	{
		Block->BlockMesh->WakeRigidBody();
		Block->BlockMesh->SetEnableGravity(false);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9980, 2.0f, FColor(0, 255, 255),
			FString::Printf(TEXT("🧲 GRAVITY GLOVE: ¡Bloque atraído hacia la mano %s!"), bIsLeftHand ? TEXT("Izquierda") : TEXT("Derecha")));
	}
}

void AYenkaVRPawn::UpdateFlyingBlocks(float DeltaTime)
{
	auto ProcessFlying = [this, DeltaTime](bool bIsLeft, AActor*& FlyingBlock, float& FlightTime, UMotionControllerComponent* MotionController, AYenkaHandAvatar* Avatar)
	{
		if (!FlyingBlock || !MotionController) return;

		FlightTime += DeltaTime;
		const float FlightDuration = 0.35f;
		const float Alpha = FMath::Clamp(FlightTime / FlightDuration, 0.0f, 1.0f);
		const float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

		const FVector TargetPalmLoc = MotionController->GetComponentLocation() + (MotionController->GetForwardVector() * 10.0f);
		FVector CurrentLoc = FlyingBlock->GetActorLocation();
		FVector NewLoc = FMath::Lerp(CurrentLoc, TargetPalmLoc, SmoothAlpha);

		FlyingBlock->SetActorLocation(NewLoc, true);

		// Arrived at palm: Check if player catches it
		if (Alpha >= 1.0f)
		{
			AYenkaBlock* Block = Cast<AYenkaBlock>(FlyingBlock);
			if (Block && Block->BlockMesh)
			{
				Block->BlockMesh->SetEnableGravity(true);
				if (Avatar && Avatar->PhysicsHandle)
				{
					Avatar->PhysicsHandle->GrabComponentAtLocationWithRotation(
						Block->BlockMesh,
						NAME_None,
						TargetPalmLoc,
						MotionController->GetComponentRotation()
					);
				}
			}
			FlyingBlock = nullptr;
		}
	};

	ProcessFlying(true, LeftFlyingBlock, LeftFlyingFlightTime, LeftController, LeftHandAvatar);
	ProcessFlying(false, RightFlyingBlock, RightFlyingFlightTime, RightController, RightHandAvatar);
}
