#include "Interact/LabBatteryPickup.h"

#include "Player/LabCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LabBattery"

ALabBatteryPickup::ALabBatteryPickup()
{
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.25f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cyl.Succeeded()) { Mesh->SetStaticMesh(Cyl.Object); }

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Mesh);
	Glow->SetLightColor(FLinearColor(0.4f, 1.f, 0.35f));
	Glow->SetIntensity(120.f);
	Glow->SetAttenuationRadius(160.f);
	Glow->SetCastShadows(false);
}

bool ALabBatteryPickup::CanInteract(const ALabCharacter* By) const
{
	return By && By->IsAliveAndFree() && By->HasFlashlight() && By->GetBattery() < 0.99f && !IsHidden() && !IsActorBeingDestroyed();
}

FText ALabBatteryPickup::GetInteractPrompt(const ALabCharacter* By) const
{
	return FText::Format(LOCTEXT("Take", "Agarrar batería (+{0}%)"), FText::AsNumber(FMath::RoundToInt(Charge * 100.f)));
}

void ALabBatteryPickup::Interact(ALabCharacter* By)
{
	if (!HasAuthority() || !CanInteract(By))
	{
		return;
	}
	By->AddBattery(Charge);
	MulticastPickedUp();
	SetLifeSpan(0.2f); // dar tiempo a que salga el multicast
	SetActorHiddenInGame(true);
}

void ALabBatteryPickup::MulticastPickedUp_Implementation()
{
	SetActorHiddenInGame(true);
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
}

#undef LOCTEXT_NAMESPACE
