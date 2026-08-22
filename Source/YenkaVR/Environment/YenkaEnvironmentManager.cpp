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

	// 1. Floor (12m x 12m)
	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		FloorMesh->SetStaticMesh(CubeMesh);
		FloorMesh->SetRelativeScale3D(FVector(12.0f, 12.0f, 0.05f));
		FloorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.5f)); // Top at Z = 0
	}
	FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 2. Ceiling (12m x 12m at Z = 3.5m)
	CeilingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CeilingMesh"));
	CeilingMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		CeilingMesh->SetStaticMesh(CubeMesh);
		CeilingMesh->SetRelativeScale3D(FVector(12.0f, 12.0f, 0.05f));
		CeilingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 350.0f));
	}
	CeilingMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// 3. Back Wall (Facing player at +X = 5.0m)
	BackWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackWallMesh"));
	BackWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		BackWallMesh->SetStaticMesh(CubeMesh);
		BackWallMesh->SetRelativeScale3D(FVector(0.1f, 12.0f, 3.6f));
		BackWallMesh->SetRelativeLocation(FVector(500.0f, 0.0f, 175.0f));
	}
	BackWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 4. Front Wall (Behind player at -X = -5.0m)
	FrontWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontWallMesh"));
	FrontWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		FrontWallMesh->SetStaticMesh(CubeMesh);
		FrontWallMesh->SetRelativeScale3D(FVector(0.1f, 12.0f, 3.6f));
		FrontWallMesh->SetRelativeLocation(FVector(-500.0f, 0.0f, 175.0f));
	}
	FrontWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 5. Left Wall (At -Y = -5.0m)
	LeftWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWallMesh"));
	LeftWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		LeftWallMesh->SetStaticMesh(CubeMesh);
		LeftWallMesh->SetRelativeScale3D(FVector(12.0f, 0.1f, 3.6f));
		LeftWallMesh->SetRelativeLocation(FVector(0.0f, -500.0f, 175.0f));
	}
	LeftWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 6. Right Wall (At +Y = +5.0m)
	RightWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWallMesh"));
	RightWallMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		RightWallMesh->SetStaticMesh(CubeMesh);
		RightWallMesh->SetRelativeScale3D(FVector(12.0f, 0.1f, 3.6f));
		RightWallMesh->SetRelativeLocation(FVector(0.0f, 500.0f, 175.0f));
	}
	RightWallMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 7. Panoramic Window Vista Plane on Back Wall
	WindowVistaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WindowVistaMesh"));
	WindowVistaMesh->SetupAttachment(SceneRoot);
	if (PlaneMesh)
	{
		WindowVistaMesh->SetStaticMesh(PlaneMesh);
		WindowVistaMesh->SetRelativeScale3D(FVector(2.4f, 6.0f, 1.0f));
		WindowVistaMesh->SetRelativeLocation(FVector(485.0f, 0.0f, 175.0f));
		WindowVistaMesh->SetRelativeRotation(FRotator(90.0f, 180.0f, 0.0f));
	}
	WindowVistaMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// 8. Thematic Prop / Fireplace / Shoji / Bookshelf
	FeaturePropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FeaturePropMesh"));
	FeaturePropMesh->SetupAttachment(SceneRoot);
	if (CubeMesh)
	{
		FeaturePropMesh->SetStaticMesh(CubeMesh);
		FeaturePropMesh->SetRelativeScale3D(FVector(0.5f, 2.2f, 1.2f));
		FeaturePropMesh->SetRelativeLocation(FVector(460.0f, 0.0f, 60.0f));
	}
	FeaturePropMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// 9. Ceiling Spotlight pointing straight down at Yenka Tower (0, 0, 90)
	TowerSpotlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("TowerSpotlight"));
	TowerSpotlight->SetupAttachment(SceneRoot);
	TowerSpotlight->SetRelativeLocation(FVector(0.0f, 0.0f, 320.0f));
	TowerSpotlight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TowerSpotlight->SetIntensity(3500.0f);
	TowerSpotlight->SetInnerConeAngle(22.0f);
	TowerSpotlight->SetOuterConeAngle(38.0f);
	TowerSpotlight->SetAttenuationRadius(500.0f);
	TowerSpotlight->SetCastShadows(true);

	// 10. Accent Lights
	AccentLight1 = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight1"));
	AccentLight1->SetupAttachment(SceneRoot);
	AccentLight1->SetRelativeLocation(FVector(250.0f, -300.0f, 220.0f));
	AccentLight1->SetIntensity(1200.0f);
	AccentLight1->SetAttenuationRadius(600.0f);

	AccentLight2 = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight2"));
	AccentLight2->SetupAttachment(SceneRoot);
	AccentLight2->SetRelativeLocation(FVector(250.0f, 300.0f, 220.0f));
	AccentLight2->SetIntensity(1200.0f);
	AccentLight2->SetAttenuationRadius(600.0f);

	// 11. Dynamic Fireplace Light
	FireplaceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireplaceLight"));
	FireplaceLight->SetupAttachment(SceneRoot);
	FireplaceLight->SetRelativeLocation(FVector(430.0f, 0.0f, 45.0f));
	FireplaceLight->SetIntensity(0.0f);
	FireplaceLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.10f));
	FireplaceLight->SetAttenuationRadius(450.0f);

	CurrentTheme = EYenkaEnvironmentTheme::ModernPenthouse;
	FireplaceFlickerTimer = 0.0f;
}

