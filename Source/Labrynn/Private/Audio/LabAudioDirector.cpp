#include "Audio/LabAudioDirector.h"

#include "Audio/LabAudio.h"
#include "Enemy/LabMinotaur.h"
#include "Player/LabCharacter.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Labrynn.h"

bool ULabAudioDirector::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE);
}

TStatId ULabAudioDirector::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULabAudioDirector, STATGROUP_Tickables);
}

void ULabAudioDirector::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	const ULabAudioSettings* S = ULabAudio::Settings();
	if (!S)
	{
		return;
	}
	ULabAudio::ApplyVolumes(&InWorld);
	bInMaze = InWorld.GetGameState<ALabGameState>() != nullptr || InWorld.GetAuthGameMode() == nullptr; // cliente: el GameState llega después
	bStarted = true;

	// Las 3 capas arrancan juntas (mismo largo y tempo) y quedan sincronizadas; solo cambia su volumen.
	for (USoundBase* M : { S->MusicCalm.Get(), S->MusicTension.Get(), S->MusicChase.Get() })
	{
		UAudioComponent* C = M ? UGameplayStatics::CreateSound2D(&InWorld, M, 1.f, 1.f, 0.f, nullptr, false, false) : nullptr;
		if (C)
		{
			C->SetVolumeMultiplier(Layers.Num() == 0 ? S->MusicGain : 0.001f);
			C->Play();
		}
		Layers.Add(C);
	}
	NextAmbient = InWorld.GetTimeSeconds() + 3.f;
	UE_LOG(LogLabrynn, Log, TEXT("Música: %d capas listas (calma)"), Layers.FilterByPredicate([](const UAudioComponent* C) { return C != nullptr; }).Num());
}

void ULabAudioDirector::Deinitialize()
{
	for (UAudioComponent* C : Layers)
	{
		if (C) { C->Stop(); }
	}
	if (Bed) { Bed->Stop(); }
	Super::Deinitialize();
}

FVector ULabAudioDirector::GetListenerLocation() const
{
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			return PC->PlayerCameraManager->GetCameraLocation();
		}
	}
	return FVector::ZeroVector;
}

void ULabAudioDirector::Tick(float DeltaTime)
{
	const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>();
	bInMaze = GS != nullptr;
	if (bInMaze && !Bed)
	{
		const ULabAudioSettings* S = ULabAudio::Settings();
		if (S && S->AmbientBed)
		{
			Bed = UGameplayStatics::CreateSound2D(GetWorld(), S->AmbientBed, S->AmbientBedVolume, 1.f, 0.f, nullptr, false, false);
			if (Bed) { Bed->FadeIn(4.f, S->AmbientBedVolume); }
		}
	}
	TickMusic(DeltaTime);
	if (bInMaze)
	{
		TickAmbience();
		TickEvents();
	}
}

