// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ShooterCharacter.h"
#include "ShooterBot.generated.h"

UCLASS()
class AShooterBot : public AShooterCharacter
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Behavior)
	class UBehaviorTree* BotBehavior;

	virtual bool IsFirstPerson() const override;

	virtual void FaceRotation(FRotator NewRotation, float DeltaTime = 0.f) override;


public:

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "StartShooting_CppSource"))
		void StartShooting();

	// Current health of the Pawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShooterBot")
		bool EnemyLOS;



protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

	// Current speed of the Pawn
	float Speed;
};