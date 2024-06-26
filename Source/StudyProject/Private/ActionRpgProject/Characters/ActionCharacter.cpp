﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionRpgProject/Characters/ActionCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ActionRpgProject/Characters/Enemy/ActionAICharacter.h"
#include "ActionRpgProject/Components/AttributeComponent.h"
#include "ActionRpgProject/Components/InventoryComponent.h"
#include "ActionRpgProject/Define/DefineVariables.h"
#include "ActionRpgProject/Game/ActionGameInstance.h"
#include "ActionRpgProject/Game/ActionPlayerState.h"
#include "ActionRpgProject/HUD/BaseHUD.h"
#include "ActionRpgProject/HUD/UIOverlay.h"
#include "ActionRpgProject/HUD/UserHealthBar.h"
#include "ActionRpgProject/Inputs/InputConfigDatas.h"
#include "ActionRpgProject/Items/ProjectileBase.h"
#include "ActionRpgProject/Items/SwordWeapon.h"
#include "ActionRpgProject/Items/Treasure.h"
#include "ActionRpgProject/Subsystems/ActorManagerSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AActionCharacter::AActionCharacter() : bIsEquipped(false), bIsAttacking(false)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(GetRootComponent());
	SpringArmComponent->TargetArmLength = 300.f;
	SpringArmComponent->bUsePawnControlRotation = true;
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory Component"));
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);

	GetMesh()->SetCollisionObjectType(ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	bReplicates = true;
}

// Called to bind functionality to input
void AActionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if(IsValid(EnhancedInputComponent))
	{
		EnhancedInputComponent->BindAction(InputConfigData->MoveAction, ETriggerEvent::Triggered, this, &AActionCharacter::Move);
		EnhancedInputComponent->BindAction(InputConfigData->LookAction, ETriggerEvent::Triggered, this, &AActionCharacter::Look);
		EnhancedInputComponent->BindAction(InputConfigData->JumpAction, ETriggerEvent::Started, this, &AActionCharacter::Jump);
		EnhancedInputComponent->BindAction(InputConfigData->EquipAction, ETriggerEvent::Started, this, &AActionCharacter::Equip);
		EnhancedInputComponent->BindAction(InputConfigData->AttackAction, ETriggerEvent::Started, this, &AActionCharacter::Attack);
		EnhancedInputComponent->BindAction(InputConfigData->InventoryAction, ETriggerEvent::Started, this, &AActionCharacter::Inventory);
		EnhancedInputComponent->BindAction(InputConfigData->LookClosetEnemyAction, ETriggerEvent::Started, this, &AActionCharacter::LookClosetEnemy);
		EnhancedInputComponent->BindAction(InputConfigData->ProjectileSkillAction, ETriggerEvent::Started, this, &AActionCharacter::SkillProjectile);
	}
}

float AActionCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
                                   AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	HandleDamage(DamageAmount);

	if(!IsAlive())
	{
		Die();
	}

	//아래로 대체 되어질 예정
	SetHUDHealth();

	//대체될 코드
	//SetHealthBarValue();
	
	return DamageAmount;
}

void AActionCharacter::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	//Super::GetHit_Implementation(ImpactPoint, Hitter);

	if(CurrentActionState == EActionState::EAS_Dodging) return;
	
	if(IsAlive())
	{
		SetActionState(EActionState::EAS_HitReact);
		PlayHitReactMontage(GFromFront);
	}
	else
	{
		Die();
	}

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
	PlayHitSound(ImpactPoint);
	SpawnHitParticle(ImpactPoint);
}

void AActionCharacter::SetOverlappingItem(AItemBase* InItem)
{
	OverlappingItem = InItem;
}

void AActionCharacter::AddGold(ATreasure* InTreasure)
{
	if(IsValid(AttributeComponent) && IsValid(UIOverlay))
	{
		AttributeComponent->AddGold(InTreasure->GetGold());
		UIOverlay->SetGold(AttributeComponent->GetGold());
	}
}

void AActionCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bIsEquipped);
	DOREPLIFETIME(ThisClass, bIsAttacking);
}

