// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultiShooterGameHUD.h"
#include "MultiShooterGameState.h"
#include "MultiShooterPlayerState.h"
#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// ─── Init ──────────────────────────────────────────────────────────────────
void AMultiShooterGameHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateScopeTexture();
}

void AMultiShooterGameHUD::DrawHUD()
{
	Super::DrawHUD();

	if (bShowMenu)
	{
		DrawMainMenu();
		return;
	}

	if (IsAiming())
	{
		DrawScopeOverlay();
		DrawAmmoCounter();
		DrawInfoPanel();
	}
	else
	{
		DrawInfoPanel();
		DrawAmmoCounter();
		DrawHealthBar();
		DrawCrosshair();
	}

	if (AMultiShooterGameState* GS = GetGS())
		if (GS->bGameOver) DrawGameOver();
}

// ─── Menu toggle ────────────────────────────────────────────────────────────
void AMultiShooterGameHUD::ToggleMenu()
{
	bShowMenu = !bShowMenu;
	bShowJoinInput = false;
	HoveredButtonIndex = -1;

	if (!PlayerOwner) return;

	if (bShowMenu)
	{
		PlayerOwner->bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerOwner->SetInputMode(Mode);
		// Disable input on pawn without pausing the world
		if (PlayerOwner->GetPawn())
		{
			PlayerOwner->GetPawn()->DisableInput(PlayerOwner);
		}
	}
	else
	{
		PlayerOwner->bShowMouseCursor = false;
		FInputModeGameOnly Mode;
		PlayerOwner->SetInputMode(Mode);
		// Re-enable input on pawn
		if (PlayerOwner->GetPawn())
		{
			PlayerOwner->GetPawn()->EnableInput(PlayerOwner);
		}
	}
}

void AMultiShooterGameHUD::HandleClick(float ScreenX, float ScreenY)
{
	if (!bShowMenu) return;

	auto InRect = [&](float X, float Y, float W, float H) {
		return ScreenX >= X && ScreenX <= X + W && ScreenY >= Y && ScreenY <= Y + H;
	};

	if (bShowJoinInput)
	{
		if (HoveredButtonIndex == 0) // Localhost
		{
			ToggleMenu();
			if (PlayerOwner) PlayerOwner->ConsoleCommand(TEXT("open 127.0.0.1:7777"));
		}
		else if (HoveredButtonIndex == 1) // LAN
		{
			ToggleMenu();
			if (PlayerOwner) PlayerOwner->ConsoleCommand(TEXT("open 192.168.1.100:7777"));
		}
		else if (HoveredButtonIndex == 2) // Back
			bShowJoinInput = false;
		return;
	}

	for (int32 i = 0; i < MenuButtons.Num(); ++i)
	{
		auto& B = MenuButtons[i];
		if (InRect(B.X, B.Y, B.W, B.H))
		{
			if (B.Text == TEXT("HOST GAME"))
			{
				ToggleMenu();
				if (PlayerOwner) PlayerOwner->ConsoleCommand(TEXT("open Lvl_Shooter?listen"));
			}
			else if (B.Text == TEXT("JOIN GAME"))
				bShowJoinInput = true;
			else if (B.Text == TEXT("QUIT"))
			{
				if (PlayerOwner) PlayerOwner->ConsoleCommand(TEXT("quit"));
			}
			return;
		}
	}
}

void AMultiShooterGameHUD::HandleBackspace()
{
}

