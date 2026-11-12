#include "Player/LabCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Interact/LabInteractable.h"
#include "Interact/LabRelic.h"
#include "Interact/LabLantern.h"
#include "Core/LabGameState.h"
#include "Maze/LabMaze.h"
#include "Player/LabCharacterMovement.h"
#include "Player/LabStaminaSettings.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "InputAction.h"
#include "Net/VoiceConfig.h"
#include "Sound/SoundAttenuation.h"
#include "Labrynn.h"

ALabCharacter::ALabCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<ULabCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	bReplicates = true;

	static ConstructorHelpers::FObjectFinder<UInputAction> IASprint(TEXT("/Game/Labrynn/Core/Input/IA_Sprint.IA_Sprint"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IAInteract(TEXT("/Game/Labrynn/Core/Input/IA_Interact.IA_Interact"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IALantern(TEXT("/Game/Labrynn/Core/Input/IA_Lantern.IA_Lantern"));
	static ConstructorHelpers::FObjectFinder<UInputAction> IATalk(TEXT("/Game/Labrynn/Core/Input/IA_Talk.IA_Talk"));
	SprintAction = IASprint.Object;
	InteractAction = IAInteract.Object;
	LanternAction = IALantern.Object;
	TalkAction = IATalk.Object;

	static ConstructorHelpers::FObjectFinder<ULabStaminaSettings> StaminaDA(TEXT("/Game/Labrynn/Core/DA_Stamina.DA_Stamina"));
	if (StaminaDA.Succeeded())
	{
		StaminaSettings = StaminaDA.Object;
	}

	GetCapsuleComponent()->InitCapsuleSize(34.f, 96.f);

	// Brazos: solo los ve el dueño. (Nombres de subobjetos iguales a la plantilla.)
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->CastShadow = false;

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// Cuerpo completo: lo ven los demás, no el dueño (el dueño ve su sombra).
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	GetMesh()->bCastHiddenShadow = true;

	// Brillo personal: luz mínima alrededor del cuerpo, visible para todos.
	PersonalGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("PersonalGlow"));
	PersonalGlow->SetupAttachment(GetCapsuleComponent());
	PersonalGlow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	PersonalGlow->SetCastShadows(false);
	PersonalGlow->SetLightColor(FLinearColor(1.f, 0.85f, 0.7f));

	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
}

void ALabCharacter::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
#endif
	MeshRelativeTransform = GetMesh()->GetRelativeTransform();
	CameraRelativeTransform = FirstPersonCameraComponent->GetRelativeTransform();
	StaminaServer = GetStaminaSettings()->MaxStamina;
	WoundsServer = 0.f;
	SyncStaminaBytes();
	PersonalGlow->SetIntensity(PersonalGlowIntensity);
	PersonalGlow->SetAttenuationRadius(PersonalGlowRadius);

	static const TCHAR* Roles[] = { TEXT("None"), TEXT("SimulatedProxy"), TEXT("AutonomousProxy"), TEXT("Authority") };
	UE_LOG(LogLabrynn, Log, TEXT("[%s] Personaje %s BeginPlay: LocalRole=%s Local=%d Body=%s Loc=%s"),
		GetNetMode() == NM_Client ? TEXT("Cliente") : TEXT("Servidor"), *GetName(),
		Roles[FMath::Clamp<int32>(GetLocalRole(), 0, 3)], IsLocallyControlled() ? 1 : 0,
		*GetNameSafe(GetMesh()->GetSkeletalMeshAsset()), *GetActorLocation().ToCompactString());
}

void ALabCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabCharacter, LifeState);
	DOREPLIFETIME(ALabCharacter, StaminaByte);
	DOREPLIFETIME(ALabCharacter, WoundsByte);
	DOREPLIFETIME(ALabCharacter, bExhausted);
	DOREPLIFETIME(ALabCharacter, ReviveByte);
	DOREPLIFETIME(ALabCharacter, DownedAtServerTime);
	DOREPLIFETIME(ALabCharacter, DownImpulse);
	DOREPLIFETIME(ALabCharacter, GasUntilServerTime);
	DOREPLIFETIME(ALabCharacter, bTalking);
}

// ---------------------------------------------------------------- Voz por proximidad

