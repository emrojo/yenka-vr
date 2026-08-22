#include "YenkaHandAvatar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInstanceDynamic.h"

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

	// 0.1. MetaHuman / Standard Skeletal Mesh Hand Component
	HandSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandSkeletalMesh"));
	HandSkeletalMesh->SetupAttachment(HandRoot);
	HandSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandSkeletalMesh->SetCastShadow(true);

	// 1. Palm (Anatomical proportions: 5cm length x 4.5cm width x 1.5cm thickness)
	PalmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PalmMesh"));
	PalmMesh->SetupAttachment(HandRoot);
	if (CubeMeshAsset.Succeeded())
	{
		PalmMesh->SetStaticMesh(CubeMeshAsset.Object);
		PalmMesh->SetRelativeScale3D(FVector(0.05f, 0.045f, 0.015f));
		PalmMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}
	PalmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PalmMesh->SetCastShadow(true);

	// 2. Thumb (0.8cm diameter, 3.0cm length)
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
	ThumbMesh->SetCastShadow(true);

	// 3. Index Finger (0.7cm diameter, 5.0cm length)
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
	IndexFinger->SetCastShadow(true);

	// 4. Middle Finger (0.7cm diameter, 5.2cm length)
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
	MiddleFinger->SetCastShadow(true);

	// 5. Ring Finger (0.0065cm diameter, 4.5cm length)
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
	RingFinger->SetCastShadow(true);

	// 6. Pinky Finger (0.006cm diameter, 3.5cm length)
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
	PinkyFinger->SetCastShadow(true);

	// 7. Translucent Keratin Fingernails (0.8cm length x 0.6cm width x 0.08cm curved plate)
	ThumbNail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThumbNail"));
	ThumbNail->SetupAttachment(ThumbMesh);
	IndexNail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IndexNail"));
	IndexNail->SetupAttachment(IndexFinger);
	MiddleNail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiddleNail"));
	MiddleNail->SetupAttachment(MiddleFinger);
	RingNail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingNail"));
	RingNail->SetupAttachment(RingFinger);
	PinkyNail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PinkyNail"));
	PinkyNail->SetupAttachment(PinkyFinger);

	TArray<UStaticMeshComponent*> Nails = { ThumbNail, IndexNail, MiddleNail, RingNail, PinkyNail };
	for (UStaticMeshComponent* Nail : Nails)
	{
		if (Nail && CubeMeshAsset.Succeeded())
		{
			Nail->SetStaticMesh(CubeMeshAsset.Object);
			Nail->SetRelativeScale3D(FVector(0.008f, 0.006f, 0.001f));
			Nail->SetRelativeLocation(FVector(0.0f, 0.0f, 1.3f));
			Nail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Nail->SetCastShadow(false);
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMatAsset.Succeeded())
	{
		PalmMesh->SetMaterial(0, BaseMatAsset.Object);
		ThumbMesh->SetMaterial(0, BaseMatAsset.Object);
		IndexFinger->SetMaterial(0, BaseMatAsset.Object);
		MiddleFinger->SetMaterial(0, BaseMatAsset.Object);
		RingFinger->SetMaterial(0, BaseMatAsset.Object);
		PinkyFinger->SetMaterial(0, BaseMatAsset.Object);
		for (UStaticMeshComponent* Nail : Nails)
		{
			if (Nail) Nail->SetMaterial(0, BaseMatAsset.Object);
		}
	}

	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	// Ultra-realistic Human Skin PBR & Subsurface Scattering defaults
	SkinTone = FLinearColor(0.86f, 0.67f, 0.57f, 1.0f); // Warm natural melanin skin tone
	SubsurfaceColor = FLinearColor(0.85f, 0.08f, 0.03f, 1.0f); // Deep dermal blood vessel light bleed
	FingernailColor = FLinearColor(0.92f, 0.82f, 0.80f, 1.0f); // Glossy translucent keratin nail
	SkinRoughness = 0.36f; // Natural human lipid layer sheen
	SubsurfaceScatteringStrength = 0.75f;

	ReplicatedGripStrength = 0.0f;
	bIsLeftHand = false;
}

void AYenkaHandAvatar::BeginPlay()
{
	Super::BeginPlay();
	ApplyHumanSkinMaterials();
}

