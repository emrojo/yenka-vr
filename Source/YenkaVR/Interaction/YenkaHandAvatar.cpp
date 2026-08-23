#include "YenkaHandAvatar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
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

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RightSkeletalMeshAsset(TEXT("/Game/Characters/MannequinsXR/Meshes/SKM_MannyXR_right.SKM_MannyXR_right"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> LeftSkeletalMeshAsset(TEXT("/Game/Characters/MannequinsXR/Meshes/SKM_MannyXR_left.SKM_MannyXR_left"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleSeqAsset(TEXT("/Game/Characters/MannequinsXR/Animations/A_MannequinsXR_Idle_Right.A_MannequinsXR_Idle_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> PointSeqAsset(TEXT("/Game/Characters/MannequinsXR/Animations/A_MannequinsXR_Point_Right.A_MannequinsXR_Point_Right"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> GraspSeqAsset(TEXT("/Game/Characters/MannequinsXR/Animations/A_MannequinsXR_Grasp_Right.A_MannequinsXR_Grasp_Right"));

	if (RightSkeletalMeshAsset.Succeeded()) RightSkeletalMesh = RightSkeletalMeshAsset.Object;
	if (LeftSkeletalMeshAsset.Succeeded()) LeftSkeletalMesh = LeftSkeletalMeshAsset.Object;
	if (IdleSeqAsset.Succeeded()) AnimIdle = IdleSeqAsset.Object;
	if (PointSeqAsset.Succeeded()) AnimPoint = PointSeqAsset.Object;
	if (GraspSeqAsset.Succeeded()) AnimGrasp = GraspSeqAsset.Object;

	// 0.1. Real-time Anatomically Poseable Mesh Hand Component
	PoseableHandMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseableHandMesh"));
	PoseableHandMesh->SetupAttachment(HandRoot);
	PoseableHandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoseableHandMesh->SetCastShadow(true);
	if (RightSkeletalMesh)
	{
		PoseableHandMesh->SetSkeletalMesh(RightSkeletalMesh);
		PoseableHandMesh->SetRelativeLocation(FVector(-5.0f, 0.0f, 0.0f));
		PoseableHandMesh->SetRelativeRotation(FRotator::ZeroRotator);
		PoseableHandMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
	}

	// 0.2. Standard Continuous Skeletal Mesh Hand Component (Hidden when using poseable mesh)
	HandSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandSkeletalMesh"));
	HandSkeletalMesh->SetupAttachment(HandRoot);
	HandSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandSkeletalMesh->SetCastShadow(false);
	HandSkeletalMesh->SetVisibility(false);

	const bool bHasSkeletalHand = RightSkeletalMesh != nullptr;

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
	UpdateHandMeshSide();
	ApplyHumanSkinMaterials();
}

void AYenkaHandAvatar::SetIsLeftHand(bool bInIsLeft)
{
	bIsLeftHand = bInIsLeft;
	UpdateHandMeshSide();
}

void AYenkaHandAvatar::OnRep_IsLeftHand()
{
	UpdateHandMeshSide();
}

void AYenkaHandAvatar::UpdateHandMeshSide()
{
	if (HandSkeletalMesh)
	{
		if (bIsLeftHand)
		{
			if (LeftSkeletalMesh)
			{
				HandSkeletalMesh->SetSkeletalMesh(LeftSkeletalMesh);
			}
			HandSkeletalMesh->SetRelativeLocation(FVector(-5.0f, 0.0f, 0.0f));
			HandSkeletalMesh->SetRelativeRotation(FRotator::ZeroRotator);
			HandSkeletalMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		}
		else
		{
			if (RightSkeletalMesh)
			{
				HandSkeletalMesh->SetSkeletalMesh(RightSkeletalMesh);
			}
			HandSkeletalMesh->SetRelativeLocation(FVector(-5.0f, 0.0f, 0.0f));
			HandSkeletalMesh->SetRelativeRotation(FRotator::ZeroRotator);
			HandSkeletalMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		}

		if (AnimIdle)
		{
			HandSkeletalMesh->PlayAnimation(AnimIdle, true);
			HandSkeletalMesh->SetPlayRate(1.0f);
		}
	}
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
	ApplyPhalanxTransforms();
}

static void ModifyPhalanxData(FPhalanxData& Data, float DeltaPitch, float DeltaYaw, float DeltaRoll)
{
	Data.Pitch = FMath::Clamp(Data.Pitch + DeltaPitch, -180.0f, 180.0f);
	Data.Yaw = FMath::Clamp(Data.Yaw + DeltaYaw, -180.0f, 180.0f);
	Data.Roll = FMath::Clamp(Data.Roll + DeltaRoll, -180.0f, 180.0f);
}

static void ModifyFingerPhalanges(FFingerPhalanges& Finger, int32 PhalanxIndex, float DeltaPitch, float DeltaYaw, float DeltaRoll)
{
	if (PhalanxIndex == 0) // All phalanges
	{
		ModifyPhalanxData(Finger.Proximal, DeltaPitch, DeltaYaw, DeltaRoll);
		ModifyPhalanxData(Finger.Intermediate, DeltaPitch, DeltaYaw, DeltaRoll);
		ModifyPhalanxData(Finger.Distal, DeltaPitch, DeltaYaw, DeltaRoll);
	}
	else if (PhalanxIndex == 1) // Proximal
	{
		ModifyPhalanxData(Finger.Proximal, DeltaPitch, DeltaYaw, DeltaRoll);
	}
	else if (PhalanxIndex == 2) // Intermediate
	{
		ModifyPhalanxData(Finger.Intermediate, DeltaPitch, DeltaYaw, DeltaRoll);
	}
	else if (PhalanxIndex == 3) // Distal
	{
		ModifyPhalanxData(Finger.Distal, DeltaPitch, DeltaYaw, DeltaRoll);
	}
}

void AYenkaHandAvatar::SetPhalanxPitch(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle)
{
	if (FingerIndex == 0) // All fingers
	{
		ModifyFingerPhalanges(ThumbPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
		ModifyFingerPhalanges(IndexPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
		ModifyFingerPhalanges(MiddlePhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
		ModifyFingerPhalanges(RingPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
		ModifyFingerPhalanges(PinkyPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
	}
	else if (FingerIndex == 1) ModifyFingerPhalanges(ThumbPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
	else if (FingerIndex == 2) ModifyFingerPhalanges(IndexPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
	else if (FingerIndex == 3) ModifyFingerPhalanges(MiddlePhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
	else if (FingerIndex == 4) ModifyFingerPhalanges(RingPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);
	else if (FingerIndex == 5) ModifyFingerPhalanges(PinkyPhalanges, PhalanxIndex, DeltaAngle, 0.0f, 0.0f);

	ApplyPhalanxTransforms();
}

void AYenkaHandAvatar::SetPhalanxYaw(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle)
{
	if (FingerIndex == 0) // All fingers
	{
		ModifyFingerPhalanges(ThumbPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
		ModifyFingerPhalanges(IndexPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
		ModifyFingerPhalanges(MiddlePhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
		ModifyFingerPhalanges(RingPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
		ModifyFingerPhalanges(PinkyPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
	}
	else if (FingerIndex == 1) ModifyFingerPhalanges(ThumbPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
	else if (FingerIndex == 2) ModifyFingerPhalanges(IndexPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
	else if (FingerIndex == 3) ModifyFingerPhalanges(MiddlePhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
	else if (FingerIndex == 4) ModifyFingerPhalanges(RingPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);
	else if (FingerIndex == 5) ModifyFingerPhalanges(PinkyPhalanges, PhalanxIndex, 0.0f, DeltaAngle, 0.0f);

	ApplyPhalanxTransforms();
}

void AYenkaHandAvatar::SetPhalanxRoll(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle)
{
	if (FingerIndex == 0) // All fingers
	{
		ModifyFingerPhalanges(ThumbPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
		ModifyFingerPhalanges(IndexPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
		ModifyFingerPhalanges(MiddlePhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
		ModifyFingerPhalanges(RingPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
		ModifyFingerPhalanges(PinkyPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
	}
	else if (FingerIndex == 1) ModifyFingerPhalanges(ThumbPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
	else if (FingerIndex == 2) ModifyFingerPhalanges(IndexPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
	else if (FingerIndex == 3) ModifyFingerPhalanges(MiddlePhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
	else if (FingerIndex == 4) ModifyFingerPhalanges(RingPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);
	else if (FingerIndex == 5) ModifyFingerPhalanges(PinkyPhalanges, PhalanxIndex, 0.0f, 0.0f, DeltaAngle);

	ApplyPhalanxTransforms();
}

void AYenkaHandAvatar::ResetPhalanx(int32 FingerIndex, int32 PhalanxIndex)
{
	auto ResetFinger = [PhalanxIndex](FFingerPhalanges& Finger)
	{
		if (PhalanxIndex == 0 || PhalanxIndex == 1) Finger.Proximal = FPhalanxData();
		if (PhalanxIndex == 0 || PhalanxIndex == 2) Finger.Intermediate = FPhalanxData();
		if (PhalanxIndex == 0 || PhalanxIndex == 3) Finger.Distal = FPhalanxData();
	};

	if (FingerIndex == 0)
	{
		ResetFinger(ThumbPhalanges);
		ResetFinger(IndexPhalanges);
		ResetFinger(MiddlePhalanges);
		ResetFinger(RingPhalanges);
		ResetFinger(PinkyPhalanges);
	}
	else if (FingerIndex == 1) ResetFinger(ThumbPhalanges);
	else if (FingerIndex == 2) ResetFinger(IndexPhalanges);
	else if (FingerIndex == 3) ResetFinger(MiddlePhalanges);
	else if (FingerIndex == 4) ResetFinger(RingPhalanges);
	else if (FingerIndex == 5) ResetFinger(PinkyPhalanges);

	ApplyPhalanxTransforms();
}

void AYenkaHandAvatar::ResetAllPhalanges()
{
	ThumbPhalanges = FFingerPhalanges();
	IndexPhalanges = FFingerPhalanges();
	MiddlePhalanges = FFingerPhalanges();
	RingPhalanges = FFingerPhalanges();
	PinkyPhalanges = FFingerPhalanges();
	ApplyPhalanxTransforms();
}

void AYenkaHandAvatar::LoadPresetPose(EHandPoseMode Mode)
{
	CurrentPoseMode = Mode;

	if (Mode == EHandPoseMode::FingerPoke)
	{
		// Push / PointGesture configuration
		ThumbPhalanges.Proximal = FPhalanxData{ -40.0f, -50.0f, 0.0f };
		ThumbPhalanges.Intermediate = FPhalanxData{ -15.0f, -20.0f, -1.0f };
		ThumbPhalanges.Distal = FPhalanxData{ 5.0f, 25.0f, 0.0f };

		IndexPhalanges.Proximal = FPhalanxData{ 0.0f, 0.0f, -5.0f };
		IndexPhalanges.Intermediate = FPhalanxData{ 0.0f, 0.0f, 15.0f };
		IndexPhalanges.Distal = FPhalanxData{ 0.0f, 0.0f, 15.0f };

		MiddlePhalanges.Proximal = FPhalanxData{ 0.0f, 0.0f, -20.0f };
		MiddlePhalanges.Intermediate = FPhalanxData{ 0.0f, 0.0f, -95.0f };
		MiddlePhalanges.Distal = FPhalanxData{ 0.0f, 0.0f, -60.0f };

		RingPhalanges.Proximal = FPhalanxData{ 0.0f, 0.0f, 20.0f };
		RingPhalanges.Intermediate = FPhalanxData{ 0.0f, 0.0f, 75.0f };
		RingPhalanges.Distal = FPhalanxData{ 0.0f, 0.0f, -80.0f };

		PinkyPhalanges.Proximal = FPhalanxData{ 0.0f, 0.0f, -20.0f };
		PinkyPhalanges.Intermediate = FPhalanxData{ 0.0f, 0.0f, -80.0f };
		PinkyPhalanges.Distal = FPhalanxData{ 0.0f, 0.0f, -95.0f };
	}
	else
	{
		// Flat base
		ThumbPhalanges = FFingerPhalanges();
		IndexPhalanges = FFingerPhalanges();
		MiddlePhalanges = FFingerPhalanges();
		RingPhalanges = FFingerPhalanges();
		PinkyPhalanges = FFingerPhalanges();
	}

	ApplyPhalanxTransforms();
}

FFingerPhalanges AYenkaHandAvatar::GetFingerPhalanges(int32 FingerIndex) const
{
	if (FingerIndex == 1) return ThumbPhalanges;
	if (FingerIndex == 2) return IndexPhalanges;
	if (FingerIndex == 3) return MiddlePhalanges;
	if (FingerIndex == 4) return RingPhalanges;
	if (FingerIndex == 5) return PinkyPhalanges;
	return IndexPhalanges;
}

FString AYenkaHandAvatar::GetDetectedGestureDescription() const
{
	auto GetAvgMag = [](const FFingerPhalanges& F) -> float
	{
		return (FMath::Abs(F.Proximal.Pitch) + FMath::Abs(F.Proximal.Yaw) + FMath::Abs(F.Proximal.Roll) +
				FMath::Abs(F.Intermediate.Pitch) + FMath::Abs(F.Intermediate.Yaw) + FMath::Abs(F.Intermediate.Roll) +
				FMath::Abs(F.Distal.Pitch) + FMath::Abs(F.Distal.Yaw) + FMath::Abs(F.Distal.Roll)) / 3.0f;
	};

	float IndexAvg = GetAvgMag(IndexPhalanges);
	float MiddleAvg = GetAvgMag(MiddlePhalanges);
	float RingAvg = GetAvgMag(RingPhalanges);
	float PinkyAvg = GetAvgMag(PinkyPhalanges);
	float ThumbAvg = GetAvgMag(ThumbPhalanges);
	float OthersAvg = (MiddleAvg + RingAvg + PinkyAvg) / 3.0f;

	if (IndexAvg < 20.0f && OthersAvg > 40.0f)
	{
		return TEXT("👉 EMPUJAR / SEÑALAR (FingerPoke)");
	}
	else if (IndexAvg > 15.0f && IndexAvg < 60.0f && ThumbAvg > 15.0f && OthersAvg > 40.0f)
	{
		return TEXT("🤏 PINZA / AGARRAR (GrabPinch)");
	}
	else if (IndexAvg < 20.0f && OthersAvg < 20.0f && ThumbAvg < 20.0f)
	{
		return TEXT("🖐️ MANO PLANA EXTENDIDA (OpenHand Base)");
	}
	else if (IndexAvg > 50.0f && OthersAvg > 50.0f && ThumbAvg > 35.0f)
	{
		return TEXT("✊ PUÑO CERRADO (Fist)");
	}
	return TEXT("🎨 GESTO PERSONALIZADO (Custom Pose)");
}

FCustomHandGesture AYenkaHandAvatar::ExportCurrentGesture(const FString& Name, const FVector& LocOffset, const FRotator& RotOffset) const
{
	FCustomHandGesture OutGesture;
	OutGesture.GestureName = Name.IsEmpty() ? TEXT("GestoPersonalizado") : Name;
	OutGesture.Thumb = ThumbPhalanges;
	OutGesture.Index = IndexPhalanges;
	OutGesture.Middle = MiddlePhalanges;
	OutGesture.Ring = RingPhalanges;
	OutGesture.Pinky = PinkyPhalanges;
	OutGesture.HandLocationOffset = LocOffset;
	OutGesture.HandRotationOffset = RotOffset;
	return OutGesture;
}

void AYenkaHandAvatar::ApplyCustomGesture(const FCustomHandGesture& InGesture)
{
	ThumbPhalanges = InGesture.Thumb;
	IndexPhalanges = InGesture.Index;
	MiddlePhalanges = InGesture.Middle;
	RingPhalanges = InGesture.Ring;
	PinkyPhalanges = InGesture.Pinky;
	ApplyPhalanxTransforms();
}

FRotator AYenkaHandAvatar::GetPhalanxDeltaRotationForBone(FName BoneName) const
{
	FString Name = BoneName.ToString().ToLower();

	auto ToRot = [](const FPhalanxData& P) -> FRotator
	{
		return FRotator(P.Pitch, P.Yaw, P.Roll);
	};

	// Thumb
	if (Name.Contains(TEXT("thumb")))
	{
		if (Name.Contains(TEXT("01")) || Name.Contains(TEXT("metacarpal")) || (Name.Contains(TEXT("proximal")) && !Name.Contains(TEXT("intermediate")) && !Name.Contains(TEXT("distal"))))
		{
			return ToRot(ThumbPhalanges.Proximal);
		}
		if (Name.Contains(TEXT("02")) || Name.Contains(TEXT("intermediate")) || Name.Contains(TEXT("proximal")))
		{
			return ToRot(ThumbPhalanges.Intermediate);
		}
		if (Name.Contains(TEXT("03")) || Name.Contains(TEXT("distal")))
		{
			return ToRot(ThumbPhalanges.Distal);
		}
	}

	// Index
	if (Name.Contains(TEXT("index")))
	{
		if (Name.Contains(TEXT("01")) || Name.Contains(TEXT("metacarpal")) || (Name.Contains(TEXT("proximal")) && !Name.Contains(TEXT("intermediate")) && !Name.Contains(TEXT("distal"))))
		{
			return ToRot(IndexPhalanges.Proximal);
		}
		if (Name.Contains(TEXT("02")) || Name.Contains(TEXT("intermediate")))
		{
			return ToRot(IndexPhalanges.Intermediate);
		}
		if (Name.Contains(TEXT("03")) || Name.Contains(TEXT("distal")))
		{
			return ToRot(IndexPhalanges.Distal);
		}
	}

	// Middle
	if (Name.Contains(TEXT("middle")))
	{
		if (Name.Contains(TEXT("01")) || Name.Contains(TEXT("metacarpal")) || (Name.Contains(TEXT("proximal")) && !Name.Contains(TEXT("intermediate")) && !Name.Contains(TEXT("distal"))))
		{
			return ToRot(MiddlePhalanges.Proximal);
		}
		if (Name.Contains(TEXT("02")) || Name.Contains(TEXT("intermediate")))
		{
			return ToRot(MiddlePhalanges.Intermediate);
		}
		if (Name.Contains(TEXT("03")) || Name.Contains(TEXT("distal")))
		{
			return ToRot(MiddlePhalanges.Distal);
		}
	}

	// Ring
	if (Name.Contains(TEXT("ring")))
	{
		if (Name.Contains(TEXT("01")) || Name.Contains(TEXT("metacarpal")) || (Name.Contains(TEXT("proximal")) && !Name.Contains(TEXT("intermediate")) && !Name.Contains(TEXT("distal"))))
		{
			return ToRot(RingPhalanges.Proximal);
		}
		if (Name.Contains(TEXT("02")) || Name.Contains(TEXT("intermediate")))
		{
			return ToRot(RingPhalanges.Intermediate);
		}
		if (Name.Contains(TEXT("03")) || Name.Contains(TEXT("distal")))
		{
			return ToRot(RingPhalanges.Distal);
		}
	}

	// Pinky
	if (Name.Contains(TEXT("pinky")) || Name.Contains(TEXT("little")))
	{
		if (Name.Contains(TEXT("01")) || Name.Contains(TEXT("metacarpal")) || (Name.Contains(TEXT("proximal")) && !Name.Contains(TEXT("intermediate")) && !Name.Contains(TEXT("distal"))))
		{
			return ToRot(PinkyPhalanges.Proximal);
		}
		if (Name.Contains(TEXT("02")) || Name.Contains(TEXT("intermediate")))
		{
			return ToRot(PinkyPhalanges.Intermediate);
		}
		if (Name.Contains(TEXT("03")) || Name.Contains(TEXT("distal")))
		{
			return ToRot(PinkyPhalanges.Distal);
		}
	}

	return FRotator::ZeroRotator;
}

void AYenkaHandAvatar::ApplyPhalanxTransforms()
{
	if (PoseableHandMesh && PoseableHandMesh->GetSkinnedAsset())
	{
		const FReferenceSkeleton& RefSkeleton = PoseableHandMesh->GetSkinnedAsset()->GetRefSkeleton();
		const int32 NumBones = RefSkeleton.GetNum();
		const TArray<FTransform>& RefBonePoses = RefSkeleton.GetRefBonePose();

		TArray<FTransform> CompTransforms;
		CompTransforms.SetNum(NumBones);

		for (int32 i = 0; i < NumBones; ++i)
		{
			FTransform LocalTransform = RefBonePoses[i];
			FName BoneName = RefSkeleton.GetBoneName(i);

			FRotator DeltaRot = GetPhalanxDeltaRotationForBone(BoneName);
			if (!DeltaRot.IsNearlyZero())
			{
				FQuat DeltaQuat = DeltaRot.Quaternion();
				LocalTransform.SetRotation(LocalTransform.GetRotation() * DeltaQuat);
			}

			int32 ParentIndex = RefSkeleton.GetParentIndex(i);
			if (ParentIndex != INDEX_NONE && CompTransforms.IsValidIndex(ParentIndex))
			{
				CompTransforms[i] = LocalTransform * CompTransforms[ParentIndex];
			}
			else
			{
				CompTransforms[i] = LocalTransform;
			}

			PoseableHandMesh->SetBoneTransformByName(BoneName, CompTransforms[i], EBoneSpaces::ComponentSpace);
		}
	}

	// Update procedural mesh rotations based on phalanx pitch and yaw
	if (IndexFinger)
	{
		float TotalPitch = IndexPhalanges.Proximal.Pitch + IndexPhalanges.Intermediate.Pitch * 0.5f + IndexPhalanges.Distal.Pitch * 0.3f;
		float TotalYaw = IndexPhalanges.Proximal.Yaw + IndexPhalanges.Intermediate.Yaw;
		IndexFinger->SetRelativeLocation(FVector(3.5f, -1.5f, 0.0f));
		IndexFinger->SetRelativeRotation(FRotator(90.0f - TotalPitch, TotalYaw, 0.0f));
	}
	if (MiddleFinger)
	{
		float TotalPitch = MiddlePhalanges.Proximal.Pitch + MiddlePhalanges.Intermediate.Pitch * 0.5f + MiddlePhalanges.Distal.Pitch * 0.3f;
		float TotalYaw = MiddlePhalanges.Proximal.Yaw + MiddlePhalanges.Intermediate.Yaw;
		MiddleFinger->SetRelativeLocation(FVector(3.8f, -0.4f, 0.0f));
		MiddleFinger->SetRelativeRotation(FRotator(90.0f - TotalPitch, TotalYaw, 0.0f));
	}
	if (RingFinger)
	{
		float TotalPitch = RingPhalanges.Proximal.Pitch + RingPhalanges.Intermediate.Pitch * 0.5f + RingPhalanges.Distal.Pitch * 0.3f;
		float TotalYaw = RingPhalanges.Proximal.Yaw + RingPhalanges.Intermediate.Yaw;
		RingFinger->SetRelativeLocation(FVector(3.5f, 0.6f, 0.0f));
		RingFinger->SetRelativeRotation(FRotator(90.0f - TotalPitch, TotalYaw, 0.0f));
	}
	if (PinkyFinger)
	{
		float TotalPitch = PinkyPhalanges.Proximal.Pitch + PinkyPhalanges.Intermediate.Pitch * 0.5f + PinkyPhalanges.Distal.Pitch * 0.3f;
		float TotalYaw = PinkyPhalanges.Proximal.Yaw + PinkyPhalanges.Intermediate.Yaw;
		PinkyFinger->SetRelativeLocation(FVector(3.0f, 1.5f, 0.0f));
		PinkyFinger->SetRelativeRotation(FRotator(90.0f - TotalPitch, TotalYaw, 0.0f));
	}
	if (ThumbMesh)
	{
		float TotalPitch = ThumbPhalanges.Proximal.Pitch + ThumbPhalanges.Intermediate.Pitch * 0.5f + ThumbPhalanges.Distal.Pitch * 0.3f;
		float TotalYaw = ThumbPhalanges.Proximal.Yaw + ThumbPhalanges.Intermediate.Yaw;
		ThumbMesh->SetRelativeLocation(FVector(1.0f, -2.2f, 0.0f));
		ThumbMesh->SetRelativeRotation(FRotator(TotalPitch, -40.0f + TotalYaw, 0.0f));
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
