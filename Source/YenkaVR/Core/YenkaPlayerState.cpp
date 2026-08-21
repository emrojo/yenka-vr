#include "YenkaPlayerState.h"
#include "Net/UnrealNetwork.h"

AYenkaPlayerState::AYenkaPlayerState()
{
	CumulativeScore = 0;
	bIsUsingVR = false;
	bIsUsingPassthroughAR = false;
}

void AYenkaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AYenkaPlayerState, CumulativeScore);
	DOREPLIFETIME(AYenkaPlayerState, bIsUsingVR);
	DOREPLIFETIME(AYenkaPlayerState, bIsUsingPassthroughAR);
}

void AYenkaPlayerState::AddScore(int32 Delta)
{
	CumulativeScore += Delta;
	OnScoreChanged.Broadcast(CumulativeScore);
}

void AYenkaPlayerState::OnRep_CumulativeScore()
{
	OnScoreChanged.Broadcast(CumulativeScore);
}
