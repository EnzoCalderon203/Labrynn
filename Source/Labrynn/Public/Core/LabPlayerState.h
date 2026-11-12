#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LabPlayerState.generated.h"

/** Estadísticas de la ronda para los premios graciosos (solo las escribe el servidor). */
USTRUCT(BlueprintType)
struct FLabStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Downs = 0;          // veces que cayó
	UPROPERTY(BlueprintReadOnly) float WoundsTaken = 0.f;  // heridas acumuladas
	UPROPERTY(BlueprintReadOnly) int32 TrapsTriggered = 0; // trampas que pisó
	UPROPERTY(BlueprintReadOnly) int32 Revives = 0;        // compañeros que levantó
	UPROPERTY(BlueprintReadOnly) float CarryMeters = 0.f;  // metros cargando a un caído
	UPROPERTY(BlueprintReadOnly) int32 Thrown = 0;         // piedras/botellas lanzadas
	UPROPERTY(BlueprintReadOnly) int32 VoiceNoises = 0;    // veces que su voz hizo ruido
	UPROPERTY(BlueprintReadOnly) float HiddenSeconds = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 Pings = 0;
};

UCLASS()
class LABRYNN_API ALabPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lab|Stats")
	FLabStats Stats;
};
