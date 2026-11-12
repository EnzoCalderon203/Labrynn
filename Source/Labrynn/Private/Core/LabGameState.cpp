#include "Core/LabGameState.h"

#include "Maze/LabMaze.h"
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
	ApplyPhase();
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
