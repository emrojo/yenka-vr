#include "YenkaBlock.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AYenkaBlock::AYenkaBlock()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMesh"));
	RootComponent = BlockMesh;

	// Assign default engine Cube mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		BlockMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	// Dimensions: Standard Jenga block ~ 7.5cm x 2.5cm x 1.5cm (Cube is 100x100x100 cm)
	SetActorScale3D(FVector(0.075f, 0.025f, 0.015f));
	BlockMesh->SetRelativeScale3D(FVector(0.075f, 0.025f, 0.015f));
	BlockMesh->SetSimulatePhysics(false); // Initially kinematic to prevent spawn explosion
	BlockMesh->SetEnableGravity(true);
	BlockMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	BlockMesh->BodyInstance.bUseCCD = true; // Continuous Collision Detection
	BlockMesh->SetMassOverrideInKg(NAME_None, 0.085f, true); // ~85g per block
	BlockMesh->SetLinearDamping(0.8f);
	BlockMesh->SetAngularDamping(0.8f);

	LayerIndex = 0;
	bIsPlacedOnTop = false;
}

void AYenkaBlock::BeginPlay()
{
	Super::BeginPlay();

	// Apply rich polished wood material with natural tone variations
	UMaterialInterface* BaseMat = BlockMesh ? BlockMesh->GetMaterial(0) : nullptr;
	if (BlockMesh && BaseMat)
	{
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (DynMat)
		{
			// Natural polished birch / oak wood color variation
			float Var = (static_cast<float>(GetTypeHash(GetActorLocation().ToString()) % 100) / 100.0f) * 0.12f - 0.06f;
			FLinearColor WoodTone(0.82f + Var, 0.58f + (Var * 0.8f), 0.36f + (Var * 0.5f), 1.0f);

			DynMat->SetVectorParameterValue(TEXT("Color"), WoodTone);
			DynMat->SetVectorParameterValue(TEXT("BaseColor"), WoodTone);
			DynMat->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
			DynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			DynMat->SetScalarParameterValue(TEXT("Specular"), 0.45f);
			BlockMesh->SetMaterial(0, DynMat);
		}
	}

	if (WoodPhysicalMaterial && BlockMesh)
	{
		BlockMesh->SetPhysMaterialOverride(WoodPhysicalMaterial);
	}
}

void AYenkaBlock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AYenkaBlock, LayerIndex);
	DOREPLIFETIME(AYenkaBlock, bIsPlacedOnTop);
}

void AYenkaBlock::SetPhysicsActive(bool bActive)
{
	if (!BlockMesh) return;

	BlockMesh->SetSimulatePhysics(true);
	BlockMesh->SetEnableGravity(true);

	if (bActive)
	{
		BlockMesh->WakeRigidBody();
	}
	else
	{
		BlockMesh->PutRigidBodyToSleep();
	}
}

void AYenkaBlock::AssignTurnAuthority(APlayerController* NewOwner)
{
	if (HasAuthority())
	{
		SetOwner(NewOwner);
	}
}

bool AYenkaBlock::HasFallen(float TableZThreshold) const
{
	return GetActorLocation().Z < TableZThreshold;
}
