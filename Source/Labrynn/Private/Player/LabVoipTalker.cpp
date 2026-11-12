#include "Player/LabVoipTalker.h"

#include "Components/AudioComponent.h"

void ULabVoipTalker::BPOnTalkingBegin_Implementation(UAudioComponent* AudioComponent)
{
	Super::BPOnTalkingBegin_Implementation(AudioComponent);
	VoiceComponent = AudioComponent;
}

void ULabVoipTalker::BPOnTalkingEnd_Implementation()
{
	Super::BPOnTalkingEnd_Implementation();
	VoiceComponent.Reset();
}
