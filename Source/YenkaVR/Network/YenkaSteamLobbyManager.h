#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "YenkaSteamLobbyManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyCreatedEvent, const FString&, RoomCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyJoinedEvent, bool, bSuccess);

/**
 * Manages Steam Lobbies creation, searching, joining by alphanumeric code, and friends list invites.
 */
UCLASS()
class YENKAVR_API UYenkaSteamLobbyManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UYenkaSteamLobbyManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Creates a new public Steam Lobby and generates a short room code */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Steam")
	void CreateLobby(int32 MaxPlayers = 6);

	/** Joins a lobby using a 4-to-6 character alphanumeric room code */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Steam")
	void JoinLobbyByCode(const FString& RoomCode);

	/** Generates a human-friendly short alphanumeric code */
	UFUNCTION(BlueprintPure, Category = "Yenka|Steam")
	FString GenerateRoomCode() const;

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnLobbyCreatedEvent OnLobbyCreated;

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnLobbyJoinedEvent OnLobbyJoined;

protected:
	IOnlineSessionPtr SessionInterface;
	FString CurrentRoomCode;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