// ─── Draw Main Menu ─────────────────────────────────────────────────────────
void AMultiShooterGameHUD::DrawMainMenu()
{
	if (!Canvas || !PlayerOwner) return;

	float SX = Canvas->SizeX, SY = Canvas->SizeY, CX = SX * 0.5f;
	float MX, MY;
	PlayerOwner->GetMousePosition(MX, MY);

	auto Hov = [&](float x, float y, float w, float h) {
		return MX >= x && MX <= x + w && MY >= y && MY <= y + h;
	};

	// Dark overlay
	FCanvasTileItem O(FVector2D(0, 0), FVector2D(SX, SY), FLinearColor(0, 0, 0, 0.7f));
	O.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(O);

	// Title
	UFont* TF = GEngine->GetLargeFont();
	float TitleY = SY * 0.2f;
	FCanvasTextItem TitleItem(FVector2D(CX - 120, TitleY), FText::FromString(TEXT("MYCS")), TF, FColor::Yellow);
	TitleItem.Scale = FVector2D(2.5f, 2.5f); TitleItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TitleItem);

	FCanvasTextItem SubTitle(FVector2D(CX - 100, TitleY + 50), FText::FromString(TEXT("Wave Survival Co-op")), GEngine->GetSmallFont(), FColor::White);
	SubTitle.EnableShadow(FLinearColor::Black); Canvas->DrawItem(SubTitle);

	float BW = 300, BH = 50, BY = SY * 0.4f, BX = CX - BW * 0.5f;
	MenuButtons.Reset();

	if (bShowJoinInput)
	{
		FCanvasTextItem Lbl(FVector2D(CX - 60, BY), FText::FromString(TEXT("Join:")), GEngine->GetSmallFont(), FColor::White);
		Lbl.EnableShadow(FLinearColor::Black); Canvas->DrawItem(Lbl);

		FMenuButton LB = {BX, BY + 40, BW, BH, TEXT("LOCALHOST (127.0.0.1)")};
		FMenuButton LB2 = {BX, BY + 105, BW, BH, TEXT("LAN GAME (192.168.1.100)")};
		FMenuButton BB = {BX, BY + 170, BW, BH, TEXT("BACK")};

		DrawButton(LB.X, LB.Y, LB.W, LB.H, LB.Text, Hov(LB.X, LB.Y, LB.W, LB.H));
		DrawButton(LB2.X, LB2.Y, LB2.W, LB2.H, LB2.Text, Hov(LB2.X, LB2.Y, LB2.W, LB2.H));
		DrawButton(BB.X, BB.Y, BB.W, BB.H, BB.Text, Hov(BB.X, BB.Y, BB.W, BB.H));

		if (Hov(LB.X, LB.Y, LB.W, LB.H)) HoveredButtonIndex = 0;
		else if (Hov(LB2.X, LB2.Y, LB2.W, LB2.H)) HoveredButtonIndex = 1;
		else if (Hov(BB.X, BB.Y, BB.W, BB.H)) HoveredButtonIndex = 2;
		else HoveredButtonIndex = -1;

		MenuButtons.Add(LB); MenuButtons.Add(LB2); MenuButtons.Add(BB);
	}
	else
	{
		FMenuButton HB = {BX, BY, BW, BH, TEXT("HOST GAME")};
		FMenuButton JB = {BX, BY + BH + 15, BW, BH, TEXT("JOIN GAME")};
		FMenuButton QB = {BX, BY + (BH + 15) * 2, BW, BH, TEXT("QUIT")};

		DrawButton(HB.X, HB.Y, HB.W, HB.H, HB.Text, Hov(HB.X, HB.Y, HB.W, HB.H));
		DrawButton(JB.X, JB.Y, JB.W, JB.H, JB.Text, Hov(JB.X, JB.Y, JB.W, JB.H));
		DrawButton(QB.X, QB.Y, QB.W, QB.H, QB.Text, Hov(QB.X, QB.Y, QB.W, QB.H));

		if (Hov(HB.X, HB.Y, HB.W, HB.H)) HoveredButtonIndex = 0;
		else if (Hov(JB.X, JB.Y, JB.W, JB.H)) HoveredButtonIndex = 1;
		else if (Hov(QB.X, QB.Y, QB.W, QB.H)) HoveredButtonIndex = 2;
		else HoveredButtonIndex = -1;

		MenuButtons.Add(HB); MenuButtons.Add(JB); MenuButtons.Add(QB);

		FCanvasTextItem Hint(FVector2D(CX - 140, BY + (BH + 15) * 3 + 40),
			FText::FromString(TEXT("Press F10 to open/close menu")), GEngine->GetSmallFont(), FColor(100, 100, 100));
		Hint.EnableShadow(FLinearColor::Black); Canvas->DrawItem(Hint);
	}
}

