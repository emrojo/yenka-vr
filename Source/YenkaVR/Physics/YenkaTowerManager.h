#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaTowerManager.generated.h"

class AYenkaBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTowerCollapsed);

USTRUCT()
struct FBlockSpawnData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	int32 LayerIndex = 0;

	UPROPERTY()
	int32 ColorIndex = 0;

	UPROPERTY()
	FLinearColor Color = FLinearColor::White;
};

/**
 * Procedural generator and state monitor for the 54 Yenka blocks.
 */
UCLASS()
class YENKAVR_API AYenkaTowerManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaTowerManager();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TableMesh;

	/** Generates the 54 blocks sequentially one-by-one on top of the calibrated table */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Tower")
	void SpawnTower();

	/** Clears and rebuilds the tower */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Tower")
	void ResetTower();

	/** Spawns the next block in the sequential build queue */
	UFUNCTION()
	void SpawnNextBlockFromQueue();

	/** Returns true while the tower is actively placing blocks one by one */
	UFUNCTION(BlueprintPure, Category = "Yenka|Tower")
	bool IsBuildingTower() const { return bIsBuildingTower; }

	/** Returns the calibrated top surface height of the Jenga board */
	UFUNCTION(BlueprintPure, Category = "Yenka|Tower")
	float GetTableSurfaceZ() const { return TableSurfaceZ; }

	/** Freezes all blocks to sleep state to avoid float drift between turns */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Physics")
	void FreezeTowerPhysics();

	/** Unfreezes blocks and assigns authority to active player */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Physics")
	void WakeTowerForPlayer(APlayerController* ActivePlayer);

	/** Validates whether tower is still standing */
	UFUNCTION(BlueprintPure, Category = "Yenka|Tower")
	bool CheckTowerStability();

	UPROPERTY(EditDefaultsOnly, Category = "Yenka|Config")
	TSubclassOf<AYenkaBlock> BlockClass;

	UPROPERTY(EditDefaultsOnly, Category = "Yenka|Config")
	FVector BlockDimensions; // e.g. (7.5, 2.5, 1.5) cm

	UPROPERTY(EditDefaultsOnly, Category = "Yenka|Appearance")
	TArray<FLinearColor> BlockColorPalette;

	UPROPERTY(EditDefaultsOnly, Category = "Yenka|Config")
	float SequentialSpawnInterval; // e.g. 0.05s between pieces

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnTowerCollapsed OnTowerCollapsed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|State")
	TArray<AYenkaBlock*> ActiveBlocks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|State")
	float TableSurfaceZ;

	UPROPERTY()
	TArray<FBlockSpawnData> PendingSpawnQueue;

	UPROPERTY()
	FTimerHandle SequentialSpawnTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|State")
	bool bIsBuildingTower;

	virtual void Tick(float DeltaTime) override;
};
