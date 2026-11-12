#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "LabMinotaurController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Hearing;

/**
 * Controller del Minotauro: el "oído" (AI Perception: Hearing). Escucha todos los ReportNoiseEvent
 * del juego (voz, jadeo, sprint, trampas) y se los pasa a ALabMinotaur::OnHeardNoise.
 */
UCLASS()
class LABRYNN_API ALabMinotaurController : public AAIController
{
	GENERATED_BODY()

public:
	ALabMinotaurController();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Lab|Minotaur")
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UFUNCTION()
	void OnPerceived(AActor* Actor, FAIStimulus Stimulus);
};
