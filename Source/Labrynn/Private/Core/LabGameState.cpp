#include "Core/LabGameState.h"

#include "Maze/LabMaze.h"
#include "Core/LabPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Labrynn.h"

ALabGameState::ALabGameState()
{
	MazeClass = ALabMaze::StaticClass();
}

void ALabGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabGameState, MazeSeed);
	DOREPLIFETIME(ALabGameState, Phase);
	DOREPLIFETIME(ALabGameState, PhaseStartTime);
	DOREPLIFETIME(ALabGameState, Awards);
	DOREPLIFETIME(ALabGameState, Pings);
}

void ALabGameState::SetPhase(ELabPhase NewPhase)
{
	check(HasAuthority());
	if (Phase == NewPhase)
	{
		return;
	}
	Phase = NewPhase;
	PhaseStartTime = GetServerWorldTimeSeconds();
	UE_LOG(LogLabrynn, Log, TEXT("Fase -> %s"), *UEnum::GetValueAsString(Phase));
	if (Phase == ELabPhase::Won || Phase == ELabPhase::Lost)
	{
		ComputeAwards();
	}
	ApplyPhase();
}

void ALabGameState::ComputeAwards()
{
	check(HasAuthority());
	Awards.Reset();
	TArray<const ALabPlayerState*> PSs;
	for (const APlayerState* P : PlayerArray)
	{
		if (const ALabPlayerState* L = Cast<ALabPlayerState>(P)) { PSs.Add(L); }
	}
	if (PSs.Num() == 0)
	{
		return;
	}
	TSet<const ALabPlayerState*> Awarded;
	// Para cada estadística, el que más tiene (si es > 0) se lleva el premio.
	auto Give = [&](const TCHAR* Title, TFunctionRef<float(const FLabStats&)> Get, TFunctionRef<FString(float)> Detail)
	{
		const ALabPlayerState* Best = nullptr;
		float BestV = 0.f;
		for (const ALabPlayerState* P : PSs)
		{
			const float V = Get(P->Stats);
			if (V > BestV) { BestV = V; Best = P; }
		}
		if (Best && Awards.Num() < 6)
		{
			Awards.Add({ Title, Best->GetPlayerName(), Detail(BestV) });
			Awarded.Add(Best);
		}
	};
	Give(TEXT("Alfombra del laberinto"), [](const FLabStats& S) { return float(S.Downs); }, [](float V) { return (int32(V) == 1 ? FString(TEXT("se cayó 1 vez")) : FString::Printf(TEXT("se cayó %d veces"), int32(V))); });
	Give(TEXT("Imán de trampas"), [](const FLabStats& S) { return float(S.TrapsTriggered); }, [](float V) { return FString::Printf(TEXT("pisó %d trampas"), int32(V)); });
	Give(TEXT("Colador humano"), [](const FLabStats& S) { return S.WoundsTaken; }, [](float V) { return FString::Printf(TEXT("%.0f de heridas"), V); });
	Give(TEXT("Ángel de la guarda"), [](const FLabStats& S) { return float(S.Revives); }, [](float V) { return FString::Printf(TEXT("levantó a %d compañeros"), int32(V)); });
	Give(TEXT("Mula de carga"), [](const FLabStats& S) { return S.CarryMeters; }, [](float V) { return FString::Printf(TEXT("cargó un cuerpo %.0f m"), V); });
	Give(TEXT("Megáfono"), [](const FLabStats& S) { return float(S.VoiceNoises); }, [](float V) { return FString::Printf(TEXT("su voz hizo ruido %d veces"), int32(V)); });
	Give(TEXT("Lanzador olímpico"), [](const FLabStats& S) { return float(S.Thrown); }, [](float V) { return FString::Printf(TEXT("tiró %d cosas"), int32(V)); });
	Give(TEXT("Rey del barril"), [](const FLabStats& S) { return S.HiddenSeconds; }, [](float V) { return FString::Printf(TEXT("%.0f s escondido"), V); });
	Give(TEXT("Señalador compulsivo"), [](const FLabStats& S) { return float(S.Pings); }, [](float V) { return FString::Printf(TEXT("%d ping%s"), int32(V), int32(V) == 1 ? TEXT("") : TEXT("s")); });
	// Nadie se queda sin nada.
	for (const ALabPlayerState* P : PSs)
	{
		if (!Awarded.Contains(P))
		{
			Awards.Add({ TEXT("Turista"), P->GetPlayerName(), TEXT("no le pasó absolutamente nada") });
		}
	}
	for (const FLabAward& A : Awards)
	{
		UE_LOG(LogLabrynn, Log, TEXT("Premio: %s → %s (%s)"), *A.Title, *A.Player, *A.Detail);
	}
}

void ALabGameState::AddPing(const FVector& Location, const FString& Player)
{
	check(HasAuthority());
	const float Now = GetServerWorldTimeSeconds();
	Pings.RemoveAll([Now](const FLabPing& P) { return Now - P.ServerTime > 10.f; });
	Pings.RemoveAll([&Player](const FLabPing& P) { return P.Player == Player; }); // uno por jugador
	FLabPing P;
	P.Location = Location;
	P.Player = Player;
	P.ServerTime = Now;
	Pings.Add(P);
	UE_LOG(LogLabrynn, Log, TEXT("Ping: %s marcó %s"), *Player, *Location.ToCompactString());
}

void ALabGameState::OnRelicTaken()
{
	if (Phase == ELabPhase::Explore)
	{
		SetPhase(ELabPhase::Escape);
	}
}

void ALabGameState::OnRep_Phase()
{
	ApplyPhase();
}

void ALabGameState::ApplyPhase()
{
	if (Maze)
	{
		Maze->SetExitOpen(Phase != ELabPhase::Explore);
	}
}

void ALabGameState::SetMazeSeed(int32 NewSeed)
{
	check(HasAuthority());
	if (NewSeed == 0)
	{
		NewSeed = 1; // 0 está reservado para "sin seed"
	}
	MazeSeed = NewSeed;
	BuildLocalMaze(); // en el servidor OnRep no se dispara solo
}

void ALabGameState::OnRep_MazeSeed()
{
	BuildLocalMaze();
}

void ALabGameState::BuildLocalMaze()
{
	if (MazeSeed == 0 || Maze != nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient; // nunca se guarda en el mapa

	Maze = World->SpawnActor<ALabMaze>(MazeClass ? *MazeClass : ALabMaze::StaticClass(), FTransform(MazeOrigin), Params);
	if (Maze)
	{
		Maze->Build(MazeSeed);
		ApplyPhase(); // la fase pudo haber llegado antes que el laberinto (ej. alguien entra en pleno escape)
	}
	else
	{
		UE_LOG(LogLabrynn, Error, TEXT("No se pudo spawnear el laberinto"));
	}
}

void ALabGameState::OnRep_Awards()
{
	UE_LOG(LogLabrynn, Log, TEXT("Premios recibidos: %d"), Awards.Num());
}