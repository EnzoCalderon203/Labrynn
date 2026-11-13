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
#include "Player/LabVoipTalker.h"
#include "Core/LabPlayerState.h"
#include "Audio/LabAudio.h"
#include "Maze/LabVisualSettings.h"
#include "Core/LabGameplaySettings.h"
#include "Interact/LabHidingSpot.h"
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
	static ConstructorHelpers::FObjectFinder<UInputAction> IACrouch(TEXT("/Game/Labrynn/Core/Input/IA_Crouch.IA_Crouch"));
	CrouchAction = IACrouch.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IAThrow(TEXT("/Game/Labrynn/Core/Input/IA_Throw.IA_Throw"));
	ThrowAction = IAThrow.Object;
	static ConstructorHelpers::FObjectFinder<UInputAction> IAPing(TEXT("/Game/Labrynn/Core/Input/IA_Ping.IA_Ping"));
	PingAction = IAPing.Object;
	SprintAction = IASprint.Object;
	InteractAction = IAInteract.Object;
	LanternAction = IALantern.Object;
	TalkAction = IATalk.Object;

	static ConstructorHelpers::FObjectFinder<ULabGameplaySettings> GameplayDA(TEXT("/Game/Labrynn/Core/DA_Gameplay.DA_Gameplay"));
	GameplaySettings = GameplayDA.Object;

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

	// Brazo extendido para dar la mano (provisorio: cilindro hacia adelante y abajo).
	ReachArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReachArm"));
	ReachArm->SetupAttachment(GetCapsuleComponent());
	ReachArm->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	ReachArm->SetVisibility(false);
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ArmMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		ReachArm->SetStaticMesh(ArmMesh.Object);
		const FVector Dir = FVector(0.75f, 0.f, -0.66f).GetSafeNormal();
		ReachArm->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector, Dir));
		ReachArm->SetRelativeLocation(FVector(20.f, 22.f, 45.f) + Dir * 35.f);
		ReachArm->SetRelativeScale3D(FVector(0.09f, 0.09f, 0.7f));
	}

	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true; // C: túneles bajos
}

void ALabCharacter::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
#endif
	MeshRelativeTransform = GetMesh()->GetRelativeTransform();
	CameraRelativeTransform = FirstPersonCameraComponent->GetRelativeTransform();
	FirstPersonMeshBaseRel = FirstPersonMesh->GetRelativeLocation();
	BodyBaseScale = GetMesh()->GetRelativeScale3D();
	StaminaServer = GetStaminaSettings()->MaxStamina;
	GetCharacterMovement()->SetCrouchedHalfHeight(GetGameplaySettings()->CrouchedHalfHeight);
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
	DOREPLIFETIME(ALabCharacter, bHiding);
	DOREPLIFETIME(ALabCharacter, bReaching);
	DOREPLIFETIME(ALabCharacter, Pull);
	DOREPLIFETIME(ALabCharacter, CarriedBody);
	DOREPLIFETIME_CONDITION(ALabCharacter, Throwables, COND_OwnerOnly);
	DOREPLIFETIME(ALabCharacter, CarriedBy);
}

// ---------------------------------------------------------------- Escondites

const ULabGameplaySettings* ALabCharacter::GetGameplaySettings() const
{
	return GameplaySettings ? GameplaySettings.Get() : GetDefault<ULabGameplaySettings>();
}

bool ALabCharacter::IsDetectableByMinotaur() const
{
	return IsAliveAndFree() && (!bHiding || HasLitLantern()); // un farol encendido delata aunque estés escondido
}

