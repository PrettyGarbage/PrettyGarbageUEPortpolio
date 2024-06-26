﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterTypes.h"
#include "InputActionValue.h"
#include "ActionRpgProject/Interfaces/HitInterface.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

UCLASS()
class STUDYPROJECT_API ABaseCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type InType);

	UFUNCTION(BlueprintCallable)
	void SetActionState(EActionState InState);

	UFUNCTION(BlueprintCallable)
	void WarpToTarget();

	FORCEINLINE EDeathPose GetDeathPose() const { return DeathPose; }
	
protected:
	virtual void BeginPlay() override;

	virtual void GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter) override;

	virtual void Attack(const FInputActionValue& InValue);

	virtual void Attack(); //Enhanced Input Method와 분리하기 위한 오버로딩

	virtual void Die();

	/* Montage */
	virtual void PlayHitReactMontage(const FName& SectionName);

	virtual void DirectionHitReact(const FVector& ImpactPoint);

	virtual void PlayHitSound(const FVector& ImpactPoint);

	virtual void SpawnHitParticle(const FVector& ImpactPoint);

	virtual void HandleDamage(float DamageAmount);

	virtual void PlayMontageSection(UAnimMontage* Montage, const FName& SectionName);
	
	virtual int32 PlayAttackMontage();
	
	virtual void StopAttackMontage();

	virtual int32 PlayRandomMontageSection(UAnimMontage* Montage, const TArray<FName>& SectionNames);

	virtual int32 PlayDeathMontage();

	virtual void PlayDodgeMontage();

	virtual void DisableCapsule();
	
	virtual bool IsAlive();
	
	virtual bool CanAttack();

	UFUNCTION()
	FVector GetTranslationWarpTarget(ABaseCharacter* InTarget);

	virtual void DestroyActor();
	
protected:
	
	UPROPERTY(VisibleAnywhere, Category= "Weapon")
	TObjectPtr<class ASwordWeapon> EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	TObjectPtr<class UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, Category="Montages")
	TObjectPtr<class UAnimMontage> HitReactMontage;

	UPROPERTY(EditAnywhere, Category="Montages")
	TObjectPtr<class UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	TObjectPtr<class UAnimMontage> DodgeMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<FName> AttackMontageSections;

	UPROPERTY(EditAnywhere, Category="Combat")
	TArray<FName> DeathMontageSections;

	UPROPERTY(BlueprintReadOnly)
	EDeathPose DeathPose;
	
	/* Components */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UAttributeComponent> AttributeComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UMotionWarpingComponent> MotionWarpComponent;

	/* Sounds and Visual Effects */
	UPROPERTY(EditAnywhere, Category="Combat")
	TObjectPtr<class USoundBase> HitSound;

	UPROPERTY(EditAnywhere, Category="Combat")
	TObjectPtr<class UParticleSystem> HitParticle;

	UPROPERTY(EditAnywhere, Category="Combat")
	EActionState CurrentActionState = EActionState::EAS_None;

	UPROPERTY(BlueprintReadOnly, Category="Combat")
	TObjectPtr<class AActor> CombatTarget;

	UPROPERTY(EditAnywhere, Category="Combat")
	double WarpTargetDistance = 75.f;
};
