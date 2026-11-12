#include "Player/LabCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
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

	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
}

void ALabCharacter::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
	FParse::Value(FCommandLine::Get(), TEXT("LabFlashlight="), FlashlightIntensity); // calibración rápida
#endif
	Flashlight->SetIntensity(FlashlightIntensity);
	Flashlight->SetAttenuationRadius(FlashlightRange);
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

	// Teclas de depuración sin asset de input (temporal hasta tener IA_ propios).
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ALabCharacter::ToggleFlashlightInput);
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
	// Predicción local inmediata; el servidor confirma.
	bFlashlightOn = !bFlashlightOn;
	ApplyFlashlightState();
	ServerSetFlashlight(bFlashlightOn);
}

void ALabCharacter::ServerSetFlashlight_Implementation(bool bOn)
{
	bFlashlightOn = bOn;
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
		Flashlight->SetVisibility(bFlashlightOn);
	}
}
