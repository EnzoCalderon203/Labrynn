#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LabPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 * PlayerController de Labrynn. Agrega los Input Mapping Contexts del jugador local.
 * BP_FirstPersonPlayerController (plantilla) hereda de esta clase vía CoreRedirects
 * y ya trae asignados IMC_Default / IMC_MouseLook.
 */
UCLASS(config = "Game")
class LABRYNN_API ALabPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALabPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Sale de la sesión y vuelve al menú (tecla Esc). */
	UFUNCTION(BlueprintCallable, Category = "Lab|Online")
	void ReturnToMenu();

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

	/** Se mantiene por compatibilidad con el Blueprint de la plantilla (no se usa en PC). */
	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

#if !UE_BUILD_SHIPPING
public:
	/** Depuración: -LabAutoRunFrom=N -LabAutoRunTo=M corre ida y vuelta con Shift entre esos segundos (input real del jugador). */
	virtual void PlayerTick(float DeltaTime) override;
private:
	float AutoRunFrom = -1.f, AutoRunTo = -1.f;
	/** Depuración: -LabAutoShot=N saca captura a los N s (mirando al compañero más cercano); -LabAutoQuit cierra después. */
	void SetupAutoShot();
	void DoAutoShot();
	FTimerHandle AutoShotTimer;
	FTimerHandle AutoQuitTimer;
#endif
};
