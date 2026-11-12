#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabMedkitPickup.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;

/** Botiquín: cura heridas (MedkitHeal del Data Asset DA_Stamina). Solo lo agarra quien está herido. */
UCLASS(Blueprintable)
class LABRYNN_API ALabMedkitPickup : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabMedkitPickup();

	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

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
