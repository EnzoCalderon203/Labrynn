#pragma once

#include "CoreMinimal.h"
#include "Traps/LabTrapBase.h"
#include "LabSpikeTrap.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;

/**
 * Pinchos con placa de presión. Aviso: clic + la placa se hunde + destello rojo. Después de WarningTime
 * suben los pinchos y lanzan por el aire (ragdoll) a quien siga encima. Se rearma.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabSpikeTrap : public ALabTrapBase
{
	GENERATED_BODY()

public:
	ALabSpikeTrap();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnStateChanged(ELabTrapState OldState) override;
	virtual void ServerFire() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UStaticMeshComponent> Plate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UInstancedStaticMeshComponent> Spikes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UPointLightComponent> WarnLight;

	/** Fuerza vertical del lanzamiento (cm/s). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap")
	float LaunchUp = 750.f;

	/** Fuerza horizontal aleatoria del lanzamiento (cm/s). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap")
	float LaunchSide = 250.f;

private:
	float SpikeAlpha = 0.f;   // 0 = escondidos, 1 = arriba
	float SpikeTarget = 0.f;
	float PlateAlpha = 0.f;
	float FlashTime = 0.f;
};
