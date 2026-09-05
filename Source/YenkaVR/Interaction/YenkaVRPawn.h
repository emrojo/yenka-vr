#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "YenkaVRPawn.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class AYenkaHandAvatar;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * VR Pawn for OpenXR / SteamVR headsets with 6DOF tracking, spectator gestures,
 * parabolic arc teleportation ("caña de teletransporte"), and snap turning.
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Teleport")
	USplineComponent* TeleportSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Teleport")
	UStaticMeshComponent* TeleportTargetRing;

	UPROPERTY(EditDefaultsOnly, Category = "Yenka|Avatar")
	TSubclassOf<AYenkaHandAvatar> HandAvatarClass;

	// Teleport configuration parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Teleport")
	float TeleportLaunchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Teleport")
	float MaxTeleportDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Teleport")
	float TeleportArcRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Teleport")
	FLinearColor ValidTeleportColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Teleport")
	FLinearColor InvalidTeleportColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Teleport")
	float SnapTurnAngle;

	// Teleport API
	UFUNCTION(BlueprintCallable, Category = "Yenka|Teleport")
	void StartTeleportTrace(bool bIsLeft);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Teleport")
	void UpdateTeleportTrace();

	UFUNCTION(BlueprintCallable, Category = "Yenka|Teleport")
	void ExecuteTeleport();

	UFUNCTION(BlueprintCallable, Category = "Yenka|Teleport")
	void CancelTeleport();

	UFUNCTION(BlueprintCallable, Category = "Yenka|Teleport")
	void ExecuteSnapTurn(float Direction);

protected:
	UPROPERTY()
	AYenkaHandAvatar* LeftHandAvatar;

	UPROPERTY()
	AYenkaHandAvatar* RightHandAvatar;

	// Teleport state
	bool bIsTeleportAiming;
	bool bTeleportUsingLeftHand;
	bool bIsTeleportTargetValid;
	FVector TeleportTargetLocation;
	FVector TeleportTargetNormal;

	UPROPERTY()
	TArray<USplineMeshComponent*> SplineMeshPool;

	UPROPERTY()
	UStaticMesh* SplineCylinderMesh;

	UPROPERTY()
	UMaterialInstanceDynamic* ArcMaterialInstance;

	UPROPERTY()
	UMaterialInstanceDynamic* RingMaterialInstance;

	// Snap turn debouncing
	bool bSnapTurnAxisReset;
	float SnapTurnCooldownTimer;

	// Input handlers
	void OnLeftThumbstickY(float Value);
	void OnRightThumbstickY(float Value);
	void OnLeftThumbstickX(float Value);
	void OnRightThumbstickX(float Value);

	// Spectator pinch zoom and world drag tracking
	bool bIsLeftGrabbingSpace;
	bool bIsRightGrabbingSpace;
	float InitialPinchDistance;
	FVector InitialLeftWorldPos;
	FVector InitialRightWorldPos;

	// --- Half-Life: Alyx Expressive Hands & Continuous Capacitive Tracking ---
	void UpdateExpressiveHandTracking(float DeltaTime);

	// --- Half-Life: Alyx "Russell Gravity Gloves" Flick & Catch System ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|GravityGloves")
	bool bEnableGravityGloves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|GravityGloves")
	float GravityGloveLockDistance = 350.0f;

	UPROPERTY()
	AActor* LeftTargetedBlock;

	UPROPERTY()
	AActor* RightTargetedBlock;

	UPROPERTY()
	AActor* LeftFlyingBlock;

	UPROPERTY()
	AActor* RightFlyingBlock;

	float LeftFlyingFlightTime;
	float RightFlyingFlightTime;

	FVector LeftPrevControllerLoc;
	FVector RightPrevControllerLoc;
	FRotator LeftPrevControllerRot;
	FRotator RightPrevControllerRot;

	void UpdateGravityGloves(float DeltaTime);
	void InitiateGravityPull(bool bIsLeftHand, AActor* TargetBlock);
	void UpdateFlyingBlocks(float DeltaTime);

	void UpdateSpectatorGestures();
	void ClearSplineMeshes();
	void BuildSplineMeshes();
};
