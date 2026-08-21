#include "YenkaSurfaceCalibrator.h"
#include "YenkaVR.h"

AYenkaSurfaceCalibrator::AYenkaSurfaceCalibrator()
{
	PrimaryActorTick.bCanEverTick = false;
	bHasPoint1 = false;
}

void AYenkaSurfaceCalibrator::RecordCornerPoint1(const FVector& ControllerLocation)
{
	Point1 = ControllerLocation;
	bHasPoint1 = true;
	UE_LOG(LogYenkaVR, Log, TEXT("Recorded Table Corner Point 1: %s"), *Point1.ToString());
}

void AYenkaSurfaceCalibrator::RecordCornerPoint2(const FVector& ControllerLocation)
{
	if (!bHasPoint1) return;

	Point2 = ControllerLocation;
	FVector ForwardDir = (Point2 - Point1).GetSafeNormal2D();
	FRotator TableRotation = FRotationMatrix::MakeFromX(ForwardDir).Rotator();
	FVector TableCenter = (Point1 + Point2) * 0.5f;

	FTransform CalibratedTransform(TableRotation, TableCenter);
	OnTableCalibrated.Broadcast(CalibratedTransform);
	UE_LOG(LogYenkaVR, Log, TEXT("Table Calibrated Manually at: %s"), *TableCenter.ToString());
}

void AYenkaSurfaceCalibrator::ApplyOpenXRPlane(const FVector& PlaneCenter, const FVector& PlaneNormal)
{
	FRotator TableRotation = FRotationMatrix::MakeFromZ(PlaneNormal).Rotator();
	FTransform CalibratedTransform(TableRotation, PlaneCenter);
	OnTableCalibrated.Broadcast(CalibratedTransform);
	UE_LOG(LogYenkaVR, Log, TEXT("Table Calibrated via OpenXR Plane at: %s"), *PlaneCenter.ToString());
}
