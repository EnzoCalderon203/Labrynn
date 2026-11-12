#include "UI/LabHUD.h"

#include "Player/LabCharacter.h"
#include "Core/LabGameState.h"
#include "Interact/LabInteractable.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ALabHUD::DrawHUD()
{
	Super::DrawHUD();

	const ALabCharacter* Ch = Cast<ALabCharacter>(GetOwningPawn());
	if (!Ch || !Canvas)
	{
		return;
	}

	UFont* Font = GEngine->GetMediumFont();
	const float X = 32.f;
	const float Y = Canvas->ClipY - 70.f;

	if (Ch->HasFlashlight())
	{
		const float B = Ch->GetBattery();
		const FLinearColor Col = B > 0.25f ? FLinearColor(0.95f, 0.85f, 0.6f) : FLinearColor(1.f, 0.3f, 0.2f);
		DrawText(FString::Printf(TEXT("LINTERNA [F]  %3d%%"), FMath::RoundToInt(B * 100.f)), Col, X, Y, Font, 1.2f);

		// Barra
		const float W = 180.f, H = 8.f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y + 26.f, W, H);
		DrawRect(Col, X, Y + 26.f, W * B, H);
	}
	else
	{
		DrawText(TEXT("SIN LINTERNA - quedate cerca del grupo"), FLinearColor(0.6f, 0.6f, 0.65f), X, Y, Font, 1.2f);
	}

	if (Ch->IsCarryingRelic())
	{
		DrawText(TEXT("LLEVÁS LA RELIQUIA (lento, sin linterna)"), FLinearColor(0.4f, 0.85f, 1.f), X, Y - 34.f, Font, 1.1f);
	}

	// Texto de interacción en el centro.
	if (const AActor* Target = Ch->FindBestInteractable())
	{
		if (const ILabInteractable* I = Cast<ILabInteractable>(Target))
		{
			const FString Prompt = FString::Printf(TEXT("[E] %s"), *I->GetInteractPrompt(Ch).ToString());
			float W, H;
			GetTextSize(Prompt, W, H, Font, 1.3f);
			DrawText(Prompt, FLinearColor::White, (Canvas->ClipX - W) * 0.5f, Canvas->ClipY * 0.62f, Font, 1.3f);
		}
	}

	// Mensajes de fase.
	if (const ALabGameState* GS = GetWorld()->GetGameState<ALabGameState>())
	{
		const float Since = GS->GetServerWorldTimeSeconds() - GS->GetPhaseStartTime();
		FString Msg;
		FLinearColor Col = FLinearColor::White;
		float Scale = 2.f;
		switch (GS->GetPhase())
		{
		case ELabPhase::Escape:
			if (Since < 6.f) { Msg = TEXT("¡LA SALIDA SE ABRIÓ!  Llevá la reliquia a la luz roja"); Col = FLinearColor(1.f, 0.35f, 0.25f); }
			break;
		case ELabPhase::Won:
			Msg = TEXT("¡ESCAPARON CON LA RELIQUIA!"); Col = FLinearColor(0.5f, 1.f, 0.6f); Scale = 3.f;
			break;
		case ELabPhase::Lost:
			Msg = TEXT("EL LABERINTO LOS DEVORÓ"); Col = FLinearColor(1.f, 0.2f, 0.2f); Scale = 3.f;
			break;
		default:
			if (Since < 6.f) { Msg = TEXT("Encuentren la reliquia en el centro del laberinto"); Col = FLinearColor(0.9f, 0.85f, 0.7f); Scale = 1.6f; }
			break;
		}
		if (!Msg.IsEmpty())
		{
			float W, H;
			GetTextSize(Msg, W, H, Font, Scale);
			DrawText(Msg, Col, (Canvas->ClipX - W) * 0.5f, Canvas->ClipY * 0.2f, Font, Scale);
		}
	}
}
