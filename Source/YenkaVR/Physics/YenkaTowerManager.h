#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaTowerManager.generated.h"

class AYenkaBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTowerCollapsed);

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

	/** Generates the 54 blocks on top of the calibrated table */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Tower")
	void SpawnTower();

	/** Clears and rebuilds the tower */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Tower")
	void ResetTower();

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

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnTowerCollapsed OnTowerCollapsed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|State")
	TArray<AYenkaBlock*> ActiveBlocks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|State")
	float TableSurfaceZ;

	virtual void Tick(float DeltaTime) override;
};
