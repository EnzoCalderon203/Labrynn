#include "Core/LabGameMode.h"

#include "Core/LabGameState.h"
#include "Core/LabPlayerController.h"
#include "Player/LabCharacter.h"
#include "UI/LabHUD.h"
#include "Interact/LabRelic.h"
#include "Interact/LabHeart.h"
#include "Interact/LabOilFlask.h"
#include "Interact/LabLantern.h"
#include "Interact/LabMedkitPickup.h"
#include "Player/LabCharacterMovement.h"
#include "Traps/LabSpikeTrap.h"
#include "Traps/LabPitTrap.h"
#include "Traps/LabGasTrap.h"
#include "Enemy/LabMinotaur.h"
#include "Interact/LabHidingSpot.h"
#include "Interact/LabThrowable.h"
#include "Core/LabPlayerState.h"
#include "Core/LabGameplaySettings.h"
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
	PlayerStateClass = ALabPlayerState::StaticClass();
	RelicClass = ALabRelic::StaticClass();
	static ConstructorHelpers::FObjectFinder<ULabGameplaySettings> GameplayDA(TEXT("/Game/Labrynn/Core/DA_Gameplay.DA_Gameplay"));
	GameplaySettings = GameplayDA.Object;
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
	SpawnTraps();
	SpawnPickups();
	SpawnHidingSpots();
	SpawnThrowables();
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

	// Prueba automática de trampas: el cliente cae a un foso, el host lo saca desde el borde,
	// y después el host pisa unos pinchos.
	static bool bAutoTrapDone = false;
	if (!bAutoTrapDone && FParse::Param(FCommandLine::Get(), TEXT("LabAutoTrap")))
	{
		bAutoTrapDone = true;
		auto FindPawns = [this](ALabCharacter*& Host, ALabCharacter*& Other)
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			Host = PC ? Cast<ALabCharacter>(PC->GetPawn()) : nullptr;
			Other = nullptr;
			for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
			{
				if (*It != Host) { Other = *It; }
			}
		};
		FTimerHandle T1, T2, T3;
		GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [this, FindPawns]()
		{
			ALabCharacter *Host, *Other; FindPawns(Host, Other);
			const ALabMaze* Maze = GetGameState<ALabGameState>()->GetMaze();
			if (Other && Maze->GetPitCells().Num() > 0)
			{
				Other->SetActorLocation(Maze->CellToWorld(Maze->GetPitCells()[0]) + FVector(0.f, 0.f, 120.f), false, nullptr, ETeleportType::TeleportPhysics);
			}
		}), 18.f, false);
		GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this, FindPawns]()
		{
			ALabCharacter *Host, *Other; FindPawns(Host, Other);
			const ALabMaze* Maze = GetGameState<ALabGameState>()->GetMaze();
			if (Host && Other && Maze->GetPitCells().Num() > 0)
			{
				// Al borde: la celda vecina abierta del callejón.
				const FIntPoint Pit = Maze->GetPitCells()[0];
				static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
				for (const FIntPoint& D : Dirs)
				{
					if (!Maze->GetLayout().HasWallBetween(Pit, D))
					{
						Host->SetActorLocation(Maze->CellToWorld(Pit + D) + FVector(0.f, 0.f, 110.f));
						break;
					}
				}
				Other->Interact(Host);
			}
		}), 23.f, false);
		GetWorldTimerManager().SetTimer(T3, FTimerDelegate::CreateWeakLambda(this, [this, FindPawns]()
		{
			ALabCharacter *Host, *Other; FindPawns(Host, Other);
			for (ALabTrapBase* Trap : Traps)
			{
				if (Host && Trap && Trap->IsA<ALabSpikeTrap>())
				{
					Host->SetActorLocation(Trap->GetActorLocation() + FVector(0.f, 0.f, 110.f), false, nullptr, ETeleportType::TeleportPhysics);
					break;
				}
			}
		}), 30.f, false);
	}

	// Preparación de pruebas de input real: solo ACOMODA al host frente a la reliquia (la E la aprieta -LabKeys).
	// -LabTeleportToRelic=N acomoda al host; -LabTeleportClientToRelic=N acomoda al primer cliente.
	for (const bool bClient : { false, true })
	{
		float TeleportAt = 0.f;
		if (!FParse::Value(FCommandLine::Get(), bClient ? TEXT("LabTeleportClientToRelic=") : TEXT("LabTeleportToRelic="), TeleportAt))
		{
			continue;
		}
		FTimerHandle TT;
		GetWorldTimerManager().SetTimer(TT, FTimerDelegate::CreateWeakLambda(this, [this, bClient]()
		{
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (!PC || !PC->GetPawn() || !Relic || PC->IsLocalController() == bClient)
				{
					continue;
				}
				const FVector Pos = Relic->GetActorLocation() + FVector(-130.f, 0.f, 60.f);
				PC->GetPawn()->SetActorLocation(Pos, false, nullptr, ETeleportType::TeleportPhysics);
				PC->ClientSetRotation(FRotator(-20.f, 0.f, 0.f)); // mirando a la reliquia (+X); llega también al cliente
				break;
			}
		}), TeleportAt, false);
	}

	// -LabTeleportToTable=N / -LabTeleportClientToTable=N: acomoda al host / cliente frente a la mesa de faroles.
	for (const bool bClient : { false, true })
	{
		float At = 0.f;
		if (!FParse::Value(FCommandLine::Get(), bClient ? TEXT("LabTeleportClientToTable=") : TEXT("LabTeleportToTable="), At))
		{
			continue;
		}
		FTimerHandle TT;
		GetWorldTimerManager().SetTimer(TT, FTimerDelegate::CreateWeakLambda(this, [this, bClient]()
		{
			const ALabMaze* Maze = GetGameState<ALabGameState>()->GetMaze();
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (!PC || !PC->GetPawn() || !Maze || PC->IsLocalController() == bClient)
				{
					continue;
				}
				const FVector Slot = Maze->GetLanternTableSlot(0).GetLocation();
				PC->GetPawn()->SetActorLocation(Slot + FVector(0.f, 130.f, 15.f), false, nullptr, ETeleportType::TeleportPhysics);
				PC->ClientSetRotation(FRotator(-35.f, -90.f, 0.f)); // mirando la mesa (-Y)
				break;
			}
		}), At, false);
	}

	// -LabTeleportToHide=N / -LabTeleportClientToHide=N: acomoda al host / cliente frente al primer escondite.
	for (const bool bClient : { false, true })
	{
		float At = 0.f;
		if (!FParse::Value(FCommandLine::Get(), bClient ? TEXT("LabTeleportClientToHide=") : TEXT("LabTeleportToHide="), At))
		{
			continue;
		}
		FTimerHandle TT;
		GetWorldTimerManager().SetTimer(TT, FTimerDelegate::CreateWeakLambda(this, [this, bClient]()
		{
			if (HidingSpots.Num() == 0) { return; }
			const ALabHidingSpot* Spot = HidingSpots[0];
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (!PC || !PC->GetPawn() || PC->IsLocalController() == bClient)
				{
					continue;
				}
				// El host queda un poco más atrás que el cliente (para no tapar el escondite).
				const FVector Pos = Spot->GetExitLocation() + Spot->GetActorForwardVector() * (bClient ? 0.f : 120.f);
				PC->GetPawn()->SetActorLocation(Pos, false, nullptr, ETeleportType::TeleportPhysics);
				PC->ClientSetRotation(FRotator(-25.f, (-Spot->GetActorForwardVector()).Rotation().Yaw, 0.f));
				break;
			}
		}), At, false);
	}

	// -LabTeleportToObstacle=N -LabObstacleType=rubble|tunnel: el host queda en la celda anterior al obstáculo, mirándolo.
	{
		float At = 0.f;
		FString Type = TEXT("rubble");
		FParse::Value(FCommandLine::Get(), TEXT("LabObstacleType="), Type);
		if (FParse::Value(FCommandLine::Get(), TEXT("LabTeleportToObstacle="), At))
		{
			FTimerHandle TT;
			GetWorldTimerManager().SetTimer(TT, FTimerDelegate::CreateWeakLambda(this, [this, Type]()
			{
				const ALabMaze* Maze = GetGameState<ALabGameState>()->GetMaze();
				// -LabObstacleClient: acomoda al primer cliente en vez del host.
				const bool bForClient = FParse::Param(FCommandLine::Get(), TEXT("LabObstacleClient"));
				APlayerController* PC = nullptr;
				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					if (It->Get() && It->Get()->IsLocalController() != bForClient) { PC = It->Get(); break; }
				}
				if (!Maze || !PC || !PC->GetPawn()) { return; }
				const bool bTunnel = Type == TEXT("tunnel");
				static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
				for (const TPair<FIntPoint, ALabMaze::EObstacle>& O : Maze->GetObstacles())
				{
					if ((O.Value == ALabMaze::EObstacle::Tunnel) != bTunnel) { continue; }
					for (const FIntPoint& D : Dirs)
					{
						const FIntPoint Prev = O.Key - D;
						if (!Maze->GetLayout().HasWallBetween(O.Key, FIntPoint(-D.X, -D.Y)) && Maze->GetLayout().IsValidCell(Prev) && !Maze->GetObstacles().Contains(Prev))
						{
							PC->GetPawn()->SetActorLocation(Maze->CellToWorld(Prev) + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
							PC->ClientSetRotation(FRotator(0.f, FVector(D.X, D.Y, 0.f).Rotation().Yaw, 0.f));
							UE_LOG(LogLabrynn, Log, TEXT("[Prueba] host frente a %s en %s (viene de %s)"), *Type, *O.Key.ToString(), *Prev.ToString());
							return;
						}
					}
				}
			}), At, false);
		}
	}

	// -LabLedgeSetup=N: cliente al pie del primer borde (mirándolo) y host detrás; -LabLedgeTurn=M: si el host está
	// arriba, lo pone en el filo mirando hacia abajo (la cámara no se puede mover con input simulado).
	{
		float SetupAt = 0.f, TurnAt = 0.f;
		const bool bSetup = FParse::Value(FCommandLine::Get(), TEXT("LabLedgeSetup="), SetupAt);
		const bool bTurn = FParse::Value(FCommandLine::Get(), TEXT("LabLedgeTurn="), TurnAt);
		auto LedgeInfo = [this](FVector& Face, FVector& Dir, FVector& LedgeCenter) -> bool
		{
			const ALabMaze* Maze = GetGameState<ALabGameState>()->GetMaze();
			if (!Maze || Maze->GetLedges().Num() == 0) { return false; }
			const TPair<FIntPoint, FIntPoint>& L = Maze->GetLedges()[0];
			Dir = FVector(L.Key.X - L.Value.X, L.Key.Y - L.Value.Y, 0.f);
			LedgeCenter = Maze->CellToWorld(L.Key);
			Face = Maze->CellToWorld(L.Value) + Dir * Maze->CellSize * 0.5f;
			return true;
		};
		if (bSetup)
		{
			FTimerHandle T1;
			GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [this, LedgeInfo]()
			{
				FVector Face, Dir, Center;
				if (!LedgeInfo(Face, Dir, Center)) { return; }
				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					APlayerController* PC = It->Get();
					if (!PC || !PC->GetPawn()) { continue; }
					const bool bClient = !PC->IsLocalController();
					PC->GetPawn()->SetActorLocation(Face - Dir * (bClient ? 45.f : 170.f) + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
					PC->ClientSetRotation(FRotator(bClient ? 0.f : -25.f, Dir.Rotation().Yaw, 0.f));
				}
				UE_LOG(LogLabrynn, Log, TEXT("[Prueba] borde: cliente al pie, host detrás"));
			}), SetupAt, false);
		}
		if (bTurn)
		{
			FTimerHandle T2;
			GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this, LedgeInfo]()
			{
				FVector Face, Dir, Center;
				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				if (!LedgeInfo(Face, Dir, Center) || !PC || !PC->GetPawn()) { return; }
				const float Z = PC->GetPawn()->GetActorLocation().Z;
				UE_LOG(LogLabrynn, Log, TEXT("[Prueba] host z=%.0f (%s del borde)"), Z, Z > 200.f ? TEXT("ARRIBA") : TEXT("abajo"));
				if (Z > 200.f)
				{
					PC->GetPawn()->SetActorLocation(FVector(Face.X, Face.Y, Z) + Dir * 45.f, false, nullptr, ETeleportType::TeleportPhysics);
					PC->SetControlRotation(FRotator(-45.f, (-Dir).Rotation().Yaw, 0.f));
				}
			}), TurnAt, false);
		}
	}

	// -LabWoundHost=N (a los 28 s): heridas al host para dejarle poca stamina (probar "se queda sin stamina y suelta").
	{
		float Wound = 0.f;
		if (FParse::Value(FCommandLine::Get(), TEXT("LabWoundHost="), Wound))
		{
			FTimerHandle TW;
			GetWorldTimerManager().SetTimer(TW, FTimerDelegate::CreateWeakLambda(this, [this, Wound]()
			{
				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				if (ALabCharacter* H = PC ? Cast<ALabCharacter>(PC->GetPawn()) : nullptr) { H->AddWound(Wound); }
			}), 28.f, false);
		}
	}

	// -LabDownClient=N: tira al primer cliente; -LabTeleportToClient=M: acomoda al host al lado de su cuerpo, mirándolo.
	{
		float DownAt = 0.f, ToAt = 0.f;
		auto FindClientPawn = [this]() -> ALabCharacter*
		{
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				if (It->Get() && !It->Get()->IsLocalController()) { return Cast<ALabCharacter>(It->Get()->GetPawn()); }
			}
			return nullptr;
		};
		if (FParse::Value(FCommandLine::Get(), TEXT("LabDownClient="), DownAt))
		{
			FTimerHandle T1;
			GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [FindClientPawn]()
			{
				if (ALabCharacter* C = FindClientPawn()) { C->KnockDown(FVector(0.f, 0.f, 150.f)); }
			}), DownAt, false);
		}
		if (FParse::Value(FCommandLine::Get(), TEXT("LabTeleportToClient="), ToAt))
		{
			FTimerHandle T2;
			GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this, FindClientPawn]()
			{
				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				ALabCharacter* C = FindClientPawn();
				if (!PC || !PC->GetPawn() || !C) { return; }
				const FVector Body = C->GetActorLocation();
				const FVector Pos = Body + FVector(-140.f, 0.f, 0.f);
				PC->GetPawn()->SetActorLocation(FVector(Pos.X, Pos.Y, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
				PC->SetControlRotation(FRotator(-30.f, 0.f, 0.f)); // mirando el cuerpo (+X)
			}), ToAt, false);
		}
	}

	// -LabTeleportToThrowable=N: host frente al primer lanzable; -LabTeleportToSpike=M: host a una celda de unos pinchos, mirándolos.
	{
		float At1 = 0.f, At2 = 0.f;
		if (FParse::Value(FCommandLine::Get(), TEXT("LabTeleportToThrowable="), At1))
		{
			FTimerHandle T1;
			GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				// -LabThrowClient: acomoda al primer cliente en vez del host.
				const bool bForClient = FParse::Param(FCommandLine::Get(), TEXT("LabThrowClient"));
				APlayerController* PC = nullptr;
				for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					if (It->Get() && It->Get()->IsLocalController() != bForClient) { PC = It->Get(); break; }
				}
				// -LabThrowKind=bottle elige la primera botella (si no, el primer lanzable).
				FString KindStr;
				FParse::Value(FCommandLine::Get(), TEXT("LabThrowKind="), KindStr);
				ALabThrowable* Target = nullptr;
				for (ALabThrowable* T : Throwables)
				{
					if (T && (KindStr != TEXT("bottle") || T->Kind == ELabThrowKind::Bottle)) { Target = T; break; }
				}
				if (!PC || !PC->GetPawn() || !Target) { return; }
				const FVector Loc = Target->GetActorLocation();
				PC->GetPawn()->SetActorLocation(Loc + FVector(-90.f, 0.f, 95.f), false, nullptr, ETeleportType::TeleportPhysics);
				PC->ClientSetRotation(FRotator(-50.f, 0.f, 0.f));
			}), At1, false);
		}
		if (FParse::Value(FCommandLine::Get(), TEXT("LabTeleportToSpike="), At2))
		{
			FTimerHandle T2;
			GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				const ALabMaze* Maze = GetGameState<ALabGameState>()->GetMaze();
				if (!PC || !PC->GetPawn() || !Maze) { return; }
				static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
				for (ALabTrapBase* Trap : Traps)
				{
					if (!Trap || !Trap->IsA<ALabSpikeTrap>()) { continue; }
					const FIntPoint C = Maze->WorldToCell(Trap->GetActorLocation());
					for (const FIntPoint& D : Dirs)
					{
						if (!Maze->GetLayout().HasWallBetween(C, D) && !Maze->GetObstacles().Contains(C + D))
						{
							PC->GetPawn()->SetActorLocation(Maze->CellToWorld(C + D) + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
							PC->SetControlRotation(FRotator(-15.f, FVector(-D.X, -D.Y, 0.f).Rotation().Yaw, 0.f));
							UE_LOG(LogLabrynn, Log, TEXT("[Prueba] host a una celda de %s"), *Trap->GetName());
							return;
						}
					}
				}
			}), At2, false);
		}
	}

	// -LabEyeTest=T -LabEyeLantern=none|hand|floor: host al inicio del pasillo recto más largo, mirando a lo largo
	// (pitch -20), para medir el brillo por franjas de distancia. hand = farol encendido en la mano; floor = farol en el piso a 3 m.
	{
		float At = 0.f;
		if (FParse::Value(FCommandLine::Get(), TEXT("LabEyeTest="), At))
		{
			FTimerHandle TE;
			GetWorldTimerManager().SetTimer(TE, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				ALabCharacter* Host = PC ? Cast<ALabCharacter>(PC->GetPawn()) : nullptr;
				const ALabMaze* Maze = GetGameState<ALabGameState>() ? GetGameState<ALabGameState>()->GetMaze() : nullptr;
				if (!Host || !Maze) { return; }
				const FLabMazeLayout& L = Maze->GetLayout();
				static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
				FIntPoint BestC(0, 0), BestD(1, 0);
				int32 Best = 0;
				for (int32 X = 0; X < L.Width; ++X)
				{
					for (int32 Y = 0; Y < L.Height; ++Y)
					{
						for (const FIntPoint& D : Dirs)
						{
							int32 N = 0;
							FIntPoint C(X, Y);
							while (!L.HasWallBetween(C, D) && !Maze->GetObstacles().Contains(C + D) && !Maze->GetPitCells().Contains(C + D) && !L.IsInCenterRoom(C + D)
								&& C.X + D.X >= 0 && C.Y + D.Y >= 0 && C.X + D.X < L.Width && C.Y + D.Y < L.Height)
							{
								C += D;
								++N;
							}
							if (N > Best && !Maze->GetObstacles().Contains(FIntPoint(X, Y))) { Best = N; BestC = FIntPoint(X, Y); BestD = D; }
						}
					}
				}
				const FVector Dir(BestD.X, BestD.Y, 0.f);
				const FVector Start = Maze->CellToWorld(BestC) - Dir * Maze->CellSize * 0.35f;
				Host->SetActorLocation(FVector(Start.X, Start.Y, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
				PC->SetControlRotation(FRotator(-20.f, Dir.Rotation().Yaw, 0.f));
				if (PC->GetHUD()) { PC->GetHUD()->bShowHUD = false; } // el HUD no entra en la medición
				FString Mode = TEXT("none");
				FParse::Value(FCommandLine::Get(), TEXT("LabEyeLantern="), Mode);
				for (TActorIterator<ALabLantern> It(GetWorld()); It; ++It)
				{
					if (Mode == TEXT("hand")) { It->SetHolder(Host); It->Refill(); break; }
					if (Mode == TEXT("floor"))
					{
						It->SetHolder(nullptr);
						It->SetActorLocation(FVector(Start.X, Start.Y, 0.f) + Dir * 300.f, false, nullptr, ETeleportType::TeleportPhysics);
						It->Refill();
						break;
					}
				}
				UE_LOG(LogLabrynn, Log, TEXT("[EyeTest] modo=%s pasillo=%d celdas (%.0f m) desde %s dir=%s piso=0"), *Mode, Best + 1,
					(Best + 0.85f) * Maze->CellSize / 100.f, *Start.ToCompactString(), *Dir.ToCompactString());
			}), At, false);
		}
	}

	// Prueba automática de stamina: el host corre (ida y vuelta) hasta quedarse sin aire; el cliente recibe heridas;
	// el host se hiere y usa un botiquín; al final las heridas del cliente llenan la barra y cae.
	static bool bAutoStaminaDone = false;
	if (!bAutoStaminaDone && FParse::Param(FCommandLine::Get(), TEXT("LabAutoStamina")))
	{
		bAutoStaminaDone = true;
		auto Host = [this]() { APlayerController* PC = GetWorld()->GetFirstPlayerController(); return PC ? Cast<ALabCharacter>(PC->GetPawn()) : nullptr; };
		auto Client = [this, Host]() { for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It) { if (*It != Host()) return *It; } return (ALabCharacter*)nullptr; };

		// El correr lo hace cada PlayerController con -LabAutoRun=Desde,Hasta (input real, incluido el sprint predicho del cliente).
		FTimerHandle TWound, TMed, TDown, TLog;
		GetWorldTimerManager().SetTimer(TLog, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
			{
				UE_LOG(LogLabrynn, Log, TEXT("[Stamina] t=%.0f %s %s: stamina=%.0f%% heridas=%.0f%% jadeando=%d vel=%.0f max=%.0f"),
					GetWorld()->GetTimeSeconds(), *It->GetName(), It->IsLocallyControlled() ? TEXT("(host)") : TEXT("(cliente)"),
					It->GetStamina01() * 100.f, It->GetWounds01() * 100.f, It->IsExhausted() ? 1 : 0,
					It->GetVelocity().Size2D(), It->GetCharacterMovement()->GetMaxSpeed());
			}
		}), 2.f, true);
		GetWorldTimerManager().SetTimer(TWound, FTimerDelegate::CreateWeakLambda(this, [Client]()
		{
			if (ALabCharacter* C = Client()) { C->AddWound(40.f); }
		}), 20.f, false);
		GetWorldTimerManager().SetTimer(TMed, FTimerDelegate::CreateWeakLambda(this, [this, Host]()
		{
			if (ALabCharacter* H = Host())
			{
				H->AddWound(30.f);
				FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (ALabMedkitPickup* M = GetWorld()->SpawnActor<ALabMedkitPickup>(ALabMedkitPickup::StaticClass(), FTransform(H->GetActorLocation()), SP))
				{
					M->Interact(H);
				}
			}
		}), 38.f, false);
		GetWorldTimerManager().SetTimer(TDown, FTimerDelegate::CreateWeakLambda(this, [Client]()
		{
			if (ALabCharacter* C = Client()) { C->AddWound(70.f, FVector(0.f, 0.f, 300.f)); }
		}), 40.f, false);
	}

	// Prueba automática del gas: el host (con linterna prendida) pisa una trampa de gas.
	static bool bAutoGasDone = false;
	if (!bAutoGasDone && FParse::Param(FCommandLine::Get(), TEXT("LabAutoGas")))
	{
		bAutoGasDone = true;
		FTimerHandle T1;
		GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			for (ALabTrapBase* Trap : Traps)
			{
				if (PC && PC->GetPawn() && Trap && Trap->IsA<ALabGasTrap>())
				{
					PC->GetPawn()->SetActorLocation(Trap->GetActorLocation() + FVector(0.f, 0.f, 110.f), false, nullptr, ETeleportType::TeleportPhysics);
					break;
				}
			}
		}), 18.f, false);
	}

	// Prueba automática del Corazón: tira al cliente, el host va al Corazón y lo usa.
	static bool bAutoHeartDone = false;
	if (!bAutoHeartDone && FParse::Param(FCommandLine::Get(), TEXT("LabAutoHeart")))
	{
		bAutoHeartDone = true;
		FTimerHandle T1, T2, T3;
		GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
			{
				if (PC && *It != PC->GetPawn()) { It->KnockDown(FVector(0.f, 0.f, 300.f)); }
			}
		}), 18.f, false);
		GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC && PC->GetPawn() && Heart)
			{
				const FVector Pos = Heart->GetActorLocation() + FVector(-170.f, -60.f, 60.f);
				PC->GetPawn()->SetActorLocation(Pos);
				PC->SetControlRotation(FRotator(-12.f, (Heart->GetActorLocation() - Pos).Rotation().Yaw - 25.f, 0.f));
			}
		}), 20.f, false);
		GetWorldTimerManager().SetTimer(T3, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (ALabCharacter* Host = PC ? Cast<ALabCharacter>(PC->GetPawn()) : nullptr)
			{
				if (Heart) { Heart->Interact(Host); }
			}
		}), 24.f, false);
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

	// Corazón del Laberinto: al lado de la reliquia.
	const FVector HeartLoc = Maze->GetCenterRoomLocation() + FVector(Maze->CellSize * 0.9f, 0.f, 45.f);
	Heart = GetWorld()->SpawnActor<ALabHeart>(HeartClass ? *HeartClass : ALabHeart::StaticClass(), FTransform(HeartLoc), P);
}

