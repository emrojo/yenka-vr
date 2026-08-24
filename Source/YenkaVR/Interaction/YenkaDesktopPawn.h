#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "YenkaHandAvatar.h"
#include "YenkaDesktopPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AYenkaBlock;

USTRUCT(BlueprintType)
struct FYenkaInteractionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Config")
	float VerticalGrabMinClearance = 0.5f; // cm (space required on both sides along long axis)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Config")
	float ProtrusionThreshold = 0.4f; // cm (distance required to consider a specific side protruding)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Config")
	float CraneTableClearanceHeight = 5.0f; // cm (elevation above table surface when outside tower)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Config")
	float CraneTowerTopClearanceHeight = 6.0f; // cm (elevation above highest tower block when over tower)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Config")
	float CraneSafeRadius = 20.0f; // cm (safety zone radius where block is at full elevation)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Config")
	float CraneTransitionRadius = 30.0f; // cm (radial distance over which elevation is smoothly interpolated)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Config")
	float CraneTopSnapRadius = 8.0f; // cm
};

UENUM(BlueprintType)
enum class ECraneGrabPhase : uint8
{
	Inactive,
	Descending,   // Mano en OpenHand bajando hasta tocar la cara superior de la pieza
	Grasping,     // Mano sobre la pieza cerrándose y transicionando a VerticalGrab
	Ascending,    // Mano y pieza subiendo juntas a la cota de elevación de grúa
	Carrying      // Transporte libre en grúa mientras se mantenga el botón izquierdo
};

UENUM(BlueprintType)
enum class EBlockFaceType : uint8
{
	None,
	TopFace,
	SmallEndFace,   // Lados pequeños: 2.5cm x 1.5cm (extremos longitudinales) -> PUSH
	LargeSideFace   // Lados mayores: 7.5cm x 1.5cm (flancos laterales) -> PULL si sobresale
};

UENUM(BlueprintType)
enum class EYenkaEditDimension : uint8
{
	LocationX      UMETA(DisplayName = "Posición X (Frente/Atrás)"),
	LocationY      UMETA(DisplayName = "Posición Y (Izquierda/Derecha)"),
	LocationZ      UMETA(DisplayName = "Posición Z (Altura/Vertical)"),
	RotationPitch  UMETA(DisplayName = "Pitch (Inclinación/Flexión)"),
	RotationYaw    UMETA(DisplayName = "Yaw (Giro/Lateral)"),
	RotationRoll   UMETA(DisplayName = "Roll (Torsión/Inclinación Lateral)"),
	AxialRotation  UMETA(DisplayName = "Eje Base Índice")
};

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

	// --- Crane / Vertical Grab Mode ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	bool bIsCraneGrabbing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	ECraneGrabPhase CraneGrabPhase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	float CraneGrabPhaseTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	FVector CraneDescendStartPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	FVector CraneBlockTargetPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	float CraneTargetElevatedZ;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	float CraneCurrentZ;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	float CraneUserYaw;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	FVector CraneInitialGrabPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Crane")
	FRotator CraneTargetRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	FVector VerticalGrabLocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	FRotator VerticalGrabRotationOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Config")
	FYenkaInteractionConfig InteractionConfig;

	UPROPERTY()
	FDateTime LastInteractionConfigFileTimestamp;

	bool LoadInteractionConfigFromDisk();
	bool SaveInteractionConfigToDisk();
	bool IsBlockTopFaceAccessible(const AYenkaBlock* Block, float MinClearance) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Pull")
	FVector LockedPullInitialPos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Pull")
	FVector LockedPullDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Pull")
	FQuat LockedPullHandQuat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Pull")
	float LockedPullPlaneZ;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Pull")
	float CurrentPullOutwardAdvance;

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
	bool IsHoveredFaceProtruding(const AYenkaBlock* Block, const FVector& HitLocation, FVector& OutProtrudingPos, FVector& OutProtrudingNorm) const;
	EBlockFaceType GetBlockHitFaceType(const AYenkaBlock* Block, const FVector& HitLocation, const FVector& HitNormal, FVector& OutFacePos, FVector& OutFaceNormal, bool& bOutIsProtruding) const;
	float CalculateCraneTargetZ(const FVector& TargetXY, float HighestBlockZ) const;
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
	FVector OpenHandLocationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	FRotator OpenHandRotationOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	float GrabStandbySeparation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	float PokeStandbySeparation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	EHandPoseMode ActiveGesturePreview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	bool bForceGesturePreview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Hand Calibration")
	EYenkaEditDimension SelectedDimension;

	void CycleNextDimension();
	void CyclePrevDimension();
	void SelectDimension(EYenkaEditDimension NewDim);
	void IncreaseSelectedDimension();
	void DecreaseSelectedDimension();
	void AdjustSelectedDimension(float Direction);
	void ResetSelectedDimension();
	FString GetSelectedDimensionName() const;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Phalanx")
	FTransform FixedPhalanxEditTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Phalanx")
	bool bHasFixedPhalanxTransform = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Yenka|Transforms")
	FString CurrentTypedTransformName;

	UPROPERTY()
	FDateTime LastTransformFileTimestamp;

	UPROPERTY()
	FDateTime LastGestureFileTimestamp;

	void CheckForLiveJsonModifications();

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
