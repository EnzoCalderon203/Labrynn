#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interact/LabInteractable.h"
#include "Interact/LabThrowable.h"
#include "Audio/LabAudioSettings.h"
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

/** Estado replicado de "me están subiendo de la mano" (se interpola igual en servidor y cliente dueño). */
USTRUCT()
struct FLabPullState
{
	GENERATED_BODY()

	UPROPERTY() bool bActive = false;
	UPROPERTY() FVector_NetQuantize From;
	UPROPERTY() FVector_NetQuantize To;
	UPROPERTY() float StartServerTime = 0.f;
	UPROPERTY() float Duration = 1.5f;
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


public:
	ALabCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Farol que lleva en la mano (o null). Lo fija ALabLantern en todas las máquinas. */
	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	class ALabLantern* GetHeldLantern() const;

	void SetHeldLantern(class ALabLantern* Lantern);

	/** ¿Tiene un farol encendido en la mano? */
	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	bool HasLitLantern() const;

	UFUNCTION(BlueprintPure, Category = "Lab|Voice")
	bool IsTalking() const { return bTalking; }

	/** Solo servidor: el sonido suena en todas las máquinas, en la posición del personaje. */
	void PlaySfxAll(ELabSfx Id);

	/** Solo servidor: estadísticas de la ronda en el PlayerState (premios). nullptr si no hay. */
	struct FLabStats* GetStatsServer() const;

	// --- Lanzables (piedras y botellas) ---
	bool CanCarryMoreThrowables() const;
	/** Solo servidor. */
	void AddThrowable(ELabThrowKind Kind);
	const TArray<ELabThrowKind>& GetThrowables() const { return Throwables; }
	bool IsAiming() const { return bAiming; }
	const TArray<FVector>& GetAimPath() const { return AimPath; }

	// --- Cargar a un caído ---
	UFUNCTION(BlueprintPure, Category = "Lab|Help")
	bool IsCarryingBody() const { return CarriedBody != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Lab|Help")
	bool IsBeingCarried() const { return CarriedBy != nullptr; }

	/** Solo servidor. */
	void StartCarry(ALabCharacter* Body);
	/** Solo servidor. Suelta al cargado en el piso (sigue caído). */
	void DropCarried();

	// --- Pie de banco y dar la mano ---
	UFUNCTION(BlueprintPure, Category = "Lab|Help")
	bool IsReaching() const { return bReaching; }

	UFUNCTION(BlueprintPure, Category = "Lab|Help")
	bool IsBeingPulled() const { return Pull.bActive; }

	/** Punto de la mano extendida (mundo). */
	FVector GetHandPoint() const;

	// --- Escondites ---
	/** Solo servidor (lo llama ALabHidingSpot). */
	void EnterHiding(class ALabHidingSpot* Spot);
	/** Solo servidor. */
	void ExitHiding();

	UFUNCTION(BlueprintPure, Category = "Lab|Hide")
	bool IsHiding() const { return bHiding; }

	/** ¿El Minotauro puede detectarlo/atacarlo? (escondido sin farol encendido = no) */
	bool IsDetectableByMinotaur() const;

	const class ULabGameplaySettings* GetGameplaySettings() const;

	/** Último volumen de ruido de voz emitido (servidor; para pruebas/depuración). */
	float LastVoiceNoiseLevel = 0.f;

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

	// Acciones propias (IMC_Labrynn, lo agrega ALabPlayerController). Assets en /Game/Labrynn/Core/Input.
	UPROPERTY(EditAnywhere, Category = "Input|Labrynn")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input|Labrynn")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input|Labrynn")
	TObjectPtr<UInputAction> LanternAction;

	UPROPERTY(EditAnywhere, Category = "Input|Labrynn")
	TObjectPtr<UInputAction> TalkAction;

	UPROPERTY(EditAnywhere, Category = "Input|Labrynn")
	TObjectPtr<UInputAction> CrouchAction;

	void CrouchPressed();
	void CrouchReleased();

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

	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

	// --- Farol (tecla F: apagar / reencender el que llevo) ---
	void LanternInput();

	UFUNCTION(Server, Reliable)
	void ServerLanternToggle();

	/** Hasta qué hora del servidor está en una corriente de aire (no se puede encender). */
	UPROPERTY(Replicated)
	float GasUntilServerTime = 0.f;

	// Lanzables: inventario (solo lo ve el dueño), apuntar (click izq. mantenido) y lanzar (soltar).
	UPROPERTY(Replicated)
	TArray<ELabThrowKind> Throwables;

	UPROPERTY(EditAnywhere, Category = "Input|Labrynn")
	TObjectPtr<UInputAction> ThrowAction;

	// Ping: la rueda del mouse marca el punto que mirás (lo ven todos unos segundos).
	UPROPERTY(EditAnywhere, Category = "Input|Labrynn")
	TObjectPtr<UInputAction> PingAction;

	void PingPressed();

	UFUNCTION(Server, Reliable)
	void ServerPing(FVector Location);

	float LastPingServerTime = -100.f;

	void ThrowPressed();
	void ThrowReleased();
	void TickAim();

