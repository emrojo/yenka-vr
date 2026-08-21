#include "YenkaHandAvatar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

AYenkaHandAvatar::AYenkaHandAvatar()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// 0. Hand Root (holds actor transform without scaling)
	HandRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HandRoot"));
	RootComponent = HandRoot;

	// 1. Palm (6cm x 5cm x 2cm)
	PalmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PalmMesh"));
	PalmMesh->SetupAttachment(HandRoot);
	if (CubeMeshAsset.Succeeded())
	{
		PalmMesh->SetStaticMesh(CubeMeshAsset.Object);
		PalmMesh->SetRelativeScale3D(FVector(0.06f, 0.05f, 0.02f));
		PalmMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}
	PalmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. Thumb
	ThumbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThumbMesh"));
	ThumbMesh->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		ThumbMesh->SetStaticMesh(CylinderMeshAsset.Object);
		ThumbMesh->SetRelativeScale3D(FVector(0.014f, 0.014f, 0.035f));
		ThumbMesh->SetRelativeLocation(FVector(-1.0f, -3.2f, 0.0f));
		ThumbMesh->SetRelativeRotation(FRotator(0.0f, -40.0f, 0.0f));
	}
	ThumbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. Index Finger
	IndexFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndexFinger"));
	IndexFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		IndexFinger->SetStaticMesh(CylinderMeshAsset.Object);
		IndexFinger->SetRelativeScale3D(FVector(0.012f, 0.012f, 0.042f));
		IndexFinger->SetRelativeLocation(FVector(4.2f, -1.8f, 0.0f));
		IndexFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	IndexFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. Middle Finger
	MiddleFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiddleFinger"));
	MiddleFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		MiddleFinger->SetStaticMesh(CylinderMeshAsset.Object);
		MiddleFinger->SetRelativeScale3D(FVector(0.012f, 0.012f, 0.046f));
		MiddleFinger->SetRelativeLocation(FVector(4.6f, -0.6f, 0.0f));
		MiddleFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	MiddleFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 5. Ring Finger
	RingFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingFinger"));
	RingFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		RingFinger->SetStaticMesh(CylinderMeshAsset.Object);
		RingFinger->SetRelativeScale3D(FVector(0.011f, 0.011f, 0.040f));
		RingFinger->SetRelativeLocation(FVector(4.2f, 0.6f, 0.0f));
		RingFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	RingFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 6. Pinky Finger
	PinkyFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PinkyFinger"));
	PinkyFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		PinkyFinger->SetStaticMesh(CylinderMeshAsset.Object);
		PinkyFinger->SetRelativeScale3D(FVector(0.010f, 0.010f, 0.032f));
		PinkyFinger->SetRelativeLocation(FVector(3.6f, 1.8f, 0.0f));
		PinkyFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	PinkyFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	ReplicatedGripStrength = 0.0f;
	bIsLeftHand = false;
}

#include "Materials/MaterialInstanceDynamic.h"

void AYenkaHandAvatar::BeginPlay()
{
	Super::BeginPlay();

	// Style hand and fingers with clean VR glove aesthetic
	TArray<UStaticMeshComponent*> HandParts = { PalmMesh, ThumbMesh, IndexFinger, MiddleFinger, RingFinger, PinkyFinger };
	UMaterialInterface* BaseMat = PalmMesh ? PalmMesh->GetMaterial(0) : nullptr;

	if (BaseMat)
	{
		UMaterialInstanceDynamic* GloveMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (GloveMat)
		{
			FLinearColor GloveColor(0.2f, 0.58f, 0.92f, 1.0f);
			GloveMat->SetVectorParameterValue(TEXT("Color"), GloveColor);
			GloveMat->SetVectorParameterValue(TEXT("BaseColor"), GloveColor);
			GloveMat->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
			GloveMat->SetScalarParameterValue(TEXT("Metallic"), 0.2f);
			GloveMat->SetScalarParameterValue(TEXT("Specular"), 0.6f);

			for (UStaticMeshComponent* Part : HandParts)
			{
				if (Part)
				{
					Part->SetMaterial(0, GloveMat);
				}
			}
		}
	}
}

void AYenkaHandAvatar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AYenkaHandAvatar, ReplicatedHandTransform);
	DOREPLIFETIME(AYenkaHandAvatar, ReplicatedGripStrength);
	DOREPLIFETIME(AYenkaHandAvatar, bIsLeftHand);
}

void AYenkaHandAvatar::SetTargetHandTransform(const FTransform& InTransform, float InGripStrength)
{
	ReplicatedHandTransform = InTransform;
	ReplicatedGripStrength = InGripStrength;
	SetActorTransform(InTransform);
	UpdateFingerPoses(InGripStrength);
}

void AYenkaHandAvatar::UpdateFingerPoses(float GripStrength)
{
	const float GripAngle = FMath::Clamp(GripStrength, 0.0f, 1.0f) * 45.0f;

	if (IndexFinger)
	{
		IndexFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
	}
	if (MiddleFinger)
	{
		MiddleFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
	}
	if (RingFinger)
	{
		RingFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
	}
	if (PinkyFinger)
	{
		PinkyFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
	}
	if (ThumbMesh)
	{
		ThumbMesh->SetRelativeRotation(FRotator(GripAngle * 0.5f, -45.0f + (GripAngle * 0.5f), 0.0f));
	}
}

void AYenkaHandAvatar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smoothly interpolate to target transform for remote spectators
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const bool bIsLocal = OwnerPawn ? OwnerPawn->IsLocallyControlled() : false;

	if (!HasAuthority() && !bIsLocal)
	{
		FTransform CurrentTransform = GetActorTransform();
		FTransform NewTransform;
		NewTransform.Blend(CurrentTransform, ReplicatedHandTransform, FMath::Clamp(DeltaTime * 15.0f, 0.0f, 1.0f));
		SetActorTransform(NewTransform);
	}
}
