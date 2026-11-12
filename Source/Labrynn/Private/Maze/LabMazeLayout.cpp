#include "Maze/LabMazeLayout.h"

namespace
{
	const FIntPoint GDirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
}

void FLabMazeLayout::Carve(const FIntPoint& A, const FIntPoint& B)
{
	const FIntPoint D = B - A;
	if (D.X == 1)       { SetVWall(B.X, B.Y, false); }
	else if (D.X == -1) { SetVWall(A.X, A.Y, false); }
	else if (D.Y == 1)  { SetHWall(B.X, B.Y, false); }
	else if (D.Y == -1) { SetHWall(A.X, A.Y, false); }
}

int32 FLabMazeLayout::CountWalls(const FIntPoint& C) const
{
	return (HasHWall(C.X, C.Y) ? 1 : 0) + (HasHWall(C.X, C.Y + 1) ? 1 : 0)
		 + (HasVWall(C.X, C.Y) ? 1 : 0) + (HasVWall(C.X + 1, C.Y) ? 1 : 0);
}

FLabMazeLayout FLabMazeLayout::Generate(int32 Seed, const FLabMazeParams& Params)
{
	FLabMazeLayout L;
	FRandomStream Rng(Seed);

	// Dimensiones impares para que la sala quede centrada.
	L.Width = Params.Width | 1;
	L.Height = Params.Height | 1;
	const int32 Room = FMath::Min(Params.CenterRoomSize | 1, FMath::Min(L.Width, L.Height) - 4);

	L.HWalls.Init(true, L.Width * (L.Height + 1));
	L.VWalls.Init(true, (L.Width + 1) * L.Height);

	L.CenterMin = FIntPoint((L.Width - Room) / 2, (L.Height - Room) / 2);
	L.CenterMax = L.CenterMin + FIntPoint(Room - 1, Room - 1);

	// 1) Sala central: abierta por dentro.
	for (int32 Y = L.CenterMin.Y; Y <= L.CenterMax.Y; ++Y)
	{
		for (int32 X = L.CenterMin.X; X <= L.CenterMax.X; ++X)
		{
			if (X < L.CenterMax.X) { L.Carve(FIntPoint(X, Y), FIntPoint(X + 1, Y)); }
			if (Y < L.CenterMax.Y) { L.Carve(FIntPoint(X, Y), FIntPoint(X, Y + 1)); }
		}
	}

	// 2) Entrada (borde sur) y salida (borde norte).
	L.Entrance = FIntPoint(Rng.RandRange(1, L.Width - 2), 0);
	L.Exit = FIntPoint(Rng.RandRange(1, L.Width - 2), L.Height - 1);
	L.SetHWall(L.Entrance.X, 0, false); // abertura exterior de la entrada

	// 3) Recursive backtracker iterativo sobre todas las celdas fuera de la sala.
	TArray<bool> Visited;
	Visited.Init(false, L.Width * L.Height);
	auto Idx = [&L](const FIntPoint& C) { return C.Y * L.Width + C.X; };
	for (int32 Y = L.CenterMin.Y; Y <= L.CenterMax.Y; ++Y)
	{
		for (int32 X = L.CenterMin.X; X <= L.CenterMax.X; ++X)
		{
			Visited[Idx(FIntPoint(X, Y))] = true;
		}
	}

	TArray<FIntPoint> Stack;
	Stack.Push(L.Entrance);
	Visited[Idx(L.Entrance)] = true;

	while (Stack.Num() > 0)
	{
		const FIntPoint Cur = Stack.Last();
		FIntPoint Options[4];
		int32 NumOptions = 0;
		for (const FIntPoint& D : GDirs)
		{
			const FIntPoint N = Cur + D;
			if (L.IsValidCell(N) && !Visited[Idx(N)])
			{
				Options[NumOptions++] = N;
			}
		}

		if (NumOptions == 0)
		{
			Stack.Pop(EAllowShrinking::No);
			continue;
		}

		const FIntPoint Next = Options[Rng.RandRange(0, NumOptions - 1)];
		L.Carve(Cur, Next);
		Visited[Idx(Next)] = true;
		Stack.Push(Next);
	}

	// 4) Puertas de la sala central: elegir celdas del perímetro exterior, en lados distintos si se puede.
	{
		TArray<TPair<FIntPoint, FIntPoint>> Candidates[4]; // por lado: (celda sala, celda afuera)
		for (int32 X = L.CenterMin.X; X <= L.CenterMax.X; ++X)
		{
			Candidates[0].Emplace(FIntPoint(X, L.CenterMin.Y), FIntPoint(X, L.CenterMin.Y - 1)); // sur
			Candidates[1].Emplace(FIntPoint(X, L.CenterMax.Y), FIntPoint(X, L.CenterMax.Y + 1)); // norte
		}
		for (int32 Y = L.CenterMin.Y; Y <= L.CenterMax.Y; ++Y)
		{
			Candidates[2].Emplace(FIntPoint(L.CenterMin.X, Y), FIntPoint(L.CenterMin.X - 1, Y)); // oeste
			Candidates[3].Emplace(FIntPoint(L.CenterMax.X, Y), FIntPoint(L.CenterMax.X + 1, Y)); // este
		}

		// Orden aleatorio de lados.
		int32 Sides[4] = { 0, 1, 2, 3 };
		for (int32 i = 3; i > 0; --i)
		{
			Swap(Sides[i], Sides[Rng.RandRange(0, i)]);
		}

		const int32 Doors = FMath::Clamp(Params.CenterRoomDoors, 1, 4);
		for (int32 d = 0; d < Doors; ++d)
		{
			const TArray<TPair<FIntPoint, FIntPoint>>& Side = Candidates[Sides[d]];
			const TPair<FIntPoint, FIntPoint>& Pick = Side[Rng.RandRange(0, Side.Num() - 1)];
			L.Carve(Pick.Key, Pick.Value);
		}
	}

	// 5) Loops: abrir algunas paredes internas extra (nunca hacia la sala central ni bordes exteriores).
	if (Params.LoopChance > 0.f)
	{
		for (int32 Y = 0; Y < L.Height; ++Y)
		{
			for (int32 X = 0; X < L.Width; ++X)
			{
				const FIntPoint C(X, Y);
				if (L.IsInCenterRoom(C)) { continue; }

				const FIntPoint East(X + 1, Y);
				if (X + 1 < L.Width && !L.IsInCenterRoom(East) && L.HasVWall(X + 1, Y) && Rng.FRand() < Params.LoopChance)
				{
					L.Carve(C, East);
				}
				const FIntPoint North(X, Y + 1);
				if (Y + 1 < L.Height && !L.IsInCenterRoom(North) && L.HasHWall(X, Y + 1) && Rng.FRand() < Params.LoopChance)
				{
					L.Carve(C, North);
				}
			}
		}
	}

	return L;
}