void AMultiShooterGameHUD::DrawButton(float X, float Y, float W, float H, const FString& Text, bool bHovered)
{
	if (!Canvas) return;

	FColor BtnColor = bHovered ? FColor(50, 70, 150, 230) : FColor(30, 30, 40, 230);
	FColor BorderColor = bHovered ? FColor::Blue : FColor(40, 40, 40);

	FCanvasTileItem Border(FVector2D(X - 1, Y - 1), FVector2D(W + 2, H + 2), BorderColor);
	Border.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Border);

	FCanvasTileItem Fill(FVector2D(X, Y), FVector2D(W, H), BtnColor);
	Fill.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Fill);

	FCanvasTextItem Label(FVector2D(X + W * 0.5f - Text.Len() * 7, Y + H * 0.5f - 10),
		FText::FromString(Text), GEngine->GetMediumFont(), FColor::White);
	Label.EnableShadow(FLinearColor::Black); Canvas->DrawItem(Label);
}

// ─── Scope texture ──────────────────────────────────────────────────────────
void AMultiShooterGameHUD::CreateScopeTexture()
{
	const int32 S = 1024, C = S / 2, LR = S / 5;
	ScopeTexture = UTexture2D::CreateTransient(S, S, PF_B8G8R8A8);
	if (!ScopeTexture) return;
	ScopeTexture->CompressionSettings = TC_VectorDisplacementmap;
	ScopeTexture->SRGB = false; ScopeTexture->AddToRoot();

	FTexture2DMipMap& Mip = ScopeTexture->GetPlatformData()->Mips[0];
	FColor* D = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));

	for (int32 Y = 0; Y < S; ++Y)
		for (int32 X = 0; X < S; ++X)
		{
			int32 DX = X - C, DY = Y - C;
			float Dist = FMath::Sqrt((float)(DX * DX + DY * DY));
			if (Dist <= LR)
			{
				bool bR = (FMath::Abs(DX) <= 2.5f && FMath::Abs(DY) >= 6 && FMath::Abs(DY) <= LR*3/5) ||
				          (FMath::Abs(DY) <= 2.5f && FMath::Abs(DX) >= 6 && FMath::Abs(DX) <= LR*3/5);
				D[Y * S + X] = bR ? FColor(0, 255, 0, 200) : FColor(0, 0, 0, 0);
			}
			else
				D[Y * S + X] = FColor(0, 0, 0, 255);
		}

	for (int32 Y = 0; Y < S; ++Y)
		for (int32 X = 0; X < S; ++X)
			if (FMath::Abs(FMath::Sqrt((float)((X-C)*(X-C) + (Y-C)*(Y-C))) - LR) <= 1.0f)
				D[Y * S + X] = FColor(0, 30, 0, 180);

	Mip.BulkData.Unlock();
	ScopeTexture->UpdateResource();
}

void AMultiShooterGameHUD::DrawScopeOverlay()
{
	if (!Canvas || !ScopeTexture) return;
	DrawTexture(ScopeTexture, 0, 0, Canvas->SizeX, Canvas->SizeY, 0, 0, 1, 1,
		FLinearColor::White, BLEND_Translucent, 1.0f, false, 0, FVector2D(0, 0));
}

