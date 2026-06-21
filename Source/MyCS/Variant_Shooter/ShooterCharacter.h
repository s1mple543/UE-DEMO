// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MyCSCharacter.h"
#include "ShooterWeaponHolder.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);

/**
 *  A player controllable first person shooter character
 *  Manages a weapon inventory through the IShooterWeaponHolder interface
 *  Manages health and death
 */
UCLASS()
class MYCS_API AShooterCharacter : public AMyCSCharacter, public IShooterWeaponHolder
{
	GENERATED_BODY()
	
	/** AI Noise emitter component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

protected:

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Switch weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** Reload input action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ReloadAction;

	/** Aim down sights input action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AimAction;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** Default camera FOV (set from camera component in constructor) */
	float DefaultFOV = 70.0f;

	/** ADS zoomed FOV — scope-like zoom */
	UPROPERTY(EditAnywhere, Category = "Aim", meta = (ClampMin = 10, ClampMax = 170))
	float AimFOV = 35.0f;

	/** Default camera relative location */
	FVector DefaultCameraOffset = FVector(-2.8f, 5.89f, 0.0f);

	/** If true, the character is currently aiming down sights */
	bool bIsAiming = false;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Timer for HP regeneration */
	FTimerHandle RegenTimer;

	/** Current HP remaining to this character */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP)
	float CurrentHP = 0.0f;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** Actor tag to grant this character when it dies */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** List of weapons picked up by the character */
	UPROPERTY(Replicated)
	TArray<AShooterWeapon*> OwnedWeapons;

	/** Weapon currently equipped and ready to shoot with */
	UPROPERTY(Replicated)
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

public:

	/** Constructor */
	AShooterCharacter();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Replication */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:

	/** Handle incoming damage */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** Handles aim inputs from either controls or UI interfaces */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward)  override;

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart()  override;

	/** Handles jump end inputs from either controls or UI interfaces */
	virtual void DoJumpEnd()  override;

	/** Handles start firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** Server RPC - start firing */
	UFUNCTION(Server, Reliable)
	void ServerStartFiring();

	/** Server RPC - stop firing */
	UFUNCTION(Server, Reliable)
	void ServerStopFiring();

	/** Handles stop firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** Handles switch weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	/** Handles reload input */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoReload();

	/** Server RPC - reload */
	UFUNCTION(Server, Reliable)
	void ServerReload();

	/** Handles start aiming (ADS) input */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoStartAim();

	/** Handles stop aiming (ADS) input */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoStopAim();

	/** Update FOV each tick when aiming */
	virtual void Tick(float DeltaSeconds) override;

public:

	//~Begin IShooterWeaponHolder interface

	/** Attaches a weapon's meshes to the owner */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;

	/** Plays the firing montage for the weapon */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** Applies weapon recoil to the owner */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** Updates the weapon's HUD with the current ammo count */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** Calculates and returns the aim location for the weapon */
	virtual FVector GetWeaponTargetLocation() override;

	/** Gives a weapon of this class to the owner */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;

	/** Activates the passed weapon */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;

	/** Deactivates the passed weapon */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;

	/** Notifies the owner that the weapon cooldown has expired and it's ready to shoot again */
	virtual void OnSemiWeaponRefire() override;

	//~End IShooterWeaponHolder interface

protected:

	/** Returns true if the character already owns a weapon of the given class */
	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** Called when this character's HP is depleted */
	void Die();

	/** RepNotify for HP changes */
	UFUNCTION()
	void OnRep_CurrentHP();

	/** Called to allow Blueprint code to react to this character's death */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** Called from the respawn timer to destroy this character and force the PC to respawn */
	void OnRespawn();

	/** Regenerate HP every 10 seconds */
	void OnHealthRegen();

public:

	/** Returns true if the character is dead */
	bool IsDead() const;

	/** Returns current HP */
	float GetCurrentHP() const { return CurrentHP; }

	/** Returns max HP */
	float GetMaxHP() const { return MaxHP; }

	/** Returns the currently equipped weapon */
	AShooterWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	/** Returns true if the character is aiming down sights */
	bool IsAiming() const { return bIsAiming; }
};
