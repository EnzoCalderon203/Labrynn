#pragma once

#include "CoreMinimal.h"
#include "LabMazeLayout.generated.h"

/** Parámetros de generación. Iguales en todas las máquinas (vienen del CDO de ALabMaze). */
USTRUCT(BlueprintType)
struct FLabMazeParams
{
	GENERATED_BODY()

	/** Ancho y alto de la grilla en celdas. Se fuerzan a impar para que la sala central quede centrada. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 7, ClampMax = 41))
	int32 Width = 15;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 7, ClampMax = 41))
	int32 Height = 15;

	/** Lado de la sala central en celdas (impar). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 1, ClampMax = 7))
	int32 CenterRoomSize = 3;

	/** Puertas que conectan la sala central con el laberinto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 1, ClampMax = 4))
	int32 CenterRoomDoors = 2;

	/** Probabilidad de abrir una pared interna extra después del backtracker (crea loops). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lab|Maze", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float LoopChance = 0.08f;
};

/**
 * Resultado de la generación: paredes como aristas de la grilla.
 * Celda (X,Y): X crece hacia +X del mundo, Y crece hacia +Y del mundo.
 * Es un tipo de datos puro y determinista: misma seed + mismos params = mismo layout.
 */
USTRUCT(BlueprintType)
struct LABRYNN_API FLabMazeLayout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lab|Maze")
	int32 Width = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lab|Maze")
	int32 Height = 0;

	/** Celda de entrada (borde Y=0). Su pared sur está abierta. */
	UPROPERTY(BlueprintReadOnly, Category = "Lab|Maze")
	FIntPoint Entrance = FIntPoint::ZeroValue;

	/** Celda de salida (borde Y=Height-1). Su pared norte se abre en la fase de escape. */
	UPROPERTY(BlueprintReadOnly, Category = "Lab|Maze")
	FIntPoint Exit = FIntPoint::ZeroValue;

	/** Esquina mínima de la sala central (inclusive). */
	UPROPERTY(BlueprintReadOnly, Category = "Lab|Maze")
	FIntPoint CenterMin = FIntPoint::ZeroValue;

	/** Esquina máxima de la sala central (inclusive). */
	UPROPERTY(BlueprintReadOnly, Category = "Lab|Maze")
	FIntPoint CenterMax = FIntPoint::ZeroValue;

	/** Paredes horizontales: (Width) x (Height+1). Índice H(x, y) = pared en el lado -Y de la celda (x, y). */
	TArray<bool> HWalls;

	/** Paredes verticales: (Width+1) x (Height). Índice V(x, y) = pared en el lado -X de la celda (x, y). */
	TArray<bool> VWalls;

	/** Genera el layout. Determinista. */
	static FLabMazeLayout Generate(int32 Seed, const FLabMazeParams& Params);

	bool HasHWall(int32 X, int32 Y) const { return HWalls[Y * Width + X]; }
	bool HasVWall(int32 X, int32 Y) const { return VWalls[Y * (Width + 1) + X]; }
	bool IsInCenterRoom(const FIntPoint& C) const
	{
		return C.X >= CenterMin.X && C.X <= CenterMax.X && C.Y >= CenterMin.Y && C.Y <= CenterMax.Y;
	}
	bool IsValidCell(const FIntPoint& C) const { return C.X >= 0 && C.Y >= 0 && C.X < Width && C.Y < Height; }

	/** Celdas sin salida (exactamente 3 paredes), fuera de la sala central. Útil para trampas e items. */
	TArray<FIntPoint> FindDeadEnds() const;

	/** Hash simple del layout, para verificar que cliente y servidor generaron lo mismo. */
	uint32 ComputeChecksum() const;

private:
	void SetHWall(int32 X, int32 Y, bool bWall) { HWalls[Y * Width + X] = bWall; }
	void SetVWall(int32 X, int32 Y, bool bWall) { VWalls[Y * (Width + 1) + X] = bWall; }
	/** Quita la pared entre dos celdas vecinas. */
	void Carve(const FIntPoint& A, const FIntPoint& B);
	int32 CountWalls(const FIntPoint& C) const;
};
