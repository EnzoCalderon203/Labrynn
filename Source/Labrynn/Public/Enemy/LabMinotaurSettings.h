#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LabMinotaurSettings.generated.h"

/** Valores del Minotauro (ajustables sin compilar en /Game/Labrynn/Core/DA_Minotaur). */
UCLASS(BlueprintType)
class LABRYNN_API ULabMinotaurSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Persiguiendo: menor que el sprint (700), así se le puede escapar corriendo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimiento")
	float ChaseSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimiento")
	float WanderSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ataque")
	float AttackRange = 170.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ataque")
	float AttackWound = 50.f;

	/** Segundos aturdido después de golpear. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ataque")
	float StunAfterHit = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oído")
	float MinNoiseStrength = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oído")
	float NoiseMemory = 8.f;

	// --- Miedo al fuego ---
	/** Un farol encendido a menos de esto, y por delante, lo frena. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuego")
	float FireFearRadius = 400.f;

	/** Qué tan "por delante" tiene que estar el farol (coseno entre dirección al objetivo y al farol). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuego")
	float FireAheadDot = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuego")
	float RetreatSeconds = 3.f;

	/** Multiplicador de gasto de aceite del farol que lo está repeliendo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuego")
	float RepelOilDrainMultiplier = 4.f;
};
