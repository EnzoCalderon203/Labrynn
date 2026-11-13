#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LabStaminaSettings.generated.h"

class USoundBase;

/**
 * Valores de stamina, heridas y movimiento. Se ajustan en el Data Asset
 * /Game/Labrynn/Core/DA_Stamina sin recompilar (el personaje lo lee en vivo).
 *
 * Modelo estilo Peak: UNA sola barra (0..MaxStamina). Las heridas bloquean la parte de
 * arriba de la barra; la stamina usable nunca supera MaxStamina - Heridas.
 * Si las heridas llenan la barra, el personaje cae (ragdoll).
 */
UCLASS(BlueprintType)
class LABRYNN_API ULabStaminaSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Barra ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barra", meta = (ClampMin = 10))
	float MaxStamina = 100.f;

	/** Recuperación por segundo cuando no se gasta. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barra", meta = (ClampMin = 0))
	float RegenPerSecond = 14.f;

	/** Segundos sin gastar antes de empezar a recuperar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barra", meta = (ClampMin = 0))
	float RegenDelay = 1.0f;

	// --- Movimiento ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimiento", meta = (ClampMin = 50))
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimiento", meta = (ClampMin = 50))
	float SprintSpeed = 700.f;

	/** Velocidad mientras jadea (stamina agotada). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimiento", meta = (ClampMin = 50))
	float ExhaustedSpeed = 230.f;

	/** Multiplicador de velocidad cargando la reliquia (se aplica sobre la velocidad actual). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimiento", meta = (ClampMin = 0.1, ClampMax = 1.0))
	float RelicSpeedMultiplier = 0.55f;

	// --- Gastos ---
	/** Gasto por segundo corriendo (100 / 9 ≈ 11 s de sprint con la barra llena). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gastos", meta = (ClampMin = 0))
	float SprintDrainPerSecond = 9.f;

	/** Gasto por segundo cargando la reliquia (aunque camine). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gastos", meta = (ClampMin = 0))
	float RelicDrainPerSecond = 2.5f;

	/** Gasto instantáneo al saltar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gastos", meta = (ClampMin = 0))
	float JumpCost = 8.f;

	// --- Jadeo (stamina en 0) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jadeo", meta = (ClampMin = 0.5))
	float ExhaustedSeconds = 4.f;

	/** Cada cuántos segundos el jadeo emite ruido (AI Perception: Hearing, tag "Jadeo"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jadeo", meta = (ClampMin = 0.1))
	float BreathNoiseInterval = 0.8f;

	/** Volumen del ruido (1 = normal) y alcance en cm para quien escuche (ej. un futuro Minotauro). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jadeo", meta = (ClampMin = 0))
	float BreathNoiseLoudness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jadeo", meta = (ClampMin = 0))
	float BreathNoiseRange = 1800.f;


	// --- Heridas ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heridas", meta = (ClampMin = 0))
	float SpikeWound = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heridas", meta = (ClampMin = 0))
	float PitWound = 25.f;

	/** Heridas con las que queda alguien al ser levantado por un compañero. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heridas", meta = (ClampMin = 0))
	float WoundsAfterRevive = 60.f;

	/** Heridas que cura un botiquín. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heridas", meta = (ClampMin = 0))
	float MedkitHeal = 50.f;
};
