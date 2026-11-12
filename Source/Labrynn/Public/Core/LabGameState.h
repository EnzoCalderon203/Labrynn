#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LabGameState.generated.h"

class ALabMaze;

/** Fase de la partida (replicada). */
UENUM(BlueprintType)
enum class ELabPhase : uint8
{
	Explore UMETA(DisplayName = "Exploración"),  // buscando la reliquia
	Escape  UMETA(DisplayName = "Escape"),       // reliquia tomada: la salida está abierta
	Won     UMETA(DisplayName = "Victoria"),
	Lost    UMETA(DisplayName = "Derrota"),
};

/** Premio gracioso del final de ronda. */
USTRUCT(BlueprintType)
struct FLabAward
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Title;
	UPROPERTY(BlueprintReadOnly) FString Player;
	UPROPERTY(BlueprintReadOnly) FString Detail;
};

/** Marca de ping (rueda del mouse). */
USTRUCT(BlueprintType)
struct FLabPing
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FVector_NetQuantize Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FString Player;
	UPROPERTY(BlueprintReadOnly) float ServerTime = 0.f;
};

/**
 * Estado global replicado de la partida.
 * La seed del laberinto vive acá: el servidor la fija una vez y cada cliente
 * construye su copia local del laberinto en OnRep_MazeSeed.
 */
UCLASS()
class LABRYNN_API ALabGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALabGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Solo servidor. Fija la seed y construye el laberinto del servidor. */
	void SetMazeSeed(int32 NewSeed);

	UFUNCTION(BlueprintPure, Category = "Lab|Maze")
	int32 GetMazeSeed() const { return MazeSeed; }

	/** Laberinto local de esta máquina (nullptr hasta que llegue la seed). */
	UFUNCTION(BlueprintPure, Category = "Lab|Maze")
	ALabMaze* GetMaze() const { return Maze; }

	/** Clase de laberinto a spawnear (se puede cambiar por un Blueprint hijo). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze")
	TSubclassOf<ALabMaze> MazeClass;

	/** Ubicación de origen del laberinto en el mundo. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze")
	FVector MazeOrigin = FVector::ZeroVector;

	UFUNCTION(BlueprintPure, Category = "Lab|Game")
	ELabPhase GetPhase() const { return Phase; }

	/** Solo servidor. */
	void SetPhase(ELabPhase NewPhase);

	/** Solo servidor. La primera vez que se toma la reliquia empieza la fase de escape. */
	void OnRelicTaken();

	/** Momento (tiempo del servidor) en que empezó la fase actual, para mensajes del HUD. */
	UFUNCTION(BlueprintPure, Category = "Lab|Game")
	float GetPhaseStartTime() const { return PhaseStartTime; }

	/** Solo servidor: calcula los premios a partir de las estadísticas de cada PlayerState. */
	void ComputeAwards();
	const TArray<FLabAward>& GetAwards() const { return Awards; }

	/** Solo servidor. */
	void AddPing(const FVector& Location, const FString& Player);
	const TArray<FLabPing>& GetPings() const { return Pings; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Awards)
	TArray<FLabAward> Awards;

	UFUNCTION()
	void OnRep_Awards();

	UPROPERTY(Replicated)
	TArray<FLabPing> Pings;

	UFUNCTION()
	void OnRep_MazeSeed();

	UFUNCTION()
	void OnRep_Phase();

	UPROPERTY(ReplicatedUsing = OnRep_Phase)
	ELabPhase Phase = ELabPhase::Explore;

	UPROPERTY(Replicated)
	float PhaseStartTime = 0.f;

	/** Aplica efectos de la fase en esta máquina (abrir salida, etc.). */
	void ApplyPhase();

	/** 0 = todavía no se eligió. */
	UPROPERTY(ReplicatedUsing = OnRep_MazeSeed)
	int32 MazeSeed = 0;

private:
	void BuildLocalMaze();

	UPROPERTY(Transient)
	TObjectPtr<ALabMaze> Maze;
};
