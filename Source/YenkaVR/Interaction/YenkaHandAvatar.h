#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaHandAvatar.generated.h"

class USkeletalMeshComponent;
class UPhysicsHandleComponent;

UENUM(BlueprintType)
enum class EHandPoseMode : uint8
{
	OpenHand,
	GrabPinch,
	FingerPoke
};

USTRUCT(BlueprintType)
struct FPhalanxData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	float Pitch = 0.0f; // Rotation around Pitch (Y-axis) in degrees (-180 to +180)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	float Yaw = 0.0f; // Rotation around Yaw (Z-axis) in degrees (-180 to +180)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	float Roll = 0.0f; // Rotation around Roll (X-axis) in degrees (-180 to +180)
};

USTRUCT(BlueprintType)
struct FFingerPhalanges
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FPhalanxData Proximal; // Base / Knuckle

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FPhalanxData Intermediate; // Middle

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FPhalanxData Distal; // Tip / Nail
};

/**
 * Replicated visual representation of a player's hand in 3D space, visible to all users.
 */
UCLASS()
class YENKAVR_API AYenkaHandAvatar : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaHandAvatar();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Updates target transform from local input (VR or Desktop mouse target) */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Hand")
	void SetTargetHandTransform(const FTransform& InTransform, float InGripStrength);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Hand")
	void SetHandPoseMode(EHandPoseMode NewPoseMode);

	// --- 3-DOF Phalanx Articulation System (Pitch, Yaw, Roll) ---
	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void SetPhalanxPitch(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void SetPhalanxYaw(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void SetPhalanxRoll(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void SetPhalanxFlexion(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle) { SetPhalanxPitch(FingerIndex, PhalanxIndex, DeltaAngle); }

	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void SetPhalanxLateral(int32 FingerIndex, int32 PhalanxIndex, float DeltaAngle) { SetPhalanxYaw(FingerIndex, PhalanxIndex, DeltaAngle); }

	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void ResetPhalanx(int32 FingerIndex, int32 PhalanxIndex);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void ResetAllPhalanges();

	UFUNCTION(BlueprintCallable, Category = "Yenka|Phalanx")
	void LoadPresetPose(EHandPoseMode Mode);

	UFUNCTION(BlueprintPure, Category = "Yenka|Phalanx")
	FFingerPhalanges GetFingerPhalanges(int32 FingerIndex) const;

	UFUNCTION(BlueprintPure, Category = "Yenka|Phalanx")
	FString GetDetectedGestureDescription() const;

	void ApplyPhalanxTransforms();
	FRotator GetPhalanxDeltaRotationForBone(FName BoneName) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FFingerPhalanges ThumbPhalanges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FFingerPhalanges IndexPhalanges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FFingerPhalanges MiddlePhalanges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FFingerPhalanges RingPhalanges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Phalanx")
	FFingerPhalanges PinkyPhalanges;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* HandRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPoseableMeshComponent* PoseableHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* HandSkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PalmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ThumbMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* IndexFinger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MiddleFinger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* RingFinger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PinkyFinger;

	// Translucent Keratin Fingernails
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ThumbNail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* IndexNail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MiddleNail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* RingNail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PinkyNail;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPhysicsHandleComponent* PhysicsHandle;

	// Human Skin PBR & Subsurface Scattering (SSS) Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Skin")
	FLinearColor SkinTone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Skin")
	FLinearColor SubsurfaceColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Skin")
	FLinearColor FingernailColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Skin")
	float SkinRoughness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Skin")
	float SubsurfaceScatteringStrength;

	// Hand Animation Sequences for Guaranteed Pose Switching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Animation")
	UAnimSequence* AnimIdle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Animation")
	UAnimSequence* AnimPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Animation")
	UAnimSequence* AnimGrasp;

	UFUNCTION(BlueprintCallable, Category = "Yenka|Hand")
	void UpdateFingerPoses(float GripStrength);

	UFUNCTION(BlueprintCallable, Category = "Yenka|Hand")
	void ApplyHumanSkinMaterials();

	UFUNCTION(BlueprintPure, Category = "Yenka|Hand")
	float GetExtendedFingertipOffset() const;

	UFUNCTION(BlueprintPure, Category = "Yenka|Hand")
	FVector GetExtendedFingertipLocalOffset() const;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	EHandPoseMode CurrentPoseMode;

	EHandPoseMode LastAppliedPoseMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Meshes")
	USkeletalMesh* RightSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yenka|Meshes")
	USkeletalMesh* LeftSkeletalMesh;

	UFUNCTION(BlueprintCallable, Category = "Yenka|Hand")
	void SetIsLeftHand(bool bInIsLeft);

	UFUNCTION()
	void OnRep_IsLeftHand();

	void UpdateHandMeshSide();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	FTransform ReplicatedHandTransform;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	float ReplicatedGripStrength;

	UPROPERTY(ReplicatedUsing = OnRep_IsLeftHand, BlueprintReadOnly, Category = "Yenka|Hand")
	bool bIsLeftHand;
};
