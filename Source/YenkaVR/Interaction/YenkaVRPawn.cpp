#include "YenkaVRPawn.h"
#include "YenkaHandAvatar.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
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

	bIsLeftGrabbingSpace = false;
	bIsRightGrabbingSpace = false;
	InitialPinchDistance = 1.0f;
}

void AYenkaVRPawn::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled() && HandAvatarClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		LeftHandAvatar = GetWorld()->SpawnActor<AYenkaHandAvatar>(HandAvatarClass, GetActorTransform(), SpawnParams);
		if (LeftHandAvatar)
		{
			LeftHandAvatar->bIsLeftHand = true;
			LeftHandAvatar->AttachToComponent(LeftController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}

		RightHandAvatar = GetWorld()->SpawnActor<AYenkaHandAvatar>(HandAvatarClass, GetActorTransform(), SpawnParams);
		if (RightHandAvatar)
		{
			RightHandAvatar->bIsLeftHand = false;
			RightHandAvatar->AttachToComponent(RightController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
	}
}

void AYenkaVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		UpdateSpectatorGestures();
	}
}

void AYenkaVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
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