void AActionCharacter::AddGold(int32 InGold)
{
	if(IsValid(AttributeComponent))
	{
		AttributeComponent->AddGold(InGold);
	}
}

int32 AActionCharacter::PlayAttackMontage()
{
	if(AttackMontageSections.Num() <= 0) return -1;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(IsValid(AnimInstance) && IsValid(AttackMontage) && AttackMontageSections.Num() > AttackComboCount)
	{
		AnimInstance->Montage_Play(AttackMontage);
		AnimInstance->Montage_JumpToSection(AttackMontageSections[AttackComboCount], AttackMontage);
		AttackComboCount += 1;
		bIsPressedAttack = false;
	}
	
	return AttackComboCount;
}

void AActionCharacter::SetItemInfos(const FAllItems& InAllItems)
{
	AActionPlayerState* ActionPlayerState = GetPlayerState<AActionPlayerState>();
	if(IsValid(ActionPlayerState))
	{
		ActionPlayerState->SetAllItems(InAllItems);
	}
}

void AActionCharacter::EquipWeapon(ASwordWeapon* OverlappingSword)
{
	OverlappingSword->Equip(GetMesh(), GRightHandSocket, this, this);
	OverlappingSword->SetOwner(this);
	OverlappingSword->SetInstigator(this);
	CurrentState = ECharacterState::ECS_Equipped;
	OverlappingItem = nullptr;
	EquippedWeapon = OverlappingSword;

	EquippedWeapon->GetWeaponBox()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquippedWeapon->IgnoreActors.Empty();
}


void AActionCharacter::AttachWeaponToBack()
{
	if(EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), GSpineSocket);
	}
}

void AActionCharacter::AttachWeaponToHand()
{
	if(EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), GRightHandSocket);
	}
}

void AActionCharacter::FinishEquipping()
{
	CurrentActionState = EActionState::EAS_None;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	SetActorEnableCollision(true);
}

bool AActionCharacter::CheckCanNextCombo()
{
	if(AttackComboCount < AttackMontageSections.Num() && bIsPressedAttack)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(IsValid(AnimInstance))
		{
			AnimInstance->Montage_JumpToSection(AttackMontageSections[AttackComboCount], AttackMontage);
			AttackComboCount += 1;
			bIsPressedAttack = false;
		}
		return true;
	}

	return false;
}

//단순히 블루프린트에서 이벤트 통지용 함수
void OnUpdateInventory()
{
	
}

void AActionCharacter::SpawnProjectile_Server_Implementation()
{
	UWorld* World = GetWorld();

	if(IsValid(GetGameInstance()))
	{
		UActorManagerSubsystem* ActorManagerSubsystem = GetGameInstance()->GetSubsystem<UActorManagerSubsystem>();
		if(IsValid(ActorManagerSubsystem))
		{
			TWeakObjectPtr<AActionAICharacter> ClosetEnemy = ActorManagerSubsystem->GetClosestEnemy(GetActorLocation());

			FVector SpawnLocation(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 50.f);
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.Instigator = this;
			AProjectileBase* ProjectileBase = World->SpawnActor<AProjectileBase>(ProjectileClass, SpawnLocation, GetActorRotation(), SpawnParameters);
			
			if(ClosetEnemy != nullptr && IsValid(ClosetEnemy.Get()))
			{
				ProjectileBase->SetTargetActor(ClosetEnemy.Get());
			}
		}
	}
}

bool AActionCharacter::SpawnProjectile_Server_Validate()
{
	return true;
}

