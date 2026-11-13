#include "UI/SLabMainMenu.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Input/SSlider.h"
#include "Audio/LabAudio.h"

#define LOCTEXT_NAMESPACE "LabMainMenu"

namespace
{
	FSlateFontInfo Font(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}
}

void SLabMainMenu::Construct(const FArguments& InArgs)
{
	Sessions = InArgs._Sessions;

	if (ULabSessionSubsystem* S = Sessions.Get())
	{
		StatusHandle = S->OnStatus.AddSP(this, &SLabMainMenu::HandleStatus);
		FindHandle = S->OnFindComplete.AddSP(this, &SLabMainMenu::HandleFindComplete);
	}

	const FString Service = Sessions.IsValid() ? Sessions->GetOnlineServiceName() : TEXT("?");
	const FString Player = Sessions.IsValid() ? Sessions->GetLocalPlayerName() : TEXT("?");

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.01f, 0.01f, 0.012f, 1.f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(520.f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 4)
				[
					SNew(STextBlock).Text(LOCTEXT("Title", "LABRYNN")).Font(Font(56))
					.ColorAndOpacity(FLinearColor(0.95f, 0.8f, 0.55f))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%s  ·  servicio: %s"), *Player, *Service)))
					.Font(Font(12)).ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[ MakeButton(LOCTEXT("Host", "Crear partida"), FOnClicked::CreateSP(this, &SLabMainMenu::OnHost)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[ MakeButton(LOCTEXT("Find", "Buscar partidas"), FOnClicked::CreateSP(this, &SLabMainMenu::OnFind)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
				[
					SNew(SBox).MaxDesiredHeight(180.f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[ SAssignNew(ResultsBox, SVerticalBox) ]
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 8, 0)
					[
						SAssignNew(IPBox, SEditableTextBox)
						.HintText(LOCTEXT("IPHint", "IP del host (ej. 100.64.0.2)"))
						.Font(Font(14))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[ MakeButton(LOCTEXT("JoinIP", "Unirse por IP"), FOnClicked::CreateSP(this, &SLabMainMenu::OnJoinIP)) ]
				]

				// Volumen por categoría (se guarda en GameUserSettings.ini).
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 2)
				[ SNew(STextBlock).Text(LOCTEXT("Vol", "Volumen")).Font(Font(14)).ColorAndOpacity(FLinearColor(0.7f, 0.65f, 0.55f)) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeVolumeRow(LOCTEXT("VMaster", "General"), ELabAudioCategory::Master) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeVolumeRow(LOCTEXT("VSfx", "Efectos"), ELabAudioCategory::SFX) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeVolumeRow(LOCTEXT("VVoice", "Voz"), ELabAudioCategory::Voice) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeVolumeRow(LOCTEXT("VMusic", "Música"), ELabAudioCategory::Music) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeVolumeRow(LOCTEXT("VAmb", "Ambiente"), ELabAudioCategory::Ambience) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 4)
				[ MakeButton(LOCTEXT("Quit", "Salir"), FOnClicked::CreateSP(this, &SLabMainMenu::OnQuit)) ]

				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 16, 0, 0)
				[
					SAssignNew(StatusText, STextBlock)
					.Text(FText::GetEmpty()).Font(Font(13))
					.ColorAndOpacity(FLinearColor(0.7f, 0.85f, 1.f))
				]
			]
		]
	];
}

SLabMainMenu::~SLabMainMenu()
{
	if (ULabSessionSubsystem* S = Sessions.Get())
	{
		S->OnStatus.Remove(StatusHandle);
		S->OnFindComplete.Remove(FindHandle);
	}
}

TSharedRef<SWidget> SLabMainMenu::MakeButton(const FText& Label, FOnClicked OnClicked)
{
	TWeakObjectPtr<ULabSessionSubsystem> Ctx = Sessions;
	return SNew(SButton)
		.HAlign(HAlign_Center)
		.ContentPadding(FMargin(16, 10))
		.OnClicked_Lambda([Ctx, OnClicked]()
		{
			ULabAudio::Play2D(Ctx.Get(), ELabSfx::UIClick);
			return OnClicked.Execute();
		})
		[
			SNew(STextBlock).Text(Label).Font(Font(18))
		];
}

TSharedRef<SWidget> SLabMainMenu::MakeVolumeRow(const FText& Label, ELabAudioCategory Cat)
{
	TWeakObjectPtr<ULabSessionSubsystem> Ctx = Sessions;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 2, 10, 2)
		[ SNew(SBox).WidthOverride(90.f) [ SNew(STextBlock).Text(Label).Font(Font(12)).ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f)) ] ]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SSlider)
			.Value(ULabAudio::GetCategoryVolume(Cat))
			.OnValueChanged_Lambda([Ctx, Cat](float V) { ULabAudio::SetCategoryVolume(Ctx.Get(), Cat, V); })
		];
}

FReply SLabMainMenu::OnHost()
{
	if (ULabSessionSubsystem* S = Sessions.Get()) { S->HostGame(4); }
	return FReply::Handled();
}

FReply SLabMainMenu::OnFind()
{
	if (ResultsBox.IsValid()) { ResultsBox->ClearChildren(); }
	if (ULabSessionSubsystem* S = Sessions.Get()) { S->FindGames(); }
	return FReply::Handled();
}

FReply SLabMainMenu::OnJoinIP()
{
	if (ULabSessionSubsystem* S = Sessions.Get()) { S->JoinByAddress(IPBox->GetText().ToString()); }
	return FReply::Handled();
}

FReply SLabMainMenu::OnQuit()
{
	if (ULabSessionSubsystem* S = Sessions.Get())
	{
		UKismetSystemLibrary::QuitGame(S, nullptr, EQuitPreference::Quit, false);
	}
	return FReply::Handled();
}

FReply SLabMainMenu::OnJoinResult(int32 Index)
{
	if (ULabSessionSubsystem* S = Sessions.Get()) { S->JoinGame(Index); }
	return FReply::Handled();
}

void SLabMainMenu::HandleStatus(const FString& Msg)
{
	if (StatusText.IsValid()) { StatusText->SetText(FText::FromString(Msg)); }
}

void SLabMainMenu::HandleFindComplete(const TArray<FLabSessionInfo>& Results)
{
	if (!ResultsBox.IsValid())
	{
		return;
	}
	ResultsBox->ClearChildren();
	for (int32 i = 0; i < Results.Num(); ++i)
	{
		const FLabSessionInfo& R = Results[i];
		const int32 Players = R.MaxPlayers - R.OpenSlots;
		const FText Label = FText::FromString(FString::Printf(TEXT("Unirse a %s  (%d/%d)  %d ms"), *R.HostName, Players, R.MaxPlayers, R.PingMs));
		ResultsBox->AddSlot().AutoHeight().Padding(0, 2)
		[
			MakeButton(Label, FOnClicked::CreateSP(this, &SLabMainMenu::OnJoinResult, i))
		];
	}
}

#undef LOCTEXT_NAMESPACE
