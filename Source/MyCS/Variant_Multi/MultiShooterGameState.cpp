// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultiShooterGameState.h"
#include "Net/UnrealNetwork.h"

AMultiShooterGameState::AMultiShooterGameState()
{
	MaxEnemies = 20;
	TotalSpawned = 0;
	TotalKilled = 0;
	EnemiesRemaining = 0;
	bGameOver = false;
	bVictory = false;
}

void AMultiShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultiShooterGameState, MaxEnemies);
	DOREPLIFETIME(AMultiShooterGameState, TotalSpawned);
	DOREPLIFETIME(AMultiShooterGameState, TotalKilled);
	DOREPLIFETIME(AMultiShooterGameState, EnemiesRemaining);
	DOREPLIFETIME(AMultiShooterGameState, bGameOver);
	DOREPLIFETIME(AMultiShooterGameState, bVictory);
}
