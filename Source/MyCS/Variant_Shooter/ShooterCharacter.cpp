// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "Net/UnrealNetwork.h"

AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD
	OnDamaged.Broadcast(1.0f);

	// cache default FOV from camera
	if (GetFirstPersonCameraComponent())
	{
		DefaultFOV = GetFirstPersonCameraComponent()->FirstPersonFieldOfView;
	}

	// Start HP regeneration on the server only
	if (HasAuthority() && !IsDead())
	{
		GetWorld()->GetTimerManager().SetTimer(RegenTimer, this, &AShooterCharacter::OnHealthRegen, 10.0f, true);
	}
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear timers
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(RegenTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);

		// Reload
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AShooterCharacter::DoReload);
		}

		// Aim down sights
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopAim);
		}
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	if (!IsDead())
	{
		if (HasAuthority())
		{
			if (CurrentWeapon)
			{
				CurrentWeapon->StartFiring();
			}
		}
		else
		{
			ServerStartFiring();
		}
	}
}

void AShooterCharacter::ServerStartFiring_Implementation()
{
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::ServerStopFiring_Implementation()
{
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	if (!IsDead())
	{
		if (HasAuthority())
		{
			if (CurrentWeapon)
			{
				CurrentWeapon->StopFiring();
			}
		}
		else
		{
			ServerStopFiring();
		}
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1 && !IsDead())
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon();
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	// stub
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	if (AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass))
	{
		// Refill magazine and switch to it
		int32 Missing = OwnedWeapon->GetMagazineSize() - OwnedWeapon->GetBulletCount();
		if (Missing > 0)
		{
			// Quick-refill: just set ammo to full, skip reload timer
			OwnedWeapon->FinishReload();
		}

		// Switch to this weapon if not already equipped
		if (CurrentWeapon != OwnedWeapon)
		{
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}
			CurrentWeapon = OwnedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
		return;
	}

	// spawn the new weapon
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

	AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

	if (AddedWeapon)
	{
		OwnedWeapons.Add(AddedWeapon);
		if (CurrentWeapon)
		{
			CurrentWeapon->DeactivateWeapon();
		}
		CurrentWeapon = AddedWeapon;
		CurrentWeapon->ActivateWeapon();
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score and notify game mode of player death
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
		GM->OnPlayerDied(GetController());
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterCharacter, CurrentHP);
	DOREPLIFETIME(AShooterCharacter, CurrentWeapon);
	DOREPLIFETIME(AShooterCharacter, OwnedWeapons);
}

void AShooterCharacter::OnRep_CurrentHP()
{
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));
}

void AShooterCharacter::OnHealthRegen()
{
	if (IsDead() || !HasAuthority()) return;

	float NewHP = FMath::Min(MaxHP, CurrentHP + 50.0f);
	if (NewHP != CurrentHP)
	{
		CurrentHP = NewHP;
		OnDamaged.Broadcast(CurrentHP / MaxHP);
	}
}

void AShooterCharacter::DoReload()
{
	if (IsDead())
	{
		return;
	}

	if (HasAuthority())
	{
		if (CurrentWeapon && !CurrentWeapon->IsReloading() && !CurrentWeapon->IsMagazineFull())
		{
			CurrentWeapon->StartReload();
		}
	}
	else
	{
		ServerReload();
	}
}

void AShooterCharacter::ServerReload_Implementation()
{
	if (CurrentWeapon && !IsDead() && !CurrentWeapon->IsReloading() && !CurrentWeapon->IsMagazineFull())
	{
		CurrentWeapon->StartReload();
	}
}

void AShooterCharacter::DoStartAim()
{
	if (IsDead())
	{
		return;
	}

	bIsAiming = true;
}

void AShooterCharacter::DoStopAim()
{
	bIsAiming = false;
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetFirstPersonCameraComponent())
	{
		float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;
		float CurrentFOV = GetFirstPersonCameraComponent()->FirstPersonFieldOfView;
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, 12.0f);
		GetFirstPersonCameraComponent()->FirstPersonFieldOfView = NewFOV;
	}
}
