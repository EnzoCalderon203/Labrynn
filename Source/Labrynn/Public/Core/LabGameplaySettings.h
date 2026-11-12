#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LabGameplaySettings.generated.h"

/** Cantidades y reglas del mundo (ajustables sin compilar en /Game/Labrynn/Core/DA_Gameplay). */
UCLASS(BlueprintType)
class LABRYNN_API ULabGameplaySettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Escondites ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escondites", meta = (ClampMin = 0, ClampMax = 30))
	int32 HidingSpotCount = 8;

	/** Multiplicador del ruido de la voz estando escondido. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escondites", meta = (ClampMin = 0, ClampMax = 1))
	float HiddenVoiceNoiseMultiplier = 0.35f;

	// --- Objetos para lanzar ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lanzables", meta = (ClampMin = 0, ClampMax = 40))
	int32 ThrowableCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lanzables", meta = (ClampMin = 1, ClampMax = 10))
	int32 MaxThrowablesCarried = 3;

	// --- Obstáculos ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstáculos", meta = (ClampMin = 0, ClampMax = 40))
	int32 RubbleCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstáculos", meta = (ClampMin = 0, ClampMax = 40))
	int32 LowTunnelCount = 5;

	/** Altura de escombros y vigas (cm). Tiene que ser menor que el salto (se valida al generar). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstáculos", meta = (ClampMin = 30))
	float RubbleHeight = 70.f;

	/** Altura libre de los túneles bajos (cm). Mayor que agachado y menor que parado (se valida). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstáculos", meta = (ClampMin = 60))
	float TunnelClearance = 125.f;

	/** Media altura de la cápsula agachado (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstáculos", meta = (ClampMin = 30))
	float CrouchedHalfHeight = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstáculos", meta = (ClampMin = 50))
	float CrouchSpeed = 220.f;

	// --- Bordes: pie de banco y dar la mano ---
	/** Altura de los bordes (cm): más que salto + paso (~135), así solo se suben con ayuda. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ayuda", meta = (ClampMin = 140))
	float LedgeHeight = 150.f;

	/** Impulso del pie de banco (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ayuda")
	float BoostLaunchUp = 620.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ayuda")
	float BoostLaunchForward = 300.f; // durante el impulso no hay frenado en el aire

	/** Distancia máxima de la mano (cm) para agarrarse (en la práctica, hay que saltar). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ayuda")
	float HandReach = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ayuda", meta = (ClampMin = 0.2))
	float PullSeconds = 1.5f;

	/** Stamina por segundo que gasta el que tira. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ayuda")
	float PullStaminaPerSecond = 30.f;

	/** Multiplicador si el de abajo carga la reliquia. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ayuda")
	float PullRelicMultiplier = 1.6f;

	// --- Cargar a un caído ---
	/** Segundos que hay que mantener E sobre un caído para cargarlo (tocar E = levantarlo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cargar", meta = (ClampMin = 0.2))
	float CarryHoldSeconds = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cargar", meta = (ClampMin = 0.1, ClampMax = 1))
	float CarrySpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cargar")
	float CarryStaminaPerSecond = 6.f;

	/** Bordes altos (pie de banco / dar la mano) en el camino entrada → centro → salida. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstáculos", meta = (ClampMin = 0, ClampMax = 10))
	int32 LedgesOnMainPath = 2;
};
