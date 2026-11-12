#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interact/LabInteractable.h"
#include "LabHidingSpot.generated.h"

class UStaticMeshComponent;
class ALabCharacter;

UENUM(BlueprintType)
enum class ELabHidingKind : uint8
{
	Barrel,
	Hay,
	Niche,
};

/**
 * Escondite (barril, paja o nicho). E para entrar/salir. Escondido: no te movés, tu cuerpo no se ve,
 * el Minotauro no te detecta (salvo que tengas un farol encendido) y tu voz hace menos ruido.
 * Una persona por escondite. Meshes provisorios: un BP hijo puede poner los del kit medieval.
 */
UCLASS(Blueprintable)
class LABRYNN_API ALabHidingSpot : public AActor, public ILabInteractable
{
	GENERATED_BODY()

public:
	ALabHidingSpot();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract(const ALabCharacter* By) const override;
	virtual void Interact(ALabCharacter* By) override;
	virtual FText GetInteractPrompt(const ALabCharacter* By) const override;

	/** Solo servidor (lo llama el personaje al salir o al caer). */
	void ClearOccupant(ALabCharacter* Who);

	ALabCharacter* GetOccupant() const { return Occupant; }

	/** Dónde queda el que sale (delante del escondite). */
	FVector GetExitLocation() const;

	/** Lo fija quien spawnea (antes de FinishSpawning). Replicado. */
	UPROPERTY(ReplicatedUsing = OnRep_Kind, BlueprintReadOnly, Category = "Lab|Hide")
	ELabHidingKind Kind = ELabHidingKind::Barrel;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Hide")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lab|Hide")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(Replicated)
	TObjectPtr<ALabCharacter> Occupant;

	UFUNCTION()
	void OnRep_Kind();

	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
};
