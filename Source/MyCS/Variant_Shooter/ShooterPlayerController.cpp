// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Shooter/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ShooterCharacter.h"
#include "MultiShooterGameState.h"
#include "MultiShooterPlayerState.h"
#include "MultiShooterGameHUD.h"
#include "MyCS.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
			if (MobileControlsWidget)
				MobileControlsWidget->AddToPlayerScreen(0);
			else
				UE_LOG(LogMyCS, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController()) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Ctx : DefaultMappingContexts)
			Subsystem->AddMappingContext(Ctx, 0);
		if (!ShouldUseTouchControls())
			for (UInputMappingContext* Ctx : MobileExcludedMappingContexts)
				Subsystem->AddMappingContext(Ctx, 0);
	}

	// F10 toggle menu
	FInputKeyBinding F10(FInputChord(EKeys::F10, false, false, false, false), IE_Pressed);
	F10.bConsumeInput = true;
	F10.KeyDelegate.BindDelegate(this, &AShooterPlayerController::ToggleMenu);
	InputComponent->KeyBindings.Add(F10);

	// Left mouse click
	FInputKeyBinding LMB(FInputChord(EKeys::LeftMouseButton, false, false, false, false), IE_Released);
	LMB.bConsumeInput = false;
	LMB.KeyDelegate.BindDelegate(this, &AShooterPlayerController::OnLeftClick);
	InputComponent->KeyBindings.Add(LMB);

	// Enter (for menu confirm)
	FInputKeyBinding Enter(FInputChord(EKeys::Enter, false, false, false, false), IE_Pressed);
	Enter.bConsumeInput = false;
	Enter.KeyDelegate.BindDelegate(this, &AShooterPlayerController::OnMenuEnter);
	InputComponent->KeyBindings.Add(Enter);

	// Backspace
	FInputKeyBinding BS(FInputChord(EKeys::BackSpace, false, false, false, false), IE_Pressed);
	BS.bConsumeInput = false;
	BS.KeyDelegate.BindDelegate(this, &AShooterPlayerController::OnMenuBackspace);
	InputComponent->KeyBindings.Add(BS);
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	InPawn->OnDestroyed.AddDynamic(this, &AShooterPlayerController::OnPawnDestroyed);
	if (AShooterCharacter* C = Cast<AShooterCharacter>(InPawn))
		C->Tags.Add(PlayerPawnTag);
}

void AShooterPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	if (AMultiShooterGameState* GS = GetWorld()->GetGameState<AMultiShooterGameState>())
		if (GS->bGameOver || GS->bVictory) return;

	if (AMultiShooterPlayerState* PS = GetPlayerState<AMultiShooterPlayerState>())
		if (PS->RemainingLives <= 0) return;

	TArray<AActor*> Starts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), Starts);
	if (Starts.Num() == 0) return;

	int32 Idx = FMath::RandRange(0, Starts.Num() - 1);
	FTransform T = Starts[Idx]->GetActorTransform();
	if (AShooterCharacter* C = GetWorld()->SpawnActor<AShooterCharacter>(CharacterClass, T))
		Possess(C);
}

// ─── Menu ──────────────────────────────────────────────────────────────────

void AShooterPlayerController::ToggleMenu()
{
	if (AMultiShooterGameHUD* H = Cast<AMultiShooterGameHUD>(GetHUD()))
		H->ToggleMenu();
}

void AShooterPlayerController::OnLeftClick()
{
	if (AMultiShooterGameHUD* H = Cast<AMultiShooterGameHUD>(GetHUD()))
	{
		if (H->IsMenuOpen())
		{
			float MX, MY;
			GetMousePosition(MX, MY);
			H->HandleClick(MX, MY);
		}
	}
}

void AShooterPlayerController::OnMenuEnter()
{
	if (AMultiShooterGameHUD* H = Cast<AMultiShooterGameHUD>(GetHUD()))
	{
		if (H->IsMenuOpen())
		{
			float MX, MY;
			GetMousePosition(MX, MY);
			H->HandleClick(MX, MY);
		}
	}
}

void AShooterPlayerController::OnMenuBackspace()
{
	if (AMultiShooterGameHUD* H = Cast<AMultiShooterGameHUD>(GetHUD()))
		H->HandleBackspace();
}

bool AShooterPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
