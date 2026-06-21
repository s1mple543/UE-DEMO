// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MultiShooterPlayerState.generated.h"

/**
 *  PlayerState for the multiplayer wave survival game.
 *  Tracks kills and deaths per player with replication.
 */
UCLASS()
class MYCS_API AMultiShooterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	AMultiShooterPlayerState();

	/** Number of kills */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	/** Number of deaths */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;

	/** Remaining lives */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 RemainingLives = 3;

	/** Add a kill to the player */
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddKill();

	/** Add a death to the player */
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddDeath();

	/** Set remaining lives */
	UFUNCTION(BlueprintCallable, Category = "Score")
	void SetRemainingLives(int32 Lives);

	/** Replication */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
