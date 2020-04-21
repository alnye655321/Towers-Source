// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Bots/ShooterBot.h"
#include "Online/ShooterPlayerState.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Weapons/ShooterWeapon.h"
#include "Bots/ShooterAIController.h"

AShooterAIController::AShooterAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 	BlackboardComp = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackBoardComp"));
 	
	BrainComponent = BehaviorComp = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("BehaviorComp"));	

	bWantsPlayerState = true;
}

void AShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AShooterBot* Bot = Cast<AShooterBot>(InPawn);

	// start behavior
	if (Bot && Bot->BotBehavior)
	{
		if (Bot->BotBehavior->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*Bot->BotBehavior->BlackboardAsset);
		}

		EnemyKeyID = BlackboardComp->GetKeyID("Enemy");
		NeedAmmoKeyID = BlackboardComp->GetKeyID("NeedAmmo");
		FollowPlayerKeyID = BlackboardComp->GetKeyID("FollowPlayer");
		WaitTimeID = BlackboardComp->GetKeyID("WaitTime");

		BehaviorComp->StartTree(*(Bot->BotBehavior));

		SetFollowPlayer(true);
		//SetWaitTime(1.0f); //currently being set on a per path basis on BP_Path
	}
}

void AShooterAIController::OnUnPossess()
{
	Super::OnUnPossess();

	BehaviorComp->StopTree();
}

void AShooterAIController::BeginInactiveState()
{
	Super::BeginInactiveState();

	AGameStateBase const* const GameState = GetWorld()->GetGameState();

	const float MinRespawnDelay = GameState ? GameState->GetPlayerRespawnDelay(this) : 1.0f;

	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &AShooterAIController::Respawn, MinRespawnDelay);
}

void AShooterAIController::Respawn()
{
	GetWorld()->GetAuthGameMode()->RestartPlayer(this);
}

//only currently finding first player pawn
void AShooterAIController::FindClosestEnemy()
{
	APawn* MyBot = GetPawn();
	if (MyBot == NULL)
	{
		return;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	float BestDistSq = MAX_FLT;
	AVRGunPawn* BestPawn = NULL;

	APawn* TestPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (TestPawn != NULL)
	{
		SetEnemy(TestPawn);
	}
}

bool AShooterAIController::FindClosestEnemyWithLOS(AVRGunPawn* ExcludeEnemy)
{
	bool bGotEnemy = false;
	//APawn* MyBot = GetPawn();
	AShooterBot* MyBot = Cast<AShooterBot>(GetPawn());
	if (MyBot != NULL)
	{
		const FVector MyLoc = MyBot->GetActorLocation();
		float BestDistSq = MAX_FLT;
		AVRGunPawn* BestPawn = NULL;

		APawn* TestPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

		if (LineOfSightTo(UGameplayStatics::GetPlayerPawn(GetWorld(), 0), MyBot->GetActorLocation()))
		{
			bGotEnemy = true;
			MyBot->EnemyLOS = true;
			SetEnemy(TestPawn);
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Enemy has line of sight"));
		}
		else
		{
			MyBot->EnemyLOS = false;
		}

		//if (TestPawn != NULL && HasWeaponLOSToEnemy(TestPawn, true) == true)
		//{
		//	const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
		//	if (DistSq < BestDistSq)
		//	{
		//		BestDistSq = DistSq;

		//		SetEnemy(TestPawn);
		//		bGotEnemy = true;
		//	}
		//}


	}


	return bGotEnemy;
}




void AShooterAIController::CheckAmmo(const class AShooterWeapon* CurrentWeapon)
{
	if (CurrentWeapon && BlackboardComp)
	{
		const int32 Ammo = CurrentWeapon->GetCurrentAmmo();
		const int32 MaxAmmo = CurrentWeapon->GetMaxAmmo();
		const float Ratio = (float) Ammo / (float) MaxAmmo;

		BlackboardComp->SetValue<UBlackboardKeyType_Bool>(NeedAmmoKeyID, (Ratio <= 0.1f));
	}
}

void AShooterAIController::SetEnemy(class APawn* InPawn)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Object>(EnemyKeyID, InPawn);
		SetFocus(InPawn);
	}
}

UObject* AShooterAIController::GetEnemy() const
{
	if (BlackboardComp)
	{
		//return Cast<AVRGunPawn>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(EnemyKeyID));
		UObject* FoundObject = BlackboardComp->GetValue<UBlackboardKeyType_Object>(EnemyKeyID); //get blackboard value as object
		return FoundObject;

	}

	return NULL;
}


void AShooterAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Look toward focus
	FVector FocalPoint = GetFocalPoint();
	if( !FocalPoint.IsZero() && GetPawn())
	{
		FVector Direction = FocalPoint - GetPawn()->GetActorLocation();
		FRotator NewControlRotation = Direction.Rotation();
		
		NewControlRotation.Yaw = FRotator::ClampAxis(NewControlRotation.Yaw);

		SetControlRotation(NewControlRotation);

		APawn* const P = GetPawn();
		if (P && bUpdatePawn)
		{
			P->FaceRotation(NewControlRotation, DeltaTime);
		}
		
	}
}


void AShooterAIController::GameHasEnded(AActor* EndGameFocus, bool bIsWinner)
{
	// Stop the behaviour tree/logic
	BehaviorComp->StopTree();

	// Stop any movement we already have
	StopMovement();

	// Cancel the repsawn timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Respawn);

	// Clear any enemy
	SetEnemy(NULL);

	// Finally stop firing
	AShooterBot* MyBot = Cast<AShooterBot>(GetPawn());
	AShooterWeapon* MyWeapon = MyBot ? MyBot->GetWeapon() : NULL;
	if (MyWeapon == NULL)
	{
		return;
	}
	MyBot->StopWeaponFire();	
}

bool AShooterAIController::HasWeaponLOSToEnemy(AActor* InEnemyActor, const bool bAnyEnemy) const
{

	AShooterBot* MyBot = Cast<AShooterBot>(GetPawn());

	bool bHasLOS = false;
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(AIWeaponLosTrace), true, GetPawn());

	TraceParams.bReturnPhysicalMaterial = true;
	FVector StartLocation = MyBot->GetActorLocation();
	StartLocation.Z += GetPawn()->BaseEyeHeight; //look from eyes

	FHitResult Hit(ForceInit);
	const FVector EndLocation = InEnemyActor->GetActorLocation();
	GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, COLLISION_WEAPON, TraceParams);
	if (Hit.bBlockingHit == true)
	{
		// Theres a blocking hit - check if its our enemy actor
		AActor* HitActor = Hit.GetActor();
		if (Hit.GetActor() != NULL)
		{
			if (HitActor == InEnemyActor)
			{
				bHasLOS = true;
			}
			else if (bAnyEnemy == true)
			{
				// Its not our actor, maybe its still an enemy ?
				ACharacter* HitChar = Cast<ACharacter>(HitActor);
				if (HitChar != NULL)
				{
					AShooterPlayerState* HitPlayerState = Cast<AShooterPlayerState>(HitChar->GetPlayerState());
					AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);
					if ((HitPlayerState != NULL) && (MyPlayerState != NULL))
					{
						if (HitPlayerState->GetTeamNum() != MyPlayerState->GetTeamNum())
						{
							bHasLOS = true;
						}
					}
				}
			}
		}
	}



	return bHasLOS;
}

void AShooterAIController::SetFollowPlayer(bool FollowPlayer)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Bool>(FollowPlayerKeyID, FollowPlayer);
	}
}

bool AShooterAIController::GetFollowPlayer()
{
	if (BlackboardComp)
	{
		return BlackboardComp->GetValue<UBlackboardKeyType_Bool>(FollowPlayerKeyID);
	}
	else
		return NULL;
}

void AShooterAIController::SetWaitTime(float WaitTime)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Float>(WaitTimeID, WaitTime);
	}
}

float AShooterAIController::GetWaitTime()
{
	if (BlackboardComp)
	{
		return BlackboardComp->GetValue<UBlackboardKeyType_Float>(WaitTimeID);
	}
	else
		return NULL;
}



void AShooterAIController::ShootEnemy()
{
	//AShooterBot* MyBot = Cast<AShooterBot>(GetPawn());
	//AShooterWeapon* MyWeapon = MyBot ? MyBot->GetWeapon() : NULL;
	//if (MyWeapon == NULL)
	//{
	//	return;
	//}

	//bool bCanShoot = false;

	////removing ammo check
	//if ((MyWeapon->GetCurrentAmmo() > 0) && (MyWeapon->CanFire() == true))
	//{
	//}
	//	

	//if (LineOfSightTo(UGameplayStatics::GetPlayerPawn(GetWorld(), 0), MyBot->GetActorLocation()))
	//{
	//	bCanShoot = true;
	//}

	//if (bCanShoot)
	//{
	//	MyBot->StartWeaponFire();
	//	MyBot->StartShooting(); //triggers call to BotPawn blueprint, for weapon firing animation handling
	//}
	//else
	//{
	//	MyBot->StopWeaponFire();
	//}
}

