#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaHandAvatar.generated.h"

class USkeletalMeshComponent;
class UPhysicsHandleComponent;

/**
 * Replicated visual representation of a player's hand in 3D space, visible to all users.
 */
UCLASS()
class YENKAVR_API AYenkaHandAvatar : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaHandAvatar();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Updates target transform from local input (VR or Desktop mouse target) */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Hand")
	void SetTargetHandTransform(const FTransform& InTransform, float InGripStrength);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* HandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* HandVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPhysicsHandleComponent* PhysicsHandle;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	FTransform ReplicatedHandTransform;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	float ReplicatedGripStrength;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Yenka|Hand")
	bool bIsLeftHand;
};
