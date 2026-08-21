#include "YenkaSteamLobbyManager.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "YenkaVR.h"

UYenkaSteamLobbyManager::UYenkaSteamLobbyManager()
{
}

void UYenkaSteamLobbyManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UYenkaSteamLobbyManager::OnCreateSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UYenkaSteamLobbyManager::OnJoinSessionComplete);
		}
	}
}

FString UYenkaSteamLobbyManager::GenerateRoomCode() const
{
	const TCHAR ValidChars[] = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
	const int32 CodeLength = 4;
	FString Code = TEXT("YK-");

	for (int32 i = 0; i < CodeLength; ++i)
	{
		int32 Index = FMath::RandRange(0, UE_ARRAY_COUNT(ValidChars) - 2);
		Code.AppendChar(ValidChars[Index]);
	}

	return Code;
}

void UYenkaSteamLobbyManager::CreateLobby(int32 MaxPlayers)
{
	if (!SessionInterface.IsValid()) return;

	CurrentRoomCode = GenerateRoomCode();

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.Set(SETTING_MAPNAME, FString(TEXT("YenkaGameMap")), EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(FName(TEXT("ROOM_CODE")), CurrentRoomCode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UYenkaSteamLobbyManager::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogYenkaVR, Log, TEXT("Steam Lobby created successfully with code: %s"), *CurrentRoomCode);
		OnLobbyCreated.Broadcast(CurrentRoomCode);
	}
	else
	{
		UE_LOG(LogYenkaVR, Error, TEXT("Failed to create Steam Lobby."));
	}
}

void UYenkaSteamLobbyManager::JoinLobbyByCode(const FString& RoomCode)
{
	UE_LOG(LogYenkaVR, Log, TEXT("Searching for Steam Lobby with code: %s"), *RoomCode);
	// Search logic matching ROOM_CODE setting
}

void UYenkaSteamLobbyManager::OnFindSessionsComplete(bool bWasSuccessful)
{
}

void UYenkaSteamLobbyManager::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	OnLobbyJoined.Broadcast(bSuccess);
}
