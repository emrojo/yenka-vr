#include "YenkaTextureGenerator.h"

float FYenkaTextureGenerator::SimpleNoise2D(float X, float Y)
{
	int32 xi = FMath::FloorToInt(X);
	int32 yi = FMath::FloorToInt(Y);
	float xf = X - xi;
	float yf = Y - yi;

	auto Hash = [](int32 x, int32 y) -> float
	{
		int32 n = x * 374761393 + y * 668265263;
		n = (n ^ (n >> 13)) * 1274126177;
		return static_cast<float>(n & 0x7fffffff) / static_cast<float>(0x7fffffff);
	};

	float v00 = Hash(xi, yi);
	float v10 = Hash(xi + 1, yi);
	float v01 = Hash(xi, yi + 1);
	float v11 = Hash(xi + 1, yi + 1);

	// Smooth cubic interpolation
	float u = xf * xf * (3.0f - 2.0f * xf);
	float v = yf * yf * (3.0f - 2.0f * yf);

	return FMath::Lerp(FMath::Lerp(v00, v10, u), FMath::Lerp(v01, v11, u), v);
}

float FYenkaTextureGenerator::FractalNoise2D(float X, float Y, int32 Octaves, float Persistence)
{
	float Total = 0.0f;
	float Frequency = 1.0f;
	float Amplitude = 1.0f;
	float MaxValue = 0.0f;

	for (int32 i = 0; i < Octaves; ++i)
	{
		Total += SimpleNoise2D(X * Frequency, Y * Frequency) * Amplitude;
		MaxValue += Amplitude;
		Amplitude *= Persistence;
		Frequency *= 2.0f;
	}

	return Total / MaxValue;
}

UTexture2D* FYenkaTextureGenerator::AllocateTexture(int32 Width, int32 Height)
{
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (Texture)
	{
		Texture->SRGB = true;
		Texture->Filter = TF_Trilinear;
	}
	return Texture;
}

UTexture2D* FYenkaTextureGenerator::CreateWoodTexture(int32 Width, int32 Height, const FColor& BaseColor, const FColor& GrainColor, float Frequency, float Turbulence)
{
	UTexture2D* Texture = AllocateTexture(Width, Height);
	if (!Texture) return nullptr;

	uint8* Pixels = static_cast<uint8*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			float NormX = static_cast<float>(X) / static_cast<float>(Width);
			float NormY = static_cast<float>(Y) / static_cast<float>(Height);

			// Wood rings with longitudinal fiber distortion
			float NoiseVal = FractalNoise2D(NormX * 8.0f, NormY * 32.0f, 4, 0.5f);
			float GrainVal = FMath::Sin((NormX * Frequency + NoiseVal * Turbulence) * 6.28318f);
			GrainVal = (GrainVal + 1.0f) * 0.5f;

			// Add fine micro-fiber striations along Y
			float MicroFiber = SimpleNoise2D(NormX * 128.0f, NormY * 8.0f) * 0.15f;
			GrainVal = FMath::Clamp(GrainVal + MicroFiber, 0.0f, 1.0f);

			FColor FinalCol = FColor(
				FMath::Lerp(BaseColor.R, GrainColor.R, GrainVal),
				FMath::Lerp(BaseColor.G, GrainColor.G, GrainVal),
				FMath::Lerp(BaseColor.B, GrainColor.B, GrainVal),
				255
			);

			int32 Index = (Y * Width + X) * 4;
			Pixels[Index + 0] = FinalCol.B;
			Pixels[Index + 1] = FinalCol.G;
			Pixels[Index + 2] = FinalCol.R;
			Pixels[Index + 3] = 255;
		}
	}

	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

UTexture2D* FYenkaTextureGenerator::CreateMarbleTexture(int32 Width, int32 Height, const FColor& StoneColor, const FColor& VeinColor, float VeinScale)
{
	UTexture2D* Texture = AllocateTexture(Width, Height);
	if (!Texture) return nullptr;

	uint8* Pixels = static_cast<uint8*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			float NormX = static_cast<float>(X) / static_cast<float>(Width);
			float NormY = static_cast<float>(Y) / static_cast<float>(Height);

			// Domain warping for organic mineral veining
			float WarpX = FractalNoise2D(NormX * 6.0f, NormY * 6.0f, 4, 0.5f);
			float WarpY = FractalNoise2D(NormX * 6.0f + 5.2f, NormY * 6.0f + 1.3f, 4, 0.5f);

			float VeinNoise = FractalNoise2D((NormX + WarpX * 0.4f) * VeinScale, (NormY + WarpY * 0.4f) * VeinScale, 5, 0.55f);
			float VeinIntensity = FMath::Abs(FMath::Sin(VeinNoise * 3.14159f * 3.0f));
			VeinIntensity = FMath::Pow(VeinIntensity, 6.0f); // Sharp, delicate veins

			FColor FinalCol = FColor(
				FMath::Lerp(StoneColor.R, VeinColor.R, VeinIntensity),
				FMath::Lerp(StoneColor.G, VeinColor.G, VeinIntensity),
				FMath::Lerp(StoneColor.B, VeinColor.B, VeinIntensity),
				255
			);

			int32 Index = (Y * Width + X) * 4;
			Pixels[Index + 0] = FinalCol.B;
			Pixels[Index + 1] = FinalCol.G;
			Pixels[Index + 2] = FinalCol.R;
			Pixels[Index + 3] = 255;
		}
	}

	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

