#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LabCharacterMovement.generated.h"

/**
 * Movimiento con sprint PREDICHO: el pedido de sprint viaja dentro de los saved moves
 * (flag comprimido), así cliente y servidor calculan la misma velocidad sin rubber-banding.
 * La velocidad final sale del estado del personaje (jadeo, reliquia) vía ALabCharacter::GetDesiredMaxSpeed.
 */
UCLASS()
class LABRYNN_API ULabCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** Pedido de sprint (input del dueño). El servidor lo recibe en los saved moves. */
	bool bWantsToSprint = false;

	virtual float GetMaxSpeed() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/** ¿Está efectivamente corriendo (pide sprint, se mueve y no jadea)? */
	bool IsSprintingNow() const;
};

class FLabSavedMove : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	uint8 bSavedWantsToSprint : 1;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;
};

class FLabNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;
	explicit FLabNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement) {}
	virtual FSavedMovePtr AllocateNewMove() override;
};
