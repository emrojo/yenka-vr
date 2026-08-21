#include "YenkaTowerManager.h"
#include "YenkaBlock.h"
#include "YenkaVR.h"
#include "Engine/World.h"

AYenkaTowerManager::AYenkaTowerManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	BlockDimensions = FVector(7.5f, 2.5f, 1.5f);
	TableSurfaceZ = 0.0f;
}

void AYenkaTowerManager::BeginPlay()
{
	Super::BeginPlay();
	TableSurfaceZ = GetActorLocation().Z;
}

void AYenkaTowerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && ActiveBlocks.Num() > 0)
	{
		int32 FallenCount = 0;
		for (AYenkaBlock* Block : ActiveBlocks)
		{
			if (Block && Block->HasFallen(TableSurfaceZ - 5.0f))
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

	for (int32 Layer = 0; Layer < TotalLayers; ++Layer)
	{
		bool bIsEvenLayer = (Layer % 2 == 0);
		float CurrentZ = Origin.Z + (Layer * BlockDimensions.Z) + (BlockDimensions.Z * 0.5f);

		for (int32 i = 0; i < BlocksPerLayer; ++i)
		{
			float Offset = (i - 1) * BlockDimensions.Y;
			FVector BlockPos;
			FRotator BlockRot;

			if (bIsEvenLayer)
			{
				BlockPos = Origin + FVector(0.0f, Offset, CurrentZ - Origin.Z);
				BlockRot = FRotator(0.0f, 0.0f, 0.0f);
			}
			else
			{
				BlockPos = Origin + FVector(Offset, 0.0f, CurrentZ - Origin.Z);
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

	FreezeTowerPhysics();
	UE_LOG(LogYenkaVR, Log, TEXT("Spawned 54-block Yenka tower successfully."));
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
		if (Block)
		{
			Block->SetPhysicsActive(false);
		}
	}
}

void AYenkaTowerManager::WakeTowerForPlayer(APlayerController* ActivePlayer)
{
	for (AYenkaBlock* Block : ActiveBlocks)
	{
		if (Block)
		{
			Block->AssignTurnAuthority(ActivePlayer);
			Block->SetPhysicsActive(true);
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
