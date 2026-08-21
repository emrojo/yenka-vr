#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "YenkaVRPawn.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class AYenkaHandAvatar;

/**
 * VR Pawn for OpenXR / SteamVR headsets with 6DOF tracking and spectator gesture support.
 */
UCLASS()
class YENKAVR_API AYenkaVRPawn : public APawn
{
	GENERATED_BODY()

public:
	AYenkaVRPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* VROrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UMotionControllerComponent* LeftController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UMotionControllerComponent* RightController;

	UPROPERTY(EditDefaultsOnly, Category = "Yenka|Avatar")
	TSubclassOf<AYenkaHandAvatar> HandAvatarClass;

protected:
	UPROPERTY()
	AYenkaHandAvatar* LeftHandAvatar;

	UPROPERTY()
	AYenkaHandAvatar* RightHandAvatar;

	// Spectator pinch zoom and world drag tracking
	bool bIsLeftGrabbingSpace;
	bool bIsRightGrabbingSpace;
	float InitialPinchDistance;
	FVector InitialLeftWorldPos;
	FVector InitialRightWorldPos;

	void UpdateSpectatorGestures();
};
