#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaBlock.generated.h"

class UStaticMeshComponent;
class UPhysicalMaterial;

/**
 * Single Yenka wooden block actor with Chaos Physics simulation and dynamic network authority.
 */
UCLASS()
class YENKAVR_API AYenkaBlock : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaBlock();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Sets physics simulation state (Active vs Sleep/Kinematic) */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Physics")
	void SetPhysicsActive(bool bActive);

	/** Sets net ownership to the turn-active player controller */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Network")
	void AssignTurnAuthority(APlayerController* NewOwner);

	/** Checks if the block has fallen below table threshold */
	UFUNCTION(BlueprintPure, Category = "Yenka|Physics")
	bool HasFallen(float TableZThreshold) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BlockMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yenka|Physics")
	UPhysicalMaterial* WoodPhysicalMaterial;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|State")
	int32 LayerIndex;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|State")
	bool bIsPlacedOnTop;
};
