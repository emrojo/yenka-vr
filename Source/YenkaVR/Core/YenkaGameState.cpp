#include "YenkaGameState.h"
#include "Net/UnrealNetwork.h"

AYenkaGameState::AYenkaGameState()
{
	ActivePlayerController = nullptr;
	TurnTimeRemaining = 45.0f;
	RoomCode = TEXT("YK-0000");
}

void AYenkaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AYenkaGameState, ActivePlayerController);
	DOREPLIFETIME(AYenkaGameState, TurnTimeRemaining);
	DOREPLIFETIME(AYenkaGameState, RoomCode);
}

void AYenkaGameState::SetActivePlayer(APlayerController* NewActivePlayer)
{
	ActivePlayerController = NewActivePlayer;
	OnActivePlayerChanged.Broadcast(ActivePlayerController);
}

void AYenkaGameState::SetTurnTimeRemaining(float TimeRemaining)
{
	TurnTimeRemaining = TimeRemaining;
	OnTurnTimeUpdated.Broadcast(TurnTimeRemaining);
}

void AYenkaGameState::OnRep_ActivePlayer()
{
	OnActivePlayerChanged.Broadcast(ActivePlayerController);
}

void AYenkaGameState::OnRep_TurnTimeRemaining()
{
	OnTurnTimeUpdated.Broadcast(TurnTimeRemaining);
}
