#include "YenkaDesktopPawn.h"
#include "YenkaHandAvatar.h"
#include "YenkaVR/Physics/YenkaBlock.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AYenkaDesktopPawn::AYenkaDesktopPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	RootComponent = CameraBoom;
	CameraBoom->TargetArmLength = 120.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	HoveredBlock = nullptr;
	GrabbedBlock = nullptr;
	bIsOrbitingCamera = false;
	PullDepthOffset = 0.0f;
}

void AYenkaDesktopPawn::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled() && HandAvatarClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		VirtualHand = GetWorld()->SpawnActor<AYenkaHandAvatar>(HandAvatarClass, GetActorTransform(), SpawnParams);
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
}

void AYenkaDesktopPawn::HandleMouseTrace()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		FHitResult HitResult;
		FVector TraceEnd = WorldLocation + (WorldDirection * 500.0f);
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		if (VirtualHand) QueryParams.AddIgnoredActor(VirtualHand);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			AYenkaBlock* HitBlock = Cast<AYenkaBlock>(HitResult.GetActor());
			HoveredBlock = HitBlock;

			if (VirtualHand)
			{
				FTransform HandTarget;
				HandTarget.SetLocation(HitResult.ImpactPoint + (HitResult.ImpactNormal * 5.0f));
				HandTarget.SetRotation(FRotationMatrix::MakeFromX(-HitResult.ImpactNormal).ToQuat());
				VirtualHand->SetTargetHandTransform(HandTarget, GrabbedBlock ? 1.0f : 0.0f);
			}
		}
	}
}

void AYenkaDesktopPawn::OnPrimaryClickPressed()
{
	if (HoveredBlock)
	{
		GrabbedBlock = HoveredBlock;
	}
}

void AYenkaDesktopPawn::OnPrimaryClickReleased()
{
	GrabbedBlock = nullptr;
}