UTexture2D* FYenkaTextureGenerator::CreateTatamiTexture(int32 Width, int32 Height)
{
	UTexture2D* Texture = AllocateTexture(Width, Height);
	if (!Texture) return nullptr;

	uint8* Pixels = static_cast<uint8*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			// Tight orthogonal straw weave
			float Reed = FMath::Abs(FMath::Sin(static_cast<float>(Y) * 0.4f));
			float CrossWeave = FMath::Abs(FMath::Sin(static_cast<float>(X) * 0.08f));

			float Tone = 0.70f + (Reed * 0.20f) + (CrossWeave * 0.10f);

			// Natural green-gold rush straw color
			uint8 R = static_cast<uint8>(FMath::Clamp(125.0f * Tone, 0.0f, 255.0f));
			uint8 G = static_cast<uint8>(FMath::Clamp(120.0f * Tone, 0.0f, 255.0f));
			uint8 B = static_cast<uint8>(FMath::Clamp(80.0f * Tone, 0.0f, 255.0f));

			int32 Index = (Y * Width + X) * 4;
			Pixels[Index + 0] = B;
			Pixels[Index + 1] = G;
			Pixels[Index + 2] = R;
			Pixels[Index + 3] = 255;
		}
	}

	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

UTexture2D* FYenkaTextureGenerator::CreateCarpetTexture(int32 Width, int32 Height, const FColor& BgColor, const FColor& BorderColor)
{
	UTexture2D* Texture = AllocateTexture(Width, Height);
	if (!Texture) return nullptr;

	uint8* Pixels = static_cast<uint8*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			float NormX = static_cast<float>(X) / static_cast<float>(Width);
			float NormY = static_cast<float>(Y) / static_cast<float>(Height);

			// Border frame detection
			float EdgeDist = FMath::Min(FMath::Min(NormX, 1.0f - NormX), FMath::Min(NormY, 1.0f - NormY));
			bool bIsBorder = (EdgeDist < 0.12f && EdgeDist > 0.03f);

			// Textile fiber noise
			float FiberNoise = SimpleNoise2D(NormX * 64.0f, NormY * 64.0f) * 0.2f;

			FColor Base = bIsBorder ? BorderColor : BgColor;

			int32 Index = (Y * Width + X) * 4;
			Pixels[Index + 0] = static_cast<uint8>(FMath::Clamp(Base.B * (0.9f + FiberNoise), 0.0f, 255.0f));
			Pixels[Index + 1] = static_cast<uint8>(FMath::Clamp(Base.G * (0.9f + FiberNoise), 0.0f, 255.0f));
			Pixels[Index + 2] = static_cast<uint8>(FMath::Clamp(Base.R * (0.9f + FiberNoise), 0.0f, 255.0f));
			Pixels[Index + 3] = 255;
		}
	}

	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

UTexture2D* FYenkaTextureGenerator::CreateCarbonFiberTexture(int32 Width, int32 Height)
{
	UTexture2D* Texture = AllocateTexture(Width, Height);
	if (!Texture) return nullptr;

	uint8* Pixels = static_cast<uint8*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			// 2x2 Twill weave diagonal pattern
			int32 CellX = (X / 8) % 2;
			int32 CellY = (Y / 8) % 2;
			bool bDiagonal = ((CellX + CellY) % 2) == 0;

			float Sheen = bDiagonal ? 0.35f : 0.15f;

			uint8 Val = static_cast<uint8>(Sheen * 255.0f);

			int32 Index = (Y * Width + X) * 4;
			Pixels[Index + 0] = Val + 10;
			Pixels[Index + 1] = Val + 5;
			Pixels[Index + 2] = Val;
			Pixels[Index + 3] = 255;
		}
	}

	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

