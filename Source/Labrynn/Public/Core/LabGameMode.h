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

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Relic")
	TSubclassOf<class ALabHeart> HeartClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Pickups")
	TSubclassOf<class ALabOilFlask> OilFlaskClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Pickups", meta = (ClampMin = 0, ClampMax = 20))
	int32 OilFlaskCount = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Pickups")
	TSubclassOf<class ALabMedkitPickup> MedkitClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Pickups", meta = (ClampMin = 0, ClampMax = 20))
	int32 MedkitCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Traps")
	TSubclassOf<class ALabSpikeTrap> SpikeTrapClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Traps")
	TSubclassOf<class ALabPitTrap> PitTrapClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Traps")
	TSubclassOf<class ALabGasTrap> GasTrapClass;

	/** Cantidad de trampas de gas (en pasillos rectos sin pinchos). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Traps", meta = (ClampMin = 0, ClampMax = 20))
	int32 GasTrapCount = 3;

	/** Cantidad de trampas de pinchos (en pasillos rectos). La de fosos está en ALabMaze::PitCount. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Traps", meta = (ClampMin = 0, ClampMax = 20))
	int32 SpikeTrapCount = 6;

	/** Segundos entre la victoria/derrota y el reinicio con un laberinto nuevo. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Game")
	float RestartDelay = 10.f;

	/** Si es distinto de 0, se usa siempre esta seed (útil para depurar). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze")
	int32 FixedSeed = 0;

	/** Jugadores máximos por partida. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Game", meta = (ClampMin = 1, ClampMax = 4))
	int32 MaxPlayers = 4;

	/** Cantidades del mundo (escondites, lanzables, obstáculos). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Gameplay")
	TObjectPtr<class ULabGameplaySettings> GameplaySettings;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Gameplay")
	TSubclassOf<class ALabHidingSpot> HidingSpotClass;

	/** Minotauro (aparece en la fase de escape). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	TSubclassOf<class ALabMinotaur> MinotaurClass;

	/** Farol (Blueprint hijo posible). En la mesa del hall hay max(1, jugadores - 1). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light")
	TSubclassOf<class ALabLantern> LanternClass;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

private:
	void CheckWinLose();
	void SpawnRelic();
	void SpawnTraps();
	void SpawnPickups();
	void EnsureLanterns();
	void SpawnMinotaur();
	void SpawnHidingSpots();

	UPROPERTY(Transient)
	TArray<TObjectPtr<class ALabHidingSpot>> HidingSpots;

	UPROPERTY(Transient)
	TObjectPtr<class ALabMinotaur> Minotaur;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class ALabLantern>> Lanterns;

	UPROPERTY(Transient)
	TObjectPtr<class ALabHeart> Heart;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class ALabTrapBase>> Traps;

	UPROPERTY(Transient)
	TObjectPtr<class ALabRelic> Relic;

	FTimerHandle CheckTimer;
	FTimerHandle RestartTimer;

	int32 SeedFromOptions = 0;
	int32 NextSpawnIndex = 0;
};
