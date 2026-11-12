#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Maze/LabMazeLayout.h"
#include "LabMaze.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * Construye el laberinto en el mundo a partir de una seed.
 * NO se replica: cada máquina lo spawnea localmente (ver ALabGameState) con la misma seed,
 * así que todas obtienen la misma geometría. Solo el estado dinámico (puertas, trampas) se replica aparte.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabMaze : public AActor
{
	GENERATED_BODY()

public:
	ALabMaze();

	/** Genera el layout y crea las instancias de paredes, piso y techo. Idempotente. */
	void Build(int32 InSeed);

	const FLabMazeLayout& GetLayout() const { return Layout; }
	int32 GetSeed() const { return Seed; }
	bool IsBuilt() const { return bBuilt; }

	/** Centro de una celda en coordenadas de mundo, a nivel del piso. */
	UFUNCTION(BlueprintPure, Category = "Lab|Maze")
	FVector CellToWorld(FIntPoint Cell) const;

	/** Celda que contiene un punto del mundo (puede quedar fuera de la grilla). */
	UFUNCTION(BlueprintPure, Category = "Lab|Maze")
	FIntPoint WorldToCell(const FVector& World) const;

	/** Transform de spawn número Index en el hall de entrada (mirando hacia el laberinto). */
	FTransform GetSpawnTransform(int32 Index) const;

	/** Abre o cierra la pared exterior de la salida (fase de escape). Llamar en todas las máquinas. */
	void SetExitOpen(bool bOpen);

	/** Centro de la sala central (para reliquia y ayuda). */
	UFUNCTION(BlueprintPure, Category = "Lab|Maze")
	FVector GetCenterRoomLocation() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lab|Maze")
	FLabMazeParams Params;

	/** Tamaño de cada celda en cm. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 200))
	float CellSize = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 200))
	float WallHeight = 350.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 10))
	float WallThickness = 40.f;

	/** Profundidad del hall de entrada (fuera de la grilla) en celdas. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 1))
	int32 EntranceHallDepth = 2;

	/** Ancho del hall de entrada en celdas (impar). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 1))
	int32 EntranceHallWidth = 3;

	/** Mesh base: un cubo de 100x100x100 con pivote centrado (por defecto, /Engine/BasicShapes/Cube). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze|Art")
	TObjectPtr<UStaticMesh> BlockMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze|Art")
	TObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze|Art")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze|Art")
	TObjectPtr<UMaterialInterface> CeilingMaterial;

	/** Exposición manual (EV). Más alto = más claro. Ajustar junto con la intensidad de la linterna. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze|Art")
	float ExposureBias = 4.f;

	/** Luz cálida y tenue del hall de entrada. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze|Art")
	float EntranceLightIntensity = 1500.f;

	/** Luz azulada de la sala central (la reliquia "brilla"). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Maze|Art")
	float CenterLightIntensity = 3000.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Lab|Maze")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Lab|Maze")
	TObjectPtr<UInstancedStaticMeshComponent> Walls;

	UPROPERTY(VisibleAnywhere, Category = "Lab|Maze")
	TObjectPtr<UInstancedStaticMeshComponent> Floors;

	UPROPERTY(VisibleAnywhere, Category = "Lab|Maze")
	TObjectPtr<UInstancedStaticMeshComponent> Ceilings;

	UPROPERTY(VisibleAnywhere, Category = "Lab|Maze")
	TObjectPtr<class UPostProcessComponent> PostProcess;

	/** Pared de la salida, separada para poder quitarla. */
	UPROPERTY(VisibleAnywhere, Category = "Lab|Maze")
	TObjectPtr<UInstancedStaticMeshComponent> ExitDoor;

private:
	void AddBox(UInstancedStaticMeshComponent* Comp, const FVector& Center, const FVector& SizeCm);
	void AddWallH(int32 X, int32 Y);
	void AddWallV(int32 X, int32 Y);
	void BuildEntranceHall();
	void AddGuideLight(const FVector& LocalPos, const FLinearColor& Color, float Intensity);

	FLabMazeLayout Layout;
	int32 Seed = 0;
	bool bBuilt = false;
};