UTexture2D* FYenkaTextureGenerator::CreateVistaTexture(int32 Width, int32 Height, EYenkaEnvironmentTheme Theme)
{
	UTexture2D* Texture = AllocateTexture(Width, Height);
	if (!Texture) return nullptr;

	uint8* Pixels = static_cast<uint8*>(Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			float U = static_cast<float>(X) / static_cast<float>(Width);
			float V = static_cast<float>(Y) / static_cast<float>(Height); // 0 is top, 1 is bottom

			FColor Color(10, 15, 30, 255);

			switch (Theme)
			{
			case EYenkaEnvironmentTheme::ModernPenthouse:
			{
				// Night City Skyline: Dark blue atmospheric gradient + illuminated skyscrapers
				float SkyGradient = FMath::Lerp(0.10f, 0.35f, 1.0f - V);
				Color = FColor(
					static_cast<uint8>(20 * SkyGradient),
					static_cast<uint8>(45 * SkyGradient),
					static_cast<uint8>(110 * SkyGradient),
					255
				);

				// Skyscraper silhouettes in bottom half
				if (V > 0.40f)
				{
					int32 BuildingIndex = static_cast<int32>(U * 24.0f);
					float BuildingHeight = 0.45f + SimpleNoise2D(static_cast<float>(BuildingIndex) * 1.7f, 0.0f) * 0.45f;
					if (V > (1.0f - BuildingHeight))
					{
						// Building body
						Color = FColor(12, 16, 25, 255);
						// Window grid lights
						bool bWindowX = (X % 12) < 6;
						bool bWindowY = (Y % 16) < 8;
						if (bWindowX && bWindowY && SimpleNoise2D(static_cast<float>(X) * 0.1f, static_cast<float>(Y) * 0.1f) > 0.4f)
						{
							Color = FColor(255, 230, 160, 255); // Warm glowing window
						}
					}
				}
				break;
			}

			case EYenkaEnvironmentTheme::CozyCabin:
			{
				// Alpine Sunset: Warm orange-gold sky with majestic jagged mountain silhouette
				float SkyBlend = FMath::Clamp((1.0f - V) * 1.5f, 0.0f, 1.0f);
				Color = FColor(
					static_cast<uint8>(FMath::Lerp(230.0f, 60.0f, V)),
					static_cast<uint8>(FMath::Lerp(130.0f, 25.0f, V)),
					static_cast<uint8>(FMath::Lerp(60.0f, 15.0f, V)),
					255
				);

				// Jagged mountain ridge
				float MountainLine = 0.50f + FractalNoise2D(U * 8.0f, 0.0f, 4, 0.5f) * 0.35f;
				if (V > (1.0f - MountainLine))
				{
					// Snow caps vs dark rock
					bool bSnow = (V < (1.0f - MountainLine + 0.10f));
					Color = bSnow ? FColor(240, 220, 230, 255) : FColor(35, 25, 30, 255);
				}
				break;
			}

			case EYenkaEnvironmentTheme::ZenGarden:
			{
				// Serene Japanese Garden: Soft pink sakura dawn
				Color = FColor(
					static_cast<uint8>(FMath::Lerp(240.0f, 210.0f, V)),
					static_cast<uint8>(FMath::Lerp(180.0f, 160.0f, V)),
					static_cast<uint8>(FMath::Lerp(200.0f, 180.0f, V)),
					255
				);

				// Distant Mount Fuji / Cherry tree canopy
				float Canopy = 0.35f + FractalNoise2D(U * 12.0f, 0.0f, 3, 0.6f) * 0.25f;
				if (V > (1.0f - Canopy))
				{
					Color = FColor(215, 120, 150, 255); // Sakura blossom pink
				}
				break;
			}

			case EYenkaEnvironmentTheme::SpaceObservatory:
			{
				// Deep Cosmic Nebula & Starfield
				float Nebula = FractalNoise2D(U * 4.0f, V * 4.0f, 5, 0.6f);
				float Stars = (SimpleNoise2D(U * 128.0f, V * 128.0f) > 0.96f) ? 255.0f : 0.0f;

				Color = FColor(
					static_cast<uint8>(FMath::Clamp(Nebula * 120.0f + Stars, 0.0f, 255.0f)),
					static_cast<uint8>(FMath::Clamp(Nebula * 30.0f + Stars, 0.0f, 255.0f)),
					static_cast<uint8>(FMath::Clamp(Nebula * 220.0f + Stars, 0.0f, 255.0f)),
					255
				);
				break;
			}

			case EYenkaEnvironmentTheme::VictorianLibrary:
			default:
			{
				// Misty Stately Estate Garden
				Color = FColor(
					static_cast<uint8>(FMath::Lerp(110.0f, 50.0f, V)),
					static_cast<uint8>(FMath::Lerp(140.0f, 75.0f, V)),
					static_cast<uint8>(FMath::Lerp(120.0f, 60.0f, V)),
					255
				);

				float GardenHedge = 0.30f + FractalNoise2D(U * 6.0f, 0.0f, 3, 0.5f) * 0.15f;
				if (V > (1.0f - GardenHedge))
				{
					Color = FColor(25, 45, 30, 255);
				}
				break;
			}
			}

			int32 Index = (Y * Width + X) * 4;
			Pixels[Index + 0] = Color.B;
			Pixels[Index + 1] = Color.G;
			Pixels[Index + 2] = Color.R;
			Pixels[Index + 3] = 255;
		}
	}

	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}
