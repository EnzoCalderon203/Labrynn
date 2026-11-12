#include "Core/LabPlayerState.h"

#include "Net/UnrealNetwork.h"

void ALabPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabPlayerState, Stats);
}
