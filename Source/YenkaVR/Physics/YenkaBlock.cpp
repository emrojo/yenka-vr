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

	// Assign default engine Cube mesh and BasicShapeMaterial with color parameter
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		BlockMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodBaseMatAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (WoodBaseMatAsset.Succeeded())
	{
		BlockMesh->SetMaterial(0, WoodBaseMatAsset.Object);
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
			// Natural polished beech / oak wood tone variation
			float HashVal = static_cast<float>(GetTypeHash(GetActorLocation().ToString()) % 100);
			float Var = (HashVal / 100.0f) * 0.14f - 0.07f;
			FLinearColor WoodTone(0.84f + Var, 0.58f + (Var * 0.75f), 0.33f + (Var * 0.5f), 1.0f);

			DynMat->SetVectorParameterValue(TEXT("Color"), WoodTone);
			DynMat->SetScalarParameterValue(TEXT("Roughness"), 0.32f);
			DynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
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