void ALabCharacter::EnterHiding(ALabHidingSpot* Spot)
{
	if (!HasAuthority() || !Spot || bHiding)
	{
		return;
	}
	HidingSpot = Spot;
	bHiding = true;
	SetActorLocation(Spot->GetActorLocation() + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	ApplyHiding();
	PlaySfxAll(ELabSfx::HideEnter);
	UE_LOG(LogLabrynn, Log, TEXT("%s se escondió en %s%s"), *GetName(), *Spot->GetName(), HasLitLantern() ? TEXT(" (¡con el farol encendido: se lo ve!)") : TEXT(""));
}

void ALabCharacter::ExitHiding()
{
	if (!HasAuthority() || !bHiding)
	{
		return;
	}
	ALabHidingSpot* Spot = HidingSpot.Get();
	bHiding = false;
	PlaySfxAll(ELabSfx::HideExit);
	HidingSpot.Reset();
	if (Spot)
	{
		Spot->ClearOccupant(this);
		SetActorLocation(Spot->GetExitLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	ApplyHiding();
	UE_LOG(LogLabrynn, Log, TEXT("%s salió del escondite"), *GetName());
}

void ALabCharacter::OnRep_Hiding()
{
	ApplyHiding();
}

void ALabCharacter::ApplyHiding()
{
	// Escondido: sin colisión ni movimiento, cuerpo invisible para todos (vos seguís viendo).
	SetActorEnableCollision(!bHiding);
	GetMesh()->SetHiddenInGame(bHiding);
	FirstPersonMesh->SetHiddenInGame(bHiding);
	if (bHiding)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else if (IsAliveAndFree())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

// ---------------------------------------------------------------- Ojos

void ALabCharacter::TickEyes(float DeltaSeconds)
{
	const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>();
	const ALabMaze* Maze = GS ? GS->GetMaze() : nullptr;
	if (!Maze)
	{
		return;
	}
	const ULabVisualSettings* V = Maze->GetVisualSettings();

	// Silueta tenue (todas las máquinas): brillo personal desde DA_Visuals.
	if (!bGlowFromSettings)
	{
		bGlowFromSettings = true;
		PersonalGlow->SetIntensity(V->PersonalGlowIntensity);
		PersonalGlow->SetAttenuationRadius(V->PersonalGlowRadius);
	}

	if (!IsLocallyControlled() || !FirstPersonCameraComponent)
	{
		return;
	}

	// Visión mínima: luz suave, sin sombras, que existe solo en esta máquina (el resto no la ve).
	if (!LocalVisionLight && V->MinVisionIntensity > 0.f)
	{
		LocalVisionLight = NewObject<UPointLightComponent>(this, TEXT("LocalVisionLight"));
		LocalVisionLight->SetupAttachment(FirstPersonCameraComponent);
		LocalVisionLight->SetIntensityUnits(ELightUnits::Unitless);
		LocalVisionLight->bUseInverseSquaredFalloff = false;
		LocalVisionLight->SetLightFalloffExponent(2.f);
		LocalVisionLight->SetCastShadows(false);
		LocalVisionLight->SetIntensity(V->MinVisionIntensity);
		LocalVisionLight->SetAttenuationRadius(V->MinVisionRadius);
		LocalVisionLight->SetLightColor(V->MinVisionColor);
		LocalVisionLight->SetIsReplicated(false);
		LocalVisionLight->RegisterComponent();
		UE_LOG(LogLabrynn, Log, TEXT("Visión mínima: luz local %.2f, alcance %.0f"), V->MinVisionIntensity, V->MinVisionRadius);
	}

	// ¿Hay un farol encendido cerca? ¿Estoy mirando uno de frente y de cerca?
	const FVector Eye = FirstPersonCameraComponent->GetComponentLocation();
	const FVector View = GetControlRotation().Vector();
	bool bNearLight = false;
	bool bGlaring = false;
	for (TActorIterator<ALabLantern> It(GetWorld()); It; ++It)
	{
		if (!It->IsLit())
		{
			continue;
		}
		const FVector To = It->GetActorLocation() - Eye;
		const float D = To.Size();
		bNearLight |= D < V->LanternNearRadius;
		bGlaring |= D < V->GlareDistance && FVector::DotProduct(View, To / FMath::Max(D, 1.f)) > V->GlareDot;
	}

	// Adaptación: sube lento en la oscuridad, se pierde rápido con luz.
	DarkAdapt = bNearLight
		? FMath::Max(0.f, DarkAdapt - V->LightAdaptSpeed * DeltaSeconds)
		: FMath::Min(1.f, DarkAdapt + DeltaSeconds / V->DarkAdaptSeconds);

	// Encandilamiento: al empezar a mirar el farol; más fuerte cuanto más adaptados estaban los ojos.
	if (bGlaring && !bWasGlaring)
	{
		GlareStrength = 0.35f + 0.65f * DarkAdapt;
		GlareTime = V->GlareSeconds;
		UE_LOG(LogLabrynn, Log, TEXT("Ojos: encandilado (fuerza %.2f, adaptación previa %.2f)"), GlareStrength, DarkAdapt);
	}
	bWasGlaring = bGlaring;
	GlareTime = FMath::Max(0.f, GlareTime - DeltaSeconds);
	const float Glare = V->GlareSeconds > 0.f ? GlareStrength * (GlareTime / V->GlareSeconds) : 0.f;

	FPostProcessSettings& PP = FirstPersonCameraComponent->PostProcessSettings;
	FirstPersonCameraComponent->PostProcessBlendWeight = 1.f;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = V->BaseExposureBias + DarkAdapt * V->DarkAdaptExtraBias + Glare * V->GlareExtraBias;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.675f + Glare * V->GlareBloom;

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("LabEyeLog")) && GetWorld()->GetTimeSeconds() >= NextEyeLog)
	{
		NextEyeLog = GetWorld()->GetTimeSeconds() + 1.f;
		UE_LOG(LogLabrynn, Log, TEXT("Ojos: adaptación=%.2f luzCerca=%d exposición=%.2f encandilamiento=%.2f"),
			DarkAdapt, bNearLight ? 1 : 0, PP.AutoExposureBias, Glare);
	}
#endif
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
	bTalking = bTalk; // también caído o cargado (la voz se procesa según el estado)
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
		// Talker propio: guarda el componente de audio de la voz para procesarla según el estado.
		ULabVoipTalker* LabTalker = NewObject<ULabVoipTalker>(GetPlayerState());
		LabTalker->RegisterWithPlayerState(GetPlayerState());
		VoiceTalker = LabTalker;
		if (VoiceTalker)
		{
			VoiceTalker->Settings.ComponentToAttachTo = GetCapsuleComponent();
			VoiceTalker->Settings.AttenuationSettings = VoiceAttenuation;
			UE_LOG(LogLabrynn, Log, TEXT("Voz: talker 3D creado para %s en %s"), *GetName(), GetNetMode() == NM_Client ? TEXT("cliente") : TEXT("servidor"));
		}
	}

	// Voz según el estado (en cada máquina, sobre la voz de este personaje): filtro, volumen y pitch.
	if (VoiceTalker && !IsLocallyControlled())
	{
		const ULabGameplaySettings* G = GetGameplaySettings();
		const float T = GetWorld()->GetTimeSeconds();
		float Vol = 1.f, LowPass = 20000.f, Pitch = 1.f;
		const TCHAR* Label = TEXT("normal");
		if (!IsAliveAndFree() && !CarriedBy)
		{
			Label = TEXT("ahogada (caído)"); LowPass = G->VoiceDownedLowPass; Vol = G->VoiceDownedVolume;
		}
		else if (CarriedBy)
		{
			Label = TEXT("rebota (cargado)"); LowPass = G->VoiceDownedLowPass * 1.6f;
			Vol = 0.7f + 0.3f * FMath::Abs(FMath::Sin(T * PI * 2.2f)); Pitch = 1.f + 0.03f * FMath::Sin(T * 13.8f);
		}
		else if (bHiding)
		{
			Label = TEXT("baja (escondido)"); LowPass = G->VoiceHiddenLowPass; Vol = G->VoiceHiddenVolume;
		}
		else if (bExhausted || GetVelocity().Size2D() > 550.f)
		{
			Label = TEXT("agitada (corriendo / sin aire)"); Vol = 0.85f + 0.15f * FMath::Sin(T * 14.f); Pitch = 1.03f;
		}
		if (IsAliveAndFree() && GetWounds01() >= G->VoiceWoundedThreshold)
		{
			// Herido: jadeante y entrecortada (temblor + cortes breves).
			Label = (FCString::Strcmp(Label, TEXT("normal")) == 0) ? TEXT("entrecortada (herido)") : Label;
			Vol *= 0.75f + 0.25f * FMath::Sin(T * 38.f);
			if (FMath::Frac(T * 3.1f) < 0.12f) { Vol *= 0.2f; }
		}
		if (UAudioComponent* VC = Cast<ULabVoipTalker>(VoiceTalker)->GetVoiceComponent())
		{
			VC->SetVolumeMultiplier(Vol);
			VC->SetLowPassFilterEnabled(LowPass < 19000.f);
			VC->SetLowPassFilterFrequency(LowPass);
			VC->SetPitchMultiplier(Pitch);
		}
		if (FCString::Strcmp(Label, *LastVoiceFxLabel) != 0)
		{
			LastVoiceFxLabel = Label;
			UE_LOG(LogLabrynn, Log, TEXT("Voz[%s] en %s: %s (filtro %.0f Hz, vol %.2f)"), *GetName(),
				GetNetMode() == NM_Client ? TEXT("cliente") : TEXT("servidor"), Label, LowPass, Vol);
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
		if (bHiding)
		{
			Level *= GetGameplaySettings()->HiddenVoiceNoiseMultiplier; // escondido: la voz hace menos ruido
		}
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), Level, this, VoiceNoiseRange * Level, FName("Voz"));
		if (FLabStats* St = GetStatsServer()) { St->VoiceNoises++; }
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
	if (bReaching)
	{
		return 0.f; // sosteniendo la mano: no se mueve
	}
	float Speed = S->WalkSpeed;
	if (bExhausted)
	{
		Speed = S->ExhaustedSpeed;
	}
	else if (bWantsSprint && CanSprint())
	{
		Speed = S->SprintSpeed;
	}
	if (CarriedBody)
	{
		Speed *= GetGameplaySettings()->CarrySpeedMultiplier; // compañero al hombro: lento
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
	if (CarriedBody)
	{
		SpendStamina(GetGameplaySettings()->CarryStaminaPerSecond * DeltaSeconds);
		if (FLabStats* St = GetStatsServer()) { St->CarryMeters += GetVelocity().Size2D() * DeltaSeconds / 100.f; }
		if (bExhausted)
		{
			UE_LOG(LogLabrynn, Log, TEXT("Cargar: %s se quedó sin aire y suelta a %s"), *GetName(), *CarriedBody->GetName());
			DropCarried();
		}
	}

	if (bHiding)
	{
		if (FLabStats* St = GetStatsServer()) { St->HiddenSeconds += DeltaSeconds; }
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
	const float WoundsBefore = WoundsServer;
	WoundsServer = FMath::Min(S->MaxStamina, WoundsServer + Amount);
	StaminaServer = FMath::Min(StaminaServer, S->MaxStamina - WoundsServer);
	SyncStaminaBytes();
	if (FLabStats* St = GetStatsServer()) { St->WoundsTaken += WoundsServer - WoundsBefore; }
	UE_LOG(LogLabrynn, Log, TEXT("%s herido: +%.0f (heridas %.0f/%.0f)"), *GetName(), Amount, WoundsServer, S->MaxStamina);

	if (WoundsServer >= S->MaxStamina)
	{
		KnockDown(Impulse); // las heridas llenaron la barra: ragdoll
	}
	else
	{
		PlaySfxAll(ELabSfx::Hurt);
		if (!Impulse.IsNearlyZero())
		{
			LaunchCharacter(Impulse, true, true); // empujón sin caer
		}
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
	GetCharacterMovement()->SetCrouchedHalfHeight(GetGameplaySettings()->CrouchedHalfHeight);
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
	if (bExhausted && GetNetMode() != NM_DedicatedServer)
	{
		if (!BreathAudio)
		{
			BreathAudio = ULabAudio::PlayAttached(ELabSfx::Breath, GetCapsuleComponent());
			if (BreathAudio) { BreathAudio->bAutoDestroy = false; }
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
	ExitHiding(); // si estaba escondido, sale
	DropCarried(); // si cargaba a alguien, caen los dos

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
	if (FLabStats* St = GetStatsServer()) { St->Downs++; }
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
	// Si lo estaban cargando, primero lo bajan.
	if (CarriedBy)
	{
		CarriedBy->DropCarried();
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
		ULabAudio::Play(this, ELabSfx::BodyFall, GetActorLocation()); // en cada máquina (OnRep)
	}
	else if (!bWasAlive && bIsAlive)
	{
		StopRagdoll();
		ULabAudio::Play(this, ELabSfx::Revive, GetActorLocation());
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
	const bool bValid = R && R->IsAliveAndFree() && !R->IsCarryingBody() && FVector::Dist2D(R->GetActorLocation(), GetActorLocation()) <= R->InteractRange + 160.f;
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
			if (FLabStats* St = R->GetStatsServer()) { St->Revives++; }
			Revive();
			return;
		}
	}
	ReviveByte = static_cast<uint8>(FMath::Clamp(ReviveProgressServer, 0.f, 1.f) * 255.f);
}

bool ALabCharacter::CanInteract(const ALabCharacter* By) const
{
	if (!By || By == this || !By->IsAliveAndFree())
	{
		return false;
	}
	if (LifeState == ELabLifeState::Downed)
	{
		return true; // levantarlo
	}
	if (!IsAliveAndFree() || bHiding)
	{
		return false;
	}
	if (bReaching)
	{
		// Agarrarse de la mano: hay que llegar (en la práctica, saltando).
		return !By->Pull.bActive && FVector::Dist(By->GetActorLocation() + FVector(0.f, 0.f, 60.f), GetHandPoint()) <= GetGameplaySettings()->HandReach;
	}
	return bIsCrouched && !By->bIsCrouched; // pie de banco
}

void ALabCharacter::Interact(ALabCharacter* By)
{
	if (!HasAuthority() || !CanInteract(By))
	{
		return;
	}
	if (LifeState == ELabLifeState::Downed)
	{
		Reviver = By; // tiene que quedarse cerca ReviveSeconds
	}
	else if (bReaching)
	{
		StartPull(By);
	}
	else
	{
		// Pie de banco: el de arriba sale impulsado hacia adelante y arriba.
		const ULabGameplaySettings* G = GetGameplaySettings();
		const FVector Vel = By->GetActorForwardVector() * G->BoostLaunchForward + FVector(0.f, 0.f, G->BoostLaunchUp);
		By->BoostIgnorePawns();
		By->LaunchCharacter(Vel, true, true);
		By->ClientLaunch(Vel); // el dueño lo predice igual (sin tirón por corrección)
		PlaySfxAll(ELabSfx::Effort);
		UE_LOG(LogLabrynn, Log, TEXT("Ayuda: %s hace pie de banco a %s (vel=%s, mira yaw=%.0f)"), *GetName(), *By->GetName(), *Vel.ToCompactString(), By->GetActorRotation().Yaw);
#if !UE_BUILD_SHIPPING
		for (const float Dt : { 0.1f, 0.3f })
		{
			FTimerHandle H;
			TWeakObjectPtr<ALabCharacter> WeakBy = By;
			GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [WeakBy, Dt]()
			{
				if (ALabCharacter* B = WeakBy.Get())
				{
					UE_LOG(LogLabrynn, Log, TEXT("Ayuda: +%.1fs vel=%s loc=%s modo=%d"), Dt, *B->GetVelocity().ToCompactString(), *B->GetActorLocation().ToCompactString(), (int32)B->GetCharacterMovement()->MovementMode.GetValue());
				}
			}), Dt, false);
		}
#endif
	}
}

FText ALabCharacter::GetInteractPrompt(const ALabCharacter* By) const
{
	if (LifeState == ELabLifeState::Downed)
	{
		const FString Name = GetPlayerState() ? GetPlayerState()->GetPlayerName() : TEXT("compañero");
		return FText::FromString(FString::Printf(TEXT("Levantar a %s (quedate cerca)"), *Name));
	}
	if (bReaching)
	{
		return FText::FromString(TEXT("Agarrarse de la mano"));
	}
	return FText::FromString(TEXT("Subir con pie de banco"));
}

// ---------------------------------------------------------------- Pie de banco y dar la mano

void ALabCharacter::ClientLaunch_Implementation(FVector Velocity)
{
	if (!HasAuthority())
	{
		BoostIgnorePawns();
		LaunchCharacter(Velocity, true, true);
	}
}

void ALabCharacter::BoostIgnorePawns()
{
	// Durante el impulso atraviesa al compañero que hace de pie de banco (si no, se choca con él).
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	// Sin frenado en el aire durante el impulso: avance constante para pasar por encima del borde.
	const float SavedBraking = GetCharacterMovement()->BrakingDecelerationFalling;
	GetCharacterMovement()->BrakingDecelerationFalling = 0.f;
	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this, SavedBraking]()
	{
		GetCharacterMovement()->BrakingDecelerationFalling = SavedBraking;
		if (IsAliveAndFree()) { GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); }
	}), 0.8f, false);
}