void AYenkaEnvironmentManager::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Dynamic Material Instances
	UMaterialInterface* BaseMat = FloorMesh ? FloorMesh->GetMaterial(0) : nullptr;
	if (BaseMat)
	{
		FloorDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		WallDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		VistaDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		FeaturePropDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);

		if (FloorMesh && FloorDynMat) FloorMesh->SetMaterial(0, FloorDynMat);
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
		FireplaceLight->SetIntensity(2200.0f + (Noise * 800.0f));
	}
}

void AYenkaEnvironmentManager::ApplyEnvironmentTheme(EYenkaEnvironmentTheme NewTheme)
{
	CurrentTheme = NewTheme;

	AYenkaTowerManager* TowerMgr = Cast<AYenkaTowerManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass()));

	const bool bIsMR = (NewTheme == EYenkaEnvironmentTheme::MixedRealityPassthrough);
	if (FloorMesh) FloorMesh->SetVisibility(!bIsMR);
	if (CeilingMesh) CeilingMesh->SetVisibility(!bIsMR);
	if (BackWallMesh) BackWallMesh->SetVisibility(!bIsMR);
	if (FrontWallMesh) FrontWallMesh->SetVisibility(!bIsMR);
	if (LeftWallMesh) LeftWallMesh->SetVisibility(!bIsMR);
	if (RightWallMesh) RightWallMesh->SetVisibility(!bIsMR);

	switch (CurrentTheme)
	{
	case EYenkaEnvironmentTheme::MixedRealityPassthrough:
	{
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(false);

		// Lights: Soft realistic ambient lighting that blends seamlessly with the player's physical room
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.98f, 0.95f));
			TowerSpotlight->SetIntensity(2500.0f);
		}
		if (AccentLight1) AccentLight1->SetIntensity(0.0f);
		if (AccentLight2) AccentLight2->SetIntensity(0.0f);
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		// Table: Modern matte frosted glass / composite board
		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.10f, 0.12f, 0.15f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.20f);
				TableMat->SetScalarParameterValue(TEXT("Metallic"), 0.15f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::ModernPenthouse:
	{
		// Floor: Rich dark smoked oak parquet
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.05f, 0.03f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
			FloorDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
		}
		// Walls: Contemporary architectural slate grey
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.15f, 0.16f, 0.18f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.70f);
			WallDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
		}
		// Window Vista: Luminous Night City Skyline
		if (VistaDynMat)
		{
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.20f, 0.40f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.10f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Warm luxurious interior spotlights
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.85f));
			TowerSpotlight->SetIntensity(3800.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.95f, 0.75f, 0.50f));
			AccentLight1->SetIntensity(1400.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(0.95f, 0.75f, 0.50f));
			AccentLight2->SetIntensity(1400.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		// Table: Polished Nero Marquina Black Marble
		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.04f, 0.04f, 0.05f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.12f); // Highly polished reflective marble
				TableMat->SetScalarParameterValue(TEXT("Metallic"), 0.1f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::CozyCabin:
	{
		// Floor: Rustic pine planks
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.26f, 0.14f, 0.07f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.55f);
		}
		// Walls: Warm cedar log cabin wood
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.20f, 0.11f, 0.05f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.65f);
		}
		// Window Vista: Snowy Alpine Mountain Peaks
		if (VistaDynMat)
		{
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.70f, 0.80f, 0.95f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.30f);
		}
		// Feature Prop: Stone Fireplace Hearth
		if (FeaturePropMesh)
		{
			FeaturePropMesh->SetVisibility(true);
			FeaturePropMesh->SetRelativeScale3D(FVector(0.5f, 2.4f, 1.4f));
			FeaturePropMesh->SetRelativeLocation(FVector(460.0f, 0.0f, 70.0f));
			if (FeaturePropDynMat)
			{
				FeaturePropDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.18f, 0.16f, 0.15f, 1.0f)); // Dark slate stone
				FeaturePropDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
			}
		}
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Warm hearthfire glow
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.82f, 0.60f));
			TowerSpotlight->SetIntensity(2800.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(1.0f, 0.60f, 0.25f));
			AccentLight1->SetIntensity(1000.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(1.0f, 0.60f, 0.25f));
			AccentLight2->SetIntensity(1000.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(2400.0f);

		// Table: Heavy hand-carved solid rustic oak
		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.32f, 0.18f, 0.09f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.50f);
				TableMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::ZenGarden:
	{
		// Floor: Japanese woven Tatami matting
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.48f, 0.46f, 0.30f, 1.0f)); // Straw tatami green-gold
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.75f);
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
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.92f, 0.70f, 0.78f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Soft, peaceful natural daylight
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(0.98f, 0.98f, 0.95f));
			TowerSpotlight->SetIntensity(3200.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.95f, 0.92f, 0.85f));
			AccentLight1->SetIntensity(1600.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(0.95f, 0.92f, 0.85f));
			AccentLight2->SetIntensity(1600.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		// Table: Deep Red Japanese Urushi Lacquer Wood
		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.42f, 0.06f, 0.05f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.18f); // Glossy lacquer
				TableMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::SpaceObservatory:
	{
		// Floor: Brushed titanium orbital plating
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.08f, 0.11f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.28f);
			FloorDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.85f);
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
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.20f, 0.05f, 0.45f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.05f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Cool cyan neon sci-fi accents
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(0.85f, 0.95f, 1.0f));
			TowerSpotlight->SetIntensity(4200.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.0f, 0.75f, 1.0f)); // Bright cyan
			AccentLight1->SetIntensity(1800.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(0.60f, 0.10f, 0.90f)); // Electric violet
			AccentLight2->SetIntensity(1800.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		// Table: Holographic Carbon Composite with Cyan Trim
		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.06f, 0.09f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.20f);
				TableMat->SetScalarParameterValue(TEXT("Metallic"), 0.65f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::VictorianLibrary:
	{
		// Floor: Rich mahogany floor with Persian rug tones
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.18f, 0.08f, 0.04f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
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
			VistaDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.40f, 0.48f, 0.42f, 1.0f));
			VistaDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
		}
		// Feature Prop: Antique Mahogany Bookshelves
		if (FeaturePropMesh)
		{
			FeaturePropMesh->SetVisibility(true);
			FeaturePropMesh->SetRelativeScale3D(FVector(0.4f, 3.2f, 2.2f));
			FeaturePropMesh->SetRelativeLocation(FVector(470.0f, 0.0f, 110.0f));
			if (FeaturePropDynMat)
			{
				FeaturePropDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.16f, 0.06f, 0.03f, 1.0f)); // Mahogany wood
				FeaturePropDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
			}
		}
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(true);

		// Lights: Banker's Lamp warm amber-emerald glow
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 0.90f, 0.70f));
			TowerSpotlight->SetIntensity(3400.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.20f, 0.85f, 0.45f)); // Emerald lamp glow
			AccentLight1->SetIntensity(1200.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(1.0f, 0.78f, 0.45f)); // Warm amber
			AccentLight2->SetIntensity(1200.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		// Table: Vintage English Walnut with leather inlay feel
		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.14f, 0.07f, 0.04f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
				TableMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			}
		}
		break;
	}

	case EYenkaEnvironmentTheme::MinimalistStudio:
	default:
	{
		// Floor: Highly polished neutral dark studio floor
		if (FloorDynMat)
		{
			FloorDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.04f, 0.04f, 0.04f, 1.0f));
			FloorDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.15f);
			FloorDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
		}
		// Walls: Infinite dark void
		if (WallDynMat)
		{
			WallDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.02f, 0.02f, 1.0f));
			WallDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.90f);
		}
		if (FeaturePropMesh) FeaturePropMesh->SetVisibility(false);
		if (WindowVistaMesh) WindowVistaMesh->SetVisibility(false);

		// Lights: Clean High-CRI Studio Spotlight
		if (TowerSpotlight)
		{
			TowerSpotlight->SetLightColor(FLinearColor(1.0f, 1.0f, 1.0f));
			TowerSpotlight->SetIntensity(4500.0f);
		}
		if (AccentLight1)
		{
			AccentLight1->SetLightColor(FLinearColor(0.6f, 0.6f, 0.7f));
			AccentLight1->SetIntensity(800.0f);
		}
		if (AccentLight2)
		{
			AccentLight2->SetLightColor(FLinearColor(0.7f, 0.6f, 0.6f));
			AccentLight2->SetIntensity(800.0f);
		}
		if (FireplaceLight) FireplaceLight->SetIntensity(0.0f);

		// Table: Matte Charcoal Board
		if (TowerMgr && TowerMgr->TableMesh)
		{
			UMaterialInstanceDynamic* TableMat = Cast<UMaterialInstanceDynamic>(TowerMgr->TableMesh->GetMaterial(0));
			if (TableMat)
			{
				TableMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.12f, 0.12f, 1.0f));
				TableMat->SetScalarParameterValue(TEXT("Roughness"), 0.30f);
				TableMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			}
		}
		break;
	}
	}
}
