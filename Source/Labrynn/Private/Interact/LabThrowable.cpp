#include "Interact/LabThrowable.h"

#include "Player/LabCharacter.h"
#include "Core/LabGameplaySettings.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

#define LOCTEXT_NAMESPACE "LabThrow"

ALabThrowable::ALabThrowable()
{
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(8.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);       // dispara los Trigger de las trampas
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	SphereMesh = Sphere.Object;
	CylinderMesh = Cyl.Object;

	Projectile = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile"));
	Projectile->bAutoActivate = false;
	Projectile->bShouldBounce = true;
	Projectile->Bounciness = 0.35f;
	Projectile->Friction = 0.4f;
	Projectile->ProjectileGravityScale = 1.f;
	Projectile->BounceVelocityStopSimulatingThreshold = 60.f;
}

void ALabThrowable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabThrowable, Kind);
	DOREPLIFETIME(ALabThrowable, bInFlight);
}

void ALabThrowable::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Kind();
	if (HasAuthority())
	{
		Projectile->OnProjectileBounce.AddDynamic(this, &ALabThrowable::OnBounce);
		Projectile->OnProjectileStop.AddDynamic(this, &ALabThrowable::OnStop);
	}
}

void ALabThrowable::OnRep_Kind()
{
	// Formas provisorias (reemplazables por el kit medieval en un BP hijo).
	if (Kind == ELabThrowKind::Bottle)
	{
		Mesh->SetStaticMesh(CylinderMesh);
		Mesh->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.22f));
	}
	else
	{
		Mesh->SetStaticMesh(SphereMesh);
		Mesh->SetRelativeScale3D(FVector(0.13f, 0.11f, 0.1f));
	}
}

bool ALabThrowable::CanInteract(const ALabCharacter* By) const
{
	return !bInFlight && By && By->IsAliveAndFree() && By->CanCarryMoreThrowables() && !IsActorBeingDestroyed();
}

FText ALabThrowable::GetInteractPrompt(const ALabCharacter* By) const
{
	return Kind == ELabThrowKind::Bottle ? LOCTEXT("Bottle", "Juntar botella") : LOCTEXT("Stone", "Juntar piedra");
}

void ALabThrowable::Interact(ALabCharacter* By)
{
	if (HasAuthority() && CanInteract(By))
	{
		By->AddThrowable(Kind);
		Destroy();
	}
}

void ALabThrowable::Launch(const FVector& Velocity, AActor* Thrower)
{
	check(HasAuthority());
	SetOwner(Thrower);
	SetInstigator(Cast<APawn>(Thrower));
	bInFlight = true;
	bImpacted = false;
	Projectile->Bounciness = Kind == ELabThrowKind::Bottle ? 0.f : 0.35f;
	Projectile->SetUpdatedComponent(Collision);
	Projectile->Velocity = Velocity;
	Projectile->Activate(true);
}

void ALabThrowable::MakeNoiseAt(const FVector& Location, float Strength)
{
	const ULabGameplaySettings* G = GetDefault<ULabGameplaySettings>();
	if (const ALabCharacter* Ch = Cast<ALabCharacter>(GetInstigator()))
	{
		G = Ch->GetGameplaySettings();
	}
	const bool bBottle = Kind == ELabThrowKind::Bottle;
	const float Range = bBottle ? G->BottleNoiseRange : G->StoneNoiseRange;
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Location, Strength, this, Range, bBottle ? FName("Botella") : FName("Piedra"));
	UE_LOG(LogLabrynn, Log, TEXT("Lanzable: %s golpeó en %s (ruido %.2f, alcance %.0f)"), bBottle ? TEXT("botella") : TEXT("piedra"), *Location.ToCompactString(), Strength, Range);
}

void ALabThrowable::OnBounce(const FHitResult& Hit, const FVector& ImpactVelocity)
{
	const ULabGameplaySettings* G = GetDefault<ULabGameplaySettings>();
	if (const ALabCharacter* Ch = Cast<ALabCharacter>(GetInstigator())) { G = Ch->GetGameplaySettings(); }

	if (Kind == ELabThrowKind::Bottle)
	{
		if (!bImpacted)
		{
			bImpacted = true;
			MakeNoiseAt(Hit.ImpactPoint, G->BottleNoise);
			MulticastImpact(Hit.ImpactPoint, true);
			SetLifeSpan(0.2f); // se rompe
			SetActorHiddenInGame(true);
			Projectile->StopMovementImmediately();
		}
		return;
	}
	// Piedra: ruido en los golpes fuertes, proporcional a la velocidad.
	const float Speed = ImpactVelocity.Size();
	if (Speed > 250.f)
	{
		MakeNoiseAt(Hit.ImpactPoint, G->StoneNoise * FMath::Clamp(Speed / 1000.f, 0.4f, 1.f));
		MulticastImpact(Hit.ImpactPoint, false);
	}
}

void ALabThrowable::OnStop(const FHitResult& Hit)
{
	if (Kind == ELabThrowKind::Stone)
	{
		bInFlight = false; // vuelve a estar para juntar
	}
}

void ALabThrowable::MulticastImpact_Implementation(FVector Location, bool bShatter)
{
	if (USoundBase* S = bShatter ? ShatterSound : ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, Location);
	}
}

#undef LOCTEXT_NAMESPACE
