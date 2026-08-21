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
	CameraBoom->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	HandAvatarClass = AYenkaHandAvatar::StaticClass();
	HoveredBlock = nullptr;
	GrabbedBlock = nullptr;
	bIsOrbitingCamera = false;
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

	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindAction("PrimaryClick", IE_Pressed, this, &AYenkaDesktopPawn::OnPrimaryClickPressed);
		PlayerInputComponent->BindAction("PrimaryClick", IE_Released, this, &AYenkaDesktopPawn::OnPrimaryClickReleased);
		PlayerInputComponent->BindAction("SecondaryClick", IE_Pressed, this, &AYenkaDesktopPawn::OnSecondaryClickPressed);
		PlayerInputComponent->BindAction("SecondaryClick", IE_Released, this, &AYenkaDesktopPawn::OnSecondaryClickReleased);
		PlayerInputComponent->BindAxis("Turn", this, &AYenkaDesktopPawn::AddOrbitYaw);
		PlayerInputComponent->BindAxis("LookUp", this, &AYenkaDesktopPawn::AddOrbitPitch);
		PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &AYenkaDesktopPawn::OnZoomIn);
		PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &AYenkaDesktopPawn::OnZoomOut);
	}
}

void AYenkaDesktopPawn::OnSecondaryClickPressed()
{
	bIsOrbitingCamera = true;
}

void AYenkaDesktopPawn::OnSecondaryClickReleased()
{
	bIsOrbitingCamera = false;
}

void AYenkaDesktopPawn::AddOrbitYaw(float Val)
{
	if (bIsOrbitingCamera && FMath::Abs(Val) > 0.01f)
	{
		AddControllerYawInput(Val);
	}
}

void AYenkaDesktopPawn::AddOrbitPitch(float Val)
{
	if (bIsOrbitingCamera && FMath::Abs(Val) > 0.01f)
	{
		AddControllerPitchInput(-Val);
	}
}

void AYenkaDesktopPawn::OnZoomIn()
{
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - 10.0f, 40.0f, 300.0f);
	}
}

void AYenkaDesktopPawn::OnZoomOut()
{
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength + 10.0f, 40.0f, 300.0f);
	}
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
