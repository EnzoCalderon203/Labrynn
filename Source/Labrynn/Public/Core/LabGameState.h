#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LabGameState.generated.h"

class ALabMaze;

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

protected:
	UFUNCTION()
	void OnRep_MazeSeed();

	/** 0 = todavía no se eligió. */
	UPROPERTY(ReplicatedUsing = OnRep_MazeSeed)
	int32 MazeSeed = 0;

private:
	void BuildLocalMaze();

	UPROPERTY(Transient)
	TObjectPtr<ALabMaze> Maze;
};
