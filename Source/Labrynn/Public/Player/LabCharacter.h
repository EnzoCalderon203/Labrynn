#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interact/LabInteractable.h"
#include "LabCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpotLightComponent;
class UInputAction;
struct FInputActionValue;

/**
 * Personaje jugable de Labrynn (primera persona, multijugador).
 *
 * Basado en el personaje de la plantilla First Person de UE 5.8:
 *  - FirstPersonMesh: brazos, solo los ve el dueño.
 *  - GetMesh(): cuerpo completo, lo ven los demás (OwnerNoSee).
 * BP_FirstPersonCharacter (contenido de la plantilla) hereda de esta clase vía CoreRedirects
 * y asigna meshes, animaciones e input. Los nombres de propiedades/componentes se mantienen
 * iguales a la plantilla para que esos valores se conserven.
 */
UENUM(BlueprintType)
enum class ELabLifeState : uint8
{
	Alive,
	Downed UMETA(DisplayName = "Caído"),  // ragdoll; un compañero lo puede levantar
	Dead   UMETA(DisplayName = "Muerto"),  // solo lo revive el Corazón del Laberinto
};

UCLASS()
class LABRYNN_API ALabCharacter : public ACharacter, public ILabInteractable
{
	GENERATED_BODY()

	/** Brazos en primera persona (solo el dueño). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	/** Linterna. Visible para todos (la luz de un compañero también te ilumina). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpotLightComponent> Flashlight;

public:
	ALabCharacter();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	bool IsFlashlightOn() const { return bFlashlightOn; }

protected:
	// --- Input (mismos nombres que la plantilla: el Blueprint ya trae los assets asignados) ---
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MouseLookAction;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void MoveInput(const FInputActionValue& Value);
	void LookInput(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoAim(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	// --- Linterna (replicada) ---
	/** Tecla F: alterna la linterna. Se pide al servidor, que replica el estado. */
	void ToggleFlashlightInput();

	UFUNCTION(Server, Reliable)
	void ServerSetFlashlight(bool bOn);

	UFUNCTION()
	void OnRep_FlashlightOn();

	UPROPERTY(ReplicatedUsing = OnRep_FlashlightOn, BlueprintReadOnly, Category = "Lab|Light")
	bool bFlashlightOn = true;

	/** Intensidad de la linterna (unidades del componente). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light")
	float FlashlightIntensity = 3000.f;

	/** Alcance de la linterna en cm. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light")
	float FlashlightRange = 1600.f;

	// --- Luz compartida: pocas linternas, batería limitada ---

	/** ¿Este jugador tiene linterna? Lo decide el GameMode (por defecto, la mitad del grupo). */
	UPROPERTY(ReplicatedUsing = OnRep_FlashlightOn, BlueprintReadOnly, Category = "Lab|Light")
	bool bHasFlashlight = false;

	/** Hasta qué hora del servidor está gaseado. */
	UPROPERTY(Replicated)
	float GasUntilServerTime = 0.f;

	/** Batería 0..255 (replicada a todos para que vean la linterna del otro debilitarse). */
	UPROPERTY(Replicated)
	uint8 BatteryByte = 255;

	/** Segundos que dura una batería llena con la linterna prendida. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light", meta = (ClampMin = 10))
	float BatterySeconds = 240.f;

	/** Por debajo de esta carga la linterna parpadea y pierde intensidad. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float LowBatteryThreshold = 0.25f;

	/** Brillo personal tenue (todos lo tienen): permite ver la silueta de los compañeros cercanos. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPointLightComponent> PersonalGlow;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light")
	float PersonalGlowIntensity = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light")
	float PersonalGlowRadius = 220.f;

	// --- Interacción (tecla E) y reliquia ---

	/** Distancia máxima para interactuar (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Interact")
	float InteractRange = 220.f;

	/** Distancia máxima para pasar la reliquia a un compañero (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Interact")
	float PassRange = 260.f;

	void InteractInput();

	UFUNCTION(Server, Reliable)
	void ServerInteract();

public:
	virtual void Tick(float DeltaSeconds) override;

	/** El interactuable más conveniente frente a la cámara (o null). Sirve en cliente (HUD) y servidor. */
	AActor* FindBestInteractable() const;