void ALabGameMode::SpawnPickups()
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	// Frascos de aceite en callejones sin salida sin foso (vale la pena explorarlos... con cuidado).
	FRandomStream Rng(GS->GetMazeSeed() * 131 + 3);
	TArray<FIntPoint> Cells = Maze->GetLayout().FindDeadEnds();
	Cells.RemoveAll([Maze](const FIntPoint& C) { return Maze->GetPitCells().Contains(C); });
	for (int32 i = Cells.Num() - 1; i > 0; --i)
	{
		Cells.Swap(i, Rng.RandRange(0, i));
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const int32 N = FMath::Min(OilFlaskCount, Cells.Num());
	for (int32 i = 0; i < N; ++i)
	{
		GetWorld()->SpawnActor<ALabOilFlask>(OilFlaskClass ? *OilFlaskClass : ALabOilFlask::StaticClass(),
			FTransform(Maze->CellToWorld(Cells[i]) + FVector(0.f, 0.f, 15.f)), P);
	}
	// Botiquines: en los siguientes callejones de la lista (sin repetir celda con un frasco).
	const int32 MEnd = FMath::Min(N + MedkitCount, Cells.Num());
	for (int32 i = N; i < MEnd; ++i)
	{
		GetWorld()->SpawnActor<ALabMedkitPickup>(MedkitClass ? *MedkitClass : ALabMedkitPickup::StaticClass(),
			FTransform(Maze->CellToWorld(Cells[i]) + FVector(0.f, 0.f, 10.f)), P);
	}
	UE_LOG(LogLabrynn, Log, TEXT("Frascos de aceite: %d, botiquines: %d"), N, MEnd - N);
}

