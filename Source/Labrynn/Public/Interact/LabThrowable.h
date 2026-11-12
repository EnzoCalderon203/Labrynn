#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabThrowable.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ELabThrowKind : uint8
{
	Stone,   // rebota; ruido medio en cada golpe fuerte; se puede volver a juntar
	Bottle,  // se rompe al primer golpe; ruido fuerte
};

/**
 * Objeto para distraer al Minotauro (piedra o botella). En el piso: E para juntarlo (máx. 3).
 * Lanzado: proyectil replicado; al golpear hace ruido (AI Hearing) y puede activar trampas.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabThrowable : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabThrowable();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

	/** Solo servidor: lanzarlo con esta velocidad. */
	void Launch(const FVector& Velocity, AActor* Thrower);

	UFUNCTION(BlueprintPure, Category = "Lab|Throw")
	bool IsInFlight() const { return bInFlight; }

	/** Lo fija quien spawnea (antes de FinishSpawning). */
	UPROPERTY(ReplicatedUsing = OnRep_Kind, BlueprintReadOnly, Category = "Lab|Throw")
	ELabThrowKind Kind = ELabThrowKind::Stone;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Throw|Audio")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Throw|Audio")
	TObjectPtr<USoundBase> ShatterSound;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Throw")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Throw")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Throw")
	TObjectPtr<UProjectileMovementComponent> Projectile;

	UPROPERTY(Replicated)
	bool bInFlight = false;

	UFUNCTION()
	void OnRep_Kind();

	UFUNCTION()
	void OnBounce(const FHitResult& Hit, const FVector& ImpactVelocity);

	UFUNCTION()
	void OnStop(const FHitResult& Hit);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpact(FVector Location, bool bShatter);

	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;

private:
	void MakeNoiseAt(const FVector& Location, float Strength);
	bool bImpacted = false;
};