void ALabCharacter::TalkPressed()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->StartTalking(); // push-to-talk nativo (OnlineVoice: Steam o NULL)
	}
	ServerSetTalking(true);
	UE_LOG(LogLabrynn, Log, TEXT("Voz: %s empieza a hablar (V)"), *GetName());
}

void ALabCharacter::TalkReleased()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->StopTalking();
	}
	ServerSetTalking(false);
	UE_LOG(LogLabrynn, Log, TEXT("Voz: %s deja de hablar"), *GetName());
}

void ALabCharacter::ServerSetTalking_Implementation(bool bTalk)
{
	bTalking = bTalk && IsAliveAndFree();
}

void ALabCharacter::TickVoice(float DeltaSeconds)
{
	// En cada máquina, la voz de los OTROS suena desde su cuerpo, con atenuación y oclusión por paredes.
	if (!VoiceTalker && !IsLocallyControlled() && GetPlayerState() && GetNetMode() != NM_DedicatedServer)
	{
		static TObjectPtr<USoundAttenuation> VoiceAttenuation;
		if (!VoiceAttenuation)
		{
			VoiceAttenuation = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("LabVoiceAttenuation"));
			VoiceAttenuation->AddToRoot();
			FSoundAttenuationSettings& A = VoiceAttenuation->Attenuation;
			A.bAttenuate = true;
			A.bSpatialize = true;
			A.AttenuationShape = EAttenuationShape::Sphere;
			A.AttenuationShapeExtents = FVector(150.f);   // radio a volumen pleno
			A.FalloffDistance = VoiceRange;
			A.bEnableOcclusion = true;                   // paredes: suena apagada y más baja
			A.OcclusionLowPassFilterFrequency = 900.f;
			A.OcclusionVolumeAttenuation = 0.35f;
			A.OcclusionInterpolationTime = 0.15f;
			A.OcclusionTraceChannel = ECC_Visibility;
		}
		VoiceTalker = UVOIPTalker::CreateTalkerForPlayer(GetPlayerState());
		if (VoiceTalker)
		{
			VoiceTalker->Settings.ComponentToAttachTo = GetCapsuleComponent();
			VoiceTalker->Settings.AttenuationSettings = VoiceAttenuation;
			UE_LOG(LogLabrynn, Log, TEXT("Voz: talker 3D creado para %s en %s"), *GetName(), GetNetMode() == NM_Client ? TEXT("cliente") : TEXT("servidor"));
		}
	}

	// Servidor: hablar hace ruido (lo puede oír una IA). Volumen ≈ nivel de la voz (si se conoce) o medio.
	if (HasAuthority() && bTalking && GetWorld()->GetTimeSeconds() >= NextVoiceNoise)
	{
		NextVoiceNoise = GetWorld()->GetTimeSeconds() + 0.4f;
		float Level = 0.6f; // voz del host (sin medición local) o sin datos todavía
		if (VoiceTalker)
		{
			Level = FMath::Max(0.3f, FMath::Clamp(VoiceTalker->GetVoiceLevel() * 4.f, 0.f, 1.f));
		}
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), Level, this, VoiceNoiseRange * Level, FName("Voz"));
		UE_LOG(LogLabrynn, Log, TEXT("Voz: ruido %.2f de %s (alcance %.0f)"), Level, *GetName(), VoiceNoiseRange * Level);
		LastVoiceNoiseLevel = Level;
	}
}

bool ALabCharacter::IsInGas() const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GS && GS->GetServerWorldTimeSeconds() < GasUntilServerTime;
}

void ALabCharacter::ApplyGas(float Seconds)
{
	if (!HasAuthority())
	{
		return;
	}
	const AGameStateBase* GS = GetWorld()->GetGameState();
	GasUntilServerTime = (GS ? GS->GetServerWorldTimeSeconds() : 0.f) + Seconds;
	if (ALabLantern* L = HeldLantern.Get())
	{
		L->Extinguish(); // ¡la corriente apagó la llama!
	}
}

// ---------------------------------------------------------------- Vida, caída y rescate

float ALabCharacter::GetBleedOutRemaining() const
{
	const AGameStateBase* GS = GetWorld()->GetGameState();
	return GS ? FMath::Max(0.f, BleedOutSeconds - (GS->GetServerWorldTimeSeconds() - DownedAtServerTime)) : 0.f;
}

float ALabCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || LifeState != ELabLifeState::Alive)
	{
		return 0.f;
	}
	// El daño genérico se convierte en heridas (no hay barra de vida aparte).
	const float Applied = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	const FVector Push = DamageCauser ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() * 300.f : FVector::ZeroVector;
	AddWound(Applied, Push);
	return Applied;
}

// ---------------------------------------------------------------- Stamina y heridas

const ULabStaminaSettings* ALabCharacter::GetStaminaSettings() const
{
	return StaminaSettings ? StaminaSettings.Get() : GetDefault<ULabStaminaSettings>();
}

bool ALabCharacter::CanSprint() const
{
	return IsAliveAndFree() && !bExhausted && StaminaByte > 0;
}

float ALabCharacter::GetDesiredMaxSpeed(bool bWantsSprint) const
{
	const ULabStaminaSettings* S = GetStaminaSettings();
	float Speed = S->WalkSpeed;
	if (bExhausted)
	{
		Speed = S->ExhaustedSpeed;
	}
	else if (bWantsSprint && CanSprint())
	{
		Speed = S->SprintSpeed;
	}
	if (IsCarryingRelic())
	{
		Speed *= S->RelicSpeedMultiplier;
	}
	return Speed;
}

void ALabCharacter::SyncStaminaBytes()
{
	const float Max = GetStaminaSettings()->MaxStamina;
	StaminaByte = static_cast<uint8>(FMath::Clamp(FMath::CeilToInt(StaminaServer / Max * 255.f), 0, 255));
	WoundsByte = static_cast<uint8>(FMath::Clamp(FMath::CeilToInt(WoundsServer / Max * 255.f), 0, 255));
}

void ALabCharacter::SpendStamina(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f)
	{
		return;
	}
	StaminaServer = FMath::Max(0.f, StaminaServer - Amount);
	LastSpendTime = GetWorld()->GetTimeSeconds();
	if (StaminaServer <= 0.f && !bExhausted)
	{
		bExhausted = true;
		ExhaustedUntil = GetWorld()->GetTimeSeconds() + GetStaminaSettings()->ExhaustedSeconds;
		NextBreathNoise = 0.f;
		UE_LOG(LogLabrynn, Log, TEXT("%s se quedó sin aire (jadeando %.1fs)"), *GetName(), GetStaminaSettings()->ExhaustedSeconds);
		ApplyExhaustedEffects();
	}
}

void ALabCharacter::TickStamina(float DeltaSeconds)
{
	if (!HasAuthority() || !IsAliveAndFree())
	{
		return;
	}
	const ULabStaminaSettings* S = GetStaminaSettings();
	const float Now = GetWorld()->GetTimeSeconds();

	// Gastos
	const ULabCharacterMovement* CMC = Cast<ULabCharacterMovement>(GetCharacterMovement());
	if (CMC && CMC->IsSprintingNow())
	{
		SpendStamina(S->SprintDrainPerSecond * DeltaSeconds);
		// Correr hace ruido (pasos rápidos): lo puede oír el Minotauro.
		if (Now >= NextSprintNoise)
		{
			NextSprintNoise = Now + 0.5f;
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 0.5f, this, 1400.f, FName("Sprint"));
		}
	}
	if (IsCarryingRelic())
	{
		SpendStamina(S->RelicDrainPerSecond * DeltaSeconds);
	}

	// Jadeo: ruido periódico (lo puede escuchar una IA con AI Perception: Hearing) y fin del jadeo.
	if (bExhausted)
	{
		if (Now >= NextBreathNoise)
		{
			NextBreathNoise = Now + S->BreathNoiseInterval;
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), S->BreathNoiseLoudness, this, S->BreathNoiseRange, FName("Jadeo"));
		}
		if (Now >= ExhaustedUntil)
		{
			bExhausted = false;
			ApplyExhaustedEffects();
		}
	}

	// Recuperación (también durante el jadeo, para salir con algo de aire).
	const float UsableMax = FMath::Max(0.f, S->MaxStamina - WoundsServer);
	if (Now - LastSpendTime >= S->RegenDelay || bExhausted)
	{
		StaminaServer = FMath::Min(UsableMax, StaminaServer + S->RegenPerSecond * DeltaSeconds);
	}
	StaminaServer = FMath::Min(StaminaServer, UsableMax);
	SyncStaminaBytes();
}