// Called when the game starts or when spawned
void AActionCharacter::BeginPlay()
{
	Super::BeginPlay();

	if(const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if(UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	Tags.Add(GTag_Player);

	AActionPlayerState* ActionPlayerState = GetPlayerState<AActionPlayerState>();
	if(IsValid(ActionPlayerState))
	{
		if(!ActionPlayerState->OnLoadItemDataDelegate.IsAlreadyBound(this, &AActionCharacter::OnLoadInventoryItems))
		{
			ActionPlayerState->OnLoadItemDataDelegate.AddDynamic(this, &AActionCharacter::OnLoadInventoryItems);
		}
	}

	InitializeOverlay();
}

void AActionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(IsValid(AttributeComponent) && IsValid(UIOverlay))
	{
		AttributeComponent->RestoreStamina(DeltaSeconds);
		UIOverlay->SetStaminaPercent(AttributeComponent->GetStaminaPercent());
	}
}

void AActionCharacter::Move(const FInputActionValue& InValue)
{
	if(CurrentActionState != EActionState::EAS_None) return;
	
	FVector2d MovementVector = InValue.Get<FVector2d>();

	const FRotator ControlRotation = GetControlRotation();
	const FRotator YawRotation(0, ControlRotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.X);
	AddMovementInput(RightDirection, MovementVector.Y);
}

void AActionCharacter::Look(const FInputActionValue& InValue)
{
	FVector2d LookAxisVector = InValue.Get<FVector2d>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AActionCharacter::Equip(const FInputActionValue& InValue)
{
	ASwordWeapon* OverlappingSword = Cast<ASwordWeapon>(OverlappingItem);
	
	if(IsValid(OverlappingSword))
	{
		if(HasAuthority())
		{
			bIsEquipped = true;
			SetEquipped();
		}
		else
		{
			Server_SetEquipped();
		}
	}
	else
	{
		if(CanDisarm())
		{
			Disarm();
		}
		else if(CanArm())
		{
			Arm();
		}
	}
}

void AActionCharacter::Attack(const FInputActionValue& InValue)
{
	if(!IsAlive()) return;
	Super::Attack(InValue);
	
	if(HasAuthority())
	{
		bIsAttacking = true;
		Multicast_AttackCombo();
	}
	else
	{
		Server_RequestAttack();
	}
	AttackProcess();
}

void AActionCharacter::AttackProcess()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(!IsValid(AnimInstance)) return;
	
	if(CanAttack() && AttackComboCount == 0)
	{
		PlayAttackMontage();
		FOnMontageEnded OnMontageEndedDelegate;
		OnMontageEndedDelegate.BindUObject(this, &AActionCharacter::FinishAttack);
		AnimInstance->Montage_SetEndDelegate(OnMontageEndedDelegate);
		CurrentActionState = EActionState::EAS_Attacking;
	}
}

void AActionCharacter::Jump()
{
	if(!IsAlive()) return;
	//Super::Jump();

	Dodge();
}

void AActionCharacter::Inventory(const FInputActionValue& InValue)
{
	if(!IsValid(InventoryComponent)) return;

	InventoryComponent->ActiveInventoryUI();
}

void AActionCharacter::LookClosetEnemy(const FInputActionValue& InputActionValue)
{
	UActionGameInstance* ActionGameInstance = GetGameInstance<UActionGameInstance>();
	if(IsValid(ActionGameInstance))
	{
		UActorManagerSubsystem* ActorManagerSubsystem = ActionGameInstance->GetSubsystem<UActorManagerSubsystem>();
		if(IsValid(ActorManagerSubsystem))
		{
			TWeakObjectPtr<AActionAICharacter> ClosetEnemy = ActorManagerSubsystem->GetClosestEnemy(GetActorLocation());
			if(ClosetEnemy.IsValid())
			{
				FVector LookAtDirection = ClosetEnemy->GetActorLocation() - GetActorLocation();
				LookAtDirection.Z = 0.f;
				FRotator LookAtRotation = UKismetMathLibrary::MakeRotFromX(LookAtDirection);
				SetActorRotation(LookAtRotation);
			}
		}
	}
}

void AActionCharacter::SkillProjectile(const FInputActionValue& InValue)
{
	if(!IsAlive()) return;

	SpawnProjectile_Server();
}

void AActionCharacter::Dodge()
{
	if(IsValid(AttributeComponent))
	{
		if(AttributeComponent->GetStaminaValue() >= 20)
		{
			AttributeComponent->UseStamina(20);
			GetCharacterMovement()->MaxWalkSpeed = 700.f;
			//SetActorEnableCollision(false);
			CurrentActionState = EActionState::EAS_Dodging;
			PlayDodgeMontage();
		}
		else
		{
			//TODO: 스태미나 부족 메시지 출력
			UKismetSystemLibrary::PrintString(GetWorld(), "Stamina is not enough");
		}
	}
}

void AActionCharacter::PlayEquipMontage(FName SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(IsValid(AnimInstance) && IsValid(EquipMontage))
	{
		AnimInstance->Montage_Play(EquipMontage);
		AnimInstance->Montage_JumpToSection(SectionName, EquipMontage);
	}
}

bool AActionCharacter::CanAttack()
{
	Super::CanAttack();
	return CurrentActionState == EActionState::EAS_None &&
		CurrentState != ECharacterState::ECS_UnEquipped;
}

bool AActionCharacter::CanDisarm() const
{
	return CurrentActionState == EActionState::EAS_None &&
		CurrentState != ECharacterState::ECS_UnEquipped &&
			EquippedWeapon;
}

bool AActionCharacter::CanArm() const
{
	return CurrentActionState == EActionState::EAS_None &&
		CurrentState == ECharacterState::ECS_UnEquipped &&
			EquippedWeapon;
}

void AActionCharacter::Arm()
{
	PlayEquipMontage(GEquipSection);
	CurrentState = ECharacterState::ECS_Equipped;
	CurrentActionState = EActionState::EAS_Equipping;
}

void AActionCharacter::Disarm()
{
	PlayEquipMontage(GUnEquipSection);
	CurrentState = ECharacterState::ECS_UnEquipped;
	CurrentActionState = EActionState::EAS_Equipping;
}

void AActionCharacter::Die()
{
	Super::Die();
	CurrentState = ECharacterState::ECS_Dead;
}

void AActionCharacter::InitializeOverlay()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if(IsValid(PlayerController))
	{
		ABaseHUD* BaseHUD = Cast<ABaseHUD>(PlayerController->GetHUD());
		if(IsValid(BaseHUD))
		{
			UIOverlay = BaseHUD->GetOverlayWidget();
			
			if(IsValid(UIOverlay) && IsValid(AttributeComponent))
			{
				UIOverlay->SetHealthPercent(AttributeComponent->GetHealthPercent());
				UIOverlay->SetStaminaPercent(1.f);
				UIOverlay->SetGold(0);
			}
		}
	}
}

