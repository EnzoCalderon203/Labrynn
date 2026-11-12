#include "Core/LabPlayerController.h"

#include "Core/LabCameraManager.h"
#include "Online/LabSessionSubsystem.h"
#include "Player/LabCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

ALabPlayerController::ALabPlayerController()
{
	PlayerCameraManagerClass = ALabCameraManager::StaticClass();

	// Valores por defecto si se usa la clase C++ directa (sin Blueprint).
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCDefault(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCMouse(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	if (IMCDefault.Succeeded()) { DefaultMappingContexts.Add(IMCDefault.Object); }
	if (IMCMouse.Succeeded()) { MobileExcludedMappingContexts.Add(IMCMouse.Object); }
}

void ALabPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalPlayerController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
#if !UE_BUILD_SHIPPING
		SetupAutoShot();
#endif
	}
}

void ALabPlayerController::ReturnToMenu()
{
	if (ULabSessionSubsystem* S = GetGameInstance()->GetSubsystem<ULabSessionSubsystem>())
	{
		S->LeaveGame();
	}
}

#if !UE_BUILD_SHIPPING
void ALabPlayerController::SetupAutoShot()
{
	// Una sola vez por proceso: si el cliente se desconecta y recarga el mapa, no repetir.
	static bool bAutoShotScheduled = false;
	float Delay = 0.f;
	if (bAutoShotScheduled || !FParse::Value(FCommandLine::Get(), TEXT("LabAutoShot="), Delay) || Delay <= 0.f)
	{
		return;
	}
	bAutoShotScheduled = true;
	GetWorldTimerManager().SetTimer(AutoShotTimer, this, &ALabPlayerController::DoAutoShot, Delay, false);
	if (FParse::Param(FCommandLine::Get(), TEXT("LabAutoQuit")))
	{
		GetWorldTimerManager().SetTimer(AutoQuitTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			ConsoleCommand(TEXT("quit"));
		}), Delay + 4.f, false);
	}
}

void ALabPlayerController::DoAutoShot()
{
	// Mirar al compañero más cercano para verificar que se ve su cuerpo completo.
	APawn* Me = GetPawn();
	if (Me)
	{
		APawn* Closest = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (TActorIterator<APawn> It(GetWorld()); It; ++It)
		{
			if (*It != Me)
			{
				const float D = FVector::Dist(It->GetActorLocation(), Me->GetActorLocation());
				if (D < BestDist) { BestDist = D; Closest = *It; }
			}
		}
		if (Closest)
		{
			const FVector Dir = Closest->GetActorLocation() - Me->GetActorLocation();
			SetControlRotation(FRotator(-5.f, Dir.Rotation().Yaw, 0.f));
			UE_LOG(LogTemp, Log, TEXT("[AutoShot] %s mira a %s (dist %.0f, yaw %.0f)"),
				*GetNameSafe(Me), *GetNameSafe(Closest), BestDist, Dir.Rotation().Yaw);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AutoShot] %s no encontró a otro jugador"), *GetNameSafe(Me));
		}
		for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
		{
			UE_LOG(LogTemp, Log, TEXT("[AutoShot]   %s local=%d linterna=%d prendida=%d bateria=%.0f%%"),
				*It->GetName(), It->IsLocallyControlled() ? 1 : 0, It->HasFlashlight() ? 1 : 0,
				It->IsFlashlightOn() ? 1 : 0, It->GetBattery() * 100.f);
		}
	}
	// Esperar un par de frames a que la cámara rote.
	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		const FString Name = GetNetMode() == NM_Client ? TEXT("Client.png") : TEXT("Server.png");
		FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir() / Name, /*bShowUI*/ false, /*bAddUniqueSuffix*/ false);
	}), 0.5f, false);
}
#endif

void ALabPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Esc: salir de la partida y volver al menú. (En PIE, Esc lo captura el editor para cerrar PIE.)
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ALabPlayerController::ReturnToMenu);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Ctx : DefaultMappingContexts)
		{
			if (Ctx) { Subsystem->AddMappingContext(Ctx, 0); }
		}
		for (UInputMappingContext* Ctx : MobileExcludedMappingContexts)
		{
			if (Ctx) { Subsystem->AddMappingContext(Ctx, 0); }
		}
	}
}
