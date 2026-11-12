#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "LabCameraManager.generated.h"

/** Camera manager de primera persona: limita el pitch. */
UCLASS()
class LABRYNN_API ALabCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	ALabCameraManager();
};
