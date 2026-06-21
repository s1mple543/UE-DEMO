// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultiShooterPlayerState.h"
#include "Net/UnrealNetwork.h"

AMultiShooterPlayerState::AMultiShooterPlayerState()
{
	Kills = 0;
	Deaths = 0;
	RemainingLives = 3;
}

void AMultiShooterPlayerState::AddKill()
{
	++Kills;
	SetScore(Kills * 100.0f);
}

void AMultiShooterPlayerState::AddDeath()
{
	++Deaths;
}

void AMultiShooterPlayerState::SetRemainingLives(int32 Lives)
{
	RemainingLives = Lives;
}

void AMultiShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMultiShooterPlayerState, Kills);
	DOREPLIFETIME(AMultiShooterPlayerState, Deaths);
	DOREPLIFETIME(AMultiShooterPlayerState, RemainingLives);
}