FVector ALabCharacter::GetHandPoint() const
{
	const FVector Fwd = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	return GetActorLocation() + Fwd * 70.f + FVector(0.f, 0.f, -20.f);
}

bool ALabCharacter::CanStartReach() const
{
	if (!IsAliveAndFree() || bHiding || bIsCrouched || IsCarryingRelic() || Pull.bActive)
	{
		return false;
	}
	if (FRotator::NormalizeAxis(GetControlRotation().Pitch) > -20.f)
	{
		return false; // hay que mirar hacia abajo
	}
	// Desnivel adelante: el piso de enfrente está bastante más abajo que mis pies.
	const FVector Fwd = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	const FVector Start = GetActorLocation() + Fwd * 90.f;
	FHitResult Hit;
	FCollisionQueryParams P(SCENE_QUERY_STAT(LabReach), false, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 500.f), ECC_Visibility, P))
	{
		return false;
	}
	const float Feet = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return Feet - Hit.ImpactPoint.Z > 100.f;
}

void ALabCharacter::InteractReleased()
{
	bInteractHeld = false;
	if (bReaching)
	{
		ServerSetReaching(false);
	}
}

void ALabCharacter::ServerSetReaching_Implementation(bool bReach)
{
	if (bReach && !CanStartReach())
	{
		return;
	}
	if (bReaching == bReach)
	{
		return;
	}
	bReaching = bReach;
	OnRep_Reaching();
	UE_LOG(LogLabrynn, Log, TEXT("Ayuda: %s %s la mano"), *GetName(), bReaching ? TEXT("estira") : TEXT("retira"));
	if (!bReaching)
	{
		if (ALabCharacter* L = Pulling.Get())
		{
			L->EndPull(false); // lo soltó
		}
	}
}

