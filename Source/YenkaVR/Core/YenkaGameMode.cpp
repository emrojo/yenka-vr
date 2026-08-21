#include "YenkaGameMode.h"
#include "YenkaGameState.h"
#include "YenkaPlayerState.h"
#include "YenkaVR/Physics/YenkaTowerManager.h"
#include "YenkaVR/Interaction/YenkaVRPawn.h"
#include "YenkaVR/Interaction/YenkaDesktopPawn.h"
#include "IXRTrackingSystem.h"
#include "YenkaVR.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AYenkaGameMode::AYenkaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = AYenkaGameState::StaticClass();
	PlayerStateClass = AYenkaPlayerState::StaticClass();
	DefaultPawnClass = AYenkaVRPawn::StaticClass();

	MaxTurnTimeSeconds = 45.0f;
	StabilizationWaitSeconds = 3.0f;
	CurrentRoundState = EYenkaRoundState::WaitingForPlayers;
	ActivePlayerIndex = -1;
}

void AYenkaGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogYenkaVR, Log, TEXT("AYenkaGameMode initialized. Ready for players."));

	// Auto-spawn Tower Manager if not already placed in the level
	if (!UGameplayStatics::GetActorOfClass(GetWorld(), AYenkaTowerManager::StaticClass()))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		GetWorld()->SpawnActor<AYenkaTowerManager>(AYenkaTowerManager::StaticClass(), FVector(80.0f, 0.0f, 70.0f), FRotator::ZeroRotator, SpawnParams);
	}
}

UClass* AYenkaGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (GEngine && GEngine->XRSystem.IsValid() && GEngine->XRSystem->IsHeadTrackingAllowed())
	{
		return AYenkaVRPawn::StaticClass();
	}
	return AYenkaDesktopPawn::StaticClass();
}

void AYenkaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (NewPlayer)
	{
		TurnOrder.Add(NewPlayer);
		UE_LOG(LogYenkaVR, Log, TEXT("Player joined: %s. Total players: %d"), *NewPlayer->GetName(), TurnOrder.Num());

		if (TurnOrder.Num() >= 1 && CurrentRoundState == EYenkaRoundState::WaitingForPlayers)
		{
			CurrentRoundState = EYenkaRoundState::CalibratingSurface;
		}
	}
}

void AYenkaGameMode::Logout(AController* Exiting)
{
	APlayerController* PC = Cast<APlayerController>(Exiting);
	if (PC)
	{
		TurnOrder.Remove(PC);
	}
	Super::Logout(Exiting);
}

void AYenkaGameMode::StartNextTurn()
{
	if (TurnOrder.Num() == 0) return;

	ActivePlayerIndex = (ActivePlayerIndex + 1) % TurnOrder.Num();
	APlayerController* ActivePC = TurnOrder[ActivePlayerIndex];

	CurrentRoundState = EYenkaRoundState::TurnActive;

	AYenkaGameState* GS = GetGameState<AYenkaGameState>();
	if (GS)
	{
		GS->SetActivePlayer(ActivePC);
		GS->SetTurnTimeRemaining(MaxTurnTimeSeconds);
	}

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &AYenkaGameMode::OnTurnTimeout, MaxTurnTimeSeconds, false);

	UE_LOG(LogYenkaVR, Log, TEXT("Turn started for player %s (45s timer)"), *ActivePC->GetName());
}

void AYenkaGameMode::EndCurrentTurn(bool bPlayerClaimedSuccess)
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	CurrentRoundState = EYenkaRoundState::TurnStabilizing;

	GetWorldTimerManager().SetTimer(StabilizationTimerHandle, this, &AYenkaGameMode::OnStabilizationComplete, StabilizationWaitSeconds, false);
}

void AYenkaGameMode::OnStabilizationComplete()
{
	// Award cumulative points to active player
	if (TurnOrder.IsValidIndex(ActivePlayerIndex))
	{
		APlayerController* ActivePC = TurnOrder[ActivePlayerIndex];
		if (AYenkaPlayerState* PS = ActivePC->GetPlayerState<AYenkaPlayerState>())
		{
			PS->AddScore(100);
		}
	}

	StartNextTurn();
}

void AYenkaGameMode::OnTurnTimeout()
{
	UE_LOG(LogYenkaVR, Warning, TEXT("Turn timed out for active player!"));
	EndCurrentTurn(false);
}

void AYenkaGameMode::HandleTowerCollapse(APlayerController* ResponsiblePlayer)
{
	CurrentRoundState = EYenkaRoundState::RoundOver;
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().ClearTimer(StabilizationTimerHandle);

	UE_LOG(LogYenkaVR, Warning, TEXT("TOWER COLLAPSED! Round over."));
}
