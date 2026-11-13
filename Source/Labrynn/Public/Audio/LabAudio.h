#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Audio/LabAudioSettings.h"
#include "LabAudio.generated.h"

class UAudioComponent;
class USceneComponent;

/**
 * Punto único para reproducir sonidos (cosméticos, en cada máquina; nunca en servidor dedicado).
 * Con -LabAudioLog cada evento se escribe en el log (sirve para pruebas con -nosound).
 */
UCLASS()
class LABRYNN_API ULabAudio : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static const ULabAudioSettings* Settings();

	/** 3D en Location (o 2D si el evento no tiene atenuación). */
	static UAudioComponent* Play(const UObject* WorldContext, ELabSfx Id, const FVector& Location, float VolumeScale = 1.f);

	/** Siempre 2D. */
	static UAudioComponent* Play2D(const UObject* WorldContext, ELabSfx Id, float VolumeScale = 1.f);

	/** Pegado a un componente (loops: fuego, latido, respiración). Devuelve el componente para cortarlo. */
	static UAudioComponent* PlayAttached(ELabSfx Id, USceneComponent* AttachTo, float VolumeScale = 1.f);

	// --- Volúmenes por categoría (se guardan en GameUserSettings.ini) ---
	static float GetCategoryVolume(ELabAudioCategory Cat);
	static void SetCategoryVolume(const UObject* WorldContext, ELabAudioCategory Cat, float Volume);
	static void ApplyVolumes(const UObject* WorldContext);

	static const TCHAR* SfxName(ELabSfx Id);

private:
	static const FLabSoundEvent* Pick(ELabSfx Id, USoundBase*& OutSound, float& OutVolume, float& OutPitch);
	static bool ShouldPlay(const UWorld* World);
	static void LogPlay(ELabSfx Id, const USoundBase* Sound, const FVector* Location, const UWorld* World);
};
