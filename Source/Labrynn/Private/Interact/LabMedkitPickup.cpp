#include "Interact/LabMedkitPickup.h"
#include "Audio/LabAudio.h"

#include "Player/LabCharacter.h"
#include "Player/LabStaminaSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LabMedkit"

ALabMedkitPickup::ALabMedkitPickup()
{
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mesh->SetRelativeScale3D(FVector(0.3f, 0.22f, 0.12f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) { Mesh->SetStaticMesh(Cube.Object); }

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Mesh);
	Glow->SetLightColor(FLinearColor(1.f, 0.95f, 0.95f));
	Glow->SetIntensity(90.f);
	Glow->SetAttenuationRadius(150.f);
	Glow->SetCastShadows(false);
}

bool ALabMedkitPickup::CanInteract(const ALabCharacter* By) const
{
	return By && By->IsAliveAndFree() && By->GetWounds01() > 0.f && !IsHidden() && !IsActorBeingDestroyed();
}

FText ALabMedkitPickup::GetInteractPrompt(const ALabCharacter* By) const
{
	return LOCTEXT("Use", "Usar botiquín (cura heridas)");
}

void ALabMedkitPickup::Interact(ALabCharacter* By)
{
	if (!HasAuthority() || !CanInteract(By))
	{
		return;
	}
	By->HealWounds(By->GetStaminaSettings()->MedkitHeal);
	MulticastPickedUp();
	SetLifeSpan(0.2f);
	SetActorHiddenInGame(true);
}

void ALabMedkitPickup::MulticastPickedUp_Implementation()
{
	SetActorHiddenInGame(true);
	ULabAudio::Play(this, ELabSfx::MedkitPickup, GetActorLocation());
}

#undef LOCTEXT_NAMESPACE