void ALabCharacter::OnRep_Reaching()
{
	ReachArm->SetVisibility(bReaching);
}

void ALabCharacter::StartPull(ALabCharacter* Lower)
{
	check(HasAuthority());
	const ULabGameplaySettings* G = GetGameplaySettings();
	const FVector Back = -FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
	Lower->Pull.bActive = true;
	Lower->Pull.From = Lower->GetActorLocation();
	Lower->Pull.To = GetActorLocation() + Back * 75.f + FVector(0.f, 0.f, 5.f); // arriba, detrás del que ayuda
	Lower->Pull.StartServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	Lower->Pull.Duration = G->PullSeconds;
	Lower->PulledBy = this;
	Pulling = Lower;
	Lower->ApplyPull();
	PlaySfxAll(ELabSfx::Effort);
	UE_LOG(LogLabrynn, Log, TEXT("Ayuda: %s se agarró de la mano de %s"), *Lower->GetName(), *GetName());
}

void ALabCharacter::EndPull(bool bSuccess)
{
	if (!HasAuthority() || !Pull.bActive)
	{
		return;
	}
	Pull.bActive = false;
	if (ALabCharacter* Helper = PulledBy.Get())
	{
		Helper->Pulling.Reset();
	}
	PulledBy.Reset();
	ApplyPull();
	UE_LOG(LogLabrynn, Log, TEXT("Ayuda: %s %s"), *GetName(), bSuccess ? TEXT("subió") : TEXT("se soltó y cae"));
}