void AYenkaHandAvatar::ApplyHumanSkinMaterials()
{
	// 1. Dynamic Human Skin Material with Subsurface Scattering Emulation
	TArray<UStaticMeshComponent*> SkinParts = { PalmMesh, ThumbMesh, IndexFinger, MiddleFinger, RingFinger, PinkyFinger };
	UMaterialInterface* BaseMat = PalmMesh ? PalmMesh->GetMaterial(0) : nullptr;

	if (BaseMat)
	{
		UMaterialInstanceDynamic* SkinMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (SkinMat)
		{
			SkinMat->SetVectorParameterValue(TEXT("Color"), SkinTone);
			SkinMat->SetVectorParameterValue(TEXT("SubsurfaceColor"), SubsurfaceColor);
			SkinMat->SetScalarParameterValue(TEXT("Roughness"), SkinRoughness);
			SkinMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			SkinMat->SetScalarParameterValue(TEXT("Specular"), 0.5f);

			for (UStaticMeshComponent* Part : SkinParts)
			{
				if (Part)
				{
					Part->SetMaterial(0, SkinMat);
				}
			}
			if (HandSkeletalMesh)
			{
				HandSkeletalMesh->SetMaterial(0, SkinMat);
			}
		}

		// 2. Translucent Glossy Keratin Fingernails Material
		UMaterialInstanceDynamic* NailMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (NailMat)
		{
			NailMat->SetVectorParameterValue(TEXT("Color"), FingernailColor);
			NailMat->SetScalarParameterValue(TEXT("Roughness"), 0.12f); // Glossy polished nail surface
			NailMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			NailMat->SetScalarParameterValue(TEXT("Specular"), 0.65f);

			TArray<UStaticMeshComponent*> Nails = { ThumbNail, IndexNail, MiddleNail, RingNail, PinkyNail };
			for (UStaticMeshComponent* Nail : Nails)
			{
				if (Nail)
				{
					Nail->SetMaterial(0, NailMat);
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
		// Finger Poke: Index Finger fully extended forward; other 4 fingers curled tightly into fist
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
	else if (CurrentPoseMode == EHandPoseMode::GrabPinch)
	{
		// Grab Pinch: Thumb and Index finger form an anatomical caliper grasping the two lateral ends (+-1.25cm) of the protruding block (2.5cm width)
		// Index finger: Reaches forward on the right lateral side (+1.25cm)
		if (IndexFinger)
		{
			IndexFinger->SetRelativeLocation(FVector(2.6f, 0.8f, 0.0f));
			IndexFinger->SetRelativeRotation(FRotator(90.0f, -20.0f, 0.0f)); // Fingertip at (4.8cm, +1.25cm, 0.0cm)
		}
		// Thumb: Reaches forward on the left lateral side (-1.25cm)
		if (ThumbMesh)
		{
			ThumbMesh->SetRelativeLocation(FVector(2.0f, -1.8f, 0.0f));
			ThumbMesh->SetRelativeRotation(FRotator(0.0f, 32.0f, 0.0f)); // Fingertip at (4.8cm, -1.25cm, 0.0cm)
		}
		// Middle, Ring, Pinky: Curled backward into palm out of the way
		if (MiddleFinger)
		{
			MiddleFinger->SetRelativeLocation(FVector(1.0f, -0.2f, -0.6f));
			MiddleFinger->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
		if (RingFinger)
		{
			RingFinger->SetRelativeLocation(FVector(0.9f, 0.6f, -0.6f));
			RingFinger->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
		if (PinkyFinger)
		{
			PinkyFinger->SetRelativeLocation(FVector(0.8f, 1.4f, -0.6f));
			PinkyFinger->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
	else
	{
		// Open / Grip mode: Natural relaxed human hand curvature
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
		// Pinch caliper fingertips reach at X = 4.8cm
		return 4.8f;
	}
	else // OpenHand
	{
		// Middle finger: Location X = 3.8cm, half-length = 2.6cm -> tip at X = 6.4cm
		return 6.4f;
	}
}

FVector AYenkaHandAvatar::GetExtendedFingertipLocalOffset() const
{
	if (CurrentPoseMode == EHandPoseMode::FingerPoke)
	{
		// Index finger: Location X = 3.5cm, Y = -1.5cm, half-length = 2.5cm -> tip at (6.0cm, -1.5cm, 0.0cm)
		return FVector(6.0f, -1.5f, 0.0f);
	}
	else if (CurrentPoseMode == EHandPoseMode::GrabPinch)
	{
		// Caliper pinch center aligns directly at (4.8cm, 0.0cm, 0.0cm)
		return FVector(4.8f, 0.0f, 0.0f);
	}
	else // OpenHand
	{
		// Middle finger: Location X = 3.8cm, Y = -0.4cm, half-length = 2.6cm -> tip at (6.4cm, -0.4cm, 0.0f)
		return FVector(6.4f, -0.4f, 0.0f);
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
