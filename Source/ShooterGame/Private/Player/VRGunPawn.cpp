// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"
#include "VRGunPawn.h"

// Sets default values
AVRGunPawn::AVRGunPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AVRGunPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVRGunPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Health = 100.0f;

}

// Called to bind functionality to input
void AVRGunPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

float AVRGunPawn::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{

	// Modify based on game rules.
	AShooterGameMode* const Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	FString DamageStr = FString::SanitizeFloat(ActualDamage);

	if (ActualDamage > 0.f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Took Damage: " + DamageStr));
		Health -= ActualDamage;
		if (Health <= 0)
		{
			//Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			//PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}

