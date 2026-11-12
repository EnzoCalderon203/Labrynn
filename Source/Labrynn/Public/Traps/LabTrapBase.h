#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LabTrapBase.generated.h"

class UBoxComponent;
class USoundBase;
class ALabCharacter;

UENUM(BlueprintType)
enum class ELabTrapState : uint8
{
	Armed,      // esperando
	Warning,    // aviso: sonido/visual, todavía hay tiempo de escapar
	Firing,     // dispara
	Cooldown,   // se está reseteando (o queda abierta para siempre)
};

/**
 * Base de trampas: estado replicado (RepNotify) con aviso previo obligatorio.
 * El servidor detecta quién pisa el disparador y avanza el estado con timers;
 * todas las máquinas reaccionan en OnStateChanged (visual + sonido).
 * Los sonidos se asignan en un Blueprint hijo (Luis).
 */
UCLASS(Abstract, Blueprintable)
class LABRYNN_API ALabTrapBase : public AActor
{
	GENERATED_BODY()

public:
	ALabTrapBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Lab|Trap")
	ELabTrapState GetTrapState() const { return State; }

	/** Tamaño de la celda (cm), para escalar visuales y disparador. Lo fija quien spawnea. */
	float CellSize = 400.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<USceneComponent> Root;

	/** Zona que dispara la trampa al pisarla. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UBoxComponent> Trigger;

	/** Segundos entre el aviso y el disparo (tiempo para reaccionar). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap", meta = (ClampMin = 0.1))
	float WarningTime = 0.6f;

	/** Segundos en estado Firing. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap", meta = (ClampMin = 0.1))
	float FiringTime = 1.2f;

	/** Segundos de cooldown antes de rearmarse. <= 0: no se rearma nunca. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap")
	float CooldownTime = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap|Audio")
	TObjectPtr<USoundBase> WarnSound;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap|Audio")
	TObjectPtr<USoundBase> ResetSound;

	/** Visual/sonido según el estado. Corre en todas las máquinas. */
	virtual void OnStateChanged(ELabTrapState OldState);

	/** Solo servidor: efecto del disparo sobre los personajes en la zona. */
	virtual void ServerFire() {}

	/** Personajes vivos dentro del disparador ahora. */
	TArray<ALabCharacter*> GetCharactersInside() const;

	void SetState(ELabTrapState NewState);

private:
	UPROPERTY(ReplicatedUsing = OnRep_State)
	ELabTrapState State = ELabTrapState::Armed;

	UFUNCTION()
	void OnRep_State(ELabTrapState OldState);

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	FTimerHandle StepTimer;
};
