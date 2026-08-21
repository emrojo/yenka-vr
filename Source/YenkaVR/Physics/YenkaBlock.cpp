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

	// Dimensions: Standard Jenga block ~ 7.5cm x 2.5cm x 1.5cm (scaled to cm in UE5)
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
