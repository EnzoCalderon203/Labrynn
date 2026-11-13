#include "Core/LabPlayerController.h"

#include "Core/LabCameraManager.h"
#include "Online/LabSessionSubsystem.h"
#include "Player/LabCharacter.h"
#include "Player/LabCharacterMovement.h"
#include "Interact/LabLantern.h"
#include "Core/LabGameState.h"
#include "Maze/LabMaze.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Labrynn.h"
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
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCLab(TEXT("/Game/Labrynn/Core/Input/IMC_Labrynn.IMC_Labrynn"));
	LabMappingContext = IMCLab.Object;
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
void ALabPlayerController::InjectKey(const FKey& Key, EInputEvent Event)
{
	UGameViewportClient* VC = GetWorld()->GetGameViewport();
	if (!VC)
	{
		return;
	}
	const FInputDeviceId Device = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	FInputKeyEventArgs Args(VC->Viewport, Device, Key, Event, FPlatformTime::Cycles64());
	const bool bHandled = VC->InputKey(Args);
	UE_LOG(LogLabrynn, Log, TEXT("[Keys] %s %s (manejada=%d)"), *Key.ToString(),
		Event == IE_Pressed ? TEXT("DOWN") : TEXT("UP"), bHandled ? 1 : 0);
}

void ALabPlayerController::TickKeyScript()
{
	if (!bKeyScriptParsed)
	{
		bKeyScriptParsed = true;
		KeyScriptStart = GetWorld()->GetTimeSeconds();
		// -LabKeysHost= / -LabKeysClient= permiten guiones distintos por rol; -LabKeys= vale para ambos.
		FString Script;
		const TCHAR* RoleKey = GetNetMode() == NM_Client ? TEXT("LabKeysClient=") : TEXT("LabKeysHost=");
		if (FParse::Value(FCommandLine::Get(), RoleKey, Script) || FParse::Value(FCommandLine::Get(), TEXT("LabKeys="), Script))
		{
			TArray<FString> Parts;
			Script.ParseIntoArray(Parts, TEXT("|"));
			for (const FString& P : Parts)
			{
				TArray<FString> F;
				P.ParseIntoArray(F, TEXT(":"));
				if (F.Num() == 3)
				{
					KeyScript.Add({ FCString::Atof(*F[0]), FKey(*F[1]), F[2][0] });
				}
			}
			UE_LOG(LogLabrynn, Log, TEXT("[Keys] guion con %d eventos"), KeyScript.Num());
		}
	}
	if (KeyScript.Num() == 0 || !IsLocalController())
	{
		return;
	}
	const float T = GetWorld()->GetTimeSeconds() - KeyScriptStart;
	for (FLabKeyEvent& E : KeyScript)
	{
		if (!E.bDone && T >= E.Time)
		{
			E.bDone = true;
			if (E.Kind == 'D' || E.Kind == 'P') { InjectKey(E.Key, IE_Pressed); }
			if (E.Kind == 'U' || E.Kind == 'P') { InjectKey(E.Key, IE_Released); }
		}
	}
	// Estado observable del personaje (resultado del input, no flags).
	if (T >= NextKeyLog)
	{
		NextKeyLog = T + 1.f;
		if (const ALabCharacter* Ch = Cast<ALabCharacter>(GetPawn()))
		{
			const ULabCharacterMovement* CMC = Cast<ULabCharacterMovement>(Ch->GetCharacterMovement());
			const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>();
			const FIntPoint Cell = GS && GS->GetMaze() ? GS->GetMaze()->WorldToCell(Ch->GetActorLocation()) : FIntPoint(-1, -1);
			UE_LOG(LogLabrynn, Log, TEXT("[Keys]   celda=%s z=%.0f agachado=%d"), *Cell.ToString(), Ch->GetActorLocation().Z, Ch->bIsCrouched ? 1 : 0);
			UE_LOG(LogLabrynn, Log, TEXT("[Keys] t=%.0f vel=%.0f max=%.0f sprintPedido=%d stamina=%.0f%% farolEncendido=%d reliquia=%d farol=%d reencendiendo=%d"),
				T, Ch->GetVelocity().Size2D(), CMC ? CMC->GetMaxSpeed() : 0.f, CMC && CMC->bWantsToSprint ? 1 : 0,
				Ch->GetStamina01() * 100.f, Ch->HasLitLantern() ? 1 : 0, Ch->IsCarryingRelic() ? 1 : 0, Ch->GetHeldLantern() ? 1 : 0, Ch->GetHeldLantern() && Ch->GetHeldLantern()->IsRelighting() ? 1 : 0);
		}
	}
}

void ALabPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	TickKeyScript();
	if (AutoRunFrom < 0.f)
	{
		// Dos parámetros separados: FParse::Value corta los valores en las comas.
		if (!FParse::Value(FCommandLine::Get(), TEXT("LabAutoRunFrom="), AutoRunFrom) ||
			!FParse::Value(FCommandLine::Get(), TEXT("LabAutoRunTo="), AutoRunTo))
		{
			AutoRunFrom = 1e9f;
		}
	}
	const float T = GetWorld()->GetTimeSeconds();
	ALabCharacter* Ch = Cast<ALabCharacter>(GetPawn());
	if (!Ch || !IsLocalController())
	{
		return;
	}
	// BUG REAL (encontrado por Enzo en PIE): antes esto escribía bWantsToSprint EN CADA FRAME, aunque no hubiera
	// -LabAutoRun, y pisaba el Shift del jugador. Ahora solo toca el sprint dentro de la ventana de prueba.
	if (AutoRunFrom > 1e8f)
	{
		return;
	}
	ULabCharacterMovement* CMC = Cast<ULabCharacterMovement>(Ch->GetCharacterMovement());
	const bool bRun = T >= AutoRunFrom && T <= AutoRunTo;
	if (CMC && (bRun || T <= AutoRunTo + 0.1f))
	{
		CMC->bWantsToSprint = bRun;
	}
	if (bRun)
	{
		// Ida y vuelta en el hall de entrada (eje Y), cambiando cada 0,5 s.
		Ch->AddMovementInput(FVector(0.f, FMath::Fmod(T, 1.f) < 0.5f ? 1.f : -1.f, 0.f), 1.f);
	}
}

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
	if (Me && !FParse::Param(FCommandLine::Get(), TEXT("LabShotNoAim")))
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
			UE_LOG(LogTemp, Log, TEXT("[AutoShot]   %s local=%d farol=%d encendido=%d aceite=%.0f%%"),
				*It->GetName(), It->IsLocallyControlled() ? 1 : 0, It->GetHeldLantern() ? 1 : 0,
				It->HasLitLantern() ? 1 : 0, It->GetHeldLantern() ? It->GetHeldLantern()->GetOil() * 100.f : 0.f);
		}
	}
	// Esperar un par de frames a que la cámara rote.
	FTimerHandle H;
	GetWorldTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		FString Name = GetNetMode() == NM_Client ? TEXT("Client.png") : TEXT("Server.png");
		FString Custom;
		if (FParse::Value(FCommandLine::Get(), TEXT("LabShotName="), Custom)) { Name = Custom + TEXT(".png"); }
		if (PlayerCameraManager)
		{
			UE_LOG(LogTemp, Log, TEXT("[AutoShot] cam=%s rot=%s fov=%.1f"), *PlayerCameraManager->GetCameraLocation().ToCompactString(),
				*PlayerCameraManager->GetCameraRotation().ToCompactString(), PlayerCameraManager->GetFOVAngle());
		}
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
		if (LabMappingContext)
		{
			Subsystem->AddMappingContext(LabMappingContext, 1);
		}
		else
		{
			UE_LOG(LogLabrynn, Error, TEXT("Falta IMC_Labrynn (correr Scripts/create_input_assets.py)"));
		}
	}
}
