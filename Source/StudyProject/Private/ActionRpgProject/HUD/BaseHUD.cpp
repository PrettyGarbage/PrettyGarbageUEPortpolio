﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionRpgProject/HUD/BaseHUD.h"

#include "ActionRpgProject/HUD/UIInventory.h"
#include "ActionRpgProject/HUD/UIOverlay.h"

void ABaseHUD::ToggleInventory()
{
	
}

void ABaseHUD::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if(IsValid(World))
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if(IsValid(PlayerController))
		{
			if(IsValid(OverlayWidgetClass))
			{
				OverlayWidget = CreateWidget<UUIOverlay>(PlayerController, OverlayWidgetClass);
				OverlayWidget->AddToViewport();
			}
		}
	}
}
