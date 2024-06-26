﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionRpgProject/Characters/Enemy/ActionBossAICharacter.h"

#include "ActionRpgProject/Characters/ActionCharacter.h"
#include "ActionRpgProject/Controller/EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/PawnSensingComponent.h"


// Sets default values
AActionBossAICharacter::AActionBossAICharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
}

void AActionBossAICharacter::CheckHit()
{
	FHitResult HitResult;
	FCollisionQueryParams Params(NAME_None, false, this);

	bool bResult = GetWorld()->SweepSingleByChannel(
		HitResult,
		GetActorLocation(),
		GetActorLocation() + AttackRange * GetActorForwardVector(),
		FQuat::Identity,
		ECC_GameTraceChannel2,
		FCollisionShape::MakeSphere(AttackRadius),
		Params);

	if(bResult && IsValid(HitResult.GetActor()))
	{
		if(IHitInterface* HitInterface = Cast<IHitInterface>(HitResult.GetActor()))
		{
			HitInterface->Execute_GetHit(HitResult.GetActor(), HitResult.ImpactPoint, GetOwner());

			UGameplayStatics::ApplyDamage(
			HitResult.GetActor(),
			AttackDamage,
			GetInstigator()->GetController(),
			this,
			UDamageType::StaticClass()
			);
		}
	}
}

// Called when the game starts or when spawned
void AActionBossAICharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializePawnSensingComponent();
}

void AActionBossAICharacter::InitializePawnSensingComponent()
{
	if(!IsValid(PawnSensingComponent)) return;

	PawnSensingComponent->OnSeePawn.AddDynamic(this, &AActionBossAICharacter::OnSeePawn);
}

void AActionBossAICharacter::OnSeePawn(APawn* Pawn)
{
	AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController());
	if(IsValid(AIController))
	{
		if(IsValid(Pawn) && Pawn->IsA<AActionCharacter>())
		{
			AIController->GetBlackboardComponent()->SetValueAsObject(AIController->TargetActorKey, Pawn);
			AIController->GetBlackboardComponent()->SetValueAsVector(AIController->EndPatrolPositionKey, Pawn->GetActorLocation());
		}
	}
}
