#include "YenkaScenarioMenu.h"
#include "YenkaVR/Environment/YenkaEnvironmentManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AYenkaScenarioMenu::AYenkaScenarioMenu()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MenuRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MenuRoot"));
	RootComponent = MenuRoot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatAsset(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));

	UStaticMesh* CubeMesh = CubeMeshAsset.Succeeded() ? CubeMeshAsset.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatAsset.Succeeded() ? BaseMatAsset.Object : nullptr;

	// 1. Dark Glass Background Backdrop Plate (90cm width x 52cm height x 1.5cm depth)
	BackgroundPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackgroundPlate"));
	BackgroundPlate->SetupAttachment(MenuRoot);
	if (CubeMesh)
	{
		BackgroundPlate->SetStaticMesh(CubeMesh);
		BackgroundPlate->SetRelativeScale3D(FVector(0.015f, 0.90f, 0.52f));
		BackgroundPlate->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}
	BackgroundPlate->SetCollisionProfileName(TEXT("BlockAll"));

	// 2. Header Title Text (Attached directly to MenuRoot with unscaled font matrix)
	HeaderText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HeaderText"));
	HeaderText->SetupAttachment(MenuRoot);
	HeaderText->SetRelativeLocation(FVector(-1.8f, 0.0f, 20.0f));
	HeaderText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	HeaderText->SetText(FText::FromString(TEXT("SELECCIONA TU ESCENARIO")));
	HeaderText->SetTextRenderColor(FColor::White);
	HeaderText->SetWorldSize(3.6f);
	HeaderText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	HeaderText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);

	// 3. Subheader Text
	SubheaderText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SubheaderText"));
	SubheaderText->SetupAttachment(MenuRoot);
	SubheaderText->SetRelativeLocation(FVector(-1.8f, 0.0f, 15.5f));
	SubheaderText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	SubheaderText->SetText(FText::FromString(TEXT("Elige una atmosfera para tu partida [Teclas 1 - 7 o Clic]")));
	SubheaderText->SetTextRenderColor(FColor(180, 220, 255));
	SubheaderText->SetWorldSize(1.8f);
	SubheaderText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	SubheaderText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);

	// 4. Setup 7 Scenario Items
	TArray<TPair<EYenkaEnvironmentTheme, FString>> ThemeConfigs = {
		{ EYenkaEnvironmentTheme::ModernPenthouse,       TEXT("1. Atico Moderno") },
		{ EYenkaEnvironmentTheme::CozyCabin,             TEXT("2. Cabana Montana") },
		{ EYenkaEnvironmentTheme::ZenGarden,             TEXT("3. Jardin Zen") },
		{ EYenkaEnvironmentTheme::SpaceObservatory,      TEXT("4. Estacion Espacial") },
		{ EYenkaEnvironmentTheme::VictorianLibrary,      TEXT("5. Biblioteca Victoriana") },
		{ EYenkaEnvironmentTheme::MinimalistStudio,      TEXT("6. Estudio Minimalista") },
		{ EYenkaEnvironmentTheme::MixedRealityPassthrough, TEXT("7. Realidad Mixta") }
	};

	TArray<FLinearColor> ThemeColors = {
		FLinearColor(0.20f, 0.60f, 1.00f), // Penthouse Cyan
		FLinearColor(1.00f, 0.55f, 0.15f), // Cabin Amber
		FLinearColor(0.95f, 0.40f, 0.60f), // Zen Sakura Pink
		FLinearColor(0.50f, 0.20f, 1.00f), // Space Violet
		FLinearColor(0.20f, 0.85f, 0.40f), // Victorian Emerald
		FLinearColor(0.70f, 0.70f, 0.70f), // Studio Silver
		FLinearColor(0.00f, 0.90f, 0.80f)  // Mixed Reality Passthrough Teal
	};

	for (int32 i = 0; i < 7; ++i)
	{
		FScenarioMenuItem Item;
		Item.Theme = ThemeConfigs[i].Key;
		Item.Title = ThemeConfigs[i].Value;
		Item.AccentColor = ThemeColors[i];

		float PosY = 0.0f;
		float PosZ = 0.0f;

		if (i < 4)
		{
			// Top row: 4 cards
			PosY = (i - 1.5f) * 21.5f; // -32.25, -10.75, +10.75, +32.25 cm
			PosZ = 6.0f;
		}
		else
		{
			// Bottom row: 3 cards
			const int32 Col = i - 4;   // 0, 1, 2
			PosY = (Col - 1.0f) * 24.0f; // -24.0, 0.0, +24.0 cm
			PosZ = -9.0f;
		}

		// Card Mesh
		FString MeshName = FString::Printf(TEXT("ButtonMesh_%d"), i);
		Item.ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(*MeshName);
		Item.ButtonMesh->SetupAttachment(MenuRoot);
		if (CubeMesh)
		{
			Item.ButtonMesh->SetStaticMesh(CubeMesh);
			Item.ButtonMesh->SetRelativeScale3D(FVector(0.02f, 0.19f, 0.12f)); // 19cm width x 12cm height
			Item.ButtonMesh->SetRelativeLocation(FVector(-0.5f, PosY, PosZ));
		}
		Item.ButtonMesh->SetCollisionProfileName(TEXT("BlockAll"));

		// Card Label (Attached directly to MenuRoot in front of the button mesh)
		FString TextName = FString::Printf(TEXT("ButtonText_%d"), i);
		Item.TitleText = CreateDefaultSubobject<UTextRenderComponent>(*TextName);
		Item.TitleText->SetupAttachment(MenuRoot);
		Item.TitleText->SetRelativeLocation(FVector(-1.8f, PosY, PosZ));
		Item.TitleText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
		Item.TitleText->SetText(FText::FromString(Item.Title));
		Item.TitleText->SetTextRenderColor(FColor::White);
		Item.TitleText->SetWorldSize(2.2f);
		Item.TitleText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		Item.TitleText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);

		MenuItems.Add(Item);
	}

	// 5. Compact Reopen Floating Button ("⚙️ Cambiar Escenario")
	ReopenButton = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReopenButton"));
	ReopenButton->SetupAttachment(MenuRoot);
	if (CubeMesh)
	{
		ReopenButton->SetStaticMesh(CubeMesh);
		ReopenButton->SetRelativeScale3D(FVector(0.015f, 0.24f, 0.06f));
		ReopenButton->SetRelativeLocation(FVector(-0.5f, -48.0f, -22.0f));
	}
	ReopenButton->SetCollisionProfileName(TEXT("BlockAll"));
	ReopenButton->SetVisibility(false);

	ReopenButtonText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ReopenButtonText"));
	ReopenButtonText->SetupAttachment(MenuRoot);
	ReopenButtonText->SetRelativeLocation(FVector(-1.8f, -48.0f, -22.0f));
	ReopenButtonText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	ReopenButtonText->SetText(FText::FromString(TEXT("Escenarios (M)")));
	ReopenButtonText->SetTextRenderColor(FColor::White);
	ReopenButtonText->SetWorldSize(2.0f);
	ReopenButtonText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	ReopenButtonText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	ReopenButtonText->SetVisibility(false);

	HoveredButtonIndex = -1;
	bIsMenuOpen = true;
	TargetScale = 1.0f;
	CurrentScale = 1.0f;
}

