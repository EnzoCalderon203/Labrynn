#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabBatteryPickup.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;

/** Batería: recarga la linterna de quien la agarra (solo si tiene linterna). Brilla apenas para encontrarla. */
UCLASS(Blueprintable)
class LABRYNN_API ALabBatteryPickup : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabBatteryPickup();

	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

	/** Carga que suma (0..1). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Pickup", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Charge = 0.5f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Pickup")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Pickup|Audio")
	TObjectPtr<USoundBase> PickupSound;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPickedUp();
};
