#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LabMenuGameMode.generated.h"

/** GameMode del mapa de menú: sin pawn, solo muestra el menú. */
UCLASS()
class LABRYNN_API ALabMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALabMenuGameMode();
};
