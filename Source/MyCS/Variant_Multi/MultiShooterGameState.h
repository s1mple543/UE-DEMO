// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MultiShooterGameState.generated.h"

/**
 *  GameState — tracks enemy spawning progress and game status on all clients.
 */
UCLASS()
class MYCS_API AMultiShooterGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	AMultiShooterGameState();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 MaxEnemies = 20;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 TotalSpawned = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 TotalKilled = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 EnemiesRemaining = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	bool bGameOver = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	bool bVictory = false;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