	UFUNCTION(Server, Reliable)
	void ServerThrow(FVector Origin, FVector Velocity);

	bool bAiming = false;
	TArray<FVector> AimPath;
	FVector GetThrowOrigin() const;
	FVector GetThrowVelocity() const;

	// Cargar a un caído.
	UPROPERTY(Replicated)
	TObjectPtr<ALabCharacter> CarriedBody;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedBy)
	TObjectPtr<ALabCharacter> CarriedBy;

	UFUNCTION()
	void OnRep_CarriedBy();
	void ApplyCarriedPose();

	UFUNCTION(Server, Reliable)
	void ServerStartCarry(ALabCharacter* Body);
	void TickCarryHold();

	/** E mantenida (cliente): para distinguir tocar (levantar) de mantener (cargar). */
	bool bInteractHeld = false;
	float InteractPressTime = 0.f;
	/** Servidor: cuándo empezó a cargar (una E que llega enseguida, por lag, no lo suelta). */
	float CarryStartTime = -100.f;
	TWeakObjectPtr<ALabCharacter> InteractHoldTarget;

	// Dar la mano (arriba: mantiene E; abajo: salta + E).
	UPROPERTY(ReplicatedUsing = OnRep_Reaching)
	bool bReaching = false;

	UFUNCTION()
	void OnRep_Reaching();

	UFUNCTION(Server, Reliable)
	void ServerSetReaching(bool bReach);

	void InteractReleased();
	/** ¿Puedo estirar la mano? (en un borde, mirando hacia abajo, con desnivel adelante) */
	bool CanStartReach() const;

	UPROPERTY(ReplicatedUsing = OnRep_Pull)
	FLabPullState Pull;

	UFUNCTION()
	void OnRep_Pull();
	void ApplyPull();
	void StartPull(ALabCharacter* Lower);
	void EndPull(bool bSuccess);
	void TickHelp(float DeltaSeconds);

	TWeakObjectPtr<ALabCharacter> Pulling;   // (arriba) a quién estoy subiendo
	TWeakObjectPtr<ALabCharacter> PulledBy;  // (abajo) quién me sube

	UFUNCTION(Client, Reliable)
	void ClientLaunch(FVector Velocity);
	void BoostIgnorePawns();

	/** Brazo extendido (provisorio: cilindro), visible para todos incluido el dueño. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ReachArm;

	// --- Audio (los sonidos viven en DA_Audio; ver ULabAudio) ---
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSfx(ELabSfx Id, FVector_NetQuantize Location);

	/** Pasos, salto y aterrizaje: cosmético, en cada máquina, desde el movimiento replicado. */
	void TickFootsteps(float DeltaSeconds);
	float StepDistance = 0.f;
	bool bWasFalling = false;
	float LastFallSpeed = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Hiding)
	bool bHiding = false;

	UFUNCTION()
	void OnRep_Hiding();
	void ApplyHiding();

	TWeakObjectPtr<class ALabHidingSpot> HidingSpot;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Gameplay")
	TObjectPtr<class ULabGameplaySettings> GameplaySettings;

	// --- Voz por proximidad (V = push-to-talk) ---
	void TalkPressed();
	void TalkReleased();

	UFUNCTION(Server, Reliable)
	void ServerSetTalking(bool bTalk);

	/** Está hablando (replicado: HUD y ruido para el Minotauro). */
	UPROPERTY(Replicated)
	bool bTalking = false;

	/** Voz 3D de ESTE personaje en las otras máquinas (se crea cuando hay PlayerState). */
	UPROPERTY(Transient)
	TObjectPtr<class UVOIPTalker> VoiceTalker;

	/** Alcance de la voz (cm); más allá no se oye. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Voice")
	float VoiceRange = 2000.f;

	/** Alcance del ruido de la voz para la IA, a volumen 1 (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Voice")
	float VoiceNoiseRange = 2500.f;

	void TickVoice(float DeltaSeconds);
	float NextVoiceNoise = 0.f;
	FString LastVoiceFxLabel;

	/** Brillo personal tenue (todos lo tienen): permite ver la silueta de los compañeros cercanos. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPointLightComponent> PersonalGlow;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light")
	float PersonalGlowIntensity = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lab|Light")
	float PersonalGlowRadius = 220.f;

	/** Visión mínima: solo existe en la máquina del dueño. */
	UPROPERTY(Transient)
	TObjectPtr<class UPointLightComponent> LocalVisionLight;

	// Agachado visual: la cámara y los brazos bajan (local) y el cuerpo se ve más bajo (los demás).
	void TickCrouchVisual(float DeltaSeconds);
	float CrouchVisualAlpha = 0.f;
	FVector FirstPersonMeshBaseRel = FVector::ZeroVector;
	FVector BodyBaseScale = FVector::OneVector;

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

	/** Lo llama la reliquia en todas las máquinas. (La velocidad sale de ULabCharacterMovement.) */
	void SetCarriedRelic(class ALabRelic* Relic, float SpeedMultiplier);

	UFUNCTION(BlueprintPure, Category = "Lab|Relic")
	bool IsCarryingRelic() const { return CarriedRelic.IsValid(); }

	/** Vivo y no caído. */
	UFUNCTION(BlueprintPure, Category = "Lab|State")
	bool IsAliveAndFree() const { return LifeState == ELabLifeState::Alive; }

	UFUNCTION(BlueprintPure, Category = "Lab|State")
	ELabLifeState GetLifeState() const { return LifeState; }

	// --- Stamina y heridas (una sola barra, estilo Peak) ---

	/** Stamina actual 0..1 (relativa a MaxStamina). */
	UFUNCTION(BlueprintPure, Category = "Lab|Stamina")
	float GetStamina01() const { return StaminaByte / 255.f; }

	/** Heridas 0..1 (relativas a MaxStamina): bloquean la parte de arriba de la barra. */
	UFUNCTION(BlueprintPure, Category = "Lab|Stamina")
	float GetWounds01() const { return WoundsByte / 255.f; }

	/** Jadeando: stamina agotada, lento, hace ruido. */
	UFUNCTION(BlueprintPure, Category = "Lab|Stamina")
	bool IsExhausted() const { return bExhausted; }

	/** ¿Puede correr ahora? (vivo, no jadea, tiene stamina). */
	bool CanSprint() const;

	/** Velocidad máxima según estado. La usa ULabCharacterMovement (cliente y servidor). */
	float GetDesiredMaxSpeed(bool bWantsSprint) const;

	/** Solo servidor. Suma heridas; si llenan la barra, cae. Impulse = empujón (lanzamiento o ragdoll). */
	void AddWound(float Amount, const FVector& Impulse = FVector::ZeroVector);

	/** Solo servidor. Cura heridas (botiquín). */
	void HealWounds(float Amount);

	/** Solo servidor. Sin heridas y stamina llena (Corazón del Laberinto). */
	void RestoreAll();

	const class ULabStaminaSettings* GetStaminaSettings() const;

	/** 0..1 mientras alguien lo está levantando. */
	UFUNCTION(BlueprintPure, Category = "Lab|State")
	float GetReviveProgress() const { return ReviveByte / 255.f; }

	/** Segundos restantes antes de morir desangrado (solo válido si está caído). */
	UFUNCTION(BlueprintPure, Category = "Lab|State")
	float GetBleedOutRemaining() const;

	/** Solo servidor. Lo deja caído (trampas, caídas). Impulse = empujón cosmético del ragdoll. */
	void KnockDown(const FVector& Impulse = FVector::ZeroVector);

	/** Solo servidor. Lo levanta (rescate o Corazón). WoundsAfter < 0: usa WoundsAfterRevive del Data Asset. Sirve para muertos. */
	void Revive(float WoundsAfter = -1.f);

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ILabInteractable: levantar a un compañero caído.
	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

