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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TableBaseMatAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (TableBaseMatAsset.Succeeded())
	{
		TableMesh->SetMaterial(0, TableBaseMatAsset.Object);
	}

	TableMesh->SetCollisionProfileName(TEXT("BlockAll"));
	TableMesh->SetSimulatePhysics(false);
	TableMesh->SetEnableGravity(false);

	BlockClass = AYenkaBlock::StaticClass();
	BlockDimensions = FVector(7.5f, 2.5f, 1.5f);
	TableSurfaceZ = 0.0f;

	// 7 Basic colors palette with vibrant wood-stain hues
	BlockColorPalette = {
		FLinearColor(0.88f, 0.16f, 0.18f, 1.0f), // 0: Red (Rojo)
		FLinearColor(0.95f, 0.45f, 0.08f, 1.0f), // 1: Orange (Naranja)
		FLinearColor(0.96f, 0.80f, 0.12f, 1.0f), // 2: Yellow (Amarillo)
		FLinearColor(0.16f, 0.72f, 0.26f, 1.0f), // 3: Green (Verde)
		FLinearColor(0.10f, 0.72f, 0.88f, 1.0f), // 4: Cyan / Light Blue (Cian)
		FLinearColor(0.18f, 0.35f, 0.90f, 1.0f), // 5: Blue (Azul)
		FLinearColor(0.68f, 0.20f, 0.82f, 1.0f)  // 6: Purple / Magenta (Púrpura)
	};
}

#include "Materials/MaterialInstanceDynamic.h"

void AYenkaTowerManager::BeginPlay()
{
	Super::BeginPlay();
	TableSurfaceZ = GetActorLocation().Z;

	// Apply dark walnut wood material to the table board
	UMaterialInterface* BaseMat = TableMesh ? TableMesh->GetMaterial(0) : nullptr;
	if (BaseMat)
	{
		UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.15f, 0.08f, 1.0f));
			DynMat->SetScalarParameterValue(TEXT("Roughness"), 0.4f);
			DynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			TableMesh->SetMaterial(0, DynMat);
		}
	}

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
	const int32 PaletteSize = (BlockColorPalette.Num() > 0) ? BlockColorPalette.Num() : 7;
	FVector Origin = GetActorLocation();
	float BaseZ = TableSurfaceZ;
	int32 GlobalBlockIndex = 0;

	for (int32 Layer = 0; Layer < TotalLayers; ++Layer)
	{
		bool bIsEvenLayer = (Layer % 2 == 0);
		// 0.4mm vertical clearance per layer prevents initial physics impulse
		float CurrentZ = BaseZ + (Layer * 1.504f) + 0.75f;

		// Random spacing between 1mm and 3mm (0.10cm to 0.30cm) between adjacent blocks
		float GapLeft = FMath::FRandRange(0.10f, 0.30f);
		float GapRight = FMath::FRandRange(0.10f, 0.30f);

		for (int32 i = 0; i < BlocksPerLayer; ++i)
		{
			float LateralOffset = 0.0f;
			if (i == 0)
			{
				LateralOffset = -(2.5f + GapLeft);
			}
			else if (i == 2)
			{
				LateralOffset = +(2.5f + GapRight);
			}

			// Slight organic 1mm longitudinal jitter for realistic hand-built look
			float LongitudinalJitter = FMath::FRandRange(-0.10f, 0.10f);

			FVector BlockPos;
			FRotator BlockRot;

			if (bIsEvenLayer)
			{
				BlockPos = FVector(Origin.X + LongitudinalJitter, Origin.Y + LateralOffset, CurrentZ);
				BlockRot = FRotator(0.0f, 0.0f, 0.0f);
			}
			else
			{
				BlockPos = FVector(Origin.X + LateralOffset, Origin.Y + LongitudinalJitter, CurrentZ);
				BlockRot = FRotator(0.0f, 90.0f, 0.0f);
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			AYenkaBlock* NewBlock = GetWorld()->SpawnActor<AYenkaBlock>(BlockClass, BlockPos, BlockRot, SpawnParams);
			if (NewBlock)
			{
				NewBlock->LayerIndex = Layer;

				// Distribute 7 basic colors evenly across all 54 blocks (gcd(3,7)=1 ensures contrasting adjacent colors)
				int32 ColorIdx = GlobalBlockIndex % PaletteSize;
				if (BlockColorPalette.IsValidIndex(ColorIdx))
				{
					NewBlock->ApplyColor(ColorIdx, BlockColorPalette[ColorIdx]);
				}

				ActiveBlocks.Add(NewBlock);
				GlobalBlockIndex++;
			}
		}
	}

	// Keep all 54 blocks in stable sleep state on spawn until player touches them
	FreezeTowerPhysics();
	UE_LOG(LogYenkaVR, Log, TEXT("Spawned 54-block Yenka tower with 1-3mm random spacing and 7-color distribution in stable sleep state."));
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
