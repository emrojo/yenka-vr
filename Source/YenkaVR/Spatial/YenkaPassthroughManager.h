#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaPassthroughManager.generated.h"

/**
 * Manages OpenXR Passthrough state and transitions between MR and virtual room.
 */
UCLASS()
class YENKAVR_API AYenkaPassthroughManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaPassthroughManager();

	UFUNCTION(BlueprintCallable, Category = "Yenka|XR")
	void SetPassthroughEnabled(bool bEnable);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Yenka|XR")
	bool bIsPassthroughActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|XR")
	AActor* VirtualRoomEnvironment;
};