void ALabGameMode::SpawnHidingSpots()
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	const FLabMazeLayout& L = Maze->GetLayout();
	const ULabGameplaySettings* S = GameplaySettings ? GameplaySettings.Get() : GetDefault<ULabGameplaySettings>();
	FRandomStream Rng(GS->GetMazeSeed() * 53 + 11);

	// Sobre todo en callejones sin salida (contra la pared del fondo, mirando a la abertura).
	TArray<FIntPoint> Cells = L.FindDeadEnds();
	Cells.RemoveAll([Maze](const FIntPoint& C) { return Maze->GetPitCells().Contains(C); });
	for (int32 i = Cells.Num() - 1; i > 0; --i)
	{
		Cells.Swap(i, Rng.RandRange(0, i));
	}
	static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	int32 Spawned = 0;
	for (int32 i = 0; i < Cells.Num() && Spawned < S->HidingSpotCount; ++i)
	{
		const FIntPoint C = Cells[i];
		FIntPoint Open(0, 0);
		for (const FIntPoint& D : Dirs)
		{
			if (!L.HasWallBetween(C, D)) { Open = D; break; }
		}
		const FVector OpenDir(Open.X, Open.Y, 0.f);
		const FVector Loc = Maze->CellToWorld(C) - OpenDir * (Maze->CellSize * 0.5f - 75.f);
		const FTransform T(OpenDir.Rotation(), Loc);
		ALabHidingSpot* Spot = GetWorld()->SpawnActorDeferred<ALabHidingSpot>(HidingSpotClass ? *HidingSpotClass : ALabHidingSpot::StaticClass(),
			T, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Spot)
		{
			Spot->Kind = static_cast<ELabHidingKind>(Rng.RandRange(0, 2));
			Spot->FinishSpawning(T);
			HidingSpots.Add(Spot);
			++Spawned;
		}
	}
	UE_LOG(LogLabrynn, Log, TEXT("Escondites: %d"), Spawned);
}

