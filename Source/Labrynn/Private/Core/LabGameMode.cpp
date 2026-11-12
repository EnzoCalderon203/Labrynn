#include "Core/LabGameMode.h"

#include "Core/LabGameState.h"
#include "Core/LabPlayerController.h"
#include "Player/LabCharacter.h"
#include "UI/LabHUD.h"
#include "Interact/LabRelic.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Maze/LabMaze.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

ALabGameMode::ALabGameMode()
{
	GameStateClass = ALabGameState::StaticClass();
	PlayerControllerClass = ALabPlayerController::StaticClass();
	DefaultPawnClass = ALabCharacter::StaticClass();
	HUDClass = ALabHUD::StaticClass();
	RelicClass = ALabRelic::StaticClass();
	bUseSeamlessTravel = false;

	// Blueprints hijos con meshes/animaciones/input asignados (contenido de la plantilla First Person).
	static ConstructorHelpers::FClassFinder<APawn> PawnBP(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	if (PawnBP.Succeeded())
	{
		DefaultPawnClass = PawnBP.Class;
	}
	static ConstructorHelpers::FClassFinder<APlayerController> PCBP(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonPlayerController"));
	if (PCBP.Succeeded())
	{
		PlayerControllerClass = PCBP.Class;
	}
}

void ALabGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	SeedFromOptions = UGameplayStatics::GetIntOption(Options, TEXT("Seed"), 0);
}

void ALabGameMode::InitGameState()
{
	Super::InitGameState();

	int32 Seed = FixedSeed != 0 ? FixedSeed : SeedFromOptions;
	if (Seed == 0)
	{
		// Seed aleatoria real: no usar FMath::Rand (en PIE puede repetirse entre sesiones).
		Seed = static_cast<int32>(FPlatformTime::Cycles64() & 0x7FFFFFFF) ^ FMath::Rand();
	}

	if (ALabGameState* GS = GetGameState<ALabGameState>())
	{
		GS->SetMazeSeed(Seed);
		UE_LOG(LogLabrynn, Log, TEXT("Servidor: seed del laberinto = %d"), GS->GetMazeSeed());
	}
}

void ALabGameMode::StartPlay()
{
	Super::StartPlay();
	SpawnRelic();
	GetWorldTimerManager().SetTimer(CheckTimer, this, &ALabGameMode::CheckWinLose, 0.25f, true);

#if !UE_BUILD_SHIPPING
	// Prueba automática del core loop: el host "teletransporta" a la reliquia, la toma y la lleva a la salida.
	static bool bAutoRelicDone = false; // solo en la primera ronda del proceso
	if (!bAutoRelicDone && FParse::Param(FCommandLine::Get(), TEXT("LabAutoRelic")))
	{
		bAutoRelicDone = true;
		FTimerHandle T1, T2;
		GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			ALabCharacter* Ch = PC ? Cast<ALabCharacter>(PC->GetPawn()) : nullptr;
			if (Ch && Relic)
			{
				Ch->SetActorLocation(Relic->GetActorLocation() - FVector(100.f, 0.f, -60.f));
				Relic->Interact(Ch);
			}
		}), 20.f, false);
		GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			const ALabGameState* GS = GetGameState<ALabGameState>();
			ALabCharacter* Ch = Relic ? Relic->GetCarrier() : nullptr;
			if (Ch && GS && GS->GetMaze())
			{
				const FLabMazeLayout& L = GS->GetMaze()->GetLayout();
				Ch->SetActorLocation(GS->GetMaze()->CellToWorld(FIntPoint(L.Exit.X, L.Height)) + FVector(0.f, 0.f, 100.f));
			}
		}), 26.f, false);
	}

	// Prueba automática del rescate: tira al jugador 2 (cliente) y el host lo levanta.
	static bool bAutoDownDone = false;
	if (!bAutoDownDone && FParse::Param(FCommandLine::Get(), TEXT("LabAutoDown")))
	{
		bAutoDownDone = true;
		FTimerHandle T1, T2;
		GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
			{
				if (!It->IsLocallyControlled()) { It->KnockDown(FVector(0.f, 250.f, 350.f)); break; }
			}
		}), 25.f, false);
		GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			ALabCharacter* Host = PC ? Cast<ALabCharacter>(PC->GetPawn()) : nullptr;
			for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
			{
				if (*It != Host && Host && !It->IsAliveAndFree())
				{
					Host->SetActorLocation(It->GetActorLocation() + FVector(-120.f, 0.f, 0.f));
					It->Interact(Host);
				}
			}
		}), 31.f, false);
	}
#endif
}

void ALabGameMode::SpawnRelic()
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc = Maze->GetCenterRoomLocation() + FVector(0.f, 0.f, 40.f);
	Relic = GetWorld()->SpawnActor<ALabRelic>(RelicClass ? *RelicClass : ALabRelic::StaticClass(), FTransform(Loc), P);
	UE_LOG(LogLabrynn, Log, TEXT("Reliquia spawneada en %s"), *Loc.ToCompactString());
}

void ALabGameMode::CheckWinLose()
{
	ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!GS || !Maze || GS->GetPhase() == ELabPhase::Won || GS->GetPhase() == ELabPhase::Lost)
	{
		return;
	}

	// Derrota: no queda nadie en pie.
	int32 Players = 0, Standing = 0;
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		if (It->GetController())
		{
			++Players;
			Standing += It->IsAliveAndFree() ? 1 : 0;
		}
	}
	if (Players > 0 && Standing == 0)
	{
		GS->SetPhase(ELabPhase::Lost);
		GetWorldTimerManager().SetTimer(RestartTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			GetWorld()->ServerTravel(TEXT("?Restart"), false);
		}), RestartDelay, false);
		return;
	}

	if (!Relic || GS->GetPhase() != ELabPhase::Escape)
	{
		return;
	}

	// Victoria: la reliquia (en manos de alguien o tirada) llegó a la sala de salida.
	if (Maze->IsInExitHall(Relic->GetActorLocation()))
	{
		GS->SetPhase(ELabPhase::Won);
		GetWorldTimerManager().SetTimer(RestartTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			// Recarga el mapa (seed nueva); los clientes viajan con el host.
			GetWorld()->ServerTravel(TEXT("?Restart"), /*bAbsolute*/ false);
		}), RestartDelay, false);
	}
}

void ALabGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaxPlayers)
	{
		ErrorMessage = TEXT("La partida está llena.");
	}
}

void ALabGameMode::RestartPlayer(AController* NewPlayer)
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	const int32 Index = NextSpawnIndex++;
	if (NewPlayer && Maze && Maze->IsBuilt())
	{
		RestartPlayerAtTransform(NewPlayer, Maze->GetSpawnTransform(Index));
	}
	else
	{
		Super::RestartPlayer(NewPlayer);
	}

	// Luz compartida: solo 1 de cada FlashlightEveryN jugadores arranca con linterna (el 1º, el 3º...).
	if (ALabCharacter* Ch = NewPlayer ? Cast<ALabCharacter>(NewPlayer->GetPawn()) : nullptr)
	{
		bool bGive = (Index % FMath::Max(1, FlashlightEveryN)) == 0;
#if !UE_BUILD_SHIPPING
		bGive |= FParse::Param(FCommandLine::Get(), TEXT("LabAllFlashlights"));
#endif
		Ch->SetHasFlashlight(bGive);
		UE_LOG(LogLabrynn, Log, TEXT("Jugador %d (%s): linterna=%d"), Index, *GetNameSafe(Ch), bGive ? 1 : 0);
	}
}
