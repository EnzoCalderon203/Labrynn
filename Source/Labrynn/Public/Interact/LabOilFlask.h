#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabOilFlask.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;

/** Frasco de aceite: recarga el farol de quien lo agarra (solo si lleva farol). Brilla apenas para encontrarlo. */
UCLASS(Blueprintable)
class LABRYNN_API ALabOilFlask : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabOilFlask();

	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

	/** Aceite que suma (0..1). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Pickup", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Oil = 0.5f;

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