void ALabGameMode::SpawnThrowables()
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	const ULabGameplaySettings* S = GameplaySettings ? GameplaySettings.Get() : GetDefault<ULabGameplaySettings>();
	const FLabMazeLayout& L = Maze->GetLayout();
	FRandomStream Rng(GS->GetMazeSeed() * 71 + 29);
	int32 Spawned = 0;
	for (int32 Tries = 0; Tries < 200 && Spawned < S->ThrowableCount; ++Tries)
	{
		const FIntPoint C(Rng.RandRange(0, L.Width - 1), Rng.RandRange(0, L.Height - 1));
		if (Maze->GetPitCells().Contains(C) || Maze->GetObstacles().Contains(C) || L.IsInCenterRoom(C))
		{
			continue;
		}
		const FVector Loc = Maze->CellToWorld(C) + FVector(Rng.FRandRange(-120.f, 120.f), Rng.FRandRange(-120.f, 120.f), 9.f);
		ALabThrowable* T = GetWorld()->SpawnActorDeferred<ALabThrowable>(ALabThrowable::StaticClass(), FTransform(Loc), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (T)
		{
			T->Kind = Rng.FRand() < 0.5f ? ELabThrowKind::Stone : ELabThrowKind::Bottle;
			T->FinishSpawning(FTransform(Loc));
			Throwables.Add(T);
			++Spawned;
		}
	}
	UE_LOG(LogLabrynn, Log, TEXT("Lanzables: %d"), Spawned);
}

