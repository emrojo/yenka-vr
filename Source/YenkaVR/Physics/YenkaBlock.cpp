#include "YenkaBlock.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Net/UnrealNetwork.h"

AYenkaBlock::AYenkaBlock()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMesh"));
	RootComponent = BlockMesh;

	// Assign default engine Cube mesh so blocks are immediately visible
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		BlockMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	// Dimensions: Standard Jenga block ~ 7.5cm x 2.5cm x 1.5cm (Cube is 100x100x100 cm)
	BlockMesh->SetRelativeScale3D(FVector(0.075f, 0.025f, 0.015f));
	BlockMesh->SetSimulatePhysics(true);
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
