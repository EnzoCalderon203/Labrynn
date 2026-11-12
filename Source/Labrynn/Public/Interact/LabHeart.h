#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabHeart.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;

/**
 * Corazón del Laberinto (la "ayuda grande" del centro). Un solo uso para todo el grupo:
 * revive a todos los caídos y muertos y recarga todas las linternas al 100 %.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabHeart : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabHeart();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Heart")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Heart")
	TObjectPtr<UPointLightComponent> Pulse;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Heart|Audio")
	TObjectPtr<USoundBase> UseSound;

	UPROPERTY(ReplicatedUsing = OnRep_Used)
	bool bUsed = false;

	UFUNCTION()
	void OnRep_Used();

private:
	float PulseTime = 0.f;
};