void AYenkaScenarioMenu::BeginPlay()
{
	Super::BeginPlay();

	// Spawn position: Floating comfortably at eye height between player (-70cm) and table (0cm)
	SetActorLocation(FVector(-40.0f, 0.0f, 110.0f));
	SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

	// Initialize clean PBR solid materials
	UMaterialInterface* BaseMat = BackgroundPlate ? BackgroundPlate->GetMaterial(0) : nullptr;
	if (BaseMat)
	{
		BgDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (BgDynMat)
		{
			BgDynMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.02f, 0.03f, 0.05f, 1.0f));
			BgDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.03f, 0.05f, 1.0f));
			BgDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.15f);
			BgDynMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			BackgroundPlate->SetMaterial(0, BgDynMat);
		}

		ReopenDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (ReopenDynMat)
		{
			ReopenDynMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.05f, 0.10f, 0.18f, 1.0f));
			ReopenDynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.10f, 0.18f, 1.0f));
			ReopenDynMat->SetScalarParameterValue(TEXT("Roughness"), 0.20f);
			ReopenButton->SetMaterial(0, ReopenDynMat);
		}

		for (int32 i = 0; i < MenuItems.Num(); ++i)
		{
			if (MenuItems[i].ButtonMesh)
			{
				UMaterialInstanceDynamic* CardMat = UMaterialInstanceDynamic::Create(BaseMat, this);
				if (CardMat)
				{
					// Dark charcoal base with rich theme tint
					FLinearColor BaseCardColor = FMath::Lerp(FLinearColor(0.06f, 0.07f, 0.09f, 1.0f), MenuItems[i].AccentColor, 0.25f);
					CardMat->SetVectorParameterValue(TEXT("BaseColor"), BaseCardColor);
					CardMat->SetVectorParameterValue(TEXT("Color"), BaseCardColor);
					CardMat->SetScalarParameterValue(TEXT("Roughness"), 0.25f);
					CardMat->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
					MenuItems[i].ButtonMesh->SetMaterial(0, CardMat);
					MenuItems[i].ButtonMat = CardMat;
				}
			}
		}
	}
}

