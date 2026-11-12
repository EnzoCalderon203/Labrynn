#pragma once

#include "CoreMinimal.h"
#include "Net/VoiceConfig.h"
#include "LabVoipTalker.generated.h"

class UAudioComponent;

/**
 * Talker de voz que guarda el UAudioComponent de la voz cuando el otro empieza a hablar,
 * para que ALabCharacter procese la voz según su estado (heridas, jadeo, caído, escondido, cargado).
 */
UCLASS()
class LABRYNN_API ULabVoipTalker : public UVOIPTalker
{
	GENERATED_BODY()

public:
	virtual void BPOnTalkingBegin_Implementation(UAudioComponent* AudioComponent) override;
	virtual void BPOnTalkingEnd_Implementation() override;

	UAudioComponent* GetVoiceComponent() const { return VoiceComponent.Get(); }

private:
	TWeakObjectPtr<UAudioComponent> VoiceComponent;
};
