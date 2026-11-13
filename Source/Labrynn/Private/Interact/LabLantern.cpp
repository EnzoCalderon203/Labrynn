#include "Interact/LabLantern.h"
#include "Audio/LabAudio.h"
#include "Components/AudioComponent.h"

#include "Player/LabCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Labrynn.h"

#define LOCTEXT_NAMESPACE "LabLantern"

ALabLantern::ALabLantern()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true; // la luz de un farol lejano igual se ve por los pasillos
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));

	// Cuerpo de vidrio + "techito" (formas básicas; Luis puede poner un mesh real en un BP hijo).
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetRelativeScale3D(FVector(0.14f, 0.14f, 0.22f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 11.f));
	Body->CastShadow = false; // la luz está adentro: que no se haga sombra a sí misma
	if (Cyl.Succeeded()) { Body->SetStaticMesh(Cyl.Object); }

	Cap = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cap"));
	Cap->SetupAttachment(Root);
	Cap->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Cap->SetRelativeScale3D(FVector(0.17f, 0.17f, 0.08f));
	Cap->SetRelativeLocation(FVector(0.f, 0.f, 26.f));
	Cap->CastShadow = false;
	if (Cone.Succeeded()) { Cap->SetStaticMesh(Cone.Object); }

	Flame = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Root);
	Flame->SetRelativeLocation(FVector(0.f, 0.f, 14.f));
	Flame->SetLightColor(FLinearColor(1.f, 0.62f, 0.28f));
	Flame->SetCastShadows(true);
	Flame->SetSourceRadius(4.f);
}

void ALabLantern::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabLantern, Holder);
	DOREPLIFETIME(ALabLantern, bLit);
	DOREPLIFETIME(ALabLantern, bRelighting);
	DOREPLIFETIME(ALabLantern, RelightEndServerTime);
	DOREPLIFETIME(ALabLantern, OilByte);
	DOREPLIFETIME(ALabLantern, RepelUntilServerTime);
}

void ALabLantern::SetRepelling(float Seconds, float Multiplier)
{
	if (HasAuthority())
	{
		RepelUntilServerTime = FMath::Max(RepelUntilServerTime, ServerNow() + Seconds);
		RepelDrainMultiplier = Multiplier;
	}
}

bool ALabLantern::IsRepelling() const
{
	return bLit && ServerNow() < RepelUntilServerTime;
}

void ALabLantern::BeginPlay()
{
	Super::BeginPlay();
	FlickerSeed = FMath::FRand() * 100.f; // cada farol parpadea distinto
	OnRep_Lit(bLit);
}

float ALabLantern::ServerNow() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GS ? GS->GetServerWorldTimeSeconds() : 0.f;
}

float ALabLantern::GetRelightProgress() const
{
	if (!bRelighting || RelightSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(1.f - (RelightEndServerTime - ServerNow()) / RelightSeconds, 0.f, 1.f);
}

void ALabLantern::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Crepitar en loop mientras la llama está encendida (cosmético, en cada máquina).
	if (bLit != bFireSoundOn && GetNetMode() != NM_DedicatedServer)
	{
		bFireSoundOn = bLit;
		if (bLit)
		{
			FireAudio = ULabAudio::PlayAttached(ELabSfx::LanternFire, GetRootComponent());
		}
		else if (FireAudio)
		{
			FireAudio->FadeOut(0.3f, 0.f);
			FireAudio = nullptr;
		}
	}

	if (HasAuthority())
	{
		// Aceite.
		if (bLit && OilServer > 0.f)
		{
			const float Drain = IsRepelling() ? RepelDrainMultiplier : 1.f; // conteniendo al Minotauro: gasta ×4
			OilServer = FMath::Max(0.f, OilServer - Drain * DeltaSeconds / OilSeconds);
			OilByte = static_cast<uint8>(FMath::CeilToInt(OilServer * 255.f));
			if (OilServer <= 0.f)
			{
				SetLit(false);
			}
		}
		// Reencendido.
		if (bRelighting && ServerNow() >= RelightEndServerTime)
		{
			bRelighting = false;
			if (OilServer > 0.f && !(Holder && Holder->IsInGas()))
			{
				SetLit(true);
			}
		}
	}

	// Llama (cosmético, todas las máquinas): parpadeo suave con ruido, más inestable con poco aceite.
	if (bLit)
	{
		FlickerSeed += DeltaSeconds;
		const float Oil = GetOil();
		const float Unstable = Oil < 0.2f ? 0.35f : 0.12f;
		const float N = FMath::PerlinNoise1D(FlickerSeed * 7.f) * 0.5f + FMath::PerlinNoise1D(FlickerSeed * 23.f) * 0.25f;
		const float LowOil = Oil < 0.2f ? FMath::Lerp(0.45f, 1.f, Oil / 0.2f) : 1.f;
		Flame->SetIntensity(LightIntensity * LowOil * (1.f + Unstable * N));
	}
}

