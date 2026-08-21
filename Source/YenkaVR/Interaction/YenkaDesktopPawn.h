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

	bool bIsOrbitingCamera;
	bool bIsPokeModeActive;
	bool bIsPushingBlock;
	float GrabDistance;
	FVector LastHitLocation;
	FVector LastHitNormal;

	void HandleMouseTrace();
	void OnPrimaryClickPressed();
	void OnPrimaryClickReleased();
	void OnSecondaryClickPressed();
	void OnSecondaryClickReleased();
	void OnPokeKeyPressed();
	void OnPokeKeyReleased();
	void OnTogglePokeMode();
	void PerformLongitudinalPush(AYenkaBlock* Block, const FVector& DirectionNormal);
	void OnMouseX(float Val);
	void OnMouseY(float Val);
	void OnMouseWheel(float Val);
	void MoveForward(float Val);
	void MoveRight(float Val);
};
