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
	virtual void StartPlay() override;

	/** Reliquia a spawnear en la sala central (puede ser un Blueprint hijo con otro mesh). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Relic")
	TSubclassOf<class ALabRelic> RelicClass;

	/** Segundos entre la victoria/derrota y el reinicio con un laberinto nuevo. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Game")
	float RestartDelay = 10.f;

	/** Si es distinto de 0, se usa siempre esta seed (útil para depurar). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze")
	int32 FixedSeed = 0;

	/** Jugadores máximos por partida. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Game", meta = (ClampMin = 1, ClampMax = 4))
	int32 MaxPlayers = 4;

	/** 1 de cada N jugadores empieza con linterna (2 = la mitad del grupo). Debug: -LabAllFlashlights. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light", meta = (ClampMin = 1, ClampMax = 4))
	int32 FlashlightEveryN = 2;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

private:
	void CheckWinLose();
	void SpawnRelic();

	UPROPERTY(Transient)
	TObjectPtr<class ALabRelic> Relic;

	FTimerHandle CheckTimer;
	FTimerHandle RestartTimer;

	int32 SeedFromOptions = 0;
	int32 NextSpawnIndex = 0;
};
