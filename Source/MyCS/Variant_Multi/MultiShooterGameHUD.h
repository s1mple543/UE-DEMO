// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MultiShooterGameHUD.generated.h"

UCLASS()
class MYCS_API AMultiShooterGameHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void DrawHUD() override;

	void ToggleMenu();
	bool IsMenuOpen() const { return bShowMenu; }
	void HandleClick(float ScreenX, float ScreenY);
	void HandleBackspace();

	int32 HoveredButtonIndex = -1;

protected:

	virtual void BeginPlay() override;

	void CreateScopeTexture();
	void DrawInfoPanel();
	void DrawHealthBar();
	void DrawAmmoCounter();
	void DrawCrosshair();
	void DrawScopeOverlay();
	void DrawGameOver();
	void DrawMainMenu();
	void DrawButton(float X, float Y, float W, float H, const FString& Text, bool bHovered);

	bool IsAiming() const;
	class AMultiShooterGameState* GetGS();
	class AMultiShooterPlayerState* GetPS();

	UPROPERTY()
	TObjectPtr<UTexture2D> ScopeTexture;

	bool bShowMenu = false;
	bool bShowJoinInput = false;

	struct FMenuButton { float X, Y, W, H; FString Text; };
	TArray<FMenuButton> MenuButtons;
};
