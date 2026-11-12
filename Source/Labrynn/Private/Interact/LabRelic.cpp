#include "Interact/LabRelic.h"

#include "Player/LabCharacter.h"
#include "Core/LabGameState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

#define LOCTEXT_NAMESPACE "LabRelic"

ALabRelic::ALabRelic()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true; // todos tienen que saber dónde está la reliquia

	Root = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
	Root->InitSphereRadius(30.f);
	Root->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.45f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (Cone.Succeeded())
	{
		Mesh->SetStaticMesh(Cone.Object);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	Glow->SetLightColor(FLinearColor(0.35f, 0.8f, 1.f));
	Glow->SetIntensity(600.f);
	Glow->SetAttenuationRadius(450.f);
	Glow->SetCastShadows(false);
}

void ALabRelic::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabRelic, Carrier);
}

bool ALabRelic::CanInteract(const ALabCharacter* By) const
{
	return By && By->IsAliveAndFree() && (Carrier == nullptr || Carrier == By);
}

FText ALabRelic::GetInteractPrompt(const ALabCharacter* By) const
{
	if (Carrier == By)
	{
		return By->FindTeammateInFront() ? LOCTEXT("Pass", "Pasar la reliquia") : LOCTEXT("Drop", "Soltar la reliquia");
	}
	return LOCTEXT("Take", "Tomar la reliquia");
}

void ALabRelic::Interact(ALabCharacter* By)
{
	check(HasAuthority());
	if (!CanInteract(By))
	{
		return;
	}

	if (Carrier == nullptr)
	{
		SetCarrier(By);
		if (ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>())
		{
			GS->OnRelicTaken(); // la primera vez arranca la fase de escape
		}
	}
	else
	{
		// La tiene él: pasarla al compañero de enfrente o soltarla.
		SetCarrier(By->FindTeammateInFront());
	}
}

void ALabRelic::SetCarrier(ALabCharacter* NewCarrier)
{
	check(HasAuthority());
	if (NewCarrier == Carrier)
	{
		return;
	}

	ALabCharacter* Old = Carrier;
	Carrier = NewCarrier;
	ApplyCarrier(Old); // en el servidor OnRep no se dispara solo

	if (!Carrier && Old)
	{
		// Soltarla en el piso, delante del portador anterior.
		const FVector Start = Old->GetActorLocation() + Old->GetActorForwardVector() * 70.f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RelicDrop), false, Old);
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0, 0, 400.f), ECC_Visibility, Params);
		SetActorLocation((bHit ? Hit.ImpactPoint : Start) + FVector(0, 0, 25.f));
		SetActorRotation(FRotator::ZeroRotator);
	}

	UE_LOG(LogLabrynn, Log, TEXT("Reliquia: %s -> %s"), *GetNameSafe(Old), *GetNameSafe(Carrier));
}

void ALabRelic::OnRep_Carrier(ALabCharacter* OldCarrier)
{
	ApplyCarrier(OldCarrier);
}

void ALabRelic::ApplyCarrier(ALabCharacter* OldCarrier)
{
	if (OldCarrier && OldCarrier != Carrier)
	{
		OldCarrier->SetCarriedRelic(nullptr, 1.f);
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	if (Carrier)
	{
		AttachToComponent(Carrier->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(CarryOffset);
		Carrier->SetCarriedRelic(this, CarrySpeedMultiplier);
	}
}

#undef LOCTEXT_NAMESPACE
