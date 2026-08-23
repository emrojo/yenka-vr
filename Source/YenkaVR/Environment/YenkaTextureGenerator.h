#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "YenkaEnvironmentManager.h"

/**
 * High-performance procedural PBR texture synthesis utility for YenkaVR.
 * Generates photorealistic wood grain, marble veining, woven textiles, carbon fiber, and panoramic sky vistas.
 */
class FYenkaTextureGenerator
{
public:
	static UTexture2D* CreateWoodTexture(int32 Width, int32 Height, const FColor& BaseColor, const FColor& GrainColor, float Frequency, float Turbulence);
	static UTexture2D* CreateMarbleTexture(int32 Width, int32 Height, const FColor& StoneColor, const FColor& VeinColor, float VeinScale);
	static UTexture2D* CreateTatamiTexture(int32 Width, int32 Height);
	static UTexture2D* CreateCarpetTexture(int32 Width, int32 Height, const FColor& BgColor, const FColor& BorderColor);
	static UTexture2D* CreateCarbonFiberTexture(int32 Width, int32 Height);

	static UTexture2D* CreateVistaTexture(int32 Width, int32 Height, EYenkaEnvironmentTheme Theme);

private:
	static float SimpleNoise2D(float X, float Y);
	static float FractalNoise2D(float X, float Y, int32 Octaves, float Persistence);
	static UTexture2D* AllocateTexture(int32 Width, int32 Height);
};
