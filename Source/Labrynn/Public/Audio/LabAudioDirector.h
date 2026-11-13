#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/LabGameState.h"
#include "LabAudioDirector.generated.h"

class UAudioComponent;

/**
 * Audio "global" de cada máquina (no en servidor dedicado):
 * - Música adaptativa: 3 capas sincronizadas (calma / tensión / persecución) con crossfade según el Minotauro.
 * - Ducking: la música baja cuando alguien habla por voz.
 * - Ambiente: cama 2D en loop + goteos, viento, crujidos y retumbes 3D al azar cerca del jugador.
 * - Stingers al abrirse la salida, ganar o perder; sonido de ping.
 */
UCLASS()
class LABRYNN_API ULabAudioDirector : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bStarted; }

	/** 0 = calma, 1 = tensión, 2 = persecución (para pruebas y HUD). */
	int32 GetMusicState() const { return MusicState; }

private:
	bool bStarted = false;
	bool bInMaze = false;

	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Layers; // calma, tensión, persecución
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Bed;

	float Weights[3] = { 1.f, 0.f, 0.f };
	float Duck = 1.f;
	bool bDucking = false;
	int32 MusicState = 0;
	float NextAmbient = 0.f;
	float LastPingTime = 0.f;
	ELabPhase LastPhase = ELabPhase::Explore;
	bool bPhaseKnown = false;

	FVector GetListenerLocation() const;
	void TickMusic(float DeltaTime);
	void TickAmbience();
	void TickEvents();
};