void ALabCharacter::AddWound(float Amount, const FVector& Impulse)
{
	if (!HasAuthority() || !IsAliveAndFree() || Amount <= 0.f)
	{
		return;
	}
	const ULabStaminaSettings* S = GetStaminaSettings();
	WoundsServer = FMath::Min(S->MaxStamina, WoundsServer + Amount);
	StaminaServer = FMath::Min(StaminaServer, S->MaxStamina - WoundsServer);
	SyncStaminaBytes();
	UE_LOG(LogLabrynn, Log, TEXT("%s herido: +%.0f (heridas %.0f/%.0f)"), *GetName(), Amount, WoundsServer, S->MaxStamina);

	if (WoundsServer >= S->MaxStamina)
	{
		KnockDown(Impulse); // las heridas llenaron la barra: ragdoll
	}
	else if (!Impulse.IsNearlyZero())
	{
		LaunchCharacter(Impulse, true, true); // empujón sin caer
	}
}

void ALabCharacter::HealWounds(float Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	WoundsServer = FMath::Max(0.f, WoundsServer - Amount);
	SyncStaminaBytes();
	UE_LOG(LogLabrynn, Log, TEXT("%s curado: -%.0f (heridas %.0f)"), *GetName(), Amount, WoundsServer);
}

void ALabCharacter::RestoreAll()
{
	if (!HasAuthority())
	{
		return;
	}
	WoundsServer = 0.f;
	StaminaServer = GetStaminaSettings()->MaxStamina;
	bExhausted = false;
	ApplyExhaustedEffects();
	SyncStaminaBytes();
}

void ALabCharacter::OnRep_Exhausted()
{
	ApplyExhaustedEffects();
}

void ALabCharacter::ApplyExhaustedEffects()
{
	// Respiración agitada: se escucha en todas las máquinas (y delata al grupo).
	const ULabStaminaSettings* S = GetStaminaSettings();
	if (bExhausted && S->BreathSound && GetNetMode() != NM_DedicatedServer)
	{
		if (!BreathAudio)
		{
			BreathAudio = UGameplayStatics::SpawnSoundAttached(S->BreathSound, GetCapsuleComponent());
		}
		else
		{
			BreathAudio->Play();
		}
	}
	else if (!bExhausted && BreathAudio)
	{
		BreathAudio->FadeOut(0.6f, 0.f);
	}
}

void ALabCharacter::SprintPressed()
{
	if (ULabCharacterMovement* CMC = Cast<ULabCharacterMovement>(GetCharacterMovement()))
	{
		CMC->bWantsToSprint = true;
	}
}

void ALabCharacter::SprintReleased()
{
	if (ULabCharacterMovement* CMC = Cast<ULabCharacterMovement>(GetCharacterMovement()))
	{
		CMC->bWantsToSprint = false;
	}
}

void ALabCharacter::KnockDown(const FVector& Impulse)
{
	check(HasAuthority());
	if (LifeState != ELabLifeState::Alive)
	{
		return;
	}
	if (ALabRelic* Relic = CarriedRelic.Get())
	{
		Relic->SetCarrier(nullptr); // se le cae la reliquia
	}
	if (ALabLantern* L = HeldLantern.Get())
	{
		L->SetHolder(nullptr); // y el farol (queda encendido en el piso, al lado del cuerpo)
	}

	const ELabLifeState Old = LifeState;
	LifeState = ELabLifeState::Downed;
	bExhausted = false;
	ApplyExhaustedEffects();
	DownImpulse = Impulse;
	if (const AGameStateBase* GS = GetWorld()->GetGameState())
	{
		DownedAtServerTime = GS->GetServerWorldTimeSeconds();
	}
	ReviveProgressServer = 0.f;
	ReviveByte = 0;
	Reviver.Reset();
	UE_LOG(LogLabrynn, Log, TEXT("%s cayó"), *GetName());
	ApplyLifeState(Old);
}

