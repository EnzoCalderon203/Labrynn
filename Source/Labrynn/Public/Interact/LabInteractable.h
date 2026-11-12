#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LabInteractable.generated.h"

class ALabCharacter;

UINTERFACE(MinimalAPI, NotBlueprintable)
class ULabInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Algo con lo que se interactúa con la tecla E (reliquia, items, Corazón del Laberinto...).
 * El personaje busca el más cercano en su campo de visión. Interact() se llama SOLO en el servidor.
 */
class LABRYNN_API ILabInteractable
{
	GENERATED_BODY()

public:
	/** ¿Puede este personaje interactuar ahora? (se evalúa en cliente para el HUD y en servidor para validar) */
	virtual bool CanInteract(const ALabCharacter* By) const = 0;

	/** Solo servidor. */
	virtual void Interact(ALabCharacter* By) = 0;

	/** Texto para el HUD, ej. "Tomar la reliquia". */
	virtual FText GetInteractPrompt(const ALabCharacter* By) const = 0;
};
