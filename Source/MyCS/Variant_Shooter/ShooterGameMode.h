// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

class AShooterNPC;
class AMultiShooterGameState;

/**
 *  GameMode: spawn 1 enemy every 15s, max 20 total.
 *  Kill all 20 to win. Each player has 3 lives — all dead = lose.
 */
UCLASS()
class MYCS_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AShooterGameMode();

protected:

	/** NPC class to spawn */
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AShooterNPC> EnemyNPCClass;

	/** Time between enemy spawns */
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = 1, ClampMax = 60))
	float SpawnInterval = 15.0f;

	/** Total enemies to spawn for victory */
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = 1, ClampMax = 100))
	int32 MaxEnemies = 20;

	/** Initial delay before first spawn */
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = 1, ClampMax = 30))
	float InitialDelay = 5.0f;

	/** Players per-life HP */
	UPROPERTY(EditAnywhere, Category = "Health")
	float PlayerMaxHP = 100.0f;

	/** How many lives each player gets */
	UPROPERTY(EditAnywhere, Category = "Lives")
	int32 PlayerLives = 3;

	// ---- runtime state ----

	/** Total enemies spawned so far */
	int32 TotalSpawned = 0;

	/** Total enemies killed (counting) */
	int32 TotalKilled = 0;

	/** Has spawning finished? */
	bool bFinishedSpawning = false;

	/** Is the game over? */
	bool bGameOver = false;

	FTimerHandle SpawnTimer;

	UPROPERTY()
	TObjectPtr<AMultiShooterGameState> MultiGameState;

protected:

	virtual void BeginPlay() override;

public:

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte);

	/** Called when an enemy dies */
	UFUNCTION()
	void OnEnemyKilled();

	/** Called when a player dies */
	void OnPlayerDied(AController* PlayerController);

	/** Check if all players are out of lives */
	void CheckDefeat();

	/** Handle victory */
	void HandleVictory();

	/** Handle defeat */
	void HandleDefeat();

protected:

	/** Spawn the next single enemy */
	void SpawnNextEnemy();

	/** Teleport all living players to random player starts */
	void TeleportPlayersToStart();

	/** Helper to get a spawn location */
	bool GetSpawnLocation(FVector& OutLocation) const;

	AMultiShooterGameState* GetMultiGameState();
};
