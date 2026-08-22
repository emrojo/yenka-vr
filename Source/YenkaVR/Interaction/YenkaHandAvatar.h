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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* HandRoot;

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

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	FTransform ReplicatedHandTransform;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	float ReplicatedGripStrength;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	bool bIsLeftHand;
};
