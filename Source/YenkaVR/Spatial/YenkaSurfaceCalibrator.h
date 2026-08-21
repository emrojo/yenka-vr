#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YenkaSurfaceCalibrator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTableCalibrated, const FTransform&, NewTableTransform);

/**
 * Surface calibration system supporting OpenXR Plane Detection (AR) and Manual 2-point touch.
 */
UCLASS()
class YENKAVR_API AYenkaSurfaceCalibrator : public AActor
{
	GENERATED_BODY()
	
public:	
	AYenkaSurfaceCalibrator();

	/** Set point 1 of table surface (origin corner) */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Calibration")
	void RecordCornerPoint1(const FVector& ControllerLocation);

	/** Set point 2 of table surface (orientation and width) */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Calibration")
	void RecordCornerPoint2(const FVector& ControllerLocation);

	/** Applies automatic plane detection from OpenXR */
	UFUNCTION(BlueprintCallable, Category = "Yenka|Calibration")
	void ApplyOpenXRPlane(const FVector& PlaneCenter, const FVector& PlaneNormal);

	UPROPERTY(BlueprintAssignable, Category = "Yenka|Events")
	FOnTableCalibrated OnTableCalibrated;

protected:
	FVector Point1;
	FVector Point2;
	bool bHasPoint1;
};
