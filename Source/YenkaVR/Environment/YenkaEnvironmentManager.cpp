#include "YenkaEnvironmentManager.h"
#include "YenkaVR/Physics/YenkaTowerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AYenkaEnvironmentManager::AYenkaEnvironmentManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CubeMesh = CubeMeshAsset.Succeeded() ? CubeMeshAsset.Object : nullptr;
	UStaticMesh* PlaneMesh = PlaneMeshAsset.Succeeded() ? PlaneMeshAsset.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatAsset.Succeeded() ? BaseMatAsset.Object : nullptr;

	// 1. Floor (7m x 7m room)
	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		FloorMesh->SetStaticMesh(CubeMesh);
		FloorMesh->SetRelativeScale3D(FVector(7.0f, 7.0f, 0.05f));
		FloorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.5f)); // Top at Z = 0
		if (BaseMat) FloorMesh->SetMaterial(0, BaseMat);
	}
	FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 2. Thematic Area Rug / Carpet under the table (2.4m x 2.0m)
	CarpetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarpetMesh"));
	CarpetMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		CarpetMesh->SetStaticMesh(CubeMesh);
		CarpetMesh->SetRelativeScale3D(FVector(2.4f, 2.0f, 0.01f));
		CarpetMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.5f));
		if (BaseMat) CarpetMesh->SetMaterial(0, BaseMat);
	}
	CarpetMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// 3. Surrounding Large Table Top (1.40m x 1.00m, 4cm thick, top at Z = 88cm right under the tower board)
	TableDeskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableDeskMesh"));
	TableDeskMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		TableDeskMesh->SetStaticMesh(CubeMesh);
		TableDeskMesh->SetRelativeScale3D(FVector(1.40f, 1.00f, 0.04f));
		TableDeskMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 86.0f));
		if (BaseMat) TableDeskMesh->SetMaterial(0, BaseMat);
	}
	TableDeskMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 4. Sturdy Table Legs (4 legs reaching from table down to floor Z = 0)
	auto SetupLeg = [this, CubeMesh, BaseMat](UStaticMeshComponent*& LegComp, const FString& Name, const FVector& Loc)
	{
		LegComp = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		LegComp->SetupAttachment(SceneRoot);
		if (CubeMesh)
		{
			LegComp->SetStaticMesh(CubeMesh);
			LegComp->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.86f));
			LegComp->SetRelativeLocation(Loc);
			if (BaseMat) LegComp->SetMaterial(0, BaseMat);
		}
		LegComp->SetCollisionProfileName(TEXT("BlockAll"));
	};

	SetupLeg(TableLeg1, TEXT("TableLeg1"), FVector(55.0f, 40.0f, 43.0f));
	SetupLeg(TableLeg2, TEXT("TableLeg2"), FVector(-55.0f, 40.0f, 43.0f));
	SetupLeg(TableLeg3, TEXT("TableLeg3"), FVector(55.0f, -40.0f, 43.0f));
	SetupLeg(TableLeg4, TEXT("TableLeg4"), FVector(-55.0f, -40.0f, 43.0f));

	// 5. Ceiling (7m x 7m at Z = 3.0m)
	CeilingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CeilingMesh"));
	CeilingMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		CeilingMesh->SetStaticMesh(CubeMesh);
		CeilingMesh->SetRelativeScale3D(FVector(7.0f, 7.0f, 0.05f));
		CeilingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
		if (BaseMat) CeilingMesh->SetMaterial(0, BaseMat);
	}
	CeilingMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// 6. Back Wall (In front of player at +X = 3.5m)
	BackWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackWallMesh"));
	BackWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		BackWallMesh->SetStaticMesh(CubeMesh);
		BackWallMesh->SetRelativeScale3D(FVector(0.1f, 7.0f, 3.0f));
		BackWallMesh->SetRelativeLocation(FVector(350.0f, 0.0f, 150.0f));
		if (BaseMat) BackWallMesh->SetMaterial(0, BaseMat);
	}
	BackWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 7. Front Wall (Behind player at -X = -3.5m)
	FrontWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontWallMesh"));
	FrontWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		FrontWallMesh->SetStaticMesh(CubeMesh);
		FrontWallMesh->SetRelativeScale3D(FVector(0.1f, 7.0f, 3.0f));
		FrontWallMesh->SetRelativeLocation(FVector(-350.0f, 0.0f, 150.0f));
		if (BaseMat) FrontWallMesh->SetMaterial(0, BaseMat);
	}
	FrontWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 8. Left Wall (At -Y = -3.5m)
	LeftWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWallMesh"));
	LeftWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		LeftWallMesh->SetStaticMesh(CubeMesh);
		LeftWallMesh->SetRelativeScale3D(FVector(7.0f, 0.1f, 3.0f));
		LeftWallMesh->SetRelativeLocation(FVector(0.0f, -350.0f, 150.0f));
		if (BaseMat) LeftWallMesh->SetMaterial(0, BaseMat);
	}
	LeftWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 9. Right Wall (At +Y = +3.5m)
	RightWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWallMesh"));
	RightWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		RightWallMesh->SetStaticMesh(CubeMesh);
		RightWallMesh->SetRelativeScale3D(FVector(7.0f, 0.1f, 3.0f));
		RightWallMesh->SetRelativeLocation(FVector(0.0f, 350.0f, 150.0f));
		if (BaseMat) RightWallMesh->SetMaterial(0, BaseMat);
	}
	RightWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 10. Large Panoramic Window Vista on Back Wall (3.8m x 1.8m)
	WindowVistaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WindowVistaMesh"));
	WindowVistaMesh->SetupAttachment(SceneRoot);
	if (PlaneMesh)
	{
		WindowVistaMesh->SetStaticMesh(PlaneMesh);
		WindowVistaMesh->SetRelativeScale3D(FVector(2.0f, 4.2f, 1.0f));
		WindowVistaMesh->SetRelativeLocation(FVector(340.0f, 0.0f, 160.0f));
		WindowVistaMesh->SetRelativeRotation(FRotator(90.0f, 180.0f, 0.0f));
		if (BaseMat) WindowVistaMesh->SetMaterial(0, BaseMat);
	}
	WindowVistaMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// 11. Feature Prop / Credenza / Fireplace Hearth
	FeaturePropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FeaturePropMesh"));
	FeaturePropMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		FeaturePropMesh->SetStaticMesh(CubeMesh);
		FeaturePropMesh->SetRelativeScale3D(FVector(0.4f, 2.0f, 0.9f));
		FeaturePropMesh->SetRelativeLocation(FVector(330.0f, 0.0f, 45.0f));
		if (BaseMat) FeaturePropMesh->SetMaterial(0, BaseMat);
	}
	FeaturePropMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 12. Main Tower Spotlight pointing straight down at Yenka Tower (0, 0, 90)
	TowerSpotlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("TowerSpotlight"));
	TowerSpotlight->SetupAttachment(SceneRoot);
	TowerSpotlight->SetRelativeLocation(FVector(0.0f, 0.0f, 280.0f));
	TowerSpotlight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TowerSpotlight->SetIntensity(4500.0f);
	TowerSpotlight->SetInnerConeAngle(24.0f);
	TowerSpotlight->SetOuterConeAngle(42.0f);
	TowerSpotlight->SetAttenuationRadius(450.0f);
	TowerSpotlight->SetCastShadows(true);

	// 13. Left Accent Light
	AccentLight1 = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight1"));
	AccentLight1->SetupAttachment(SceneRoot);
	AccentLight1->SetRelativeLocation(FVector(180.0f, -220.0f, 190.0f));
	AccentLight1->SetIntensity(1500.0f);
	AccentLight1->SetAttenuationRadius(500.0f);

	// 14. Right Accent Light
	AccentLight2 = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight2"));
	AccentLight2->SetupAttachment(SceneRoot);
	AccentLight2->SetRelativeLocation(FVector(180.0f, 220.0f, 190.0f));
	AccentLight2->SetIntensity(1500.0f);
	AccentLight2->SetAttenuationRadius(500.0f);

	// 15. Dynamic Fireplace Light
	FireplaceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireplaceLight"));
	FireplaceLight->SetupAttachment(SceneRoot);
	FireplaceLight->SetRelativeLocation(FVector(310.0f, 0.0f, 45.0f));
	FireplaceLight->SetIntensity(0.0f);
	FireplaceLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.10f));
	FireplaceLight->SetAttenuationRadius(350.0f);

	CurrentTheme = EYenkaEnvironmentTheme::ModernPenthouse;
	FireplaceFlickerTimer = 0.0f;
}