void AYenkaScenarioMenu::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth scaling animation for entire MenuRoot
	if (!FMath::IsNearlyEqual(CurrentScale, TargetScale, 0.01f))
	{
		CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, DeltaTime, 12.0f);
		SetActorScale3D(FVector(CurrentScale, CurrentScale, CurrentScale));

		if (CurrentScale <= 0.05f && !bIsMenuOpen)
		{
			SetActorHiddenInGame(true);
			if (ReopenButton) ReopenButton->SetVisibility(true);
			if (ReopenButtonText) ReopenButtonText->SetVisibility(true);
		}
	}
}

void AYenkaScenarioMenu::SelectThemeByIndex(int32 Index)
{
	if (MenuItems.IsValidIndex(Index))
	{
		SelectTheme(MenuItems[Index].Theme);
	}
}

void AYenkaScenarioMenu::SelectTheme(EYenkaEnvironmentTheme Theme)
{
	AYenkaEnvironmentManager* EnvMgr = Cast<AYenkaEnvironmentManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaEnvironmentManager::StaticClass()));
	if (EnvMgr)
	{
		EnvMgr->ApplyEnvironmentTheme(Theme);
	}

	// Close menu smoothly after selection
	bIsMenuOpen = false;
	TargetScale = 0.0f;
}

void AYenkaScenarioMenu::ToggleMenuVisibility()
{
	bIsMenuOpen = !bIsMenuOpen;
	TargetScale = bIsMenuOpen ? 1.0f : 0.0f;

	if (bIsMenuOpen)
	{
		SetActorHiddenInGame(false);
		if (ReopenButton) ReopenButton->SetVisibility(false);
		if (ReopenButtonText) ReopenButtonText->SetVisibility(false);
	}
}

void AYenkaScenarioMenu::ProcessRayHit(const FHitResult& HitResult, bool bIsClick)
{
	UPrimitiveComponent* HitComp = HitResult.GetComponent();
	if (!HitComp) return;

	if (HitComp == ReopenButton)
	{
		if (bIsClick)
		{
			ToggleMenuVisibility();
		}
		return;
	}

	int32 HitIndex = -1;
	for (int32 i = 0; i < MenuItems.Num(); ++i)
	{
		if (HitComp == MenuItems[i].ButtonMesh)
		{
			HitIndex = i;
			break;
		}
	}

	if (HitIndex != -1)
	{
		// Highlight hovered button
		if (HoveredButtonIndex != HitIndex)
		{
			if (HoveredButtonIndex != -1 && MenuItems.IsValidIndex(HoveredButtonIndex) && MenuItems[HoveredButtonIndex].ButtonMat)
			{
				FLinearColor NormalColor = FMath::Lerp(FLinearColor(0.06f, 0.07f, 0.09f, 1.0f), MenuItems[HoveredButtonIndex].AccentColor, 0.25f);
				MenuItems[HoveredButtonIndex].ButtonMat->SetVectorParameterValue(TEXT("BaseColor"), NormalColor);
				MenuItems[HoveredButtonIndex].ButtonMat->SetVectorParameterValue(TEXT("Color"), NormalColor);
			}

			HoveredButtonIndex = HitIndex;
			if (MenuItems[HitIndex].ButtonMat)
			{
				FLinearColor HoverColor = FMath::Lerp(FLinearColor(0.20f, 0.22f, 0.26f, 1.0f), MenuItems[HitIndex].AccentColor, 0.85f);
				MenuItems[HitIndex].ButtonMat->SetVectorParameterValue(TEXT("BaseColor"), HoverColor);
				MenuItems[HitIndex].ButtonMat->SetVectorParameterValue(TEXT("Color"), HoverColor);
			}
		}

		if (bIsClick)
		{
			SelectThemeByIndex(HitIndex);
		}
	}
	else if (HoveredButtonIndex != -1)
	{
		// Un-hover
		if (MenuItems.IsValidIndex(HoveredButtonIndex) && MenuItems[HoveredButtonIndex].ButtonMat)
		{
			FLinearColor NormalColor = FMath::Lerp(FLinearColor(0.06f, 0.07f, 0.09f, 1.0f), MenuItems[HoveredButtonIndex].AccentColor, 0.25f);
			MenuItems[HoveredButtonIndex].ButtonMat->SetVectorParameterValue(TEXT("BaseColor"), NormalColor);
			MenuItems[HoveredButtonIndex].ButtonMat->SetVectorParameterValue(TEXT("Color"), NormalColor);
		}
		HoveredButtonIndex = -1;
	}
}
