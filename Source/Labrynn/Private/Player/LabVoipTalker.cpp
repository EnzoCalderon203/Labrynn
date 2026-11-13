#include "Player/LabVoipTalker.h"

#include "Components/AudioComponent.h"
#include "Audio/LabAudio.h"
#include "Sound/SoundClass.h"

void ULabVoipTalker::BPOnTalkingBegin_Implementation(UAudioComponent* AudioComponent)
{
	Super::BPOnTalkingBegin_Implementation(AudioComponent);
	VoiceComponent = AudioComponent;
	const ULabAudioSettings* S = ULabAudio::Settings();
	if (AudioComponent && S)
	{
		if (const TObjectPtr<USoundClass>* C = S->Classes.Find(ELabAudioCategory::Voice))
		{
			AudioComponent->SoundClassOverride = *C;
		}
	}
}

void ULabVoipTalker::BPOnTalkingEnd_Implementation()
{
	Super::BPOnTalkingEnd_Implementation();
	VoiceComponent.Reset();
}
