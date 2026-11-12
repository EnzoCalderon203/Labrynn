#include "UI/LabHUD.h"

#include "Player/LabCharacter.h"
#include "Interact/LabLantern.h"
#include "Enemy/LabMinotaur.h"
#include "Core/LabGameState.h"
#include "Interact/LabInteractable.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "RenderUtils.h" // GWhiteTexture: FCanvasTriangleItem exige textura (con nullptr el engine hace assert)
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

	// Corriente de aire: velo frío y aviso.
	if (Ch->IsInGas())
	{
		DrawRect(FLinearColor(0.35f, 0.45f, 0.6f, 0.18f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		const FString G = TEXT("¡Una corriente de aire! La llama no prende acá");
		float GW, GH;
		GetTextSize(G, GW, GH, Font, 1.5f);
		DrawText(G, FLinearColor(0.75f, 0.85f, 1.f), (Canvas->ClipX - GW) * 0.5f, Canvas->ClipY * 0.3f, Font, 1.5f);
	}
	// ---- Esquina inferior izquierda: stamina (con heridas), linterna, reliquia ----
	const float X = 32.f;
	const float BarW = 280.f;

	// 1) Barra única de stamina. [stamina | vacío | heridas] — las heridas ocupan el final, en rojo oscuro.
	{
		const float Y = Canvas->ClipY - 112.f;
		const float H = 14.f;
		const float Stam = Ch->GetStamina01();
		const float Wound = Ch->GetWounds01();
		const bool bExh = Ch->IsExhausted();
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), X - 2.f, Y - 2.f, BarW + 4.f, H + 4.f);
		DrawRect(FLinearColor(0.12f, 0.12f, 0.12f, 0.9f), X, Y, BarW, H);                       // vacío recuperable
		const FLinearColor StamCol = bExh ? FLinearColor(1.f, 0.55f, 0.1f) : FLinearColor(0.95f, 0.92f, 0.8f);
		DrawRect(StamCol, X, Y, BarW * Stam, H);
		if (Wound > 0.f)
		{
			// Bloque de heridas con rayas para distinguirlo del vacío.
			const float WX = X + BarW * (1.f - Wound);
			DrawRect(FLinearColor(0.45f, 0.05f, 0.05f, 1.f), WX, Y, BarW * Wound, H);
			for (float SX = WX + 3.f; SX < X + BarW; SX += 8.f)
			{
				DrawRect(FLinearColor(0.7f, 0.1f, 0.1f, 1.f), SX, Y, 2.f, H);
			}
		}
		const FString Label = bExh ? TEXT("¡SIN AIRE! (jadeando)") : (Wound > 0.f ? TEXT("STAMINA   [Shift] correr   (rojo = heridas)") : TEXT("STAMINA   [Shift] correr"));
		DrawText(Label, bExh ? FLinearColor(1.f, 0.6f, 0.2f) : FLinearColor(0.75f, 0.75f, 0.75f), X, Y - 22.f, Font, 1.f);
	}

	// 2) Farol.
	{
		const float Y = Canvas->ClipY - 58.f;
		if (const ALabLantern* L = Ch->GetHeldLantern())
		{
			const float Oil = L->GetOil();
			const FLinearColor Col = Oil > 0.2f ? FLinearColor(1.f, 0.75f, 0.4f) : FLinearColor(1.f, 0.3f, 0.2f);
			FString State = L->IsLit() ? TEXT("encendido") : (L->IsRelighting() ? TEXT("encendiendo...") : TEXT("apagado [F] reencender"));
			if (Oil <= 0.f) { State = TEXT("sin aceite"); }
			DrawText(FString::Printf(TEXT("FAROL  aceite %3d%%  - %s"), FMath::RoundToInt(Oil * 100.f), *State), Col, X, Y, Font, 1.f);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X, Y + 22.f, 180.f, 7.f);
			DrawRect(Col, X, Y + 22.f, 180.f * Oil, 7.f);
			if (L->IsRepelling())
			{
				const FString R = TEXT("¡Lo estás conteniendo!  (el aceite se va rápido)");
				float RW, RH;
				GetTextSize(R, RW, RH, Font, 1.6f);
				DrawText(R, FLinearColor(1.f, 0.7f, 0.2f), (Canvas->ClipX - RW) * 0.5f, Canvas->ClipY * 0.45f, Font, 1.6f);
			}
			if (L->IsRelighting())
			{
				DrawRect(FLinearColor(1.f, 0.9f, 0.5f), X + 190.f, Y + 22.f, 80.f * L->GetRelightProgress(), 7.f);
			}
		}
		else
		{
			DrawText(TEXT("SIN FAROL - quedate cerca de la luz"), FLinearColor(0.6f, 0.6f, 0.65f), X, Y, Font, 1.f);
		}
	}

	// 3) Ícono de reliquia (rombo celeste) a la derecha de la barra.
	if (Ch->IsCarryingRelic())
	{
		const FVector2D C(X + BarW + 34.f, Canvas->ClipY - 100.f);
		const float R = 16.f;
		FCanvasTriangleItem Top(FVector2D(C.X, C.Y - R), FVector2D(C.X - R * 0.7f, C.Y), FVector2D(C.X + R * 0.7f, C.Y), GWhiteTexture);
		FCanvasTriangleItem Bot(FVector2D(C.X, C.Y + R), FVector2D(C.X - R * 0.7f, C.Y), FVector2D(C.X + R * 0.7f, C.Y), GWhiteTexture);
		Top.SetColor(FLinearColor(0.4f, 0.85f, 1.f));
		Bot.SetColor(FLinearColor(0.25f, 0.6f, 0.85f));
		Canvas->DrawItem(Top);
		Canvas->DrawItem(Bot);
		DrawText(TEXT("RELIQUIA"), FLinearColor(0.4f, 0.85f, 1.f), C.X + 22.f, C.Y - 9.f, Font, 1.f);
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

	// Minotauro cerca: viñeta roja que late (más fuerte cuanto más cerca) + aviso.
	for (TActorIterator<ALabMinotaur> It(GetWorld()); It; ++It)
	{
		const float D = FVector::Dist(It->GetActorLocation(), Ch->GetActorLocation());
		if (D < 1800.f)
		{
			const float Near = 1.f - D / 1800.f;
			const float Beat = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * (It->IsChasing() ? 9.f : 4.f));
			const float A = Near * (0.25f + 0.35f * Beat);
			const float Edge = 60.f + 120.f * Near;
			DrawRect(FLinearColor(0.6f, 0.f, 0.f, A), 0.f, 0.f, Canvas->ClipX, Edge);
			DrawRect(FLinearColor(0.6f, 0.f, 0.f, A), 0.f, Canvas->ClipY - Edge, Canvas->ClipX, Edge);
			DrawRect(FLinearColor(0.6f, 0.f, 0.f, A), 0.f, 0.f, Edge, Canvas->ClipY);
			DrawRect(FLinearColor(0.6f, 0.f, 0.f, A), Canvas->ClipX - Edge, 0.f, Edge, Canvas->ClipY);
			if (D < 1100.f)
			{
				const FString W = It->IsChasing() ? TEXT("¡TE OYÓ! ¡CORRÉ!") : TEXT("Algo enorme respira cerca... silencio");
				float WW, WH;
				GetTextSize(W, WW, WH, Font, 1.4f);
				DrawText(W, FLinearColor(1.f, 0.35f, 0.3f), (Canvas->ClipX - WW) * 0.5f, Canvas->ClipY * 0.36f, Font, 1.4f);
			}
		}
	}

	// Voz: indicador propio y sobre los compañeros que hablan (si están al alcance de la voz).
	if (Ch->IsTalking())
	{
		DrawText(TEXT("HABLANDO [V]"), FLinearColor(0.5f, 1.f, 0.6f), Canvas->ClipX - 170.f, 30.f, Font, 1.2f);
	}
	for (TActorIterator<ALabCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != Ch && It->IsTalking() && FVector::Dist(It->GetActorLocation(), Ch->GetActorLocation()) < 2000.f)
		{
			const FVector P = Project(It->GetActorLocation() + FVector(0.f, 0.f, 110.f));
			if (P.Z > 0.f)
			{
				DrawText(TEXT("( habla )"), FLinearColor(0.6f, 1.f, 0.7f), P.X - 30.f, P.Y, Font, 1.f);
			}
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
			else if (Since < 12.f) { Msg = TEXT("Algo despertó en el laberinto... no hagan ruido"); Col = FLinearColor(1.f, 0.2f, 0.15f); Scale = 1.7f; }
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