	/** Compañero vivo y libre más cercano frente a mí, dentro de PassRange (o null). */
	ALabCharacter* FindTeammateInFront() const;

	/** Lo llama la reliquia en todas las máquinas. Ajusta velocidad y bloquea la linterna. */
	void SetCarriedRelic(class ALabRelic* Relic, float SpeedMultiplier);

	UFUNCTION(BlueprintPure, Category = "Lab|Relic")
	bool IsCarryingRelic() const { return CarriedRelic.IsValid(); }

	/** Vivo y no caído. */
	UFUNCTION(BlueprintPure, Category = "Lab|State")
	bool IsAliveAndFree() const { return LifeState == ELabLifeState::Alive; }

	UFUNCTION(BlueprintPure, Category = "Lab|State")
	ELabLifeState GetLifeState() const { return LifeState; }

	UFUNCTION(BlueprintPure, Category = "Lab|State")
	float GetHealth() const { return Health; }

	/** 0..1 mientras alguien lo está levantando. */
	UFUNCTION(BlueprintPure, Category = "Lab|State")
	float GetReviveProgress() const { return ReviveByte / 255.f; }

	/** Segundos restantes antes de morir desangrado (solo válido si está caído). */
	UFUNCTION(BlueprintPure, Category = "Lab|State")
	float GetBleedOutRemaining() const;

	/** Solo servidor. Lo deja caído (trampas, caídas). Impulse = empujón cosmético del ragdoll. */
	void KnockDown(const FVector& Impulse = FVector::ZeroVector);

	/** Solo servidor. Lo levanta con HealthAfter de salud (rescate o Corazón). Sirve también para muertos. */
	void Revive(float HealthAfter = 50.f);

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ILabInteractable: levantar a un compañero caído.
	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Lab|State")
	float MaxHealth = 100.f;

	/** Segundos caído antes de morir. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|State")
	float BleedOutSeconds = 30.f;

	/** Segundos que tarda un compañero en levantarte. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|State")
	float ReviveSeconds = 3.f;

	UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category = "Lab|State")
	ELabLifeState LifeState = ELabLifeState::Alive;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lab|State")
	float Health = 100.f;

	UPROPERTY(Replicated)
	uint8 ReviveByte = 0;

	/** Hora del servidor en que cayó (para el contador de desangrado). */
	UPROPERTY(Replicated)
	float DownedAtServerTime = 0.f;

	/** Empujón del ragdoll (replicado para que todos lo vean salir volando parecido). */
	UPROPERTY(Replicated)
	FVector_NetQuantize10 DownImpulse;

	UFUNCTION()
	void OnRep_LifeState(ELabLifeState OldState);

private:
	void ApplyLifeState(ELabLifeState OldState);
	void StartRagdoll();
	void StopRagdoll();
	void TickRevive(float DeltaSeconds);

	TWeakObjectPtr<ALabCharacter> Reviver;
	float ReviveProgressServer = 0.f;
	FTransform MeshRelativeTransform;
	FTransform CameraRelativeTransform;
	bool bRagdoll = false;
	EVisibilityBasedAnimTickOption SavedTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

public:
	/** Solo servidor. */
	void SetHasFlashlight(bool bHas);

	/** Solo servidor. Suma batería (Amount en 0..1). Para pickups de baterías. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lab|Light")
	void AddBattery(float Amount);

	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	bool HasFlashlight() const { return bHasFlashlight; }

	/** Solo servidor. Gas: apaga la linterna y no deja prenderla durante Seconds (se renueva mientras siga en la nube). */
	void ApplyGas(float Seconds);

	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	bool IsInGas() const;

	/** 0..1 */
	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	float GetBattery() const { return BatteryByte / 255.f; }

private:
	void ApplyFlashlightState();
	void UpdateFlashlightIntensity(float DeltaSeconds);

	TWeakObjectPtr<class ALabRelic> CarriedRelic;
	float BaseWalkSpeed = 450.f;

	/** Batería precisa (solo servidor); BatteryByte es su versión replicada. */
	float BatteryServer = 1.f;
	float FlickerTimer = 0.f;
	float FlickerFactor = 1.f;
};
