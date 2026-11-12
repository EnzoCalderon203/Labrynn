#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LabHUD.generated.h"

/**
 * HUD mínimo provisional (Canvas, sin assets): batería de la linterna y ayudas de teclas.
 * Luis lo reemplaza por un UMG; los datos salen de ALabCharacter (GetBattery, HasFlashlight).
 */
UCLASS()
class LABRYNN_API ALabHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
