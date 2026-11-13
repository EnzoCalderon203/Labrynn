#pragma once

#include "CoreMinimal.h"
#include "Traps/LabTrapBase.h"
#include "LabGasTrap.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Gas que apaga la luz. Aviso: siseo + luz verde que pulsa. Después, durante FiringTime,
 * a quien esté en la nube se le apaga la linterna (y no puede prenderla) y ve todo verde.
 * No hace daño: es una trampa "social" (el grupo queda a oscuras de golpe).
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabGasTrap : public ALabTrapBase
{
	GENERATED_BODY()

public:
	virtual ELabSfx GetFireSfx() const override { return ELabSfx::GasFire; }

	ALabGasTrap();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnStateChanged(ELabTrapState OldState) override;
	virtual void ServerFire() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UStaticMeshComponent> Vent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UPointLightComponent> GasLight;

private:
	void ServerGasPulse();
	FTimerHandle PulseTimer;
	float Glow = 0.f;
	float GlowTarget = 0.f;
};
