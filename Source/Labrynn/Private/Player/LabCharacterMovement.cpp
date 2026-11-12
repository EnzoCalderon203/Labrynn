#include "Player/LabCharacterMovement.h"

#include "Player/LabCharacter.h"
#include "Player/LabStaminaSettings.h"
#include "Core/LabGameplaySettings.h"
#include "GameFramework/Character.h"

float ULabCharacterMovement::GetMaxSpeed() const
{
	if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking || MovementMode == MOVE_Falling)
	{
		if (const ALabCharacter* Ch = Cast<ALabCharacter>(CharacterOwner))
		{
			if (IsCrouching())
			{
				// Agachado: lento (y más lento con la reliquia).
				const float Crouch = Ch->GetGameplaySettings()->CrouchSpeed;
				return Ch->IsCarryingRelic() ? Crouch * Ch->GetStaminaSettings()->RelicSpeedMultiplier : Crouch;
			}
			return Ch->GetDesiredMaxSpeed(bWantsToSprint);
		}
	}
	return Super::GetMaxSpeed();
}

bool ULabCharacterMovement::IsSprintingNow() const
{
	const ALabCharacter* Ch = Cast<ALabCharacter>(CharacterOwner);
	return bWantsToSprint && Ch && Ch->CanSprint() && Velocity.SizeSquared2D() > FMath::Square(150.f);
}

void ULabCharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* ULabCharacterMovement::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		ULabCharacterMovement* MutableThis = const_cast<ULabCharacterMovement*>(this);
		MutableThis->ClientPredictionData = new FLabNetworkPredictionData_Client(*this);
	}
	return ClientPredictionData;
}

// ---------------------------------------------------------------- Saved move

void FLabSavedMove::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = false;
}

uint8 FLabSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	return Result;
}

bool FLabSavedMove::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	if (bSavedWantsToSprint != static_cast<FLabSavedMove*>(NewMove.Get())->bSavedWantsToSprint)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FLabSavedMove::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
	if (const ULabCharacterMovement* CMC = Cast<ULabCharacterMovement>(Character->GetCharacterMovement()))
	{
		bSavedWantsToSprint = CMC->bWantsToSprint;
	}
}

void FLabSavedMove::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);
	if (ULabCharacterMovement* CMC = Cast<ULabCharacterMovement>(Character->GetCharacterMovement()))
	{
		CMC->bWantsToSprint = bSavedWantsToSprint;
	}
}

FSavedMovePtr FLabNetworkPredictionData_Client::AllocateNewMove()
{
	return FSavedMovePtr(new FLabSavedMove());
}
