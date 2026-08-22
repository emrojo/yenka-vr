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

	// 1. Palm (5cm x 4.5cm x 1.5cm)
	PalmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PalmMesh"));
	PalmMesh->SetupAttachment(HandRoot);
	if (CubeMeshAsset.Succeeded())
	{
		PalmMesh->SetStaticMesh(CubeMeshAsset.Object);
		PalmMesh->SetRelativeScale3D(FVector(0.05f, 0.045f, 0.015f));
		PalmMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}
	PalmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. Thumb (slender 0.8cm diameter, 3.0cm length)
	ThumbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThumbMesh"));
	ThumbMesh->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		ThumbMesh->SetStaticMesh(CylinderMeshAsset.Object);
		ThumbMesh->SetRelativeScale3D(FVector(0.008f, 0.008f, 0.030f));
		ThumbMesh->SetRelativeLocation(FVector(0.5f, -2.5f, 0.0f));
		ThumbMesh->SetRelativeRotation(FRotator(0.0f, -40.0f, 0.0f));
	}
	ThumbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. Index Finger (slender and pointed: 0.7cm diameter, 5.0cm length)
	IndexFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndexFinger"));
	IndexFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		IndexFinger->SetStaticMesh(CylinderMeshAsset.Object);
		IndexFinger->SetRelativeScale3D(FVector(0.007f, 0.007f, 0.050f));
		IndexFinger->SetRelativeLocation(FVector(3.5f, -1.5f, 0.0f));
		IndexFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	IndexFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. Middle Finger (slender: 0.7cm diameter, 5.2cm length)
	MiddleFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiddleFinger"));
	MiddleFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		MiddleFinger->SetStaticMesh(CylinderMeshAsset.Object);
		MiddleFinger->SetRelativeScale3D(FVector(0.007f, 0.007f, 0.052f));
		MiddleFinger->SetRelativeLocation(FVector(3.8f, -0.4f, 0.0f));
		MiddleFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	MiddleFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 5. Ring Finger (slender: 0.0065cm diameter, 4.5cm length)
	RingFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingFinger"));
	RingFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		RingFinger->SetStaticMesh(CylinderMeshAsset.Object);
		RingFinger->SetRelativeScale3D(FVector(0.0065f, 0.0065f, 0.045f));
		RingFinger->SetRelativeLocation(FVector(3.5f, 0.6f, 0.0f));
		RingFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	RingFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 6. Pinky Finger (slender: 0.006cm diameter, 3.5cm length)
	PinkyFinger = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PinkyFinger"));
	PinkyFinger->SetupAttachment(HandRoot);
	if (CylinderMeshAsset.Succeeded())
	{
		PinkyFinger->SetStaticMesh(CylinderMeshAsset.Object);
		PinkyFinger->SetRelativeScale3D(FVector(0.006f, 0.006f, 0.035f));
		PinkyFinger->SetRelativeLocation(FVector(3.0f, 1.5f, 0.0f));
		PinkyFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
	PinkyFinger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GloveBaseMatAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (GloveBaseMatAsset.Succeeded())
	{
		PalmMesh->SetMaterial(0, GloveBaseMatAsset.Object);
		ThumbMesh->SetMaterial(0, GloveBaseMatAsset.Object);
		IndexFinger->SetMaterial(0, GloveBaseMatAsset.Object);
		MiddleFinger->SetMaterial(0, GloveBaseMatAsset.Object);
		RingFinger->SetMaterial(0, GloveBaseMatAsset.Object);
		PinkyFinger->SetMaterial(0, GloveBaseMatAsset.Object);
	}

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
			FLinearColor GloveColor(0.12f, 0.65f, 0.98f, 1.0f); // Sleek cyan-blue VR glove
			GloveMat->SetVectorParameterValue(TEXT("Color"), GloveColor);
			GloveMat->SetScalarParameterValue(TEXT("Roughness"), 0.2f);
			GloveMat->SetScalarParameterValue(TEXT("Metallic"), 0.3f);

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

	DOREPLIFETIME(AYenkaHandAvatar, CurrentPoseMode);
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

void AYenkaHandAvatar::SetHandPoseMode(EHandPoseMode NewPoseMode)
{
	CurrentPoseMode = NewPoseMode;
	UpdateFingerPoses(ReplicatedGripStrength);
}

void AYenkaHandAvatar::UpdateFingerPoses(float GripStrength)
{
	if (CurrentPoseMode == EHandPoseMode::FingerPoke)
	{
		// Finger Poke: Only Index Finger is extended forward; all other 4 fingers are tightly tucked into a fist far back
		if (IndexFinger)
		{
			IndexFinger->SetRelativeLocation(FVector(3.5f, -1.5f, 0.0f));
			IndexFinger->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f)); // Pointing forward along +X
		}
		if (ThumbMesh)
		{
			ThumbMesh->SetRelativeLocation(FVector(0.5f, -2.0f, 0.3f));
			ThumbMesh->SetRelativeRotation(FRotator(0.0f, -75.0f, 0.0f)); // Tucked tightly against fist
		}
		if (MiddleFinger)
		{
			MiddleFinger->SetRelativeLocation(FVector(1.0f, -0.4f, -0.6f));
			MiddleFinger->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f)); // Curled vertical into palm
		}
		if (RingFinger)
		{
			RingFinger->SetRelativeLocation(FVector(0.9f, 0.6f, -0.6f));
			RingFinger->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f)); // Curled vertical into palm
		}
		if (PinkyFinger)
		{
			PinkyFinger->SetRelativeLocation(FVector(0.8f, 1.5f, -0.6f));
			PinkyFinger->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f)); // Curled vertical into palm
		}
	}
	else
	{
		// Open / Grip mode
		const float GripAngle = FMath::Clamp(GripStrength, 0.0f, 1.0f) * 45.0f;

		if (IndexFinger)
		{
			IndexFinger->SetRelativeLocation(FVector(3.5f, -1.5f, 0.0f));
			IndexFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
		}
		if (MiddleFinger)
		{
			MiddleFinger->SetRelativeLocation(FVector(3.8f, -0.4f, 0.0f));
			MiddleFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
		}
		if (RingFinger)
		{
			RingFinger->SetRelativeLocation(FVector(3.5f, 0.6f, 0.0f));
			RingFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
		}
		if (PinkyFinger)
		{
			PinkyFinger->SetRelativeLocation(FVector(3.0f, 1.5f, 0.0f));
			PinkyFinger->SetRelativeRotation(FRotator(90.0f - GripAngle, 0.0f, 0.0f));
		}
		if (ThumbMesh)
		{
			ThumbMesh->SetRelativeLocation(FVector(0.5f, -2.5f, 0.0f));
			ThumbMesh->SetRelativeRotation(FRotator(GripAngle * 0.5f, -45.0f + (GripAngle * 0.5f), 0.0f));
		}
	}
}

float AYenkaHandAvatar::GetExtendedFingertipOffset() const
{
	if (CurrentPoseMode == EHandPoseMode::FingerPoke)
	{
		// Index finger: Location X = 3.5cm, half-length = 2.5cm -> tip at X = 6.0cm
		return 6.0f;
	}
	else if (CurrentPoseMode == EHandPoseMode::GrabPinch)
	{
		// Bent fingers during pinch: tip at ~3.5cm
		return 3.5f;
	}
	else // OpenHand
	{
		// Middle finger: Location X = 3.8cm, half-length = 2.6cm -> tip at X = 6.4cm
		return 6.4f;
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
