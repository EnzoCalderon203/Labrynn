#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
UCLASS()
class LABRYNN_API ALabCharacter : public ACharacter
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

public:
	virtual void Tick(float DeltaSeconds) override;

	/** Solo servidor. */
	void SetHasFlashlight(bool bHas);

	/** Solo servidor. Suma batería (Amount en 0..1). Para pickups de baterías. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lab|Light")
	void AddBattery(float Amount);

	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	bool HasFlashlight() const { return bHasFlashlight; }

	/** 0..1 */
	UFUNCTION(BlueprintPure, Category = "Lab|Light")
	float GetBattery() const { return BatteryByte / 255.f; }

private:
	void ApplyFlashlightState();
	void UpdateFlashlightIntensity(float DeltaSeconds);

	/** Batería precisa (solo servidor); BatteryByte es su versión replicada. */
	float BatteryServer = 1.f;
	float FlickerTimer = 0.f;
	float FlickerFactor = 1.f;
};
