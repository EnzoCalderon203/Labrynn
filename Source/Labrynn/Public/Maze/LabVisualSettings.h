#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LabVisualSettings.generated.h"

/**
 * Visibilidad sin farol (ajustable sin compilar en /Game/Labrynn/Core/DA_Visuals).
 * Objetivo: sin farol se distinguen formas y caminos, pero no trampas ni detalles.
 */
UCLASS(BlueprintType)
class LABRYNN_API ULabVisualSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Rayos de luna (grietas del techo) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luna", meta = (ClampMin = 0, ClampMax = 40))
	int32 MoonBeamCount = 9;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luna")
	float MoonIntensity = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luna")
	FLinearColor MoonColor = FLinearColor(0.55f, 0.68f, 1.f);

	/** Cuánto "se ve" el haz en la niebla volumétrica. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luna")
	float MoonVolumetricScattering = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luna")
	float MoonConeAngle = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Luna")
	FLinearColor CrackEmissive = FLinearColor(0.6f, 0.75f, 1.f) * 4.f;

	// --- Niebla (hace visibles los haces) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Niebla")
	float FogDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Niebla")
	bool bVolumetricFog = true;

	// --- Musgo apenas luminoso en la base de las paredes ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Musgo", meta = (ClampMin = 0, ClampMax = 200))
	int32 MossCellCount = 110;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Musgo")
	FLinearColor MossEmissive = FLinearColor(0.04f, 0.32f, 0.08f) * 4.f;

	// --- Adaptación ocular (cámara local) ---
	/** Exposición base (EV). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ojos")
	float BaseExposureBias = 4.0f;

	/** EV extra con los ojos totalmente adaptados a la oscuridad. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ojos")
	float DarkAdaptExtraBias = 2.8f;

	/** Segundos lejos de la luz para adaptarse por completo (lento a propósito). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ojos", meta = (ClampMin = 0.5))
	float DarkAdaptSeconds = 10.f;

	/** Velocidad (1/s) con la que se pierde la adaptación cerca de una luz. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ojos")
	float LightAdaptSpeed = 2.f;

	/** Un farol encendido a menos de esto cuenta como "estoy en la luz". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ojos")
	float LanternNearRadius = 550.f;

	// --- Encandilamiento ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encandilamiento")
	float GlareDistance = 230.f;

	/** Qué tan de frente hay que mirar el farol (coseno). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encandilamiento")
	float GlareDot = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encandilamiento")
	float GlareExtraBias = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encandilamiento")
	float GlareBloom = 4.f;

	/** Duración del encandilamiento (s). Se multiplica por cuánto estaban adaptados los ojos. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encandilamiento")
	float GlareSeconds = 1.2f;

	// --- Silueta de compañeros ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Silueta")
	float PersonalGlowIntensity = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Silueta")
	float PersonalGlowRadius = 320.f;
};