void ALabGameMode::SpawnMinotaur()
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	// En el callejón sin salida (sin foso) más lejos de todos los jugadores: aparece lejos, no encima de nadie.
	FIntPoint Best(-1, -1);
	float BestDist = -1.f;
	float TestDist = 0.f; // pruebas: -LabMinotaurSpawnDist=N lo pone a ~N cm del jugador más cercano
#if !UE_BUILD_SHIPPING
	FParse::Value(FCommandLine::Get(), TEXT("LabMinotaurSpawnDist="), TestDist);
#endif
	float BestErr = TNumericLimits<float>::Max();
	for (const FIntPoint& C : Maze->GetLayout().FindDeadEnds())
	{
		if (Maze->GetPitCells().Contains(C))
		{
			continue;
		}
		float MinD = TNumericLimits<float>::Max();
		for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
		{
			MinD = FMath::Min(MinD, FVector::Dist2D(It->GetActorLocation(), Maze->CellToWorld(C)));
		}
		if (TestDist > 0.f)
		{
			if (FMath::Abs(MinD - TestDist) < BestErr)
			{
				BestErr = FMath::Abs(MinD - TestDist);
				BestDist = MinD;
				Best = C;
			}
		}
		else if (MinD > BestDist)
		{
			BestDist = MinD;
			Best = C;
		}
	}
	if (Best.X < 0)
	{
		return;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Minotaur = GetWorld()->SpawnActor<ALabMinotaur>(MinotaurClass ? *MinotaurClass : ALabMinotaur::StaticClass(),
		FTransform(Maze->CellToWorld(Best) + FVector(0.f, 0.f, 140.f)), P);
	UE_LOG(LogLabrynn, Log, TEXT("Minotauro spawneado en la celda %s (a %.0f cm del jugador más cercano)"), *Best.ToString(), BestDist);
}