// ─── Ammo counter ──────────────────────────────────────────────────────────
void AMultiShooterGameHUD::DrawAmmoCounter()
{
	if (!Canvas || !PlayerOwner) return;
	AShooterCharacter* C = Cast<AShooterCharacter>(PlayerOwner->GetPawn());
	if (!C) return;
	AShooterWeapon* W = C->GetCurrentWeapon();
	if (!W) return;

	int32 B = W->GetBulletCount(), M = W->GetMagazineSize();
	bool bR = W->IsReloading();
	FString T = bR ? TEXT("RELOADING...") : FString::Printf(TEXT("%d / %d"), B, M);
	FColor Col = (!B && !bR) ? FColor::Red : FColor::White;

	float TW = T.Len() * 14.0f, TH = 24.0f;
	float X = Canvas->SizeX - TW - 30.0f, Y = Canvas->SizeY - 130.0f;

	FCanvasTileItem BG(FVector2D(X - 4, Y - 2), FVector2D(TW + 8, TH + 4), FLinearColor(0, 0, 0, 0.5f));
	BG.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(BG);

	FCanvasTextItem L(FVector2D(X, Y), FText::FromString(T), GEngine->GetMediumFont(), Col);
	L.EnableShadow(FLinearColor::Black); Canvas->DrawItem(L);

	if (B == 0 && !bR)
	{
		FString RT = TEXT("[ R ] RELOAD");
		float RW = RT.Len() * 11.0f;
		FCanvasTileItem RBG(FVector2D(X + (TW - RW) * 0.5f - 2, Y + TH + 2), FVector2D(RW + 4, 18), FLinearColor(0.8f, 0.1f, 0.1f, 0.6f));
		RBG.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(RBG);
		FCanvasTextItem RL(FVector2D(X + (TW - RW) * 0.5f, Y + TH + 3), FText::FromString(RT), GEngine->GetSmallFont(), FColor::White);
		RL.EnableShadow(FLinearColor::Black); Canvas->DrawItem(RL);
	}
}

// ─── Health bar ────────────────────────────────────────────────────────────
void AMultiShooterGameHUD::DrawHealthBar()
{
	if (!Canvas || !PlayerOwner) return;
	AShooterCharacter* C = Cast<AShooterCharacter>(PlayerOwner->GetPawn());
	if (!C) return;

	float Pct = FMath::Max(0.0f, C->GetCurrentHP() / C->GetMaxHP());
	float BW = 200, BH = 16, X = Canvas->SizeX - BW - 30, Y = Canvas->SizeY - 40;

	FCanvasTileItem BG(FVector2D(X, Y), FVector2D(BW, BH), FLinearColor(0, 0, 0, 0.5f));
	BG.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(BG);

	FColor BC = Pct > 0.5f ? FColor::Green : (Pct > 0.25f ? FColor::Orange : FColor::Red);
	FCanvasTileItem Fill(FVector2D(X + 2, Y + 2), FVector2D((BW - 4) * Pct, BH - 4), BC);
	Fill.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Fill);

	FString T = FString::Printf(TEXT("HP: %.0f / %.0f"), C->GetCurrentHP(), C->GetMaxHP());
	FCanvasTextItem L(FVector2D(X + 4, Y + BH + 1), FText::FromString(T), GEngine->GetSmallFont(), FColor::White);
	L.EnableShadow(FLinearColor::Black); Canvas->DrawItem(L);
}