void ALabLantern::SetLit(bool bNewLit)
{
	const bool bOld = bLit;
	bLit = bNewLit;
	if (!bLit)
	{
		bRelighting = false;
	}
	OnRep_Lit(bOld); // en el servidor OnRep no se dispara solo
}

void ALabLantern::OnRep_Lit(bool bWasLit)
{
	Flame->SetVisibility(bLit);
	Flame->SetAttenuationRadius(LightRadius);
	if (bWasLit != bLit && GetNetMode() != NM_DedicatedServer)
	{
		ULabAudio::Play(this, bLit ? ELabSfx::LanternLight : ELabSfx::LanternOut, GetActorLocation());
	}
}

bool ALabLantern::CanInteract(const ALabCharacter* By) const
{
	if (!By || !By->IsAliveAndFree())
	{
		return false;
	}
	if (Holder == By)
	{
		return true; // pasar o soltar
	}
	return Holder == nullptr && !By->GetHeldLantern() && !By->IsCarryingRelic() && !By->IsCarryingBody();
}

FText ALabLantern::GetInteractPrompt(const ALabCharacter* By) const
{
	if (Holder == By)
	{
		const ALabCharacter* Mate = By->FindTeammateInFront();
		return (Mate && !Mate->GetHeldLantern() && !Mate->IsCarryingRelic()) ? LOCTEXT("Pass", "Pasar el farol") : LOCTEXT("Drop", "Dejar el farol en el piso");
	}
	return LOCTEXT("Take", "Tomar el farol");
}

void ALabLantern::Interact(ALabCharacter* By)
{
	if (!HasAuthority() || !CanInteract(By))
	{
		return;
	}
	if (Holder == nullptr)
	{
		SetHolder(By);
		return;
	}
	ALabCharacter* Mate = By->FindTeammateInFront();
	SetHolder((Mate && !Mate->GetHeldLantern() && !Mate->IsCarryingRelic()) ? Mate : nullptr);
}

void ALabLantern::SetHolder(ALabCharacter* NewHolder)
{
	check(HasAuthority());
	if (NewHolder == Holder)
	{
		return;
	}
	ALabCharacter* Old = Holder;
	Holder = NewHolder;
	ApplyHolder(Old);
	if (Holder)
	{
		Holder->PlaySfxAll(ELabSfx::LanternTake);
	}

	if (!Holder && Old)
	{
		// Al piso, delante del portador anterior. Sigue encendido.
		const FVector Start = Old->GetActorLocation() + Old->GetActorForwardVector() * 60.f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LanternDrop), false, Old);
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0, 0, 400.f), ECC_Visibility, Params);
		SetActorLocation(bHit ? Hit.ImpactPoint : Start);
		SetActorRotation(FRotator::ZeroRotator);
	}
	UE_LOG(LogLabrynn, Log, TEXT("Farol %s: %s -> %s"), *GetName(), *GetNameSafe(Old), *GetNameSafe(Holder));
}

void ALabLantern::OnRep_Holder(ALabCharacter* OldHolder)
{
	ApplyHolder(OldHolder);
}

void ALabLantern::ApplyHolder(ALabCharacter* OldHolder)
{
	if (OldHolder && OldHolder != Holder)
	{
		OldHolder->SetHeldLantern(nullptr);
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	if (Holder)
	{
		AttachToComponent(Holder->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(HoldOffset);
		Holder->SetHeldLantern(this);
	}
}

void ALabLantern::ToggleByHolder(ALabCharacter* By)
{
	if (!HasAuthority() || By != Holder)
	{
		return;
	}
	if (bLit)
	{
		SetLit(false);
		UE_LOG(LogLabrynn, Log, TEXT("Farol %s apagado por %s"), *GetName(), *By->GetName());
	}
	else if (!bRelighting && OilServer > 0.f && !By->IsInGas())
	{
		bRelighting = true;
		RelightEndServerTime = ServerNow() + RelightSeconds;
		UE_LOG(LogLabrynn, Log, TEXT("Farol %s: reencendiendo (%.0f s)"), *GetName(), RelightSeconds);
	}
}

void ALabLantern::Extinguish()
{
	if (HasAuthority() && (bLit || bRelighting))
	{
		SetLit(false);
		UE_LOG(LogLabrynn, Log, TEXT("Farol %s: la corriente de aire apagó la llama"), *GetName());
	}
}

void ALabLantern::AddOil(float Amount)
{
	if (HasAuthority())
	{
		OilServer = FMath::Clamp(OilServer + Amount, 0.f, 1.f);
		OilByte = static_cast<uint8>(FMath::CeilToInt(OilServer * 255.f));
	}
}

void ALabLantern::Refill()
{
	if (HasAuthority())
	{
		AddOil(1.f);
		SetLit(true);
	}
}

#undef LOCTEXT_NAMESPACE
