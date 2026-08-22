#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaVR/Environment/YenkaEnvironmentManager.h"
#include "YenkaScenarioMenu.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FScenarioMenuItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Menu")
	EYenkaEnvironmentTheme Theme;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Menu")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Menu")
	FLinearColor AccentColor;

	UPROPERTY()
	UStaticMeshComponent* ButtonMesh;

	UPROPERTY()
	UTextRenderComponent* TitleText;

	UPROPERTY()
	UMaterialInstanceDynamic* ButtonMat;
};

/**
 * 3D in-world interactive Scenario Selection Menu for VR and Desktop.
 */
UCLASS()
class YENKAVR_API AYenkaScenarioMenu : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaScenarioMenu();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Selects an environment theme by index [0..5] or theme enum */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Menu")
	void SelectThemeByIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Menu")
	void SelectTheme(EYenkaEnvironmentTheme Theme);

	/** Toggles visibility of the 3D scenario selector menu */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Menu")
	void ToggleMenuVisibility();

	/** Handles cursor / VR laser trace hit against menu buttons */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Menu")
	void ProcessRayHit(const FHitResult& HitResult, bool bIsClick);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MenuRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BackgroundPlate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* HeaderText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* SubheaderText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ReopenButton;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* ReopenButtonText;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Menu")
	TArray<FScenarioMenuItem> MenuItems;

	int32 HoveredButtonIndex;
	bool bIsMenuOpen;
	float TargetScale;
	float CurrentScale;

	UPROPERTY()
	UMaterialInstanceDynamic* BgDynMat;

	UPROPERTY()
	UMaterialInstanceDynamic* ReopenDynMat;
};
