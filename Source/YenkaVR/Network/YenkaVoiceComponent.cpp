#include "YenkaVoiceComponent.h"
#include "YenkaVR.h"

UYenkaVoiceComponent::UYenkaVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsMuted = false;
	SpatialAttenuationSettings = nullptr;
}

void UYenkaVoiceComponent::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogYenkaVR, Log, TEXT("UYenkaVoiceComponent initialized on %s"), *GetOwner()->GetName());
}

void UYenkaVoiceComponent::SetMuted(bool bMute)
{
	bIsMuted = bMute;
	UE_LOG(LogYenkaVR, Log, TEXT("Voice muted status changed to: %s"), bIsMuted ? TEXT("MUTED") : TEXT("UNMUTED"));
}