TArray<FIntPoint> FLabMazeLayout::FindDeadEnds() const
{
	TArray<FIntPoint> Out;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FIntPoint C(X, Y);
			if (!IsInCenterRoom(C) && C != Entrance && CountWalls(C) == 3)
			{
				Out.Add(C);
			}
		}
	}
	return Out;
}

bool FLabMazeLayout::HasWallBetween(const FIntPoint& C, const FIntPoint& Dir) const
{
	if (Dir.X == 1)  { return HasVWall(C.X + 1, C.Y); }
	if (Dir.X == -1) { return HasVWall(C.X, C.Y); }
	if (Dir.Y == 1)  { return HasHWall(C.X, C.Y + 1); }
	return HasHWall(C.X, C.Y);
}

TArray<FIntPoint> FLabMazeLayout::FindStraightCorridors() const
{
	TArray<FIntPoint> Out;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FIntPoint C(X, Y);
			if (IsInCenterRoom(C) || C == Entrance || C == Exit)
			{
				continue;
			}
			const bool bN = HasHWall(X, Y + 1), bS = HasHWall(X, Y), bE = HasVWall(X + 1, Y), bW = HasVWall(X, Y);
			if ((bN && bS && !bE && !bW) || (bE && bW && !bN && !bS))
			{
				Out.Add(C);
			}
		}
	}
	return Out;
}

uint32 FLabMazeLayout::ComputeChecksum() const
{
	uint32 Hash = HashCombine(GetTypeHash(Width), GetTypeHash(Height));
	Hash = HashCombine(Hash, GetTypeHash(Entrance));
	Hash = HashCombine(Hash, GetTypeHash(Exit));
	for (int32 i = 0; i < HWalls.Num(); ++i) { Hash = HashCombine(Hash, HWalls[i] ? 0x9E3779B9u + i : i); }
	for (int32 i = 0; i < VWalls.Num(); ++i) { Hash = HashCombine(Hash, VWalls[i] ? 0x85EBCA6Bu + i : i); }
	return Hash;
}
