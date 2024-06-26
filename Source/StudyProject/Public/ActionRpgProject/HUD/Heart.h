﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Heart.generated.h"

UENUM(BlueprintType)
enum class EHeartStatus : uint8
{
	EHS_None UMETA(DisplayName="None"),
	EHS_Empty UMETA(DisplayName="Empty"),
	EHS_Half UMETA(DisplayName="Half"),
	EHS_Full UMETA(DisplayName="Full")
};

UCLASS()
class STUDYPROJECT_API UHeart : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	void SetHealth(float Amount);

	EHeartStatus GetHealthStatus();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UTexture2D> MaxHeartTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UTexture2D> HalfHeartTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UTexture2D> EmptyHeartTexture;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UImage> HeartImage;

	EHeartStatus HeartStatus = EHeartStatus::EHS_None;
};
