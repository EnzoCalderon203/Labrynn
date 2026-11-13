#include "Audio/LabAudio.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "UObject/StrongObjectPtr.h"
#include "Labrynn.h"

namespace
{
	const TCHAR* GAudioSection = TEXT("Labrynn.Audio");
	const TCHAR* GSettingsPath = TEXT("/Game/Labrynn/Core/DA_Audio.DA_Audio");

	TMap<ELabSfx, int32>& LastVariant()
	{
		static TMap<ELabSfx, int32> Map;
		return Map;
	}

	bool AudioLogEnabled()
	{
		static const bool b = FParse::Param(FCommandLine::Get(), TEXT("LabAudioLog"));
		return b;
	}

	const TCHAR* CatKey(ELabAudioCategory Cat)
	{
		switch (Cat)
		{
		case ELabAudioCategory::SFX: return TEXT("SFX");
		case ELabAudioCategory::Voice: return TEXT("Voice");
		case ELabAudioCategory::Music: return TEXT("Music");
		case ELabAudioCategory::Ambience: return TEXT("Ambience");
		default: return TEXT("Master");
		}
	}
}

const ULabAudioSettings* ULabAudio::Settings()
{
	static TStrongObjectPtr<ULabAudioSettings> Cached;
	static bool bWarned = false;
	if (!Cached.IsValid())
	{
		Cached.Reset(LoadObject<ULabAudioSettings>(nullptr, GSettingsPath));
		if (!Cached.IsValid() && !bWarned)
		{
			bWarned = true;
			UE_LOG(LogLabrynn, Warning, TEXT("Audio: falta %s (correr Scripts/setup_audio.py). El juego sigue sin sonido."), GSettingsPath);
		}
	}
	return Cached.Get();
}

const TCHAR* ULabAudio::SfxName(ELabSfx Id)
{
	static FString Buf;
	Buf = StaticEnum<ELabSfx>()->GetNameStringByValue(static_cast<int64>(Id));
	return *Buf;
}

bool ULabAudio::ShouldPlay(const UWorld* World)
{
	return World && World->GetNetMode() != NM_DedicatedServer && World->IsGameWorld();
}

const FLabSoundEvent* ULabAudio::Pick(ELabSfx Id, USoundBase*& OutSound, float& OutVolume, float& OutPitch)
{
	OutSound = nullptr;
	const ULabAudioSettings* S = Settings();
	const FLabSoundEvent* E = S ? S->Events.Find(Id) : nullptr;
	if (!E || E->Variants.Num() == 0)
	{
		return nullptr;
	}
	// Variante al azar, sin repetir la última (para que pasos e impactos no suenen a metralleta).
	int32 Index = FMath::RandRange(0, E->Variants.Num() - 1);
	if (E->Variants.Num() > 1)
	{
		const int32* Last = LastVariant().Find(Id);
		if (Last && *Last == Index)
		{
			Index = (Index + 1 + FMath::RandRange(0, E->Variants.Num() - 2)) % E->Variants.Num();
		}
	}
	LastVariant().Add(Id, Index);
	OutSound = E->Variants[Index];
	OutVolume = FMath::FRandRange(E->VolumeMin, E->VolumeMax);
	OutPitch = FMath::FRandRange(E->PitchMin, E->PitchMax);
	return OutSound ? E : nullptr;
}

void ULabAudio::LogPlay(ELabSfx Id, const USoundBase* Sound, const FVector* Location, const UWorld* World)
{
	if (AudioLogEnabled())
	{
		UE_LOG(LogLabrynn, Log, TEXT("Sfx[%s]: %s -> %s %s"), World && World->GetNetMode() == NM_Client ? TEXT("cliente") : TEXT("servidor"),
			SfxName(Id), *GetNameSafe(Sound), Location ? *Location->ToCompactString() : TEXT("(2D)"));
	}
}

UAudioComponent* ULabAudio::Play(const UObject* WorldContext, ELabSfx Id, const FVector& Location, float VolumeScale)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!ShouldPlay(World))
	{
		return nullptr;
	}
	USoundBase* Sound; float Vol, Pitch;
	const FLabSoundEvent* E = Pick(Id, Sound, Vol, Pitch);
	if (!E)
	{
		return nullptr;
	}
	if (!E->Attenuation)
	{
		LogPlay(Id, Sound, nullptr, World);
		return UGameplayStatics::SpawnSound2D(World, Sound, Vol * VolumeScale, Pitch, 0.f, E->Concurrency);
	}
	LogPlay(Id, Sound, &Location, World);
	return UGameplayStatics::SpawnSoundAtLocation(World, Sound, Location, FRotator::ZeroRotator, Vol * VolumeScale, Pitch, 0.f,
		E->Attenuation, E->Concurrency, true);
}

UAudioComponent* ULabAudio::Play2D(const UObject* WorldContext, ELabSfx Id, float VolumeScale)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!ShouldPlay(World))
	{
		return nullptr;
	}
	USoundBase* Sound; float Vol, Pitch;
	const FLabSoundEvent* E = Pick(Id, Sound, Vol, Pitch);
	if (!E)
	{
		return nullptr;
	}
	LogPlay(Id, Sound, nullptr, World);
	return UGameplayStatics::SpawnSound2D(World, Sound, Vol * VolumeScale, Pitch, 0.f, E->Concurrency);
}

UAudioComponent* ULabAudio::PlayAttached(ELabSfx Id, USceneComponent* AttachTo, float VolumeScale)
{
	UWorld* World = AttachTo ? AttachTo->GetWorld() : nullptr;
	if (!ShouldPlay(World))
	{
		return nullptr;
	}
	USoundBase* Sound; float Vol, Pitch;
	const FLabSoundEvent* E = Pick(Id, Sound, Vol, Pitch);
	if (!E)
	{
		return nullptr;
	}
	const FVector Loc = AttachTo->GetComponentLocation();
	LogPlay(Id, Sound, &Loc, World);
	return UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
		true, Vol * VolumeScale, Pitch, 0.f, E->Attenuation, E->Concurrency, true);
}

float ULabAudio::GetCategoryVolume(ELabAudioCategory Cat)
{
	float V = 1.f;
	if (GConfig)
	{
		GConfig->GetFloat(GAudioSection, CatKey(Cat), V, GGameUserSettingsIni);
	}
	return FMath::Clamp(V, 0.f, 1.f);
}

void ULabAudio::SetCategoryVolume(const UObject* WorldContext, ELabAudioCategory Cat, float Volume)
{
	if (GConfig)
	{
		GConfig->SetFloat(GAudioSection, CatKey(Cat), FMath::Clamp(Volume, 0.f, 1.f), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	ApplyVolumes(WorldContext);
}

void ULabAudio::ApplyVolumes(const UObject* WorldContext)
{
	const ULabAudioSettings* S = Settings();
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!S || !S->SettingsMix || !ShouldPlay(World))
	{
		return;
	}
	for (const TPair<ELabAudioCategory, TObjectPtr<USoundClass>>& It : S->Classes)
	{
		if (It.Value)
		{
			UGameplayStatics::SetSoundMixClassOverride(World, S->SettingsMix, It.Value, GetCategoryVolume(It.Key), 1.f, 0.1f, true);
		}
	}
	UGameplayStatics::PushSoundMixModifier(World, S->SettingsMix);
}
