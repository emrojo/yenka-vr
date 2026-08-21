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

	LayerIndex = 0;
	bIsPlacedOnTop = false;
}

void AYenkaBlock::BeginPlay()
{
	Super::BeginPlay();

	// Apply realistic warm wood color dynamic material
	UMaterialInterface* BaseMat = BlockMesh ? BlockMesh->GetMaterial(0) : nullptr;
	if (BlockMesh && BaseMat)
	{
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.62f, 0.38f, 1.0f));
			DynMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.85f, 0.62f, 0.38f, 1.0f));
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

	if (bActive)
	{
		BlockMesh->SetSimulatePhysics(true);
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