void ALabGameMode::SpawnTraps()
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	const FLabMazeLayout& L = Maze->GetLayout();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Fosos: en las celdas que el laberinto ya dejó con hueco (deterministas por seed).
	for (const FIntPoint& Cell : Maze->GetPitCells())
	{
		ALabPitTrap* Pit = GetWorld()->SpawnActorDeferred<ALabPitTrap>(PitTrapClass ? *PitTrapClass : ALabPitTrap::StaticClass(),
			FTransform(Maze->CellToWorld(Cell)), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Pit)
		{
			Pit->CellSize = Maze->CellSize;
			Pit->PitDepth = Maze->PitDepth;
			Pit->FinishSpawning(FTransform(Maze->CellToWorld(Cell)));
			Traps.Add(Pit);
		}
	}

	// Pinchos: en pasillos rectos, lejos de la entrada.
	FRandomStream Rng(GS->GetMazeSeed() * 31 + 7);
	TArray<FIntPoint> Cells = L.FindStraightCorridors();
	Cells.RemoveAll([&L, Maze](const FIntPoint& C)
	{
		return FMath::Abs(C.X - L.Entrance.X) + FMath::Abs(C.Y - L.Entrance.Y) <= 3 || Maze->GetObstacles().Contains(C);
	});
	for (int32 i = Cells.Num() - 1; i > 0; --i)
	{
		Cells.Swap(i, Rng.RandRange(0, i));
	}
	for (int32 i = 0; i < FMath::Min(SpikeTrapCount, Cells.Num()); ++i)
	{
		ALabSpikeTrap* Spike = GetWorld()->SpawnActorDeferred<ALabSpikeTrap>(SpikeTrapClass ? *SpikeTrapClass : ALabSpikeTrap::StaticClass(),
			FTransform(Maze->CellToWorld(Cells[i])), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Spike)
		{
			Spike->CellSize = Maze->CellSize;
			Spike->FinishSpawning(FTransform(Maze->CellToWorld(Cells[i])));
			Traps.Add(Spike);
		}
	}
	// Gas: en los siguientes pasillos de la lista (nunca en la misma celda que unos pinchos).
	const int32 GasStart = FMath::Min(SpikeTrapCount, Cells.Num());
	const int32 GasEnd = FMath::Min(GasStart + GasTrapCount, Cells.Num());
	for (int32 i = GasStart; i < GasEnd; ++i)
	{
		ALabGasTrap* Gas = GetWorld()->SpawnActorDeferred<ALabGasTrap>(GasTrapClass ? *GasTrapClass : ALabGasTrap::StaticClass(),
			FTransform(Maze->CellToWorld(Cells[i])), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Gas)
		{
			Gas->CellSize = Maze->CellSize;
			Gas->FinishSpawning(FTransform(Maze->CellToWorld(Cells[i])));
			Traps.Add(Gas);
		}
	}
	UE_LOG(LogLabrynn, Log, TEXT("Trampas: %d fosos, %d pinchos, %d gas"), Maze->GetPitCells().Num(), GasStart, GasEnd - GasStart);
}

