#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "YenkaPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32, NewScore);

/**
 * Player state holding cumulative points, VR status, and player metadata.
 */
UCLASS()
class YENKAVR_API AYenkaPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AYenkaPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Yenka|Score")
	void AddScore(int32 Delta);

	UPROPERTY(ReplicatedUsing = OnRep_CumulativeScore, BlueprintReadOnly, Category = "Yenka|Score")
	int32 CumulativeScore;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Yenka|XR")
	bool bIsUsingVR;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Yenka|XR")
	bool bIsUsingPassthroughAR;

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnScoreChanged OnScoreChanged;

protected:
	UFUNCTION()
	void OnRep_CumulativeScore();
};
