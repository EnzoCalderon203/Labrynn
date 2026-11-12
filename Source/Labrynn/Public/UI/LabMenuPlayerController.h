#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LabMenuPlayerController.generated.h"

class SLabMainMenu;

/** PlayerController del menú: crea el widget Slate y pone el input en modo UI. */
UCLASS()
class LABRYNN_API ALabMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	TSharedPtr<SLabMainMenu> Menu;
};
