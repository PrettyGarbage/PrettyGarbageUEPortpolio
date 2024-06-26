﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionRpgProject/Components/InventoryComponent.h"

#include "ActionRpgProject/Characters/ActionCharacter.h"
#include "ActionRpgProject/Components/AttributeComponent.h"
#include "ActionRpgProject/HUD/UIInventory.h"
#include "ActionRpgProject/HUD/UserHealthBar.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

//디버그용 함수임
void UInventoryComponent::DecreaseHP()
{
	AActionCharacter* ActionCharacter = Cast<AActionCharacter>(GetOwner());
	if(IsValid(ActionCharacter))
	{
		UAttributeComponent* AttributeComponent = ActionCharacter->FindComponentByClass<UAttributeComponent>();
		if(IsValid(AttributeComponent))
		{
			AttributeComponent->ReceiveDamage(.5f);
			if(IsValid(HealthBarWidget))
			{
				HealthBarWidget->UpdateHealthBar();
			}
		}
	}
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	//CreateHealthBar();

	CreateInteractWidget();

	CreateInventoryWidget();
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TraceItemToPickUp();
}

void UInventoryComponent::ActiveInventoryUI()
{
	UWorld* World = GetWorld();
	if(IsValid(World))
	{
		APlayerController* PlayerController = Cast<APlayerController>(World->GetFirstPlayerController());

		if(IsValid(PlayerController))
		{
			OpenInventory(PlayerController);
		}
	}
}

FHitResult UInventoryComponent::TraceItemToPickUp()
{
	AActionCharacter* ActionCharacter = Cast<AActionCharacter>(GetOwner());
	FHitResult HitRetArray;
	if(IsValid(ActionCharacter))
	{
		FVector StartPosition = ActionCharacter->GetActorLocation() - FVector(0, 0, 60.f);
		FVector EndPosition = StartPosition + ActionCharacter->GetActorForwardVector() * 150.f;
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(ActionCharacter);

		bool bIsHit = UKismetSystemLibrary::SphereTraceSingleByProfile(GetWorld(),
		StartPosition,
		EndPosition, 30.f,
		FName("Item"), false, ActorsToIgnore,
		EDrawDebugTrace::None, HitRetArray, true);
	}

	return HitRetArray;
}

void UInventoryComponent::OpenInventory(APlayerController* PlayerController)
{
	if(IsValid(InventoryWidget) && IsValid(PlayerController))
	{
		InventoryWidget->AddToViewport();
		PlayerController->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}

	UActorComponent* AttributeComponent = GetOwner()->GetComponentByClass(UAttributeComponent::StaticClass());
	if(IsValid(AttributeComponent))
	{
		UAttributeComponent* AttributeComp = Cast<UAttributeComponent>(AttributeComponent);
		if(IsValid(AttributeComp))
		{
			InventoryWidget->SetMoneyText(AttributeComp->GetGold());
		}
	}
}

void UInventoryComponent::CreateHealthBar()
{
	if(IsValid(HealthBarWidgetClass))
	{
		HealthBarWidget = CreateWidget<UUserHealthBar>(GetWorld(), HealthBarWidgetClass);
		if(IsValid(HealthBarWidget))
		{
			HealthBarWidget->SetPositionInViewport(FVector2d(5,5), true);
			
			HealthBarWidget->AddToViewport();
		}
	}
}

void UInventoryComponent::CreateInteractWidget()
{
	if(IsValid(InteractWidgetClass))
	{
		InteractWidget = CreateWidget<UUserWidget>(GetWorld(), InteractWidgetClass);
	}
}

void UInventoryComponent::CreateInventoryWidget()
{
	if(IsValid(InventoryWidgetClass))
	{
		InventoryWidget = CreateWidget<UUIInventory>(GetWorld(), InventoryWidgetClass);
	}
}

