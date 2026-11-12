#include "Traps/LabGasTrap.h"

#include "Player/LabCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ALabGasTrap::ALabGasTrap()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Vent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Vent"));
	Vent->SetupAttachment(Root);
	Vent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Vent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.03f));
	Vent->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
	if (Cyl.Succeeded()) { Vent->SetStaticMesh(Cyl.Object); }

	GasLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GasLight"));
	GasLight->SetupAttachment(Root);
	GasLight->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	GasLight->SetLightColor(FLinearColor(0.35f, 1.f, 0.2f));
	GasLight->SetIntensity(0.f);
	GasLight->SetAttenuationRadius(450.f);
	GasLight->SetCastShadows(false);

	WarningTime = 1.0f;
	FiringTime = 6.f;
	CooldownTime = 8.f;
}

void ALabGasTrap::BeginPlay()
{
	// La nube cubre la celda entera y un poco más.
	Trigger->SetBoxExtent(FVector(CellSize * 0.6f, CellSize * 0.6f, 150.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	Super::BeginPlay();
}

void ALabGasTrap::OnStateChanged(ELabTrapState OldState)
{
	Super::OnStateChanged(OldState);
	switch (GetTrapState())
	{
	case ELabTrapState::Warning: GlowTarget = 0.4f; break;
	case ELabTrapState::Firing:  GlowTarget = 1.f; break;
	default:                     GlowTarget = 0.f; break;
	}
}

void ALabGasTrap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Glow = FMath::FInterpTo(Glow, GlowTarget, DeltaSeconds, 3.f);
	const float Wobble = 0.75f + 0.25f * FMath::Sin(GetWorld()->GetTimeSeconds() * (GetTrapState() == ELabTrapState::Warning ? 25.f : 4.f));
	// Tenue a propósito: el gas "apaga la luz", no tiene que iluminar el pasillo.
	GasLight->SetIntensity(Glow * 350.f * Wobble);
}

void ALabGasTrap::ServerFire()
{
	ServerGasPulse();
	GetWorldTimerManager().SetTimer(PulseTimer, this, &ALabGasTrap::ServerGasPulse, 0.25f, true);
}

void ALabGasTrap::ServerGasPulse()
{
	if (GetTrapState() != ELabTrapState::Firing)
	{
		GetWorldTimerManager().ClearTimer(PulseTimer);
		return;
	}
	for (ALabCharacter* Ch : GetCharactersInside())
	{
		Ch->ApplyGas(0.6f); // se renueva mientras siga adentro
	}
}
