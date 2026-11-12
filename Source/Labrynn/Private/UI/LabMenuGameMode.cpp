#include "UI/LabMenuGameMode.h"

#include "UI/LabMenuPlayerController.h"

ALabMenuGameMode::ALabMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ALabMenuPlayerController::StaticClass();
}
