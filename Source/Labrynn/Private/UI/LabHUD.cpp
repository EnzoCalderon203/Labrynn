#include "UI/LabHUD.h"

#include "Player/LabCharacter.h"
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
}
