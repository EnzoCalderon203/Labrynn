#include "Enemy/LabMinotaurController.h"

#include "Enemy/LabMinotaur.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Hearing.h"

ALabMinotaurController::ALabMinotaurController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
	HearingConfig->HearingRange = 6000.f;
	// Oye a todos (los jugadores no tienen "equipo" para la IA).
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(1.f);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(UAISense_Hearing::StaticClass());
	Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ALabMinotaurController::OnPerceived);
}

void ALabMinotaurController::OnPerceived(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}
	if (ALabMinotaur* M = Cast<ALabMinotaur>(GetPawn()))
	{
		M->OnHeardNoise(Stimulus.StimulusLocation, Stimulus.Strength, Stimulus.Tag);
	}
}
