#include "YenkaTowerManager.h"
#include "YenkaBlock.h"
#include "YenkaVR.h"
#include "Engine/World.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AYenkaTowerManager::AYenkaTowerManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	TableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableMesh"));
	RootComponent = TableMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		TableMesh->SetStaticMesh(CubeMeshAsset.Object);
		TableMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.04f)); // 35cm x 35cm board, 4cm thick
		TableMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f)); // Top surface at Z = 0
	}
	TableMesh->SetCollisionProfileName(TEXT("BlockAll"));
	TableMesh->SetSimulatePhysics(false);
	TableMesh->SetEnableGravity(false);

	BlockClass = AYenkaBlock::StaticClass();
	BlockDimensions = FVector(7.5f, 2.5f, 1.5f);
	TableSurfaceZ = 0.0f;
}

void AYenkaTowerManager::BeginPlay()
{
	Super::BeginPlay();
	TableSurfaceZ = GetActorLocation().Z;

	if (HasAuthority())
	{
		SpawnTower();
	}
}

void AYenkaTowerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && ActiveBlocks.Num() > 0)
	{
		int32 FallenCount = 0;
		for (AYenkaBlock* Block : ActiveBlocks)
		{
			if (Block && Block->HasFallen(TableSurfaceZ - 50.0f))
			{
				FallenCount++;
			}
		}

		// If more than 2 blocks have fallen down, tower collapsed
		if (FallenCount > 2)
		{
			OnTowerCollapsed.Broadcast();
		}
	}
}

void AYenkaTowerManager::SpawnTower()
{
	if (!HasAuthority() || !BlockClass) return;

	ResetTower();

	const int32 TotalLayers = 18;
	const int32 BlocksPerLayer = 3;
	FVector Origin = GetActorLocation();
	float BaseZ = TableSurfaceZ;

	for (int32 Layer = 0; Layer < TotalLayers; ++Layer)
	{
		bool bIsEvenLayer = (Layer % 2 == 0);
		// 0.05mm micro-clearance between layers prevents physics overlap explosion
		float CurrentZ = BaseZ + (Layer * 1.505f) + 0.75f;

		for (int32 i = 0; i < BlocksPerLayer; ++i)
		{
			float Offset = (i - 1) * 2.505f;
			FVector BlockPos;
			FRotator BlockRot;

			if (bIsEvenLayer)
			{
				BlockPos = FVector(Origin.X, Origin.Y + Offset, CurrentZ);
				BlockRot = FRotator(0.0f, 0.0f, 0.0f);
			}
			else
			{
				BlockPos = FVector(Origin.X + Offset, Origin.Y, CurrentZ);
				BlockRot = FRotator(0.0f, 90.0f, 0.0f);
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			AYenkaBlock* NewBlock = GetWorld()->SpawnActor<AYenkaBlock>(BlockClass, BlockPos, BlockRot, SpawnParams);
			if (NewBlock)
			{
				NewBlock->LayerIndex = Layer;
				ActiveBlocks.Add(NewBlock);
			}
		}
	}

	// Activate full physics simulation with Chaos dynamics and gravity
	WakeTowerForPlayer(nullptr);
	UE_LOG(LogYenkaVR, Log, TEXT("Spawned 54-block Yenka tower successfully with full dynamic physics."));
}

void AYenkaTowerManager::ResetTower()
{
	for (AYenkaBlock* Block : ActiveBlocks)
	{
		if (Block)
		{
			Block->Destroy();
		}
	}
	ActiveBlocks.Empty();
}

void AYenkaTowerManager::FreezeTowerPhysics()
{
	for (AYenkaBlock* Block : ActiveBlocks)
	{
		if (Block && Block->BlockMesh)
		{
			Block->BlockMesh->PutRigidBodyToSleep();
		}
	}
}

void AYenkaTowerManager::WakeTowerForPlayer(APlayerController* ActivePlayer)
{
	for (AYenkaBlock* Block : ActiveBlocks)
	{
		if (Block && Block->BlockMesh)
		{
			Block->AssignTurnAuthority(ActivePlayer);
			Block->BlockMesh->SetSimulatePhysics(true);
			Block->BlockMesh->SetEnableGravity(true);
			Block->BlockMesh->WakeRigidBody();
		}
	}
}

bool AYenkaTowerManager::CheckTowerStability()
{
	// Center of mass / velocity check
	for (AYenkaBlock* Block : ActiveBlocks)
	{
		if (Block && Block->BlockMesh)
		{
			if (Block->BlockMesh->GetPhysicsLinearVelocity().Size() > 2.0f)
			{
				return false; // Still moving
			}
		}
	}
	return true;
}
