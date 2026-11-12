#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LabGameMode.generated.h"

/**
 * GameMode (solo existe en el servidor / host).
 * Elige la seed del laberinto y coloca a los jugadores en el hall de entrada.
 * Opción de URL para pruebas: ?Seed=1234 fija la seed.
 */
UCLASS()
class LABRYNN_API ALabGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALabGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Si es distinto de 0, se usa siempre esta seed (útil para depurar). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze")
	int32 FixedSeed = 0;

	/** Jugadores máximos por partida. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Game", meta = (ClampMin = 1, ClampMax = 4))
	int32 MaxPlayers = 4;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

private:
	int32 SeedFromOptions = 0;
	int32 NextSpawnIndex = 0;
};
