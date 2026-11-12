#include "UI/LabHUD.h"

#include "Player/LabCharacter.h"
#include "Core/LabGameState.h"
#include "Interact/LabInteractable.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
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

	// Gas: pantalla verde y aviso.
	if (Ch->IsInGas())
	{
		DrawRect(FLinearColor(0.15f, 0.5f, 0.05f, 0.35f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		const FString G = TEXT("¡GAS! La linterna no prende - salí de la nube");
		float GW, GH;
		GetTextSize(G, GW, GH, Font, 1.5f);
		DrawText(G, FLinearColor(0.7f, 1.f, 0.4f), (Canvas->ClipX - GW) * 0.5f, Canvas->ClipY * 0.3f, Font, 1.5f);
	}
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

	// Estado propio: caído o muerto.
	if (Ch->GetLifeState() != ELabLifeState::Alive)
	{
		FString Msg;
		if (Ch->GetLifeState() == ELabLifeState::Downed)
		{
			Msg = FString::Printf(TEXT("ESTÁS EN EL PISO - ¡gritale a alguien que te levante!  (%ds)"), FMath::CeilToInt(Ch->GetBleedOutRemaining()));
		}
		else
		{
			Msg = TEXT("MORISTE - el Corazón del Laberinto puede revivirte");
		}
		float W, H;
		GetTextSize(Msg, W, H, Font, 1.6f);
		DrawText(Msg, FLinearColor(1.f, 0.3f, 0.25f), (Canvas->ClipX - W) * 0.5f, Canvas->ClipY * 0.75f, Font, 1.6f);
		if (Ch->GetReviveProgress() > 0.f)
		{
			const float BW = 300.f;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), (Canvas->ClipX - BW) * 0.5f, Canvas->ClipY * 0.75f + 36.f, BW, 10.f);
			DrawRect(FLinearColor(0.4f, 1.f, 0.5f), (Canvas->ClipX - BW) * 0.5f, Canvas->ClipY * 0.75f + 36.f, BW * Ch->GetReviveProgress(), 10.f);
		}
	}

	// Marcadores de compañeros caídos (se ven a través de las paredes: así se coordina el rescate).
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == Ch || It->GetLifeState() != ELabLifeState::Downed)
		{
			continue;
		}
		const FVector P = Project(It->GetActorLocation() + FVector(0.f, 0.f, 60.f));
		if (P.Z > 0.f) // delante de la cámara
		{
			const FString Mark = FString::Printf(TEXT("! AYUDA %ds"), FMath::CeilToInt(It->GetBleedOutRemaining()));
			DrawText(Mark, FLinearColor(1.f, 0.4f, 0.2f), P.X - 40.f, P.Y, Font, 1.2f);
		}
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
