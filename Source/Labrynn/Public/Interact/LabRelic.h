#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabRelic.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USphereComponent;
class ALabCharacter;

/**
 * La reliquia: está en la sala central. Pesada: quien la carga va lento y no puede usar la linterna.
 * Brilla un poco (ilumina alrededor del que la lleva). Se puede pasar a un compañero o soltar (tecla E).
 * Replicada: el servidor decide quién la carga; los clientes la adjuntan en OnRep_Carrier.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabRelic : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabRelic();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ILabInteractable
	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

	/** Solo servidor. NewCarrier = nullptr la deja en el piso frente al portador anterior. */
	void SetCarrier(ALabCharacter* NewCarrier);

	UFUNCTION(BlueprintPure, Category = "Lab|Relic")
	ALabCharacter* GetCarrier() const { return Carrier; }

	/** Obsoleto: la lentitud y el gasto de stamina por cargarla están en el Data Asset DA_Stamina. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Relic", meta = (ClampMin = 0.1, ClampMax = 1.0))
	float CarrySpeedMultiplier = 0.55f;

	/** Dónde se sostiene, relativo a la cápsula del portador (adelante, a la altura del pecho). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Relic")
	FVector CarryOffset = FVector(45.f, 0.f, 10.f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Relic")
	TObjectPtr<USphereComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Relic")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Relic")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(ReplicatedUsing = OnRep_Carrier)
	TObjectPtr<ALabCharacter> Carrier;

	UFUNCTION()
	void OnRep_Carrier(ALabCharacter* OldCarrier);

	bool bTakeSoundPlayed = false;

private:
	/** Aplica adjuntado y efectos sobre portador viejo/nuevo. Corre en todas las máquinas. */
	void ApplyCarrier(ALabCharacter* OldCarrier);
};