void ALabGameMode::CheckWinLose()
{
	ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!GS || !Maze || GS->GetPhase() == ELabPhase::Won || GS->GetPhase() == ELabPhase::Lost)
	{
		return;
	}

	// Fase de escape: aparece el Minotauro (una vez).
	if (GS->GetPhase() == ELabPhase::Escape && !Minotaur)
	{
		SpawnMinotaur();
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

void ALabGameMode::EnsureLanterns()
{
	const ALabGameState* GS = GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	Lanterns.RemoveAll([](const TObjectPtr<ALabLantern>& L) { return !IsValid(L); });
	const int32 Desired = FMath::Max(1, GetNumPlayers() - 1);
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	while (Lanterns.Num() < Desired)
	{
		ALabLantern* L = GetWorld()->SpawnActor<ALabLantern>(LanternClass ? *LanternClass : ALabLantern::StaticClass(),
			Maze->GetLanternTableSlot(Lanterns.Num()), P);
		if (!L)
		{
			break;
		}
		Lanterns.Add(L);
		UE_LOG(LogLabrynn, Log, TEXT("Farol %d en la mesa (jugadores=%d)"), Lanterns.Num(), GetNumPlayers());
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

	// Luz compartida: tiene que haber max(1, jugadores - 1) faroles en la mesa del hall.
	EnsureLanterns();
}

// ---------------------------------------------------------------- Diagnóstico de red (para la prueba por Steam)

void ALabGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	const APlayerState* PS = NewPlayer ? NewPlayer->PlayerState : nullptr;
	UNetConnection* Conn = NewPlayer ? NewPlayer->GetNetConnection() : nullptr;
	UE_LOG(LogLabrynn, Log, TEXT("Red: entró %s (id=%s, dirección=%s, local=%d). Jugadores: %d"),
		PS ? *PS->GetPlayerName() : TEXT("?"), PS ? *PS->GetUniqueId().ToString() : TEXT("?"),
		Conn ? *Conn->LowLevelGetRemoteAddress(true) : TEXT("-"), NewPlayer && NewPlayer->IsLocalController() ? 1 : 0, GetNumPlayers());
}

void ALabGameMode::Logout(AController* Exiting)
{
	const APlayerState* PS = Exiting ? Exiting->PlayerState : nullptr;
	UE_LOG(LogLabrynn, Log, TEXT("Red: salió %s. Quedan: %d"), PS ? *PS->GetPlayerName() : TEXT("?"), FMath::Max(0, GetNumPlayers() - 1));
	Super::Logout(Exiting);
}