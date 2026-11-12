#include "Core/LabPlayerController.h"

#include "Core/LabCameraManager.h"
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

#if !UE_BUILD_SHIPPING
void ALabPlayerController::SetupAutoShot()
{
	float Delay = 0.f;
	if (!FParse::Value(FCommandLine::Get(), TEXT("LabAutoShot="), Delay) || Delay <= 0.f)
	{
		return;
	}
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
