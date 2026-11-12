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
#include "Labrynn.h"

ALabCharacter::ALabCharacter()
{
	bReplicates = true;

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

	// Linterna en la cámara.
	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(FirstPersonCameraComponent);
	Flashlight->SetRelativeLocation(FVector(10.f, 15.f, -10.f));
	Flashlight->SetInnerConeAngle(14.f);
	Flashlight->SetOuterConeAngle(30.f);
	Flashlight->SetCastShadows(true);
	Flashlight->bUseInverseSquaredFalloff = true;

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
	FParse::Value(FCommandLine::Get(), TEXT("LabFlashlight="), FlashlightIntensity); // calibración rápida
	FParse::Value(FCommandLine::Get(), TEXT("LabBatterySeconds="), BatterySeconds);
#endif
	Flashlight->SetIntensity(FlashlightIntensity);
	Flashlight->SetAttenuationRadius(FlashlightRange);
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	MeshRelativeTransform = GetMesh()->GetRelativeTransform();
	CameraRelativeTransform = FirstPersonCameraComponent->GetRelativeTransform();
	Health = MaxHealth;
	PersonalGlow->SetIntensity(PersonalGlowIntensity);
	PersonalGlow->SetAttenuationRadius(PersonalGlowRadius);
	ApplyFlashlightState();

	static const TCHAR* Roles[] = { TEXT("None"), TEXT("SimulatedProxy"), TEXT("AutonomousProxy"), TEXT("Authority") };
	UE_LOG(LogLabrynn, Log, TEXT("[%s] Personaje %s BeginPlay: LocalRole=%s Local=%d Body=%s Loc=%s"),
		GetNetMode() == NM_Client ? TEXT("Cliente") : TEXT("Servidor"), *GetName(),
		Roles[FMath::Clamp<int32>(GetLocalRole(), 0, 3)], IsLocallyControlled() ? 1 : 0,
		*GetNameSafe(GetMesh()->GetSkeletalMeshAsset()), *GetActorLocation().ToCompactString());
}

void ALabCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALabCharacter, bFlashlightOn);
	DOREPLIFETIME(ALabCharacter, bHasFlashlight);
	DOREPLIFETIME(ALabCharacter, BatteryByte);
	DOREPLIFETIME(ALabCharacter, LifeState);
	DOREPLIFETIME(ALabCharacter, Health);
	DOREPLIFETIME(ALabCharacter, ReviveByte);
	DOREPLIFETIME(ALabCharacter, DownedAtServerTime);
	DOREPLIFETIME(ALabCharacter, DownImpulse);
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
	const float Applied = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Max(0.f, Health - Applied);
	if (Health <= 0.f)
	{
		const FVector Push = DamageCauser ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal() * 300.f : FVector::ZeroVector;
		KnockDown(Push + FVector(0, 0, 200.f));
	}
	return Applied;
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

	const ELabLifeState Old = LifeState;
	Health = 0.f;
	LifeState = ELabLifeState::Downed;
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

