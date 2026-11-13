#include "Interact/LabOilFlask.h"
#include "Audio/LabAudio.h"

#include "Player/LabCharacter.h"
#include "Interact/LabLantern.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LabOilFlask"

ALabOilFlask::ALabOilFlask()
{
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.18f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) { Mesh->SetStaticMesh(Sphere.Object); }

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Mesh);
	Glow->SetLightColor(FLinearColor(1.f, 0.7f, 0.25f)); // ámbar, como el aceite
	Glow->SetIntensity(110.f);
	Glow->SetAttenuationRadius(160.f);
	Glow->SetCastShadows(false);
}

bool ALabOilFlask::CanInteract(const ALabCharacter* By) const
{
	const ALabLantern* L = By ? By->GetHeldLantern() : nullptr;
	return By && By->IsAliveAndFree() && L && L->GetOil() < 0.99f && !IsHidden() && !IsActorBeingDestroyed();
}

FText ALabOilFlask::GetInteractPrompt(const ALabCharacter* By) const
{
	return FText::Format(LOCTEXT("Take", "Cargar aceite en el farol (+{0}%)"), FText::AsNumber(FMath::RoundToInt(Oil * 100.f)));
}

void ALabOilFlask::Interact(ALabCharacter* By)
{
	if (!HasAuthority() || !CanInteract(By))
	{
		return;
	}
	By->GetHeldLantern()->AddOil(Oil);
	MulticastPickedUp();
	SetLifeSpan(0.2f);
	SetActorHiddenInGame(true);
}

void ALabOilFlask::MulticastPickedUp_Implementation()
{
	SetActorHiddenInGame(true);
	ULabAudio::Play(this, ELabSfx::OilPickup, GetActorLocation());
}

#undef LOCTEXT_NAMESPACE
