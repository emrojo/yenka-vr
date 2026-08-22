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
	BlockMesh->SetSimulatePhysics(true);
	BlockMesh->SetEnableGravity(true);
	BlockMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	BlockMesh->BodyInstance.bUseCCD = true; // Continuous Collision Detection
	BlockMesh->BodyInstance.PositionSolverIterationCount = 16;
	BlockMesh->BodyInstance.VelocitySolverIterationCount = 8;
	BlockMesh->SetMassOverrideInKg(NAME_None, 0.085f, true); // ~85g per block
	BlockMesh->SetLinearDamping(1.0f);
	BlockMesh->SetAngularDamping(1.0f);

	LayerIndex = 0;
	bIsPlacedOnTop = false;
	ColorIndex = 0;
	BlockColor = FLinearColor(0.88f, 0.16f, 0.18f, 1.0f);
}

#include "PhysicalMaterials/PhysicalMaterial.h"

void AYenkaBlock::BeginPlay()
{
	Super::BeginPlay();

	UpdateMaterialColor();

	if (BlockMesh)
	{
		if (WoodPhysicalMaterial)
		{
			BlockMesh->SetPhysMaterialOverride(WoodPhysicalMaterial);
		}
		else
		{
			UPhysicalMaterial* DynPhysMat = NewObject<UPhysicalMaterial>(this, TEXT("DynWoodPhysMat"));
			if (DynPhysMat)
			{
				// Reduced friction allows individual blocks to slide out smoothly without toppling the whole layer
				DynPhysMat->Friction = 0.20f;
				DynPhysMat->StaticFriction = 0.22f;
				DynPhysMat->Restitution = 0.05f;
				DynPhysMat->FrictionCombineMode = EFrictionCombineMode::Min;
				DynPhysMat->RestitutionCombineMode = EFrictionCombineMode::Min;
				BlockMesh->SetPhysMaterialOverride(DynPhysMat);
			}
		}
	}
}

void AYenkaBlock::ApplyColor(int32 InColorIndex, const FLinearColor& InColor)
{
	ColorIndex = InColorIndex;
	BlockColor = InColor;
	UpdateMaterialColor();
}

void AYenkaBlock::OnRep_BlockColor()
{
	UpdateMaterialColor();
}

void AYenkaBlock::UpdateMaterialColor()
{
	if (!BlockMesh) return;

	UMaterialInterface* CurrentMat = BlockMesh->GetMaterial(0);
	UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(CurrentMat);

	if (!DynMat && CurrentMat)
	{
		DynMat = UMaterialInstanceDynamic::Create(CurrentMat, this);
		BlockMesh->SetMaterial(0, DynMat);
	}

	if (DynMat)
	{
		// Preserve natural wood sheen and roughness while applying distinct stained dye
		float HashVal = static_cast<float>(GetTypeHash(GetActorLocation().ToString()) % 100);
		float Var = (HashVal / 100.0f) * 0.06f - 0.03f;
		FLinearColor FinalColor(
			FMath::Clamp(BlockColor.R + Var, 0.0f, 1.0f),
			FMath::Clamp(BlockColor.G + Var, 0.0f, 1.0f),
			FMath::Clamp(BlockColor.B + Var, 0.0f, 1.0f),
			1.0f
		);

		DynMat->SetVectorParameterValue(TEXT("Color"), FinalColor);
		DynMat->SetScalarParameterValue(TEXT("Roughness"), 0.32f);
		DynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
	}
}

void AYenkaBlock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AYenkaBlock, LayerIndex);
	DOREPLIFETIME(AYenkaBlock, bIsPlacedOnTop);
	DOREPLIFETIME(AYenkaBlock, BlockColor);
	DOREPLIFETIME(AYenkaBlock, ColorIndex);
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