void ALabCharacter::OnRep_Pull()
{
	ApplyPull();
}

void ALabCharacter::ApplyPull()
{
	if (Pull.bActive)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else if (IsAliveAndFree() && !bHiding)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling); // aterriza solo (arriba o abajo)
	}
}

void ALabCharacter::TickHelp(float DeltaSeconds)
{
	// El que está siendo subido (servidor y dueño interpolan igual, con datos replicados).
	if (Pull.bActive && (HasAuthority() || IsLocallyControlled()))
	{
		const float Now = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		const float Alpha = FMath::Clamp((Now - Pull.StartServerTime) / FMath::Max(0.1f, Pull.Duration), 0.f, 1.f);
		const FVector Pos = FMath::Lerp(FVector(Pull.From), FVector(Pull.To), Alpha) + FVector(0.f, 0.f, FMath::Sin(Alpha * PI) * 40.f);
		SetActorLocation(Pos, false, nullptr, ETeleportType::TeleportPhysics);
		if (HasAuthority() && Alpha >= 1.f)
		{
			EndPull(true);
		}
	}

	// El que tira: gasta stamina; si se queda sin, suelta.
	if (HasAuthority())
	{
		if (ALabCharacter* L = Pulling.Get())
		{
			const ULabGameplaySettings* G = GetGameplaySettings();
			const float Mult = L->IsCarryingRelic() ? G->PullRelicMultiplier : 1.f;
			SpendStamina(G->PullStaminaPerSecond * Mult * DeltaSeconds);
			if (StaminaServer <= 0.f || !bReaching || !IsAliveAndFree())
			{
				UE_LOG(LogLabrynn, Log, TEXT("Ayuda: %s no aguanta más y suelta a %s"), *GetName(), *L->GetName());
				L->EndPull(false);
			}
		}
	}
}

void ALabCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickStamina(DeltaSeconds);
	TickVoice(DeltaSeconds);
	TickEyes(DeltaSeconds);
	TickHelp(DeltaSeconds);
	TickCrouchVisual(DeltaSeconds);
	TickFootsteps(DeltaSeconds);
	if (IsLocallyControlled())
	{
		TickCarryHold();
		TickAim();
	}

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
	if ((bRagdoll || CarriedBy) && IsLocallyControlled()) // caído o al hombro: ve desde su cuerpo
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
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ALabCharacter::InteractInput);
			EIC->BindAction(InteractAction, ETriggerEvent::Completed, this, &ALabCharacter::InteractReleased); // soltar la mano
		}
		if (LanternAction)  { EIC->BindAction(LanternAction, ETriggerEvent::Started, this, &ALabCharacter::LanternInput); }
		if (PingAction)
		{
			EIC->BindAction(PingAction, ETriggerEvent::Started, this, &ALabCharacter::PingPressed);
		}
		if (ThrowAction)
		{
			EIC->BindAction(ThrowAction, ETriggerEvent::Started, this, &ALabCharacter::ThrowPressed);
			EIC->BindAction(ThrowAction, ETriggerEvent::Completed, this, &ALabCharacter::ThrowReleased);
		}
		if (CrouchAction)
		{
			EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ALabCharacter::CrouchPressed);
			EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ALabCharacter::CrouchReleased);
		}
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
	if (!IsAliveAndFree())
	{
		return;
	}
	// En un borde, mirando hacia abajo y sin nada que interactuar: E (mantenida) = estirar la mano.
	AActor* Target = bHiding || CarriedBody ? nullptr : FindBestInteractable();
	if (!bHiding && !CarriedBody && !Target && CanStartReach())
	{
		ServerSetReaching(true);
		return;
	}
	// Sobre un caído: tocar E = levantar; mantener E = cargarlo (lo resuelve TickCarryHold).
	bInteractHeld = true;
	InteractPressTime = GetWorld()->GetTimeSeconds();
	ALabCharacter* Body = Cast<ALabCharacter>(Target);
	InteractHoldTarget = (Body && !Body->IsAliveAndFree()) ? Body : nullptr;
	ServerInteract();
}

// ---------------------------------------------------------------- Estadísticas y ping

FLabStats* ALabCharacter::GetStatsServer() const
{
	ALabPlayerState* PS = HasAuthority() ? GetPlayerState<ALabPlayerState>() : nullptr;
	return PS ? &PS->Stats : nullptr;
}

void ALabCharacter::PingPressed()
{
	if (LifeState == ELabLifeState::Dead)
	{
		return; // los muertos no marcan (los caídos sí: "¡estoy acá!")
	}
	const FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	const FVector End = Start + GetControlRotation().Vector() * GetGameplaySettings()->PingRange;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(LabPing), false, this);
	const FVector Loc = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Q) ? Hit.ImpactPoint : End;
	ServerPing(Loc);
}

void ALabCharacter::ServerPing_Implementation(FVector Location)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const ULabGameplaySettings* G = GetGameplaySettings();
	if (Now - LastPingServerTime < G->PingCooldown || LifeState == ELabLifeState::Dead
		|| FVector::Dist(Location, GetActorLocation()) > G->PingRange + 300.f)
	{
		return;
	}
	LastPingServerTime = Now;
	if (ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>())
	{
		GS->AddPing(Location, GetPlayerState() ? GetPlayerState()->GetPlayerName() : GetName());
		if (FLabStats* St = GetStatsServer()) { St->Pings++; }
	}
}

// ---------------------------------------------------------------- Lanzables

bool ALabCharacter::CanCarryMoreThrowables() const
{
	return Throwables.Num() < GetGameplaySettings()->MaxThrowablesCarried;
}

void ALabCharacter::AddThrowable(ELabThrowKind Kind)
{
	if (HasAuthority() && CanCarryMoreThrowables())
	{
		Throwables.Add(Kind);
		PlaySfxAll(ELabSfx::Pickup);
		UE_LOG(LogLabrynn, Log, TEXT("Lanzable: %s juntó una %s (lleva %d)"), *GetName(), Kind == ELabThrowKind::Bottle ? TEXT("botella") : TEXT("piedra"), Throwables.Num());
	}
}

FVector ALabCharacter::GetThrowOrigin() const
{
	return FirstPersonCameraComponent->GetComponentLocation() + GetControlRotation().Vector() * 45.f;
}

FVector ALabCharacter::GetThrowVelocity() const
{
	const ULabGameplaySettings* G = GetGameplaySettings();
	return GetControlRotation().Vector() * G->ThrowSpeed + FVector(0.f, 0.f, G->ThrowUpBoost);
}