void ULabAudioDirector::TickMusic(float DeltaTime)
{
	const ULabAudioSettings* S = ULabAudio::Settings();
	if (!S || Layers.Num() < 3)
	{
		return;
	}
	const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>();
	float Target[3] = { 1.f, 0.f, 0.f };
	FString Why = TEXT("exploración");
	if (GS && (GS->GetPhase() == ELabPhase::Won || GS->GetPhase() == ELabPhase::Lost))
	{
		Target[0] = 0.f; // fin de ronda: suena el stinger
		Why = TEXT("fin de ronda");
	}
	else if (GS && GS->GetPhase() == ELabPhase::Escape)
	{
		// Con el Minotauro suelto, la calma nunca vuelve del todo.
		Target[0] = 0.4f; Target[1] = 0.6f;
		Why = TEXT("escape");
		const FVector L = GetListenerLocation();
		for (TActorIterator<ALabMinotaur> It(GetWorld()); It; ++It)
		{
			const float D = FVector::Dist(It->GetActorLocation(), L);
			// Histéresis: una vez en un estado, cuesta un 35 % más de distancia salir (no oscila en el borde).
			const float Hyst = 1.35f;
			const float ChaseD = S->ChaseDistance * (MusicState == 2 ? Hyst : 1.f);
			const float CloseD = S->ChaseDistance * 0.45f * (MusicState == 2 ? Hyst : 1.f);
			const float TensionD = S->TensionDistance * (MusicState >= 1 ? Hyst : 1.f);
			if ((It->IsChasing() && D < ChaseD) || D < CloseD)
			{
				Target[0] = 0.f; Target[1] = 0.3f; Target[2] = 1.f;
				Why = FString::Printf(TEXT("persecución, Minotauro a %.0f m"), D / 100.f);
			}
			else if (D < TensionD)
			{
				Target[0] = 0.f; Target[1] = 1.f;
				Why = FString::Printf(TEXT("tensión, Minotauro a %.0f m"), D / 100.f);
			}
		}
	}
	const int32 NewState = Target[2] > 0.5f ? 2 : (Target[1] > 0.8f ? 1 : 0);
	if (NewState != MusicState)
	{
		static const TCHAR* Names[] = { TEXT("calma"), TEXT("tensión"), TEXT("persecución") };
		UE_LOG(LogLabrynn, Log, TEXT("Música: %s -> %s (%s)"), Names[MusicState], Names[NewState], *Why);
		MusicState = NewState;
	}

	// Ducking: alguien habla por voz → la música baja.
	bool bTalk = false;
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		bTalk |= It->IsTalking();
	}
	if (bTalk != bDucking)
	{
		bDucking = bTalk;
		UE_LOG(LogLabrynn, Log, TEXT("Música: ducking %s"), bTalk ? TEXT("ON (alguien habla)") : TEXT("OFF"));
	}
	Duck = FMath::FInterpConstantTo(Duck, bTalk ? S->VoiceDuckVolume : 1.f, DeltaTime, 2.5f);

	for (int32 i = 0; i < 3; ++i)
	{
		const float Speed = Target[i] > Weights[i] ? S->CrossfadeSpeed * 1.5f : S->CrossfadeSpeed; // entra rápido, sale lento
		Weights[i] = FMath::FInterpConstantTo(Weights[i], Target[i], DeltaTime, Speed);
		if (Layers[i])
		{
			// Nunca 0 exacto: la capa sigue sonando en silencio y no pierde la sincronía.
			Layers[i]->SetVolumeMultiplier(FMath::Max(0.001f, Weights[i] * Duck * S->MusicGain));
		}
	}
}

void ULabAudioDirector::TickAmbience()
{
	const ULabAudioSettings* S = ULabAudio::Settings();
	const float Now = GetWorld()->GetTimeSeconds();
	if (!S || Now < NextAmbient)
	{
		return;
	}
	NextAmbient = Now + FMath::FRandRange(S->AmbientInterval.X, S->AmbientInterval.Y);
	const float R = FMath::FRand();
	const ELabSfx Id = R < 0.45f ? ELabSfx::AmbDrip : (R < 0.68f ? ELabSfx::AmbWind : (R < 0.86f ? ELabSfx::AmbCreak : ELabSfx::AmbRumble));
	const FVector Dir = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
	ULabAudio::Play(GetWorld(), Id, GetListenerLocation() + Dir * FMath::FRandRange(500.f, 1600.f) + FVector(0.f, 0.f, 120.f));
}

void ULabAudioDirector::TickEvents()
{
	const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>();
	if (!GS)
	{
		return;
	}
	// Stingers de fase (no en el primer frame: un cliente que entra tarde no los escucha de golpe).
	const ELabPhase Phase = GS->GetPhase();
	if (bPhaseKnown && Phase != LastPhase)
	{
		const ELabSfx Id = Phase == ELabPhase::Escape ? ELabSfx::StingerExit : (Phase == ELabPhase::Won ? ELabSfx::StingerVictory : ELabSfx::StingerDefeat);
		if (Phase != ELabPhase::Explore)
		{
			ULabAudio::Play2D(GetWorld(), Id);
			UE_LOG(LogLabrynn, Log, TEXT("Audio: stinger %s"), ULabAudio::SfxName(Id));
		}
	}
	LastPhase = Phase;
	bPhaseKnown = true;

	// Ping nuevo → "blip" para todos.
	for (const FLabPing& P : GS->GetPings())
	{
		if (P.ServerTime > LastPingTime)
		{
			LastPingTime = P.ServerTime;
			ULabAudio::Play2D(GetWorld(), ELabSfx::Ping);
		}
	}
}
