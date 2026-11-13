#pragma once

#include "CoreMinimal.h"
#include "Traps/LabTrapBase.h"
#include "LabPitTrap.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInterface;

/**
 * Piso falso sobre un foso (en callejones sin salida, ver ALabMaze::PitCells).
 * Aviso: el piso tiembla y cruje. Después se abre y queda abierto para siempre.
 * Quien cae al fondo queda caído/atrapado; un compañero lo saca desde el borde (al levantarlo
 * sale a la celda vecina, ver ALabCharacter::Revive).
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabPitTrap : public ALabTrapBase
{
	GENERATED_BODY()

public:
	virtual ELabSfx GetFireSfx() const override { return ELabSfx::PitFire; }

	ALabPitTrap();
	virtual void Tick(float DeltaSeconds) override;

	/** Lo fija quien spawnea (igual a ALabMaze::PitDepth). */
	float PitDepth = 260.f;

	/** Material del piso falso (igual al piso del laberinto, para que no se note). */
	UPROPERTY(EditDefaultsOnly, Category = "Lab|Trap")
	TObjectPtr<UMaterialInterface> FakeFloorMaterial;

protected:
	virtual void BeginPlay() override;
	virtual void OnStateChanged(ELabTrapState OldState) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UStaticMeshComponent> FakeFloor;

	/** Fondo del foso: quien llega acá queda caído. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Trap")
	TObjectPtr<UBoxComponent> Bottom;

private:
	UFUNCTION()
	void OnBottomOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	float ShakeTime = 0.f;
};