void ALabCharacter::ThrowPressed()
{
	bAiming = Throwables.Num() > 0 && IsAliveAndFree() && !bHiding && !bReaching;
}

void ALabCharacter::ThrowReleased()
{
	if (bAiming)
	{
		bAiming = false;
		AimPath.Reset();
		ServerThrow(GetThrowOrigin(), GetThrowVelocity());
	}
}

void ALabCharacter::TickAim()
{
	if (!bAiming)
	{
		return;
	}
	// Arco de trayectoria (se dibuja en el HUD).
	FPredictProjectilePathParams P(8.f, GetThrowOrigin(), GetThrowVelocity(), 2.5f, ECC_WorldStatic, this);
	P.SimFrequency = 12.f;
	P.bTraceWithCollision = true;
	FPredictProjectilePathResult R;
	UGameplayStatics::PredictProjectilePath(this, P, R);
	AimPath.Reset();
	for (const FPredictProjectilePathPointData& D : R.PathData)
	{
		AimPath.Add(D.Location);
	}
}

void ALabCharacter::ServerThrow_Implementation(FVector Origin, FVector Velocity)
{
	if (Throwables.Num() == 0 || !IsAliveAndFree() || bHiding)
	{
		return;
	}
	// Validación anti-trampa simple: el origen cerca de la cámara y la velocidad acotada.
	if (FVector::Dist(Origin, GetActorLocation()) > 250.f)
	{
		Origin = GetActorLocation() + FVector(0.f, 0.f, 60.f);
	}
	const ULabGameplaySettings* G = GetGameplaySettings();
	Velocity = Velocity.GetClampedToMaxSize(G->ThrowSpeed + G->ThrowUpBoost + 50.f);

	const ELabThrowKind Kind = Throwables.Pop();
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALabThrowable* T = GetWorld()->SpawnActorDeferred<ALabThrowable>(ALabThrowable::StaticClass(), FTransform(Origin), this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (T)
	{
		T->Kind = Kind;
		T->FinishSpawning(FTransform(Origin));
		T->Launch(Velocity, this);
		PlaySfxAll(ELabSfx::Throw);
		if (FLabStats* St = GetStatsServer()) { St->Thrown++; }
		UE_LOG(LogLabrynn, Log, TEXT("Lanzable: %s lanzó una %s (quedan %d)"), *GetName(), Kind == ELabThrowKind::Bottle ? TEXT("botella") : TEXT("piedra"), Throwables.Num());
	}
}

void ALabCharacter::TickCarryHold()
{
	ALabCharacter* Body = InteractHoldTarget.Get();
	if (!bInteractHeld || !Body || CarriedBody)
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - InteractPressTime >= GetGameplaySettings()->CarryHoldSeconds)
	{
		bInteractHeld = false;
		InteractHoldTarget.Reset();
		ServerStartCarry(Body);
	}
}

void ALabCharacter::ServerStartCarry_Implementation(ALabCharacter* Body)
{
	if (!Body || Body == this || Body->IsAliveAndFree() || Body->CarriedBy || CarriedBody || !IsAliveAndFree()
		|| IsCarryingRelic() || bReaching || bHiding || FVector::Dist2D(Body->GetActorLocation(), GetActorLocation()) > InteractRange + 150.f)
	{
		return;
	}
	StartCarry(Body);
}

void ALabCharacter::StartCarry(ALabCharacter* Body)
{
	check(HasAuthority());
	if (ALabLantern* L = HeldLantern.Get())
	{
		L->SetHolder(nullptr); // con alguien al hombro no hay farol: lo suelta (o lo lleva otro)
	}
	Body->Reviver.Reset();
	CarriedBody = Body;
	Body->CarriedBy = this;
	Body->ApplyCarriedPose();
	PlaySfxAll(ELabSfx::CarryLift);
	UE_LOG(LogLabrynn, Log, TEXT("Cargar: %s carga al hombro a %s"), *GetName(), *Body->GetName());
}

void ALabCharacter::DropCarried()
{
	ALabCharacter* Body = CarriedBody;
	if (!HasAuthority() || !Body)
	{
		return;
	}
	CarriedBody = nullptr;
	Body->CarriedBy = nullptr;
	Body->ApplyCarriedPose(); // vuelve al ragdoll, al lado
	const FVector Side = GetActorForwardVector() * 70.f + FVector(0.f, 0.f, 40.f);
	Body->SetActorLocation(GetActorLocation() + Side, false, nullptr, ETeleportType::TeleportPhysics);
	PlaySfxAll(ELabSfx::Drop);
	UE_LOG(LogLabrynn, Log, TEXT("Cargar: %s suelta a %s"), *GetName(), *Body->GetName());
}

void ALabCharacter::OnRep_CarriedBy()
{
	ApplyCarriedPose();
}

void ALabCharacter::ApplyCarriedPose()
{
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (CarriedBy)
	{
		// Al hombro: sin física ni colisión, pegado a la cápsula del que carga (pose provisoria, cuerpo cruzado).
		bRagdoll = false;
		BodyMesh->SetAllBodiesSimulatePhysics(false);
		BodyMesh->SetSimulatePhysics(false);
		BodyMesh->SetCollisionProfileName(TEXT("NoCollision"));
		BodyMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		BodyMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -10.f), FRotator(0.f, 0.f, 90.f));
		SetActorEnableCollision(false);
		AttachToComponent(CarriedBy->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(5.f, -18.f, 85.f));
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorEnableCollision(true);
		if (!IsAliveAndFree())
		{
			BodyMesh->SetRelativeTransform(MeshRelativeTransform);
			DownImpulse = FVector::ZeroVector;
			StartRagdoll(); // vuelve a desparramarse en el piso
		}
	}
}

