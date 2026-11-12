#include "UI/LabMenuPlayerController.h"

#include "UI/SLabMainMenu.h"
#include "Online/LabSessionSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "Widgets/SWeakWidget.h"

void ALabMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalPlayerController() || !GetWorld()->GetGameViewport())
	{
		return;
	}

	ULabSessionSubsystem* Sessions = GetGameInstance()->GetSubsystem<ULabSessionSubsystem>();
	Menu = SNew(SLabMainMenu).Sessions(Sessions);
	GetWorld()->GetGameViewport()->AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(Menu.ToSharedRef()));

	FInputModeUIOnly Mode; // sin SetWidgetToFocus: el SCompoundWidget no es enfocable (los botones sí)
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;

#if !UE_BUILD_SHIPPING
	// Pruebas automáticas: -LabMenuHost crea partida; -LabMenuJoin busca (reintenta) y se une a la primera.
	// Solo la primera vez por proceso (al volver al menú tras una desconexión, no repetir).
	static bool bAutoMenuDone = false;
	if (bAutoMenuDone)
	{
		return;
	}
	bAutoMenuDone = true;
	if (FParse::Param(FCommandLine::Get(), TEXT("LabMenuHost")))
	{
		Sessions->HostGame(4);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("LabMenuJoin")))
	{
		Sessions->OnFindComplete.AddWeakLambda(this, [this, Sessions](const TArray<FLabSessionInfo>& Results)
		{
			if (Results.Num() > 0)
			{
				Sessions->JoinGame(0);
				return;
			}
			FTimerHandle Retry;
			GetWorldTimerManager().SetTimer(Retry, FTimerDelegate::CreateWeakLambda(this, [Sessions]() { Sessions->FindGames(); }), 3.f, false);
		});
		Sessions->FindGames();
	}
#endif
}

void ALabMenuPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Menu.IsValid() && GetWorld() && GetWorld()->GetGameViewport())
	{
		GetWorld()->GetGameViewport()->RemoveAllViewportWidgets();
	}
	Menu.Reset();
	Super::EndPlay(Reason);
}
