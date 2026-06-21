// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"

AShooterProjectile::AShooterProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Component"));

	CollisionComponent->SetSphereRadius(16.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));

	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bShouldBounce = true;

	HitDamageType = UDamageType::StaticClass();
}

void AShooterProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Only ignore the instigator (shooter) so they don't shoot themselves.
	// Other players are NOT ignored — friendly fire is enabled.
	CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
}

void AShooterProjectile::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(DestructionTimer);
}

void AShooterProjectile::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHit)
	{
		return;
	}

	bHit = true;

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MakeNoise(NoiseLoudness, GetInstigator(), GetActorLocation(), NoiseRange, NoiseTag);

	if (bExplodeOnHit)
	{
		ExplosionCheck(GetActorLocation());
	}
	else
	{
		ProcessHit(Other, OtherComp, Hit.ImpactPoint, -Hit.ImpactNormal);
	}

	BP_OnProjectileHit(Hit);

	if (DeferredDestructionTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(DestructionTimer, this, &AShooterProjectile::OnDeferredDestruction, DeferredDestructionTime, false);
	}
	else
	{
		Destroy();
	}
}

void AShooterProjectile::ExplosionCheck(const FVector& ExplosionCenter)
{
	TArray<FOverlapResult> Overlaps;

	FCollisionShape OverlapShape;
	OverlapShape.SetSphere(ExplosionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (!bDamageOwner)
	{
		QueryParams.AddIgnoredActor(GetInstigator());
	}

	GetWorld()->OverlapMultiByObjectType(Overlaps, ExplosionCenter, FQuat::Identity, ObjectParams, OverlapShape, QueryParams);

	TArray<AActor*> DamagedActors;

	for (const FOverlapResult& CurrentOverlap : Overlaps)
	{
		if (DamagedActors.Find(CurrentOverlap.GetActor()) == INDEX_NONE)
		{
			DamagedActors.Add(CurrentOverlap.GetActor());

			const FVector& ExplosionDir = CurrentOverlap.GetActor()->GetActorLocation() - GetActorLocation();
			ProcessHit(CurrentOverlap.GetActor(), CurrentOverlap.GetComponent(), GetActorLocation(), ExplosionDir.GetSafeNormal());
		}
	}
}

void AShooterProjectile::ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection)
{
	// Damage any character, BUT prevent NPC-vs-NPC damage
	if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
	{
		if (GetInstigator() && GetInstigator()->GetController())
		{
			APawn* InstigatorPawn = GetInstigator();
			bool bInstigatorIsPlayer = InstigatorPawn->IsPlayerControlled();
			bool bTargetIsPlayer = HitCharacter->IsPlayerControlled();

			// Skip damage if NEITHER is a player (NPC shooting NPC)
			if (bInstigatorIsPlayer || bTargetIsPlayer)
			{
				UGameplayStatics::ApplyDamage(HitCharacter, HitDamage, GetInstigator()->GetController(), this, HitDamageType);
			}
		}
	}

	if (HitComp->IsSimulatingPhysics())
	{
		HitComp->AddImpulseAtLocation(HitDirection * PhysicsForce, HitLocation);
	}
}

void AShooterProjectile::OnDeferredDestruction()
{
	Destroy();
}