void ALabCharacter::Revive(float WoundsAfter)
{
	check(HasAuthority());
	if (LifeState == ELabLifeState::Alive)
	{
		return;
	}
	// Si está en el fondo de un foso, sale a la celda vecina abierta.
	if (GetActorLocation().Z < -100.f)
	{
		const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>();
		if (const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr)
		{
			const FIntPoint Cell = Maze->WorldToCell(GetActorLocation());
			static const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
			for (const FIntPoint& D : Dirs)
			{
				if (Maze->GetLayout().IsValidCell(Cell + D) && !Maze->GetLayout().HasWallBetween(Cell, D))
				{
					SetActorLocation(Maze->CellToWorld(Cell + D) + FVector(0.f, 0.f, 110.f), false, nullptr, ETeleportType::TeleportPhysics);
					break;
				}
			}
		}
	}

	const ELabLifeState Old = LifeState;
	LifeState = ELabLifeState::Alive;
	const ULabStaminaSettings* S = GetStaminaSettings();
	// Se levanta herido (nunca con la barra llena de heridas, o volvería a caer).
	WoundsServer = FMath::Clamp(WoundsAfter < 0.f ? S->WoundsAfterRevive : WoundsAfter, 0.f, S->MaxStamina * 0.9f);
	StaminaServer = S->MaxStamina - WoundsServer;
	SyncStaminaBytes();
	ReviveProgressServer = 0.f;
	ReviveByte = 0;
	Reviver.Reset();
	UE_LOG(LogLabrynn, Log, TEXT("%s se levantó (heridas %.0f)"), *GetName(), WoundsServer);
	ApplyLifeState(Old);
}

void ALabCharacter::OnRep_LifeState(ELabLifeState OldState)
{
	ApplyLifeState(OldState);
}

void ALabCharacter::ApplyLifeState(ELabLifeState OldState)
{
	const bool bWasAlive = OldState == ELabLifeState::Alive;
	const bool bIsAlive = LifeState == ELabLifeState::Alive;

	if (bWasAlive && !bIsAlive)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		// Los compañeros pueden pararse encima del cuerpo, y la cápsula no sostiene al propio ragdoll
		// (en el cliente dueño la cápsula queda quieta y "paraba" el cuerpo desde adentro).
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
		StartRagdoll();
	}
	else if (!bWasAlive && bIsAlive)
	{
		StopRagdoll();
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void ALabCharacter::StartRagdoll()
{
	if (bRagdoll)
	{
		return;
	}
	bRagdoll = true;

	USkeletalMeshComponent* Body = GetMesh();
	// Desadjuntar de la cápsula: el servidor mueve la cápsula a la pelvis y, adjunto, el ragdoll la seguiría
	// en un bucle (bug real encontrado en la prueba: el cuerpo terminó a 2 km).
	Body->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Body->SetOwnerNoSee(false); // el caído ve su propio cuerpo desparramado (parte de la gracia)
	// En UE 5.8 los primitivos "WorldSpaceRepresentation" nunca se muestran al dueño: quitar la marca.
	Body->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::None;
	Body->MarkRenderStateDirty();
	// El dueño no renderizaba su cuerpo y el mesh no actualizaba huesos: el ragdoll quedaba "parado".
	SavedTickOption = Body->VisibilityBasedAnimTickOption;
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->SetCollisionProfileName(TEXT("Ragdoll"));
	Body->SetAllBodiesSimulatePhysics(true);
	Body->SetSimulatePhysics(true);
	Body->WakeAllRigidBodies();
	if (!DownImpulse.IsNearlyZero())
	{
		Body->AddImpulse(DownImpulse, NAME_None, /*bVelChange*/ true);
	}
	FirstPersonMesh->SetVisibility(false);

	if (IsLocallyControlled())
	{
		// Cámara afuera del cuerpo (se actualiza en Tick siguiendo la pelvis).
		FirstPersonCameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		FirstPersonCameraComponent->bUsePawnControlRotation = false;
	}
}

void ALabCharacter::StopRagdoll()
{
	if (!bRagdoll)
	{
		return;
	}
	bRagdoll = false;

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetAllBodiesSimulatePhysics(false);
	Body->SetSimulatePhysics(false);
	Body->SetCollisionProfileName(TEXT("CharacterMesh"));
	Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	Body->SetRelativeTransform(MeshRelativeTransform);
	Body->VisibilityBasedAnimTickOption = SavedTickOption;
	Body->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	Body->MarkRenderStateDirty();
	Body->SetOwnerNoSee(true);
	FirstPersonMesh->SetVisibility(true);

	FirstPersonCameraComponent->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::KeepRelativeTransform, FName("head"));
	FirstPersonCameraComponent->SetRelativeTransform(CameraRelativeTransform);
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
}