// ─── Info panel ────────────────────────────────────────────────────────────
void AMultiShooterGameHUD::DrawInfoPanel()
{
	if (!Canvas) return;
	AMultiShooterGameState* GS = GetGS();
	AMultiShooterPlayerState* PS = GetPS();

	auto BG = [&](float Y, float W, float H) {
		FCanvasTileItem B(FVector2D(12, Y), FVector2D(W, H), FLinearColor(0, 0, 0, 0.4f));
		B.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(B);
	};
	auto LN = [&](float Y, const FString& T, const FColor& C) {
		FCanvasTextItem I(FVector2D(18, Y), FText::FromString(T), GEngine->GetSmallFont(), C);
		I.EnableShadow(FLinearColor::Black); Canvas->DrawItem(I);
	};

	float CY = 12, LH = 21;
	if (GS && !GS->bGameOver)
	{
		FString T; FColor C = FColor::Yellow;
		if (GS->TotalSpawned < GS->MaxEnemies)
			T = FString::Printf(TEXT("Enemies: %d / %d    Killed: %d"), GS->EnemiesRemaining, GS->MaxEnemies, GS->TotalKilled);
		else if (GS->EnemiesRemaining > 0)
			{ T = FString::Printf(TEXT("Enemies: %d    FINAL WAVE!"), GS->EnemiesRemaining); C = FColor::Orange; }
		else
			{ T = TEXT("All enemies cleared!"); C = FColor::Green; }
		float TW = T.Len() * 9 + 12;
		BG(CY, FMath::Max(TW, 280.0f), LH + 12);
		LN(CY + 6, T, C);
		CY += LH + 16;
	}
	if (PS)
	{
		FString KS = FString::Printf(TEXT("Kills: %d    Score: %d"), PS->Kills, FMath::RoundToInt(PS->GetScore()));
		FColor LC = PS->RemainingLives > 2 ? FColor::Cyan : (PS->RemainingLives > 0 ? FColor::Orange : FColor::Red);
		FString LS = FString::Printf(TEXT("Lives: %d    Deaths: %d"), PS->RemainingLives, PS->Deaths);
		float MW = FMath::Max(KS.Len(), LS.Len()) * 9 + 12;
		BG(CY, FMath::Max(MW, 250.0f), LH * 2 + 12);
		LN(CY + 6, KS, FColor::White);
		LN(CY + LH + 6, LS, LC);
	}
}

// ─── Crosshair ─────────────────────────────────────────────────────────────
void AMultiShooterGameHUD::DrawCrosshair()
{
	if (!Canvas) return;
	float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.5f;
	DrawLine(CX, CY - 12, CX, CY - 4, FColor::White, 2);
	DrawLine(CX, CY + 4, CX, CY + 12, FColor::White, 2);
	DrawLine(CX - 12, CY, CX - 4, CY, FColor::White, 2);
	DrawLine(CX + 4, CY, CX + 12, CY, FColor::White, 2);
	FCanvasTileItem D(FVector2D(CX - 1.5f, CY - 1.5f), FVector2D(3, 3), FLinearColor::White);
	D.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(D);
}

// ─── Game over ─────────────────────────────────────────────────────────────
void AMultiShooterGameHUD::DrawGameOver()
{
	if (!Canvas) return;
	AMultiShooterGameState* GS = GetGS();
	float CX = Canvas->SizeX * 0.5f, CY = Canvas->SizeY * 0.5f;
	FCanvasTileItem O(FVector2D(0, 0), FVector2D(Canvas->SizeX, Canvas->SizeY), FLinearColor(0, 0, 0, 0.35f));
	O.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(O);
	FString M = (GS && GS->bVictory) ? TEXT("VICTORY!") : TEXT("GAME OVER");
	FColor CO = (GS && GS->bVictory) ? FColor::Green : FColor::Red;
	FCanvasTextItem B(FVector2D(CX - 100, CY - 40), FText::FromString(M), GEngine->GetLargeFont(), CO);
	B.Scale = FVector2D(2, 2); B.EnableShadow(FLinearColor::Black); Canvas->DrawItem(B);
	if (GS && GS->bVictory)
	{
		FCanvasTextItem S(FVector2D(CX - 80, CY + 10), FText::FromString(TEXT("All enemies cleared!")), GEngine->GetSmallFont(), FColor::White);
		S.EnableShadow(FLinearColor::Black); Canvas->DrawItem(S);
	}
}

// ─── Helpers ──────────────────────────────────────────────────────────────
bool AMultiShooterGameHUD::IsAiming() const
{
	if (!PlayerOwner) return false;
	if (AShooterCharacter* C = Cast<AShooterCharacter>(PlayerOwner->GetPawn()))
		return C->IsAiming();
	return false;
}

AMultiShooterGameState* AMultiShooterGameHUD::GetGS()
{
	return GetWorld() ? GetWorld()->GetGameState<AMultiShooterGameState>() : nullptr;
}

AMultiShooterPlayerState* AMultiShooterGameHUD::GetPS()
{
	return PlayerOwner ? PlayerOwner->GetPlayerState<AMultiShooterPlayerState>() : nullptr;
}
