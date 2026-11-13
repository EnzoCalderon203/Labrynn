#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Audio/LabAudioSettings.h"
#include "LabMinotaur.generated.h"

class UPointLightComponent;
class USoundBase;
class ALabCharacter;
class ALabLantern;
class ULabMinotaurSettings;

/**
 * El Minotauro: aparece en la fase de escape. No usa NavMesh: se mueve por la grilla del laberinto
 * con BFS hacia el último ruido fuerte que oyó (voz, jadeo, sprint, trampas, objetos lanzados).
 * Le teme al fuego: un farol encendido cerca y por delante lo frena, retrocede y busca otro camino.
 * Si alcanza a alguien, le deja heridas grandes y lo tira en ragdoll; después queda aturdido.
 * Valores en /Game/Labrynn/Core/DA_Minotaur.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabMinotaur : public ACharacter
{
	GENERATED_BODY()

public:
	ALabMinotaur();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Solo servidor (lo llama el controller cuando la percepción oye algo). */
	void OnHeardNoise(const FVector& Location, float Strength, FName Tag);

	UFUNCTION(BlueprintPure, Category = "Lab|Minotaur")
	bool IsChasing() const { return bChasing; }

	UFUNCTION(BlueprintPure, Category = "Lab|Minotaur")
	bool IsRepelled() const { return bRepelled; }

	const ULabMinotaurSettings* GetSettings() const;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	TObjectPtr<ULabMinotaurSettings> Settings;





protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Minotaur")
	TObjectPtr<UPointLightComponent> EyesGlow;

	UPROPERTY(Replicated)
	bool bChasing = false;

	UPROPERTY(Replicated)
	bool bRepelled = false;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSfx(ELabSfx Id);

private:
	void ServerThink();
	void RepathTo(const FIntPoint& TargetCell, bool bAvoidFire);
	void PickWanderTarget();
	void TryAttack();
	/** Farol encendido que lo asusta ahora (cerca y entre él y su objetivo), o null. */
	ALabLantern* FindScaryLantern(const FVector& GoalDir) const;
	class ALabMaze* GetMaze() const;

	TArray<FIntPoint> Path;
	FIntPoint GoalCell = FIntPoint(-1, -1);
	float NoiseStrength = 0.f;
	float NoiseTime = -100.f;
	float StunnedUntil = 0.f;
	float RetreatUntil = 0.f;
	FVector RetreatDir = FVector::ZeroVector;
	TWeakObjectPtr<ALabLantern> RepellingLantern;
	float NextThink = 0.f;
	float NextFootstep = 0.f;
	float NextSnort = 0.f;
	float NextLog = 0.f;
};
