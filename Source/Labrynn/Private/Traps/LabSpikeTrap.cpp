#include "Traps/LabSpikeTrap.h"

#include "Player/LabCharacter.h"
#include "Player/LabStaminaSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float SpikeHiddenZ = -70.f;
	constexpr float SpikeUpZ = 10.f;
	constexpr float PlateUpZ = 2.f;
	constexpr float PlateDownZ = -1.5f;
}

ALabSpikeTrap::ALabSpikeTrap()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlateMat(TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"));

	// Placa: apenas distinta del piso (se nota si la mirás con la linterna).
	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(Root);
	Plate->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Plate->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.05f));
	Plate->SetRelativeLocation(FVector(0.f, 0.f, PlateUpZ));
	if (Cube.Succeeded()) { Plate->SetStaticMesh(Cube.Object); }
	if (PlateMat.Succeeded()) { Plate->SetMaterial(0, PlateMat.Object); }

	Spikes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Spikes"));
	Spikes->SetupAttachment(Root);
	Spikes->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Spikes->SetRelativeLocation(FVector(0.f, 0.f, SpikeHiddenZ));
	if (Cone.Succeeded()) { Spikes->SetStaticMesh(Cone.Object); }

	WarnLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WarnLight"));
	WarnLight->SetupAttachment(Root);
	WarnLight->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	WarnLight->SetLightColor(FLinearColor(1.f, 0.1f, 0.05f));
	WarnLight->SetIntensity(0.f);
	WarnLight->SetAttenuationRadius(300.f);
	WarnLight->SetCastShadows(false);

	WarningTime = 0.6f;
	FiringTime = 1.2f;
	CooldownTime = 3.f;
}

void ALabSpikeTrap::BeginPlay()
{
	Super::BeginPlay();

	// Grilla de pinchos que cubre casi toda la celda (pisar al lado de la placa también es peligroso).
	Spikes->ClearInstances();
	const int32 N = 5;
	const float Span = CellSize * 0.8f;
	for (int32 i = 0; i < N; ++i)
	{
		for (int32 j = 0; j < N; ++j)
		{
			const FVector P(-Span * 0.5f + Span * i / (N - 1), -Span * 0.5f + Span * j / (N - 1), 0.f);
			Spikes->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(0.18f, 0.18f, 0.6f)));
		}
	}
	// Simplificación: pisar cualquier parte de la celda activa la trampa, y el daño cubre la misma zona.
	// La placa del centro es la pista visual.
	Trigger->SetBoxExtent(FVector(CellSize * 0.42f, CellSize * 0.42f, 60.f));
}

void ALabSpikeTrap::OnStateChanged(ELabTrapState OldState)
{
	Super::OnStateChanged(OldState);
	switch (GetTrapState())
	{
	case ELabTrapState::Warning: PlateAlpha = 1.f; FlashTime = WarningTime; break;
	case ELabTrapState::Firing:  SpikeTarget = 1.f; break;
	case ELabTrapState::Cooldown: SpikeTarget = 0.f; break;
	case ELabTrapState::Armed:   PlateAlpha = 0.f; SpikeTarget = 0.f; break;
	}
}

void ALabSpikeTrap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Pinchos: suben rápido, bajan lento.
	const float Speed = SpikeTarget > SpikeAlpha ? 12.f : 1.5f;
	SpikeAlpha = FMath::FInterpConstantTo(SpikeAlpha, SpikeTarget, DeltaSeconds, Speed);
	Spikes->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(SpikeHiddenZ, SpikeUpZ, SpikeAlpha)));

	Plate->SetRelativeLocation(FVector(0.f, 0.f, FMath::Lerp(PlateUpZ, PlateDownZ, PlateAlpha)));

	// Destello rojo durante el aviso.
	if (FlashTime > 0.f)
	{
		FlashTime -= DeltaSeconds;
		const float Blink = FMath::Fmod(FlashTime * 12.f, 2.f) < 1.f ? 1.f : 0.2f;
		WarnLight->SetIntensity(FlashTime > 0.f ? 2500.f * Blink : 0.f);
	}
}

void ALabSpikeTrap::ServerFire()
{
	for (ALabCharacter* Ch : GetCharactersInside())
	{
		// Herida + lanzamiento. Si las heridas llenan la barra, AddWound lo deja en ragdoll.
		const FVector Side(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f);
		Ch->AddWound(Ch->GetStaminaSettings()->SpikeWound, Side.GetSafeNormal() * LaunchSide + FVector(0.f, 0.f, LaunchUp));
	}
}
