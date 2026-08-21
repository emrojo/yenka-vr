#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "YenkaGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActivePlayerChanged, APlayerController*, NewActivePlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnTimeUpdated, float, TimeRemaining);

/**
 * Replicated GameState holding turn status, room code, and global round state.
 */
UCLASS()
class YENKAVR_API AYenkaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AYenkaGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetActivePlayer(APlayerController* NewActivePlayer);
	void SetTurnTimeRemaining(float TimeRemaining);

	UPROPERTY(ReplicatedUsing = OnRep_ActivePlayer, BlueprintReadOnly, Category = "Yenka|State")
	APlayerController* ActivePlayerController;

	UPROPERTY(ReplicatedUsing = OnRep_TurnTimeRemaining, BlueprintReadOnly, Category = "Yenka|State")
	float TurnTimeRemaining;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|State")
	FString RoomCode;

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnActivePlayerChanged OnActivePlayerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnTurnTimeUpdated OnTurnTimeUpdated;

protected:
	UFUNCTION()
	void OnRep_ActivePlayer();

	UFUNCTION()
	void OnRep_TurnTimeRemaining();
};