void AActionCharacter::SetHUDHealth()
{
	if(IsValid(UIOverlay) && IsValid(AttributeComponent))
	{
		UIOverlay->SetHealthPercent(AttributeComponent->GetHealthPercent());
	}
}

void AActionCharacter::SetHealthBarValue()
{
	if(IsValid(InventoryComponent))
	{
		InventoryComponent->GetHealthBarWidget()->UpdateHealthBar();
	}
}

void AActionCharacter::FinishAttack(UAnimMontage* InAnimMontage, bool bInterrupted)
{
	AttackComboCount = 0;
	bIsPressedAttack = false;
	bIsAttacking = false;
}

void AActionCharacter::OnLoadInventoryItems(const FAllItems& InAllItems)
{
	AllItems = InAllItems;
	OnUpdateInventory();
}

void AActionCharacter::OnRep_Equipped()
{
	if(bIsEquipped)
	{
		SetEquipped();
	}
}

void AActionCharacter::OnRep_Attack()
{
	UKismetSystemLibrary::PrintString(GetWorld(), "OnRep_Attack");
	if(bIsAttacking)
	{
		if(AttackComboCount == 0)
		{
			AttackProcess();
		}
		else
		{
			CheckCanNextCombo();
		}
	}
}

void AActionCharacter::Server_SetEquipped_Implementation()
{
	bIsEquipped = true;
	
	SetEquipped();
}

void AActionCharacter::Server_RequestAttack_Implementation()
{
	bIsAttacking = true;
	bIsPressedAttack = true;
	Multicast_AttackCombo();
	AttackProcess();
}

void AActionCharacter::SetEquipped()
{
	if(IsValid(OverlappingItem))
	{
		ASwordWeapon* OverlappingSword = Cast<ASwordWeapon>(OverlappingItem);
		if(IsValid(OverlappingSword))
		{
			EquipWeapon(OverlappingSword);
		}
	}
}

void AActionCharacter::Multicast_AttackCombo_Implementation()
{
	bIsPressedAttack = true;
}