protected:
	/** Valores de stamina/heridas/velocidades. Por defecto /Game/Labrynn/Core/DA_Stamina. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Stamina")
	TObjectPtr<class ULabStaminaSettings> StaminaSettings;

	UPROPERTY(Replicated)
	uint8 StaminaByte = 255;

	UPROPERTY(Replicated)
	uint8 WoundsByte = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Exhausted)
	bool bExhausted = false;

	UFUNCTION()
	void OnRep_Exhausted();

	/** Shift: correr (va dentro de los saved moves del movimiento, ver ULabCharacterMovement). */
	void SprintPressed();
	void SprintReleased();

	/** Respiración agitada (loop mientras jadea). */
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> BreathAudio;

	/** Segundos caído antes de morir. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|State")
	float BleedOutSeconds = 30.f;

	/** Segundos que tarda un compañero en levantarte. */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|State")
	float ReviveSeconds = 3.f;

	UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category = "Lab|State")
	ELabLifeState LifeState = ELabLifeState::Alive;

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

	// Stamina (precisa, solo servidor; los Byte son la versión replicada).
	void TickStamina(float DeltaSeconds);
	void SpendStamina(float Amount);
	void SyncStaminaBytes();
	void ApplyExhaustedEffects();
	float StaminaServer = 100.f;
	float WoundsServer = 0.f;
	float LastSpendTime = -100.f;
	float ExhaustedUntil = 0.f;
	float NextBreathNoise = 0.f;
	float NextSprintNoise = 0.f;

	// Ojos (solo jugador local): adaptación lenta a la oscuridad y encandilamiento breve.
	void TickEyes(float DeltaSeconds);
	float DarkAdapt = 0.f;      // 0 = ojos de luz, 1 = totalmente adaptados a la oscuridad
	float GlareTime = 0.f;      // segundos de encandilamiento restantes
	float GlareStrength = 0.f;
	bool bWasGlaring = false;
	bool bGlowFromSettings = false;
	float NextEyeLog = 0.f;

public:
	/** Solo servidor. Corriente de aire: apaga el farol que lleva y no deja reencenderlo durante Seconds. */
	void ApplyGas(float Seconds);

	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	bool IsInGas() const;

private:
	TWeakObjectPtr<class ALabRelic> CarriedRelic;
	TWeakObjectPtr<class ALabLantern> HeldLantern;
};