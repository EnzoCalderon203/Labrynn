#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LabHUD.generated.h"

/**
 * HUD mínimo provisional (Canvas, sin assets): aceite del farol y ayudas de teclas.
 * Luis lo reemplaza por un UMG; los datos salen de ALabCharacter (GetHeldLantern, GetStamina01, GetWounds01).
 */
UCLASS()
class LABRYNN_API ALabHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