void ALabCharacter::TickRevive(float DeltaSeconds)
{
	ALabCharacter* R = Reviver.Get();
	const bool bValid = R && R->IsAliveAndFree() && FVector::Dist2D(R->GetActorLocation(), GetActorLocation()) <= R->InteractRange + 160.f;
	if (!bValid)
	{
		Reviver.Reset();
		ReviveProgressServer = 0.f;
	}
	else
	{
		ReviveProgressServer += DeltaSeconds / FMath::Max(0.1f, ReviveSeconds);
		if (ReviveProgressServer >= 1.f)
		{
			Revive();
			return;
		}
	}
	ReviveByte = static_cast<uint8>(FMath::Clamp(ReviveProgressServer, 0.f, 1.f) * 255.f);
}

bool ALabCharacter::CanInteract(const ALabCharacter* By) const
{
	return LifeState == ELabLifeState::Downed && By && By != this && By->IsAliveAndFree();
}

void ALabCharacter::Interact(ALabCharacter* By)
{
	if (CanInteract(By))
	{
		Reviver = By; // tiene que quedarse cerca ReviveSeconds
	}
}

FText ALabCharacter::GetInteractPrompt(const ALabCharacter* By) const
{
	const FString Name = GetPlayerState() ? GetPlayerState()->GetPlayerName() : TEXT("compañero");
	return FText::FromString(FString::Printf(TEXT("Levantar a %s (quedate cerca)"), *Name));
}

void ALabCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickStamina(DeltaSeconds);
	TickVoice(DeltaSeconds);

	// Servidor: caído → desangrarse / ser levantado. La cápsula sigue al ragdoll (para distancias de rescate).
	if (HasAuthority() && LifeState == ELabLifeState::Downed)
	{
		if (GetBleedOutRemaining() <= 0.f)
		{
			LifeState = ELabLifeState::Dead;
			ReviveByte = 0;
			Reviver.Reset();
			UE_LOG(LogLabrynn, Log, TEXT("%s murió desangrado"), *GetName());
		}
		else
		{
			TickRevive(DeltaSeconds);
		}
	}
	if (HasAuthority() && bRagdoll)
	{
		const FVector Pelvis = GetMesh()->GetSocketLocation(FName("pelvis"));
		SetActorLocation(Pelvis + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), false, nullptr, ETeleportType::None);
	}

	// Dueño caído: cámara afuera, mirando al propio cuerpo.
	if (bRagdoll && IsLocallyControlled())
	{
		const FVector Pelvis = GetMesh()->GetSocketLocation(FName("pelvis"));
		const FVector CamPos = Pelvis + FVector(-160.f, -60.f, 170.f);
		FirstPersonCameraComponent->SetWorldLocationAndRotation(CamPos, (Pelvis - CamPos).Rotation());
	}

}

void ALabCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ALabCharacter::DoJumpStart);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ALabCharacter::DoJumpEnd);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALabCharacter::MoveInput);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALabCharacter::LookInput);
		EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ALabCharacter::LookInput);

		// Acciones propias (IMC_Labrynn). Started = al apretar; Completed = al soltar.
		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ALabCharacter::SprintPressed);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ALabCharacter::SprintReleased);
		}
		if (InteractAction) { EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ALabCharacter::InteractInput); }
		if (LanternAction)  { EIC->BindAction(LanternAction, ETriggerEvent::Started, this, &ALabCharacter::LanternInput); }
		if (TalkAction)
		{
			EIC->BindAction(TalkAction, ETriggerEvent::Started, this, &ALabCharacter::TalkPressed);
			EIC->BindAction(TalkAction, ETriggerEvent::Completed, this, &ALabCharacter::TalkReleased);
		}
		if (!SprintAction || !InteractAction || !LanternAction)
		{
			UE_LOG(LogLabrynn, Error, TEXT("Faltan Input Actions de /Game/Labrynn/Core/Input (correr Scripts/create_input_assets.py)"));
		}
	}
	else
	{
		UE_LOG(LogLabrynn, Error, TEXT("'%s' no tiene EnhancedInputComponent."), *GetNameSafe(this));
	}

}

// ---------------------------------------------------------------- Interacción

