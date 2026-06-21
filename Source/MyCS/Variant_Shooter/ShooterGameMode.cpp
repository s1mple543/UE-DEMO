// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameMode.h"
#include "ShooterNPC.h"
#include "ShooterCharacter.h"
#include "MultiShooterGameState.h"
#include "MultiShooterPlayerState.h"
#include "MultiShooterGameHUD.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"

AShooterGameMode::AShooterGameMode()
{
	GameStateClass = AMultiShooterGameState::StaticClass();
	PlayerStateClass = AMultiShooterPlayerState::StaticClass();
	HUDClass = AMultiShooterGameHUD::StaticClass();
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
	MultiGameState = GetMultiGameState();

	// Update GameState display
	if (AMultiShooterGameState* GS = GetMultiGameState())
	{
		GS->MaxEnemies = MaxEnemies;
		GS->TotalSpawned = 0;
		GS->TotalKilled = 0;
		GS->EnemiesRemaining = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[MultiShooter] Spawning 1 enemy every %.0fs, max %d, each player %d lives"),
		SpawnInterval, MaxEnemies, PlayerLives);

	// Schedule first spawn
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterGameMode::SpawnNextEnemy, InitialDelay, false);
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	// unused stub — kept for ShooterCharacter::Die() compatibility
}

// ===== Spawning =====

void AShooterGameMode::SpawnNextEnemy()
{
	if (bGameOver) return;

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);

	if (!EnemyNPCClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MultiShooter] EnemyNPCClass not set!"));
		return;
	}

	FVector SpawnLoc;
	if (!GetSpawnLocation(SpawnLoc))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MultiShooter] No spawn location — will retry in 2s"));
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterGameMode::SpawnNextEnemy, 2.0f, false);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AShooterNPC* NPC = GetWorld()->SpawnActor<AShooterNPC>(EnemyNPCClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
	if (!NPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MultiShooter] Spawn failed — retrying in 2s"));
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterGameMode::SpawnNextEnemy, 2.0f, false);
		return;
	}

	++TotalSpawned;

	NPC->CurrentHP = 100.0f;
	NPC->OnPawnDeath.AddDynamic(this, &AShooterGameMode::OnEnemyKilled);

	if (AMultiShooterGameState* GS = GetMultiGameState())
	{
		GS->TotalSpawned = TotalSpawned;
		GS->EnemiesRemaining = FMath::Max(0, TotalSpawned - TotalKilled);
	}

	UE_LOG(LogTemp, Verbose, TEXT("[MultiShooter] Spawned enemy %d/%d"), TotalSpawned, MaxEnemies);

	// Schedule next or mark finished
	if (TotalSpawned >= MaxEnemies)
	{
		bFinishedSpawning = true;
		UE_LOG(LogTemp, Log, TEXT("[MultiShooter] All %d enemies spawned! Kill them all to win."), MaxEnemies);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterGameMode::SpawnNextEnemy, SpawnInterval, false);
	}
}

// ===== Enemy killed =====

void AShooterGameMode::OnEnemyKilled()
{
	if (bGameOver) return;

	++TotalKilled;

	if (AMultiShooterGameState* GS = GetMultiGameState())
	{
		GS->TotalKilled = TotalKilled;
		GS->EnemiesRemaining = FMath::Max(0, TotalSpawned - TotalKilled);
	}

	UE_LOG(LogTemp, Verbose, TEXT("[MultiShooter] Enemy killed — %d/%d total, %d alive"), TotalKilled, MaxEnemies, FMath::Max(0, TotalSpawned - TotalKilled));

	// Check victory: all enemies spawned AND all killed
	if (bFinishedSpawning && TotalKilled >= MaxEnemies)
	{
		HandleVictory();
	}
}

// ===== Player death & lives =====

void AShooterGameMode::OnPlayerDied(AController* PlayerController)
{
	if (bGameOver) return;

	AMultiShooterPlayerState* PS = PlayerController ? PlayerController->GetPlayerState<AMultiShooterPlayerState>() : nullptr;
	if (!PS) return;

	// Decrement this player's lives
	int32 NewLives = PS->RemainingLives - 1;
	PS->SetRemainingLives(NewLives);
	PS->AddDeath();

	UE_LOG(LogTemp, Log, TEXT("[MultiShooter] Player %s died — %d lives left"), *PS->GetPlayerName(), NewLives);

	// Don't respawn if out of lives
	if (NewLives <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[MultiShooter] Player %s eliminated"), *PS->GetPlayerName());
		CheckDefeat();
	}
}

void AShooterGameMode::CheckDefeat()
{
	// All players must have 0 lives for defeat
	bool bAllDead = true;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AMultiShooterPlayerState* MPS = Cast<AMultiShooterPlayerState>(PS))
		{
			if (MPS->RemainingLives > 0)
			{
				bAllDead = false;
				break;
			}
		}
		else
		{
			// Non-multi player states don't count
		}
	}

	if (bAllDead)
	{
		HandleDefeat();
	}
}

// ===== End conditions =====

void AShooterGameMode::HandleVictory()
{
	bGameOver = true;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);

	UE_LOG(LogTemp, Log, TEXT("[MultiShooter] VICTORY! All %d enemies killed!"), MaxEnemies);

	if (AMultiShooterGameState* GS = GetMultiGameState())
	{
		GS->bGameOver = true;
		GS->bVictory = true;
	}
}

void AShooterGameMode::HandleDefeat()
{
	bGameOver = true;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);

	UE_LOG(LogTemp, Log, TEXT("[MultiShooter] DEFEAT — all players eliminated"));

	if (AMultiShooterGameState* GS = GetMultiGameState())
	{
		GS->bGameOver = true;
		GS->bVictory = false;
	}
}

// ===== Helpers =====

bool AShooterGameMode::GetSpawnLocation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
		FVector Origin = PlayerStarts.Num() > 0 ? PlayerStarts[0]->GetActorLocation() : FVector(0, 0, 100);

		FNavLocation NavLoc;
		if (NavSys->GetRandomReachablePointInRadius(Origin, 3000.0f, NavLoc))
		{
			OutLocation = NavLoc.Location;
			return true;
		}
	}

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	if (PlayerStarts.Num() > 0)
	{
		int32 Index = FMath::RandRange(0, PlayerStarts.Num() - 1);
		OutLocation = PlayerStarts[Index]->GetActorLocation();
		OutLocation.Z += 100.0f;
		return true;
	}

	OutLocation = FVector::ZeroVector;
	return false;
}

AMultiShooterGameState* AShooterGameMode::GetMultiGameState()
{
	if (!MultiGameState)
	{
		MultiGameState = Cast<AMultiShooterGameState>(GameState);
	}
	return MultiGameState;
}
