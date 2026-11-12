#include "Interact/LabHidingSpot.h"

#include "Player/LabCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

#define LOCTEXT_NAMESPACE "LabHide"

ALabHidingSpot::ALabHidingSpot()
{
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cyl.Object;
}

void ALabHidingSpot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabHidingSpot, Kind);
	DOREPLIFETIME(ALabHidingSpot, Occupant);
}

void ALabHidingSpot::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Kind();
}

void ALabHidingSpot::OnRep_Kind()
{
	// Formas provisorias. Mirando hacia +X local (la salida).
	switch (Kind)
	{
	case ELabHidingKind::Barrel:
		Mesh->SetStaticMesh(CylinderMesh);
		Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.25f));
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, 62.f));
		break;
	case ELabHidingKind::Hay:
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetRelativeScale3D(FVector(1.1f, 1.6f, 1.1f));
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
		break;
	case ELabHidingKind::Niche:
		// Nicho: un "hueco" oscuro contra la pared; no bloquea (se entra caminando).
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetRelativeScale3D(FVector(0.15f, 1.0f, 1.9f));
		Mesh->SetRelativeLocation(FVector(-45.f, 0.f, 95.f));
		break;
	}
}

bool ALabHidingSpot::CanInteract(const ALabCharacter* By) const
{
	return By && By->IsAliveAndFree() && !By->IsCarryingRelic() && (Occupant == nullptr || Occupant == By);
}

FText ALabHidingSpot::GetInteractPrompt(const ALabCharacter* By) const
{
	if (Occupant == By)
	{
		return LOCTEXT("Out", "Salir del escondite");
	}
	switch (Kind)
	{
	case ELabHidingKind::Barrel: return LOCTEXT("Barrel", "Esconderse en el barril");
	case ELabHidingKind::Hay:    return LOCTEXT("Hay", "Esconderse en la paja");
	default:                     return LOCTEXT("Niche", "Esconderse en el nicho");
	}
}

void ALabHidingSpot::Interact(ALabCharacter* By)
{
	if (!HasAuthority() || !CanInteract(By))
	{
		return;
	}
	if (Occupant == By)
	{
		By->ExitHiding();
		return;
	}
	Occupant = By;
	By->EnterHiding(this);
}

void ALabHidingSpot::ClearOccupant(ALabCharacter* Who)
{
	if (HasAuthority() && Occupant == Who)
	{
		Occupant = nullptr;
	}
}

FVector ALabHidingSpot::GetExitLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * 110.f + FVector(0.f, 0.f, 100.f);
}

#undef LOCTEXT_NAMESPACE