AActor* ALabCharacter::FindBestInteractable() const
{
	const AController* C = GetController();
	const FVector Eye = FirstPersonCameraComponent ? FirstPersonCameraComponent->GetComponentLocation() : GetActorLocation();
	const FVector View = (C ? C->GetControlRotation() : GetActorRotation()).Vector();
	const FVector ViewFlat = FVector(View.X, View.Y, 0.f).GetSafeNormal();

	AActor* Best = nullptr;
	float BestScore = -1.f;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		ILabInteractable* I = Cast<ILabInteractable>(*It);
		if (!I || !I->CanInteract(this))
		{
			continue;
		}
		const FVector To = It->GetActorLocation() - Eye;
		// Compañeros caídos: distancia horizontal y más alcance (para sacarlos de un foso desde el borde).
		const bool bIsBody = It->IsA<ALabCharacter>();
		const float Dist = bIsBody ? FVector(To.X, To.Y, 0.f).Size() : To.Size();
		const float Range = bIsBody ? InteractRange + 100.f : InteractRange;
		if (Dist > Range || (bIsBody && FMath::Abs(To.Z) > 450.f))
		{
			continue;
		}
		// Ángulo en el plano (tolerante: los objetos en el piso quedan "abajo" de la vista).
		const float Dot = FVector::DotProduct(ViewFlat, FVector(To.X, To.Y, 0.f).GetSafeNormal());
		if (Dot < 0.3f && Dist > 60.f)
		{
			continue;
		}
		const float Score = Dot - Dist / InteractRange * 0.5f;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = *It;
		}
	}
	return Best;
}

ALabCharacter* ALabCharacter::FindTeammateInFront() const
{
	const FVector Fwd = GetActorForwardVector();
	ALabCharacter* Best = nullptr;
	float BestDist = PassRange;
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == this || !It->IsAliveAndFree())
		{
			continue;
		}
		const FVector To = It->GetActorLocation() - GetActorLocation();
		const float Dist = To.Size();
		if (Dist < BestDist && FVector::DotProduct(Fwd, To.GetSafeNormal()) > 0.5f)
		{
			BestDist = Dist;
			Best = *It;
		}
	}
	return Best;
}

void ALabCharacter::InteractInput()
{
	if (IsAliveAndFree())
	{
		ServerInteract();
	}
}

void ALabCharacter::ServerInteract_Implementation()
{
	if (!IsAliveAndFree())
	{
		return;
	}
	if (AActor* Target = FindBestInteractable())
	{
		Cast<ILabInteractable>(Target)->Interact(this);
	}
}

void ALabCharacter::SetCarriedRelic(ALabRelic* Relic, float SpeedMultiplier)
{
	// La velocidad la calcula ULabCharacterMovement::GetMaxSpeed a partir de este estado (en todas las máquinas).
	CarriedRelic = Relic;

	// Con la reliquia en brazos no se puede llevar farol: se cae al piso (sigue encendido).
	if (Relic && HasAuthority())
	{
		if (ALabLantern* L = HeldLantern.Get())
		{
			L->SetHolder(nullptr);
		}
	}
}

void ALabCharacter::MoveInput(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	DoMove(V.X, V.Y);
}

void ALabCharacter::LookInput(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	DoAim(V.X, V.Y);
}

void ALabCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ALabCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

bool ALabCharacter::CanJumpInternal_Implementation() const
{
	return Super::CanJumpInternal_Implementation() && !bExhausted && IsAliveAndFree();
}

void ALabCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	SpendStamina(GetStaminaSettings()->JumpCost); // solo actúa en el servidor
}

void ALabCharacter::DoJumpStart()
{
	Jump();
}

void ALabCharacter::DoJumpEnd()
{
	StopJumping();
}

void ALabCharacter::LanternInput()
{
	if (HeldLantern.IsValid() && IsAliveAndFree())
	{
		ServerLanternToggle();
	}
}

void ALabCharacter::ServerLanternToggle_Implementation()
{
	if (ALabLantern* L = HeldLantern.Get())
	{
		L->ToggleByHolder(this);
	}
}

ALabLantern* ALabCharacter::GetHeldLantern() const
{
	return HeldLantern.Get();
}

void ALabCharacter::SetHeldLantern(ALabLantern* Lantern)
{
	HeldLantern = Lantern;
}

bool ALabCharacter::HasLitLantern() const
{
	const ALabLantern* L = HeldLantern.Get();
	return L && L->IsLit();
}