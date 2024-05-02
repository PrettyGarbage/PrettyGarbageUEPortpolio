﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionRpgProject/Subsystems/ActorManagerSubsystem.h"

#include "ActionRpgProject/Characters/ActionCharacter.h"
#include "ActionRpgProject/Characters/Enemy/ActionAICharacter.h"
#include "ActionRpgProject/Game/ActionGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UActorManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldEnemies.Empty();

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActionAICharacter::StaticClass(), AllActors);

	for(AActor* Actor : AllActors)
	{
		if(Actor->IsA<AActionAICharacter>())
		{
			WorldEnemies.Add(Cast<AActionAICharacter>(Actor));
		}
	}

	LoadJsonData();

	GetWorld()->GetTimerManager().SetTimer(SearchHandle, this, &UActorManagerSubsystem::DrawNearIconAroundPlayer, 0.5f, true);
}



void UActorManagerSubsystem::DrawNearIconAroundPlayer()
{
	AActionCharacter* Player = Cast<AActionCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if(IsValid(Player))
	{
		FVector PlayerLocation = Player->GetActorLocation();
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(Player);

		TArray<FHitResult> HitResults;
		float SearchRadius = FMath::Square(SearchDistance);

		FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(SearchDistance);
		if(GetWorld()->SweepMultiByChannel(HitResults, PlayerLocation, PlayerLocation, FQuat::Identity, ECollisionChannel::ECC_Pawn, SphereCollisionShape, CollisionParams))
		{
			for(const FHitResult& HitResult : HitResults)
			{
				if(IsValid(HitResult.GetActor()) && HitResult.GetActor()->IsA<AActionAICharacter>())
				{
					AActionAICharacter* Enemy = Cast<AActionAICharacter>(HitResult.GetActor());
					float DistanceFromPlayer = (Enemy->GetActorLocation() - PlayerLocation).SizeSquared();
					if(DistanceFromPlayer <= SearchRadius && !EnemiesInRange.Contains(Enemy))
					{
						EnemiesInRange.Add(Enemy);
						Enemy->ShowPivot(true);
					}
					else if(DistanceFromPlayer > SearchRadius && EnemiesInRange.Contains(Enemy))
					{
						EnemiesInRange.Remove(Enemy);
						Enemy->ShowPivot(false);
					}
				}
			}
		}
		//UKismetSystemLibrary::DrawDebugSphere(GetWorld(), PlayerLocation, SearchDistance, 12, FLinearColor::Red, 0.1f, 0.1f);
	}
}

void UActorManagerSubsystem::AddEnemy(AActionAICharacter* InEnemy)
{
	WorldEnemies.Add(InEnemy);
}

void UActorManagerSubsystem::RemoveEnemy(AActionAICharacter* InEnemy)
{
	WorldEnemies.Remove(InEnemy);

	if(EnemiesInRange.Contains(InEnemy))
	{
		EnemiesInRange.Remove(InEnemy);
	}
}

TWeakObjectPtr<AActionAICharacter> UActorManagerSubsystem::GetClosestEnemy(const FVector& InLocation)
{
	TWeakObjectPtr<AActionAICharacter> ClosestEnemy;
	float MinDistance = TNumericLimits<float>::Max();

	for(TWeakObjectPtr<AActionAICharacter> Enemy : EnemiesInRange)
	{
		if(Enemy.IsValid())
		{
			float Distance = (Enemy->GetActorLocation() - InLocation).SizeSquared();
			if(Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestEnemy = Enemy;
			}
		}
	}

	return ClosestEnemy;
}


void UActorManagerSubsystem::ClearEnemies()
{
	WorldEnemies.Empty();
}

void UActorManagerSubsystem::ClearSearchTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(SearchHandle);
}

void UActorManagerSubsystem::LoadJsonData()
{
	if(FJsonSerializer::Deserialize(EnemyJsonReader, EnemyJsonObject))
	{
		TArray<TSharedPtr<FJsonValue>> Enemies = EnemyJsonObject->GetArrayField("Enemies");
		for(const TSharedPtr<FJsonValue>& Enemy : Enemies)
		{
			TSharedPtr<FJsonObject> EnemyObject = Enemy->AsObject();
			FString Name = EnemyObject->GetStringField("Name");
			float HP = EnemyObject->GetNumberField("HP");

			EnemyTableData.Add(MakeShared<FEnemyTableRow>(Name, HP));
			UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Name : %s, HP : %f"), *Name, HP));
		}
	}
}

FEnemyTableRow UActorManagerSubsystem::GetEnemyData(const FString& InName)
{
	//EnemyTableData에서 InName과 일치하는 데이터를 찾아서 반환
	for(const TSharedRef<FEnemyTableRow>& EnemyData : EnemyTableData)
	{
		if(EnemyData->Name.Equals(InName))
		{
			return *EnemyData;
		}
	}

	return FEnemyTableRow();
}