void ALabCharacter::Revive(float HealthAfter)
{
	check(HasAuthority());
	if (LifeState == ELabLifeState::Alive)
	{
		return;
	}
	const ELabLifeState Old = LifeState;
	LifeState = ELabLifeState::Alive;
	Health = FMath::Clamp(HealthAfter, 1.f, MaxHealth);
	ReviveProgressServer = 0.f;
	ReviveByte = 0;
	Reviver.Reset();
	UE_LOG(LogLabrynn, Log, TEXT("%s se levantó (salud %.0f)"), *GetName(), Health);
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
	ApplyFlashlightState();
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
	const bool bValid = R && R->IsAliveAndFree() && FVector::Dist(R->GetActorLocation(), GetActorLocation()) <= R->InteractRange + 60.f;
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
			Revive(50.f);
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

	// Servidor: descarga de batería.
	if (HasAuthority() && bHasFlashlight && bFlashlightOn && BatteryServer > 0.f)
	{
		BatteryServer = FMath::Max(0.f, BatteryServer - DeltaSeconds / BatterySeconds);
		const uint8 NewByte = static_cast<uint8>(FMath::CeilToInt(BatteryServer * 255.f));
		if (NewByte != BatteryByte)
		{
			BatteryByte = NewByte;
		}
		if (BatteryServer <= 0.f)
		{
			bFlashlightOn = false; // se agotó
			ApplyFlashlightState();
		}
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
	if (bRagdoll && IsLocallyControlled())
	{
		const FVector Pelvis = GetMesh()->GetSocketLocation(FName("pelvis"));
		const FVector CamPos = Pelvis + FVector(-160.f, -60.f, 170.f);
		FirstPersonCameraComponent->SetWorldLocationAndRotation(CamPos, (Pelvis - CamPos).Rotation());
	}

	// Todas las máquinas: intensidad según batería, con parpadeo cosmético cuando queda poca.
	UpdateFlashlightIntensity(DeltaSeconds);
}

void ALabCharacter::UpdateFlashlightIntensity(float DeltaSeconds)
{
	if (!Flashlight || !Flashlight->IsVisible())
	{
		return;
	}
	const float Battery = GetBattery();
	float Strength = 1.f;
	if (Battery < LowBatteryThreshold)
	{
		// Se debilita y parpadea más seguido cuanto menos carga queda.
		const float Low = Battery / FMath::Max(LowBatteryThreshold, KINDA_SMALL_NUMBER); // 0..1
		FlickerTimer -= DeltaSeconds;
		if (FlickerTimer <= 0.f)
		{
			const bool bDip = FMath::FRand() > 0.35f + 0.6f * Low;
			FlickerFactor = bDip ? FMath::FRandRange(0.05f, 0.4f) : 1.f;
			FlickerTimer = bDip ? FMath::FRandRange(0.04f, 0.12f) : FMath::FRandRange(0.1f, 0.2f + 1.5f * Low);
		}
		Strength = FMath::Lerp(0.35f, 1.f, Low) * FlickerFactor;
	}
	Flashlight->SetIntensity(FlashlightIntensity * Strength);
}

void ALabCharacter::SetHasFlashlight(bool bHas)
{
	check(HasAuthority());
	bHasFlashlight = bHas;
	ApplyFlashlightState();
}

void ALabCharacter::AddBattery(float Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	BatteryServer = FMath::Clamp(BatteryServer + Amount, 0.f, 1.f);
	BatteryByte = static_cast<uint8>(FMath::CeilToInt(BatteryServer * 255.f));
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
	}
	else
	{
		UE_LOG(LogLabrynn, Error, TEXT("'%s' no tiene EnhancedInputComponent."), *GetNameSafe(this));
	}

	// Teclas sin asset de input (temporal hasta tener IA_ propios).
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ALabCharacter::ToggleFlashlightInput);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ALabCharacter::InteractInput);
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
		const float Dist = To.Size();
		if (Dist > InteractRange)
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
	CarriedRelic = Relic;
	// Se aplica en todas las máquinas (servidor y cliente dueño deben coincidir para la predicción de movimiento).
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SpeedMultiplier;

	if (Relic)
	{
		// Con las manos ocupadas no hay linterna.
		if (HasAuthority())
		{
			bFlashlightOn = false;
		}
		ApplyFlashlightState();
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

void ALabCharacter::DoJumpStart()
{
	Jump();
}

void ALabCharacter::DoJumpEnd()
{
	StopJumping();
}

void ALabCharacter::ToggleFlashlightInput()
{
	if (!bHasFlashlight || IsCarryingRelic() || (!bFlashlightOn && GetBattery() <= 0.f))
	{
		return; // sin linterna o sin batería: no hace nada (TODO: sonido de "clic" vacío)
	}
	// Predicción local inmediata; el servidor confirma.
	bFlashlightOn = !bFlashlightOn;
	ApplyFlashlightState();
	ServerSetFlashlight(bFlashlightOn);
}

void ALabCharacter::ServerSetFlashlight_Implementation(bool bOn)
{
	// El servidor valida: no se puede prender sin linterna o sin batería.
	bFlashlightOn = bOn && bHasFlashlight && BatteryServer > 0.f && !IsCarryingRelic();
	ApplyFlashlightState(); // en el listen server OnRep no se dispara solo
}

void ALabCharacter::OnRep_FlashlightOn()
{
	ApplyFlashlightState();
}

void ALabCharacter::ApplyFlashlightState()
{
	if (Flashlight)
	{
		Flashlight->SetVisibility(bHasFlashlight && bFlashlightOn && !IsCarryingRelic() && IsAliveAndFree());
	}
}
