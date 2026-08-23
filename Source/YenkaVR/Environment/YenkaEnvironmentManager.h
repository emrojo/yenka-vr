#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaEnvironmentManager.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USpotLightComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EYenkaEnvironmentTheme : uint8
{
	ModernPenthouse          UMETA(DisplayName = "Ático de Lujo Moderno"),
	CozyCabin                UMETA(DisplayName = "Cabaña de Montaña"),
	ZenGarden                UMETA(DisplayName = "Jardín Zen Japonés"),
	SpaceObservatory         UMETA(DisplayName = "Observatorio Espacial"),
	VictorianLibrary         UMETA(DisplayName = "Biblioteca Victoriana"),
	MinimalistStudio         UMETA(DisplayName = "Estudio Minimalista"),
	MixedRealityPassthrough  UMETA(DisplayName = "Realidad Mixta (Passthrough)")
};

/**
 * Manages dynamic 3D room geometries, materials, lighting, and table finishes for all Yenka environments.
 */
UCLASS()
class YENKAVR_API AYenkaEnvironmentManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaEnvironmentManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Applies the chosen environment theme dynamically in real-time */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Environment")
	void ApplyEnvironmentTheme(EYenkaEnvironmentTheme NewTheme);

	UFUNCTION(BlueprintPure, Category = "Yenka|Environment")
	EYenkaEnvironmentTheme GetCurrentTheme() const { return CurrentTheme; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FloorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CarpetMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TableDeskMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TableLeg1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TableLeg2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TableLeg3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TableLeg4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CeilingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BackWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FrontWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LeftWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* RightWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WindowVistaMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FeaturePropMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpotLightComponent* TowerSpotlight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* AccentLight1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* AccentLight2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* FireplaceLight;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Environment")
	EYenkaEnvironmentTheme CurrentTheme;

	// Dynamic Material Instances
	UPROPERTY()
	UMaterialInstanceDynamic* FloorDynMat;

	UPROPERTY()
	UMaterialInstanceDynamic* CarpetDynMat;

	UPROPERTY()
	UMaterialInstanceDynamic* TableDynMat;

	UPROPERTY()
	UMaterialInstanceDynamic* WallDynMat;

	UPROPERTY()
	UMaterialInstanceDynamic* VistaDynMat;

	UPROPERTY()
	UMaterialInstanceDynamic* FeaturePropDynMat;

	float FireplaceFlickerTimer;
};
