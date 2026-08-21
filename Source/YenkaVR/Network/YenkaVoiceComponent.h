#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YenkaVoiceComponent.generated.h"

class UAudioComponent;
class USoundAttenuation;

/**
 * Manages 3D positional voice chat and spatialized audio attenuation attached to player avatars.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class YENKAVR_API UYenkaVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UYenkaVoiceComponent();

	virtual void BeginPlay() override;

	/** Toggles player microphone transmission */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Voice")
	void SetMuted(bool bMute);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Yenka|Audio")
	USoundAttenuation* SpatialAttenuationSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Audio")
	bool bIsMuted;
};
