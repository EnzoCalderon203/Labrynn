#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabLantern.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;
class ALabCharacter;

/**
 * Farol medieval: la única fuente de luz "fuerte" del grupo. Luz cálida omnidireccional con parpadeo de llama
 * y radio limitado (el grupo tiene que ir junto). Gasta aceite mientras está encendido.
 * - E: tomarlo (si no lo lleva nadie), pasarlo al compañero de enfrente o soltarlo (sigue iluminando en el piso).
 * - F (quien lo lleva): apagar al instante; reencender tarda RelightSeconds.
 * - Corrientes de aire (trampa) apagan la llama.
 * Todo con autoridad del servidor; portador, encendido y aceite replicados.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabLantern : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabLantern();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// ILabInteractable
	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

	/** Solo servidor. nullptr = soltarlo en el piso delante del portador anterior. */
	void SetHolder(ALabCharacter* NewHolder);

	/** Solo servidor. F del portador: apaga, o empieza a reencender. */
	void ToggleByHolder(ALabCharacter* By);

	/** Solo servidor. Apaga la llama (corriente de aire). */
	void Extinguish();

	/** Solo servidor. Suma aceite (0..1). */
	void AddOil(float Amount);

	/** Solo servidor. Aceite lleno y encendido (Corazón del Laberinto). */
	void Refill();

	UFUNCTION(BlueprintPure, Category = "Lab|Lantern")
	ALabCharacter* GetHolder() const { return Holder; }

	UFUNCTION(BlueprintPure, Category = "Lab|Lantern")
	bool IsLit() const { return bLit; }

	UFUNCTION(BlueprintPure, Category = "Lab|Lantern")
	bool IsRelighting() const { return bRelighting; }

	/** 0..1 */
	UFUNCTION(BlueprintPure, Category = "Lab|Lantern")
	float GetOil() const { return OilByte / 255.f; }

	/** 0..1 del reencendido en curso (para el HUD). */
	float GetRelightProgress() const;

	// --- Ajustes (Blueprint hijo) ---
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Lantern")
	float LightIntensity = 900.f;

	/** Radio de la luz (cm): limitado a propósito. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Lantern")
	float LightRadius = 650.f;

	/** Segundos de aceite lleno encendido. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Lantern", meta = (ClampMin = 10))
	float OilSeconds = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Lantern", meta = (ClampMin = 0))
	float RelightSeconds = 3.f;

	/** Dónde se sostiene, relativo a la cápsula del portador. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Lantern")
	FVector HoldOffset = FVector(28.f, 34.f, -30.f);

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Lantern|Audio")
	TObjectPtr<USoundBase> ExtinguishSound;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Lantern|Audio")
	TObjectPtr<USoundBase> LightSound;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Lantern")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Lantern")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Lantern")
	TObjectPtr<UStaticMeshComponent> Cap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Lantern")
	TObjectPtr<UPointLightComponent> Flame;

	UPROPERTY(ReplicatedUsing = OnRep_Holder)
	TObjectPtr<ALabCharacter> Holder;

	UPROPERTY(ReplicatedUsing = OnRep_Lit)
	bool bLit = true;

	UPROPERTY(Replicated)
	bool bRelighting = false;

	UPROPERTY(Replicated)
	float RelightEndServerTime = 0.f;

	UPROPERTY(Replicated)
	uint8 OilByte = 255;

	UFUNCTION()
	void OnRep_Holder(ALabCharacter* OldHolder);

	UFUNCTION()
	void OnRep_Lit(bool bWasLit);

private:
	void ApplyHolder(ALabCharacter* OldHolder);
	void SetLit(bool bNewLit);
	float ServerNow() const;

	float OilServer = 1.f;
	float FlickerSeed = 0.f;
};
