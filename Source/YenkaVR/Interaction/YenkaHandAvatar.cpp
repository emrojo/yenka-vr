#include "YenkaHandAvatar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Animation/AnimSequence.h"
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

	// 0.1. MetaHuman / Standard Continuous Skeletal Mesh Hand Component
	HandSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandSkeletalMesh"));
	HandSkeletalMesh->SetupAttachment(HandRoot);
	HandSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandSkeletalMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshAsset(TEXT("/Game/Characters/MannequinsXR/Meshes/SKM_MannyXR_right.SKM_MannyXR_right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleSeqAsset(TEXT("/Game/Characters/MannequinsXR/Animations/A_MannequinsXR_Idle_Right.A_MannequinsXR_Idle_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> PointSeqAsset(TEXT("/Game/Characters/MannequinsXR/Animations/A_MannequinsXR_Point_Right.A_MannequinsXR_Point_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> GraspSeqAsset(TEXT("/Game/Characters/MannequinsXR/Animations/A_MannequinsXR_Grasp_Right.A_MannequinsXR_Grasp_Right"));

	if (IdleSeqAsset.Succeeded()) AnimIdle = IdleSeqAsset.Object;
	if (PointSeqAsset.Succeeded()) AnimPoint = PointSeqAsset.Object;
	if (GraspSeqAsset.Succeeded()) AnimGrasp = GraspSeqAsset.Object;

	const bool bHasSkeletalHand = SkeletalMeshAsset.Succeeded();
	if (bHasSkeletalHand)
	{
		HandSkeletalMesh->SetSkeletalMesh(SkeletalMeshAsset.Object);
		// MannyXR right hand: scaled to 0.5 (half-size), rotated -90 deg to face pieces directly
		// Offset wrist by -5.0cm so palm center is at HandRoot (0,0,0) and fingertips reach +2.5cm
		HandSkeletalMesh->SetRelativeLocation(FVector(-5.0f, 0.0f, 0.0f));
		HandSkeletalMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		HandSkeletalMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));

		if (AnimIdle)
		{
			HandSkeletalMesh->PlayAnimation(AnimIdle, true);
			HandSkeletalMesh->SetPlayRate(1.0f);
		}
	}

	// 1. Palm
	PalmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PalmMesh"));
	PalmMesh->SetupAttachment(HandRoot);
	if (CubeMeshAsset.Succeeded())
	{
		PalmMesh->SetStaticMesh(CubeMeshAsset.Object);
		PalmMesh->SetRelativeScale3D(FVector(0.05f, 0.045f, 0.015f));
		PalmMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}
	PalmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PalmMesh->SetCastShadow(!bHasSkeletalHand);
	PalmMesh->SetVisibility(!bHasSkeletalHand);

	// 2. Thumb
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
	ThumbMesh->SetCastShadow(!bHasSkeletalHand);
	ThumbMesh->SetVisibility(!bHasSkeletalHand);

	// 3. Index Finger
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
	IndexFinger->SetCastShadow(!bHasSkeletalHand);
	IndexFinger->SetVisibility(!bHasSkeletalHand);

	// 4. Middle Finger
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
	MiddleFinger->SetCastShadow(!bHasSkeletalHand);
	MiddleFinger->SetVisibility(!bHasSkeletalHand);

	// 5. Ring Finger
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
	RingFinger->SetCastShadow(!bHasSkeletalHand);
	RingFinger->SetVisibility(!bHasSkeletalHand);

	// 6. Pinky Finger
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
	PinkyFinger->SetCastShadow(!bHasSkeletalHand);
	PinkyFinger->SetVisibility(!bHasSkeletalHand);

	// 7. Keratin Fingernails
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
			Nail->SetVisibility(!bHasSkeletalHand);
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
	LastAppliedPoseMode = static_cast<EHandPoseMode>(255);
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
	if (CurrentPoseMode != LastAppliedPoseMode)
	{
		UpdateFingerPoses(InGripStrength);
	}
}

void AYenkaHandAvatar::SetHandPoseMode(EHandPoseMode NewPoseMode)
{
	if (CurrentPoseMode != NewPoseMode)
	{
		CurrentPoseMode = NewPoseMode;
		UpdateFingerPoses(ReplicatedGripStrength);
	}
}

void AYenkaHandAvatar::UpdateFingerPoses(float GripStrength)
{
	LastAppliedPoseMode = CurrentPoseMode;

	if (HandSkeletalMesh)
	{
		if (CurrentPoseMode == EHandPoseMode::FingerPoke)
		{
			if (AnimPoint)
			{
				HandSkeletalMesh->PlayAnimation(AnimPoint, false);
				HandSkeletalMesh->SetPosition(AnimPoint->GetPlayLength() * 0.90f);
				HandSkeletalMesh->SetPlayRate(0.0f);
			}
		}
		else if (CurrentPoseMode == EHandPoseMode::GrabPinch)
		{
			if (AnimGrasp)
			{
				HandSkeletalMesh->PlayAnimation(AnimGrasp, false);
				HandSkeletalMesh->SetPosition(AnimGrasp->GetPlayLength() * 0.70f);
				HandSkeletalMesh->SetPlayRate(0.0f);
			}
		}
		else // OpenHand
		{
			if (AnimIdle)
			{
				HandSkeletalMesh->PlayAnimation(AnimIdle, true);
				HandSkeletalMesh->SetPlayRate(1.0f);
			}
		}
	}

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
		// Scaled 0.5: Index finger tip at X = 2.5cm
		return 2.5f;
	}
	else if (CurrentPoseMode == EHandPoseMode::GrabPinch)
	{
		// Scaled 0.5: Pinch caliper fingertips reach at X = 1.8cm
		return 1.8f;
	}
	else // OpenHand
	{
		// Scaled 0.5: Middle finger tip at X = 2.5cm
		return 2.5f;
	}
}

FVector AYenkaHandAvatar::GetExtendedFingertipLocalOffset() const
{
	if (CurrentPoseMode == EHandPoseMode::FingerPoke)
	{
		// Scaled 0.5: Index finger extended forward tip at (2.5cm, 0.0cm, 0.0cm)
		return FVector(2.5f, 0.0f, 0.0f);
	}
	else if (CurrentPoseMode == EHandPoseMode::GrabPinch)
	{
		// Scaled 0.5: Caliper pinch center aligns directly at (1.8cm, 0.0cm, 0.0cm)
		return FVector(1.8f, 0.0f, 0.0f);
	}
	else // OpenHand
	{
		// Scaled 0.5: Middle finger tip aligns at (2.5cm, 0.0cm, 0.0cm)
		return FVector(2.5f, 0.0f, 0.0f);
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
