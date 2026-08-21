#include "YenkaPassthroughManager.h"
#include "YenkaVR.h"

AYenkaPassthroughManager::AYenkaPassthroughManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsPassthroughActive = false;
	VirtualRoomEnvironment = nullptr;
}

void AYenkaPassthroughManager::SetPassthroughEnabled(bool bEnable)
{
	bIsPassthroughActive = bEnable;

	if (VirtualRoomEnvironment)
	{
		// Hide or show virtual room based on passthrough state
		VirtualRoomEnvironment->SetActorHiddenInGame(bIsPassthroughActive);
	}

	UE_LOG(LogYenkaVR, Log, TEXT("Passthrough MR mode set to: %s"), bIsPassthroughActive ? TEXT("ENABLED") : TEXT("DISABLED"));
}
