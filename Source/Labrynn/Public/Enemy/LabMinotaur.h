#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LabMinotaur.generated.h"

class UPointLightComponent;
class USoundBase;
class ALabCharacter;

/**
 * El Minotauro: aparece en la fase de escape. No usa NavMesh: se mueve por la grilla del laberinto
 * con BFS hacia el último ruido fuerte que oyó (voz, jadeo, sprint, trampas). Si alcanza a alguien,
 * le deja heridas grandes y lo tira en ragdoll; después queda aturdido unos segundos (justo, no injusto).
 * Solo el servidor decide; el movimiento se replica con el CharacterMovement (los clientes ven caminar la animación).
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

	// --- Ajustes ---
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	float ChaseSpeed = 600.f;   // < sprint (700): se le puede escapar corriendo

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	float WanderSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	float AttackRange = 170.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	float AttackWound = 50.f;

	/** Segundos aturdido después de golpear (ventana para escapar o rescatar). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	float StunAfterHit = 2.5f;

	/** Ruidos más débiles que esto se ignoran. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	float MinNoiseStrength = 0.25f;

	/** Un ruido viejo deja de importar después de estos segundos. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur")
	float NoiseMemory = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur|Audio")
	TObjectPtr<USoundBase> FootstepSound;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur|Audio")
	TObjectPtr<USoundBase> SnortSound;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Minotaur|Audio")
	TObjectPtr<USoundBase> RoarSound;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Minotaur")
	TObjectPtr<UPointLightComponent> EyesGlow;

	UPROPERTY(Replicated)
	bool bChasing = false;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRoar();

private:
	void ServerThink();
	void RepathTo(const FIntPoint& TargetCell);
	void PickWanderTarget();
	void TryAttack();
	class ALabMaze* GetMaze() const;

	TArray<FIntPoint> Path;   // celdas a recorrer (la primera es la próxima)
	FIntPoint GoalCell = FIntPoint(-1, -1);
	FVector NoiseLocation = FVector::ZeroVector;
	float NoiseStrength = 0.f;
	float NoiseTime = -100.f;
	float StunnedUntil = 0.f;
	float NextThink = 0.f;
	float NextFootstep = 0.f;
	float NextSnort = 0.f;
	float NextLog = 0.f;
};
