#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Online/LabSessionSubsystem.h"
#include "Audio/LabAudioSettings.h"

class SVerticalBox;
class SEditableTextBox;
class STextBlock;

/**
 * Menú principal mínimo en Slate (sin assets): Crear / Buscar / Unirse / IP / Salir.
 * Provisional: Luis lo puede reemplazar por un UMG con el mismo ULabSessionSubsystem.
 */
class SLabMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLabMainMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ULabSessionSubsystem>, Sessions)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SLabMainMenu() override;

private:
	FReply OnHost();
	FReply OnFind();
	FReply OnJoinIP();
	FReply OnQuit();
	FReply OnJoinResult(int32 Index);

	void HandleStatus(const FString& Msg);
	void HandleFindComplete(const TArray<FLabSessionInfo>& Results);

	TSharedRef<SWidget> MakeButton(const FText& Label, FOnClicked OnClicked);
	TSharedRef<SWidget> MakeVolumeRow(const FText& Label, ELabAudioCategory Cat);

	TWeakObjectPtr<ULabSessionSubsystem> Sessions;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SVerticalBox> ResultsBox;
	TSharedPtr<SEditableTextBox> IPBox;
	FDelegateHandle StatusHandle, FindHandle;
};
