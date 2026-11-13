#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LabAudioSettings.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundConcurrency;
class USoundClass;
class USoundMix;

/** Todos los eventos de sonido del juego (los llena Scripts/setup_audio.py desde SourceAudio/manifest.json). */
UENUM(BlueprintType)
enum class ELabSfx : uint8
{
	Footstep, Jump, Land, BodyFall, Hurt, Revive, Effort, Breath, Pickup, Drop, Throw, CarryLift, HideEnter, HideExit,
	Ping, UIClick,
	LanternLight, LanternOut, LanternFire, LanternTake, OilPickup, MedkitPickup,
	StoneImpact, BottleShatter,
	TrapWarn, SpikeFire, PitFire, GasFire, TrapReset,
	HeartUse, Heartbeat, RelicTake,
	StingerExit, StingerVictory, StingerDefeat,
	MinoStep, MinoSnort, MinoRoar, MinoFireFear, MinoAttack,
	AmbDrip, AmbWind, AmbRumble, AmbCreak,
};

/** Categorías con volumen propio en el menú (cada una es una Sound Class). */
UENUM(BlueprintType)
enum class ELabAudioCategory : uint8
{
	Master, SFX, Voice, Music, Ambience,
};

/** Un evento: variantes elegidas al azar (sin repetir la anterior) con volumen y pitch aleatorios. */
USTRUCT(BlueprintType)
struct FLabSoundEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TArray<TObjectPtr<USoundBase>> Variants;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	float VolumeMin = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	float VolumeMax = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	float PitchMin = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	float PitchMax = 1.f;

	/** Vacío = sonido 2D (UI, stingers). Los alcances coinciden con lo que oye el Minotauro. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundAttenuation> Attenuation;

	/** Límite de voces simultáneas (para que 4 jugadores no saturen). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundConcurrency> Concurrency;
};

/** /Game/Labrynn/Core/DA_Audio: sonidos, música, mezcla y parámetros de la música adaptativa. */
UCLASS(BlueprintType)
class LABRYNN_API ULabAudioSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eventos")
	TMap<ELabSfx, FLabSoundEvent> Events;

	// --- Música adaptativa: 3 capas del mismo largo/tempo que suenan juntas y se mezclan ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música")
	TObjectPtr<USoundBase> MusicCalm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música")
	TObjectPtr<USoundBase> MusicTension;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música")
	TObjectPtr<USoundBase> MusicChase;

	/** Minotauro a menos de esto (cm) → tensión. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música")
	float TensionDistance = 3500.f;

	/** Minotauro persiguiendo y a menos de esto (cm), o muy cerca → persecución. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música")
	float ChaseDistance = 2200.f;

	/** Velocidad del crossfade entre capas (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música")
	float CrossfadeSpeed = 0.6f;

	/** Volumen de la música mientras alguien habla por voz (ducking). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música", meta = (ClampMin = 0, ClampMax = 1))
	float VoiceDuckVolume = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Música")
	float MusicGain = 0.8f;

	// --- Ambiente ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambiente")
	TObjectPtr<USoundBase> AmbientBed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambiente")
	float AmbientBedVolume = 0.7f;

	/** Segundos entre sonidos sueltos de ambiente (goteo, viento, crujidos, retumbes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambiente")
	FVector2D AmbientInterval = FVector2D(5.f, 12.f);

	// --- Mezcla (volúmenes del menú) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mezcla")
	TObjectPtr<USoundMix> SettingsMix;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mezcla")
	TMap<ELabAudioCategory, TObjectPtr<USoundClass>> Classes;
};
