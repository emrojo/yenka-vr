#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "YenkaHandAvatar.h"
#include "YenkaDesktopPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	EHandPoseMode ActiveGesturePreview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	bool bForceGesturePreview;

	void AdjustHandOffsetX(float Delta);
	void AdjustHandOffsetY(float Delta);
	void AdjustHandOffsetZ(float Delta);
	void AdjustHandPitch(float Delta);
	void AdjustHandYaw(float Delta);
	void AdjustHandRoll(float Delta);
	void QuickRotateYaw90();
	void QuickRotateRoll90();
	void QuickRotatePitch90();
	void ResetHandCalibration();
	void SetGesturePush();
	void SetGestureGrab();
	void SetGestureOpen();
	void CycleGesture();
	void UpdatePersistentCalibrationHUD();

	void OnCalibXPlus() { AdjustHandOffsetX(+0.5f); }
	void OnCalibXMinus() { AdjustHandOffsetX(-0.5f); }
	void OnCalibYMinus() { AdjustHandOffsetY(-0.5f); }
	void OnCalibYPlus() { AdjustHandOffsetY(+0.5f); }
	void OnCalibZPlus() { AdjustHandOffsetZ(+0.5f); }
	void OnCalibZMinus() { AdjustHandOffsetZ(-0.5f); }

	void OnCalibPitchMinus() { AdjustHandPitch(-15.0f); }
	void OnCalibPitchPlus() { AdjustHandPitch(+15.0f); }
	void OnCalibYawMinus() { AdjustHandYaw(-15.0f); }
	void OnCalibYawPlus() { AdjustHandYaw(+15.0f); }
	void OnCalibRollMinus() { AdjustHandRoll(-15.0f); }
	void OnCalibRollPlus() { AdjustHandRoll(+15.0f); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx Edit")
	bool bIsPhalanxEditMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx Edit")
	int32 SelectedFinger; // 0=All, 1=Thumb, 2=Index, 3=Middle, 4=Ring, 5=Pinky

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx Edit")
	int32 SelectedPhalanx; // 0=All, 1=Proximal, 2=Intermediate, 3=Distal

	void TogglePhalanxEditMode();
	void SelectFinger(int32 FingerIdx);
	void SelectPhalanx(int32 PhalanxIdx);
	void OnPhalanxPitchPlus();
	void OnPhalanxPitchMinus();
	void OnPhalanxYawPlus();
	void OnPhalanxYawMinus();
	void OnPhalanxRollPlus();
	void OnPhalanxRollMinus();
	void OnPhalanxFlexionPlus() { OnPhalanxPitchPlus(); }
	void OnPhalanxFlexionMinus() { OnPhalanxPitchMinus(); }
	void OnPhalanxLateralPlus() { OnPhalanxYawPlus(); }
	void OnPhalanxLateralMinus() { OnPhalanxYawMinus(); }
	void OnResetSelectedPhalanx();
	void OnKeyZPressed();
	void OnKeyXPressed();
	void OnKeyCPressed();
	void OnKeyVPressed();
	void OnKeyQPressed();
	void OnKeyMPressed();
	void OnKeyBPressed();

	// --- Custom Gesture System (Creation, Naming, JSON Persistence & Cycling) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Gestures")
	TArray<FCustomHandGesture> CustomGesturesList;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Gestures")
	int32 ActiveCustomGestureIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Gestures")
	bool bIsNamingCustomGesture = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Gestures")
	FString CurrentTypedGestureName;

	void StartNamingGesture();
	void ConfirmSaveGesture();
	void CancelNamingGesture();
	void OnAnyKeyPressed(FKey Key);
	void OnKeyEnterPressed();
	void OnKeyEscapePressed();
	void OnKeyBackspacePressed();
	void OnKeyLeftBracketPressed();
	void OnKeyRightBracketPressed();
	void OnKeyDeletePressed();

	void CycleNextCustomGesture();
	void CyclePrevCustomGesture();
	void LoadCustomGestureByIndex(int32 Index);
	void LoadCustomGestureByName(const FString& Name);

	bool SaveCustomGesturesToDisk();
	bool LoadCustomGesturesFromDisk();

	UFUNCTION(Exec)
	void SaveHandGesture(const FString& Name);

	UFUNCTION(Exec)
	void LoadHandGesture(const FString& Name);

	UFUNCTION(Exec)
	void ListHandGestures();

	UFUNCTION(Exec)
	void DeleteHandGesture(const FString& Name);

	// --- Custom Hand Position Transform System (Creation, Naming, JSON Persistence & Cycling) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Transforms")
	TArray<FCustomHandTransform> CustomTransformsList;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Transforms")
	int32 ActiveCustomTransformIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Transforms")
	bool bIsNamingCustomTransform = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Transforms")
	FString CurrentTypedTransformName;

	void StartNamingTransform();
	void ConfirmSaveTransform();
	void CancelNamingTransform();

	void OnKeyF8Pressed();
	void OnKeyF9Pressed();
	void OnKeyF10Pressed();

	void CycleNextCustomTransform();
	void CyclePrevCustomTransform();
	void LoadCustomTransformByIndex(int32 Index);
	void LoadCustomTransformByName(const FString& Name);

	bool SaveCustomTransformsToDisk();
	bool LoadCustomTransformsFromDisk();

	UFUNCTION(Exec)
	void SaveHandTransform(const FString& Name);

	UFUNCTION(Exec)
	void LoadHandTransform(const FString& Name);

	UFUNCTION(Exec)
	void ListHandTransforms();

	UFUNCTION(Exec)
	void ReloadHandTransforms();

	UFUNCTION(Exec)
	void ReloadHandGestures();

	void OnKeyHPressed();

	UFUNCTION(Exec)
	void CycleHandModel();

	UFUNCTION(Exec)
	void SetHandModel(const FString& ModelName);

	UFUNCTION(Exec)
	void ListHandModels();

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
