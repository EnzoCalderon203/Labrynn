#include "Core/LabPlayerController.h"

#include "Core/LabCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

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
	}
}

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
