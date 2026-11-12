#include "Interact/LabHeart.h"

#include "Player/LabCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

#define LOCTEXT_NAMESPACE "LabHeart"

ALabHeart::ALabHeart()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mesh->SetRelativeScale3D(FVector(0.45f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) { Mesh->SetStaticMesh(Sphere.Object); }

	Pulse = CreateDefaultSubobject<UPointLightComponent>(TEXT("Pulse"));
	Pulse->SetupAttachment(Mesh);
	Pulse->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // en espacio escalado del mesh (≈ 55 cm)
	Pulse->SetLightColor(FLinearColor(1.f, 0.12f, 0.15f));
	Pulse->SetAttenuationRadius(500.f);
	Pulse->SetCastShadows(false);
}

void ALabHeart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabHeart, bUsed);
}

void ALabHeart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Late: doble pulso tipo corazón. Usado: brasa apagada.
	PulseTime += DeltaSeconds;
	const float T = FMath::Fmod(PulseTime, 1.1f);
	const float Beat = FMath::Max(FMath::Exp(-30.f * FMath::Square(T - 0.1f)), 0.6f * FMath::Exp(-30.f * FMath::Square(T - 0.35f)));
	Pulse->SetIntensity(bUsed ? 60.f : 300.f + 2200.f * Beat);
}

bool ALabHeart::CanInteract(const ALabCharacter* By) const
{
	return !bUsed && By && By->IsAliveAndFree();
}

FText ALabHeart::GetInteractPrompt(const ALabCharacter* By) const
{
	return LOCTEXT("Use", "Usar el Corazón (cura y revive a todos, recarga las luces; 1 vez)");
}

void ALabHeart::Interact(ALabCharacter* By)
{
	if (!HasAuthority() || !CanInteract(By))
	{
		return;
	}
	bUsed = true;
	OnRep_Used();

	int32 Revived = 0;
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsAliveAndFree())
		{
			It->Revive(0.f);
			++Revived;
		}
		It->RestoreAll();   // sin heridas, stamina llena
		It->AddBattery(1.f);
	}
	UE_LOG(LogLabrynn, Log, TEXT("Corazón usado por %s: %d revivido(s), linternas recargadas"), *By->GetName(), Revived);
}

void ALabHeart::OnRep_Used()
{
	if (bUsed && UseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, UseSound, GetActorLocation());
	}
}

#undef LOCTEXT_NAMESPACE
