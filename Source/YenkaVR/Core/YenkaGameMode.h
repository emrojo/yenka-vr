#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YenkaGameMode.generated.h"

class AYenkaTowerManager;
class AYenkaPlayerState;

UENUM(BlueprintType)
enum class EYenkaRoundState : uint8
{
	WaitingForPlayers,
	CalibratingSurface,
	TurnActive,
	TurnStabilizing,
	RoundOver
};

/**
 * Server-authoritative GameMode managing turn state machine, 45-second timer, and scoring.
 */
UCLASS()
class YENKAVR_API AYenkaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYenkaGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Starts the next player's turn */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Turn")
	void StartNextTurn();

	/** Called when the active player releases a block and claims a valid placement */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Turn")
	void EndCurrentTurn(bool bPlayerClaimedSuccess);

	/** Handles tower collapse */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Game")
	void HandleTowerCollapse(APlayerController* ResponsiblePlayer);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yenka|Config")
	float MaxTurnTimeSeconds = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yenka|Config")
	float StabilizationWaitSeconds = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|State")
	EYenkaRoundState CurrentRoundState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|State")
	int32 ActivePlayerIndex;

	UPROPERTY()
	TArray<APlayerController*> TurnOrder;

	FTimerHandle TurnTimerHandle;
	FTimerHandle StabilizationTimerHandle;

	void OnTurnTimeout();
	void OnStabilizationComplete();
};