void ALabCharacter::ServerInteract_Implementation()
{
	if (!IsAliveAndFree())
	{
		return;
	}
	if (bHiding)
	{
		ExitHiding(); // escondido, E solo sirve para salir
		return;
	}
	if (CarriedBody)
	{
		DropCarried(); // cargando a alguien, E lo suelta
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

void ALabCharacter::CrouchPressed()
{
	if (IsAliveAndFree() && !bHiding)
	{
		Crouch(); // replicado por el CharacterMovement
	}
}

void ALabCharacter::CrouchReleased()
{
	UnCrouch();
}

void ALabCharacter::TickCrouchVisual(float DeltaSeconds)
{
	// El motor compensa la cápsula subiendo el modelo, así que la cabeza (y la cámara pegada a ella) no bajaba.
	const float Target = (bIsCrouched && !bRagdoll && !CarriedBy) ? 1.f : 0.f;
	const float Old = CrouchVisualAlpha;
	CrouchVisualAlpha = FMath::FInterpTo(CrouchVisualAlpha, Target, DeltaSeconds, 10.f);
	if (FMath::IsNearlyEqual(Old, CrouchVisualAlpha) && FMath::IsNearlyEqual(CrouchVisualAlpha, Target))
	{
		return;
	}
	const float StandHalf = GetClass()->GetDefaultObject<ALabCharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float Drop = 2.f * (StandHalf - GetCharacterMovement()->GetCrouchedHalfHeight()) * 0.85f;
	if (IsLocallyControlled())
	{
		// Mi vista: bajan los brazos y con ellos la cámara (pegada a la cabeza de los brazos).
		FirstPersonMesh->SetRelativeLocation(FirstPersonMeshBaseRel - FVector(0.f, 0.f, Drop * CrouchVisualAlpha));
	}
	else
	{
		// Los demás: el cuerpo se achica hacia los pies (provisorio hasta tener animación de agachado).
		const float ScaleZ = FMath::Lerp(1.f, 0.62f, CrouchVisualAlpha);
		GetMesh()->SetRelativeScale3D(FVector(BodyBaseScale.X, BodyBaseScale.Y, BodyBaseScale.Z * ScaleZ));
	}
}

bool ALabCharacter::CanJumpInternal_Implementation() const
{
	return Super::CanJumpInternal_Implementation() && !bExhausted && IsAliveAndFree() && !bHiding;
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
	if (HeldLantern.IsValid() && IsAliveAndFree() && !bReaching) // con la mano estirada no se usa el farol
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
// ---------------------------------------------------------------- Audio

void ALabCharacter::PlaySfxAll(ELabSfx Id)
{
	if (HasAuthority())
	{
		MulticastSfx(Id, GetActorLocation());
	}
}

void ALabCharacter::MulticastSfx_Implementation(ELabSfx Id, FVector_NetQuantize Location)
{
	ULabAudio::Play(this, Id, Location);
}

void ALabCharacter::TickFootsteps(float DeltaSeconds)
{
	if (GetNetMode() == NM_DedicatedServer || bRagdoll || CarriedBy || bHiding || !IsAliveAndFree())
	{
		bWasFalling = false;
		return;
	}
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const bool bFalling = CMC->IsFalling();
	if (bFalling)
	{
		if (!bWasFalling && GetVelocity().Z > 150.f)
		{
			ULabAudio::Play(this, ELabSfx::Jump, Feet + FVector(0.f, 0.f, 120.f));
		}
		LastFallSpeed = FMath::Min(LastFallSpeed, GetVelocity().Z);
	}
	else if (bWasFalling)
	{
		// Aterrizaje: más fuerte cuanto más rápido caía.
		if (LastFallSpeed < -250.f)
		{
			ULabAudio::Play(this, ELabSfx::Land, Feet, FMath::Clamp(-LastFallSpeed / 900.f, 0.4f, 1.f));
		}
		StepDistance = 0.f;
	}
	if (!bFalling)
	{
		LastFallSpeed = 0.f;
	}
	bWasFalling = bFalling;

	const float Speed = GetVelocity().Size2D();
	if (bFalling || Speed < 60.f || !CMC->IsMovingOnGround())
	{
		return;
	}
	// Agachado: pasos cortos y suaves; corriendo: largos y fuertes (se oyen tan lejos como el ruido de sprint).
	const bool bRun = Speed > 500.f;
	const float Stride = bIsCrouched ? 100.f : (bRun ? 190.f : 150.f);
	StepDistance += Speed * DeltaSeconds;
	if (StepDistance >= Stride)
	{
		StepDistance = 0.f;
		ULabAudio::Play(this, ELabSfx::Footstep, Feet, bIsCrouched ? 0.3f : (bRun ? 1.f : 0.6f));
	}
}