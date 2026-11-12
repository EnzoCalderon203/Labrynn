#include "Traps/LabTrapBase.h"

#include "Player/LabCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Labrynn.h"

ALabTrapBase::ALabTrapBase()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	SetNetUpdateFrequency(20.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetBoxExtent(FVector(120.f, 120.f, 60.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
}

void ALabTrapBase::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &ALabTrapBase::OnTriggerOverlap);
	}
	OnStateChanged(State);
}

void ALabTrapBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabTrapBase, State);
}

void ALabTrapBase::OnTriggerOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	const ALabCharacter* Ch = Cast<ALabCharacter>(Other);
	if (!Ch || !Ch->IsAliveAndFree() || State != ELabTrapState::Armed)
	{
		return;
	}
	UE_LOG(LogLabrynn, Log, TEXT("Trampa %s activada por %s"), *GetName(), *Ch->GetName());
	SetState(ELabTrapState::Warning);
	GetWorldTimerManager().SetTimer(StepTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetState(ELabTrapState::Firing);
		ServerFire();
		GetWorldTimerManager().SetTimer(StepTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			SetState(ELabTrapState::Cooldown);
			if (CooldownTime > 0.f)
			{
				GetWorldTimerManager().SetTimer(StepTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					SetState(ELabTrapState::Armed);
				}), CooldownTime, false);
			}
		}), FiringTime, false);
	}), WarningTime, false);
}

void ALabTrapBase::SetState(ELabTrapState NewState)
{
	const ELabTrapState Old = State;
	State = NewState;
	OnStateChanged(Old); // en el servidor OnRep no se dispara solo
}

void ALabTrapBase::OnRep_State(ELabTrapState OldState)
{
	OnStateChanged(OldState);
}

void ALabTrapBase::OnStateChanged(ELabTrapState OldState)
{
	if (OldState == State || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	USoundBase* S = nullptr;
	switch (State)
	{
	case ELabTrapState::Warning: S = WarnSound; break;
	case ELabTrapState::Firing:  S = FireSound; break;
	case ELabTrapState::Armed:   S = ResetSound; break;
	default: break;
	}
	if (S)
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, GetActorLocation());
	}
}

TArray<ALabCharacter*> ALabTrapBase::GetCharactersInside() const
{
	TArray<AActor*> Overlaps;
	Trigger->GetOverlappingActors(Overlaps, ALabCharacter::StaticClass());
	TArray<ALabCharacter*> Out;
	for (AActor* A : Overlaps)
	{
		ALabCharacter* Ch = Cast<ALabCharacter>(A);
		if (Ch && Ch->IsAliveAndFree())
		{
			Out.Add(Ch);
		}
	}
	return Out;
}
