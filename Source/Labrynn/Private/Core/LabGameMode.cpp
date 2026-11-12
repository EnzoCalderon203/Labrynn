#include "Core/LabGameMode.h"

#include "Core/LabGameState.h"
#include "Core/LabPlayerController.h"
#include "Player/LabCharacter.h"
#include "UI/LabHUD.h"
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