void AYenkaEnvironmentManager::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Dynamic Material Instances from base material
	UMaterialInterface* BaseMat = FloorMesh ? FloorMesh->GetMaterial(0) : nullptr;
	if (!BaseMat)
	{
		BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	if (BaseMat)
	{
		FloorDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		CarpetDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		TableDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		WallDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		VistaDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		FeaturePropDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);

		if (FloorMesh && FloorDynMat) FloorMesh->SetMaterial(0, FloorDynMat);
		if (CarpetMesh && CarpetDynMat) CarpetMesh->SetMaterial(0, CarpetDynMat);

		if (TableDeskMesh && TableDynMat) TableDeskMesh->SetMaterial(0, TableDynMat);
		if (TableLeg1 && TableDynMat) TableLeg1->SetMaterial(0, TableDynMat);
		if (TableLeg2 && TableDynMat) TableLeg2->SetMaterial(0, TableDynMat);
		if (TableLeg3 && TableDynMat) TableLeg3->SetMaterial(0, TableDynMat);
		if (TableLeg4 && TableDynMat) TableLeg4->SetMaterial(0, TableDynMat);

		if (CeilingMesh && WallDynMat) CeilingMesh->SetMaterial(0, WallDynMat);
		if (BackWallMesh && WallDynMat) BackWallMesh->SetMaterial(0, WallDynMat);
		if (FrontWallMesh && WallDynMat) FrontWallMesh->SetMaterial(0, WallDynMat);
		if (LeftWallMesh && WallDynMat) LeftWallMesh->SetMaterial(0, WallDynMat);
		if (RightWallMesh && WallDynMat) RightWallMesh->SetMaterial(0, WallDynMat);

		if (WindowVistaMesh && VistaDynMat) WindowVistaMesh->SetMaterial(0, VistaDynMat);
		if (FeaturePropMesh && FeaturePropDynMat) FeaturePropMesh->SetMaterial(0, FeaturePropDynMat);
	}

	ApplyEnvironmentTheme(CurrentTheme);
}

void AYenkaEnvironmentManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Fireplace dynamic light flicker effect when Cozy Cabin is active
	if (CurrentTheme == EYenkaEnvironmentTheme::CozyCabin && FireplaceLight)
	{
		FireplaceFlickerTimer += DeltaTime * 12.0f;
		const float Noise = (FMath::Sin(FireplaceFlickerTimer) * 0.25f) + (FMath::Sin(FireplaceFlickerTimer * 2.3f) * 0.15f);
		FireplaceLight->SetIntensity(2400.0f + (Noise * 800.0f));
	}
}

void AYenkaEnvironmentManager::ApplyEnvironmentTheme(EYenkaEnvironmentTheme NewTheme)
{
	CurrentTheme = NewTheme;

	AYenkaTowerManager* TowerMgr = Cast<AYenkaTowerManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass()));

	const bool bIsMR = (NewTheme == EYenkaEnvironmentTheme::MixedRealityPassthrough);
	if (FloorMesh) FloorMesh->SetVisibility(!bIsMR);
	if (CarpetMesh) CarpetMesh->SetVisibility(!bIsMR);
	if (TableDeskMesh) TableDeskMesh->SetVisibility(true);
	if (TableLeg1) TableLeg1->SetVisibility(true);
	if (TableLeg2) TableLeg2->SetVisibility(true);
	if (TableLeg3) TableLeg3->SetVisibility(true);
	if (TableLeg4) TableLeg4->SetVisibility(true);
	if (CeilingMesh) CeilingMesh->SetVisibility(!bIsMR);
	if (BackWallMesh) BackWallMesh->SetVisibility(!bIsMR);
	if (FrontWallMesh) FrontWallMesh->SetVisibility(!bIsMR);
	if (LeftWallMesh) LeftWallMesh->SetVisibility(!bIsMR);
	if (RightWallMesh) RightWallMesh->SetVisibility(!bIsMR);

	FString ThemeDisplayName = TEXT("Ático Moderno");

	switch (CurrentTheme)
	{
	case EYenkaEnvironmentTheme::MixedRealityPassthrough:
	{
		ThemeDisplayName = TEXT("Realidad Mixta (Passthrough)");
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(false);

		// Table: Clean frosted minimalist surface
		if (TableDynMat)
		{
			TableDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.14f, 0.16f, 1.0f));
			TableDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
			TableDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.10f);
		}

		// Lights: Soft realistic ambient lighting
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.98f, 0.95f));
			TowerSpotlight->SetIntensity(2800.0f);
		}
		if (AccentLight1) AccentLight1->SetIntensity(0.0f);
		if (AccentLight2) AccentLight2->SetIntensity(0.0f);
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.10f, 0.12f, 0.15f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.20f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::ModernPenthouse:
	{
		ThemeDisplayName = TEXT("Ático de Lujo Moderno");
		// Floor: Rich dark smoked oak parquet
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.05f, 0.03f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
			FloorDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
		}
		// Area Rug: Cream & Charcoal modern geometric rug
		if (CarpetDynMat)
		{
			CarpetDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.20f, 0.20f, 0.22f, 1.0f));
			CarpetDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.90f);
		}
		// Table Desk: Polished Nero Marquina Black Marble with Golden Brass Legs
		if (TableDynMat)
		{
			TableDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.03f, 0.04f, 1.0f));
			TableDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.10f); // Ultra polished
			TableDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.20f);
		}
		// Walls: Contemporary architectural slate grey
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.14f, 0.15f, 0.17f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.70f);
		}
		// Window Vista: Luminous Night City Skyline
		if (VistaDynMat)
		{
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 0.35f, 0.70f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.10f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Warm luxurious interior spotlights
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.85f));
			TowerSpotlight->SetIntensity(4200.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.95f, 0.75f, 0.50f));
			AccentLight1->SetIntensity(1600.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(0.95f, 0.75f, 0.50f));
			AccentLight2->SetIntensity(1600.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.04f, 0.04f, 0.05f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.12f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::CozyCabin:
	{
		ThemeDisplayName = TEXT("Cabaña de Montaña");
		// Floor: Rustic pine planks
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.24f, 0.13f, 0.06f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.55f);
		}
		// Area Rug: Warm red-brown woven wool rug
		if (CarpetDynMat)
		{
			CarpetDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.10f, 0.08f, 1.0f));
			CarpetDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.95f);
		}
		// Table: Heavy hand-carved solid rustic oak table
		if (TableDynMat)
		{
			TableDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.28f, 0.15f, 0.08f, 1.0f));
			TableDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.45f);
		}
		// Walls: Warm cedar log cabin wood
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.20f, 0.11f, 0.05f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.65f);
		}
		// Window Vista: Snowy Alpine Mountain Peaks in sunset
		if (VistaDynMat)
		{
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.70f, 0.60f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.30f);
		}
		// Feature Prop: Stone Fireplace Hearth
		if (FeaturePropMesh)
		{
			FeaturePropMesh->SetVisibility(true);
			if (FeaturePropDynMat)
			{
				FeaturePropDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.18f, 0.16f, 0.15f, 1.0f));
				FeaturePropDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
			}
		}
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Warm hearthfire glow
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.82f, 0.60f));
			TowerSpotlight->SetIntensity(3200.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(1.0f, 0.60f, 0.25f));
			AccentLight1->SetIntensity(1400.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(1.0f, 0.60f, 0.25f));
			AccentLight2->SetIntensity(1400.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(2400.0f);

		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.32f, 0.18f, 0.09f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.50f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::ZenGarden:
	{
		ThemeDisplayName = TEXT("Jardín Zen Japonés");
		// Floor: Japanese woven Tatami matting
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.48f, 0.46f, 0.30f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.75f);
		}
		// Rug: Dark charcoal Zen bamboo mat
		if (CarpetDynMat)
		{
			CarpetDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 0.14f, 0.12f, 1.0f));
			CarpetDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.80f);
		}
		// Table: Deep Red Japanese Urushi Lacquer Wood
		if (TableDynMat)
		{
			TableDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.45f, 0.06f, 0.05f, 1.0f));
			TableDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.16f); // High gloss
		}
		// Walls: Clean bamboo timber & white shoji screen
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.82f, 0.74f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.60f);
		}
		// Window Vista: Blooming Cherry Blossom (Sakura) Garden
		if (VistaDynMat)
		{
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.55f, 0.70f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Soft peaceful daylight
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(0.98f, 0.98f, 0.95f));
			TowerSpotlight->SetIntensity(3600.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.95f, 0.92f, 0.85f));
			AccentLight1->SetIntensity(1800.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(0.95f, 0.92f, 0.85f));
			AccentLight2->SetIntensity(1800.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.42f, 0.06f, 0.05f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.18f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::SpaceObservatory:
	{
		ThemeDisplayName = TEXT("Observatorio Espacial");
		// Floor: Brushed titanium orbital plating
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.08f, 0.11f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.28f);
			FloorDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.85f);
		}
		// Rug: Deep navy-violet energy deck plate
		if (CarpetDynMat)
		{
			CarpetDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.08f, 0.18f, 1.0f));
			CarpetDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.30f);
			CarpetDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.50f);
		}
		// Table: Holographic Carbon Composite with Cyan Trim
		if (TableDynMat)
		{
			TableDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.05f, 0.09f, 1.0f));
			TableDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.18f);
			TableDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.70f);
		}
		// Walls: Dark sci-fi carbon hull
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.04f, 0.06f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
			WallDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.70f);
		}
		// Window Vista: Deep Cosmic Nebula & Starfield
		if (VistaDynMat)
		{
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.40f, 0.10f, 0.90f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.05f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Cool cyan & electric violet sci-fi accents
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(0.85f, 0.95f, 1.0f));
			TowerSpotlight->SetIntensity(4500.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.0f, 0.85f, 1.0f)); // Bright cyan
			AccentLight1->SetIntensity(2200.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(0.70f, 0.15f, 1.0f)); // Electric violet
			AccentLight2->SetIntensity(2200.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.06f, 0.09f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.20f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::VictorianLibrary:
	{
		ThemeDisplayName = TEXT("Biblioteca Victoriana");
		// Floor: Rich mahogany floor
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.18f, 0.08f, 0.04f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
		}
		// Area Rug: Ornate Persian Burgundy & Gold rug
		if (CarpetDynMat)
		{
			CarpetDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.40f, 0.06f, 0.08f, 1.0f));
			CarpetDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.95f);
		}
		// Table: Vintage English Walnut with brass legs
		if (TableDynMat)
		{
			TableDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.08f, 0.04f, 1.0f));
			TableDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.30f);
		}
		// Walls: Dark British racing green vintage wallpaper
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.12f, 0.08f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.80f);
		}
		// Window Vista: Stately manor garden in soft fog
		if (VistaDynMat)
		{
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.45f, 0.55f, 0.48f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
		}
		// Feature Prop: Antique Mahogany Bookshelves
		if (FeaturePropMesh)
		{
			FeaturePropMesh->SetVisibility(true);
			if (FeaturePropDynMat)
			{
				FeaturePropDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.06f, 0.03f, 1.0f));
				FeaturePropDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
			}
		}
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Banker's Lamp warm amber-emerald glow
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.90f, 0.70f));
			TowerSpotlight->SetIntensity(3600.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.20f, 0.85f, 0.45f)); // Emerald lamp glow
			AccentLight1->SetIntensity(1500.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(1.0f, 0.78f, 0.45f)); // Warm amber
			AccentLight2->SetIntensity(1500.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.14f, 0.07f, 0.04f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::MinimalistStudio:
	default:
	{
		ThemeDisplayName = TEXT("Estudio Minimalista");
		// Floor: Polished white architectural concrete floor
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.35f, 0.36f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.20f);
		}
		// Area Rug: Scandinavian light grey felt rug
		if (CarpetDynMat)
		{
			CarpetDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.55f, 0.55f, 1.0f));
			CarpetDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.90f);
		}
		// Table: Matte White Quartz Designer Desk
		if (TableDynMat)
		{
			TableDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.85f, 0.88f, 1.0f));
			TableDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
		}
		// Walls: Clean Scandinavian gallery white
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.75f, 0.75f, 0.78f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.80f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(false);

		// Lights: Clean High-CRI Studio Spotlight
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 1.0f, 1.0f));
			TowerSpotlight->SetIntensity(4800.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.85f, 0.90f, 1.0f));
			AccentLight1->SetIntensity(1200.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(1.0f, 0.95f, 0.90f));
			AccentLight2->SetIntensity(1200.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.80f, 0.80f, 0.82f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
			}
		}
		break;
	}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(777, 4.0f, FColor::Green,
			FString::Printf(TEXT("🌍 ESCENARIO CAMBIADO: %s"), *ThemeDisplayName));
	}
}
