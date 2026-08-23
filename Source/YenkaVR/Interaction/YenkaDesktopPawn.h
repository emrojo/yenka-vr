#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "YenkaDesktopPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AYenkaHandAvatar;
class AYenkaBlock;

/**
 * Desktop / Flat PC Pawn with orbital camera and mouse-driven virtual hand.
 */
UCLASS()
class YENKAVR_API AYenkaDesktopPawn : public APawn
{
	GENERATED_BODY()

public:
	AYenkaDesktopPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Yenka|Avatar")
	TSubclassOf<AYenkaHandAvatar> HandAvatarClass;

protected:
	UPROPERTY()
	AYenkaHandAvatar* VirtualHand;

	UPROPERTY()
	AYenkaBlock* HoveredBlock;

	UPROPERTY()
	AYenkaBlock* GrabbedBlock;

	UPROPERTY()
	AYenkaBlock* LockedPushBlock;

	bool bIsOrbitingCamera;
	bool bIsPokeModeActive;
	bool bIsPushingBlock;
	bool bIsLockedPerpendicular;
	FVector LockedRadialDirection;
	float LockedFloorZ;
	float GrabDistance;
	float CurrentPushAdvance;
	float CurrentPushDisplacement;
	FVector PushBlockInitialLocation;
	FVector PushApproachNormal;
	FVector PushLongitudinalAxis;
	FVector LastHitLocation;
	FVector LastHitNormal;
	float LastPrimaryClickTime;

	static constexpr float HAND_LENGTH = 11.0f;
	static constexpr float PROXIMITY_THRESHOLD = 2.0f * HAND_LENGTH;
	static constexpr float TOWER_BASE_RADIUS = 3.75f;
	static constexpr float INSPECTION_SAFE_RADIUS = TOWER_BASE_RADIUS + 1.0f; // 1.0cm outside tower
	static constexpr float PUSH_STANDBY_SEPARATION = 1.0f; // 1.0cm from block face
	static constexpr float GRAB_STANDBY_SEPARATION = 1.0f; // 1.0cm from block edge
	static constexpr float PUSH_VERTICAL_OFFSET = 0.0f;

	void HandleMouseTrace();
	void OnPrimaryClickPressed();
	void OnPrimaryClickReleased();
	void OnSecondaryClickPressed();
	void OnSecondaryClickReleased();
	void OnPokeKeyPressed();
	void OnPokeKeyReleased();
	void OnTogglePokeMode();
	void PerformLongitudinalPush(AYenkaBlock* Block, const FVector& DirectionNormal);
	FRotator GetHorizontalFacingRotation(const FVector& TargetLocation) const;
	FVector GetBlockStandOffLocation(const AYenkaBlock* Block, const FVector& ViewOrigin, FVector& OutApproachNormal, float Clearance = 8.0f, const FVector& LocalFingertipOffset = FVector(6.0f, -1.5f, 0.0f)) const;
	FVector GetBlockChosenFacePos(const AYenkaBlock* Block, const FVector& ApproachNormal) const;
	bool IsBlockProtruding(const AYenkaBlock* Block, FVector& OutProtrudingEdgePos, FVector& OutProtrudingNormal) const;
	void OnMouseX(float Val);
	void OnMouseY(float Val);
	void OnMouseWheel(float Val);
	void MoveForward(float Val);
	void MoveRight(float Val);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	FVector GrabHandLocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	FRotator GrabHandRotationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	FVector PokeHandLocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	FRotator PokeHandRotationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	float GrabStandbySeparation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	float PokeStandbySeparation;

	void AdjustHandOffsetX(float Delta);
	void AdjustHandOffsetY(float Delta);
	void AdjustHandOffsetZ(float Delta);
	void AdjustHandPitch(float Delta);
	void AdjustHandYaw(float Delta);
	void AdjustHandRoll(float Delta);
	void ResetHandCalibration();
	void DisplayHandCalibrationOnScreen();

	void OnCalibXPlus() { AdjustHandOffsetX(+0.25f); }
	void OnCalibXMinus() { AdjustHandOffsetX(-0.25f); }
	void OnCalibYMinus() { AdjustHandOffsetY(-0.25f); }
	void OnCalibYPlus() { AdjustHandOffsetY(+0.25f); }
	void OnCalibZPlus() { AdjustHandOffsetZ(+0.25f); }
	void OnCalibZMinus() { AdjustHandOffsetZ(-0.25f); }
	void OnCalibPitchMinus() { AdjustHandPitch(-2.5f); }
	void OnCalibPitchPlus() { AdjustHandPitch(+2.5f); }
	void OnCalibYawMinus() { AdjustHandYaw(-2.5f); }
	void OnCalibYawPlus() { AdjustHandYaw(+2.5f); }
	void OnCalibRollMinus() { AdjustHandRoll(-2.5f); }
	void OnCalibRollPlus() { AdjustHandRoll(+2.5f); }

	void OnSelectScenario1() { SelectScenarioTheme(0); }
	void OnSelectScenario2() { SelectScenarioTheme(1); }
	void OnSelectScenario3() { SelectScenarioTheme(2); }
	void OnSelectScenario4() { SelectScenarioTheme(3); }
	void OnSelectScenario5() { SelectScenarioTheme(4); }
	void OnSelectScenario6() { SelectScenarioTheme(5); }
	void OnSelectScenario7() { SelectScenarioTheme(6); }
	void SelectScenarioTheme(int32 ThemeIndex);
	void OnToggleScenarioMenu();
};
