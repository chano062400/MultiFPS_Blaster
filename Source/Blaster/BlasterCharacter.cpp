
#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "Components/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathlibrary.h"
#include "Blaster/Animation/BlasterAnimInstance.h"
#include "Blaster.h"
#include "Blaster/PlayerController/BlasterController.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Components/TimelineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blaster/BlasterPlayerState.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blaster/Components/BuffComponent.h"
#include "Components/BoxComponent.h"
#include "Components/LagCompensationComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/PlayerStart/TeamPlayerStart.h"

ABlasterCharacter::ABlasterCharacter()
{

	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->TargetArmLength = 600.f;
	SpringArm->bUsePawnControlRotation = true;

	Grenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade"));
	Grenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	Grenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraBoom"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw= false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
	OverHeadWidget->SetupAttachment(GetRootComponent());

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComp"));
	Combat->SetIsReplicated(true);

	BuffComp = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	BuffComp->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));


	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh); // 메쉬 콜리전 타입을 새로 만든 타입 SkeletaMesh로 설정함으로써 캡슐에 총알이 맞지 않도록 함.
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	/* Hit Boxes (Server Side Rewind) */

	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);

	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);


	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);


	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);


	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);


	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);


	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);


	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);


	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	backpack->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("backpack"), backpack);

	blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	blanket->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("blanket"), blanket);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	for (auto HitPairs : HitCollisionBoxes)
	{
		if (HitPairs.Value)
		{
			HitPairs.Value->SetCollisionObjectType(ECC_HitBox);
			HitPairs.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			HitPairs.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			HitPairs.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}



void ABlasterCharacter::OnRep_OverlappiongWeapon(AWeapon* PrevWeapon)
{
	if (OverlappingWeapon) // 지금 닿은 무기
	{ 
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if(PrevWeapon) // 전에 닿은 무기
		PrevWeapon->ShowPickupWidget(false);

}


void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((Camera->GetComponentLocation() - GetActorLocation()).Size() < CameraThresHold) // 카메라랑 캐릭터랑 너무 가까워지면(CameraThreshold보다 가까워지면)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true; //소유자한테 보이지 않음 (bOwnerNoSee가 true면)
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false; //소유자한테 보이지 않음 (bOwnerNoSee가 true면)
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}

}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = this->GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}



void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);

	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack); //타임라인을 재생하면 DissolveCurve를 사용하고 DissolveTrack에 바인드된 콜백을 호출.
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}
	}

	OverlappingWeapon = Weapon;

	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else if(!Weapon->bDestroyWeapon)
	{
		Weapon->Dropped();
	}
	
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}

		if (Combat->TheFlag)
		{
			Combat->TheFlag->Dropped();
		}
	}
}

void ABlasterCharacter::Elim(bool PlayerLeftGame)
{
	DropOrDestroyWeapons();

	MulticastElim(PlayerLeftGame);

}

void ABlasterCharacter::ElimTimerFinished()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (BlasterGameMode && !bLeftGame)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}

	
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterGameMode && BlasterPlayerState)
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;

	if (BlasterController)
	{
		BlasterController->SetHUDWeaponAmmo(0);
	}

	bElimed = true;
	PlayElimMontage();

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);

		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	GetCharacterMovement()->DisableMovement(); //움직임 멈춤
	GetCharacterMovement()->StopMovementImmediately(); // 캐릭터 회전 멈춤

	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	if (Combat)
	{
		Combat->FirePressed(false);	
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (ElimBotEffect)
	{
		FVector ElimBotSpawnLocation(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnLocation, GetActorRotation());
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
	}

	bool bHideSniperScope = IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::ETW_SniperRifle;
	
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}

	if (bHideSniperScope) 
	{
		ShowSniperScopeWidget(false);
	}
	GetWorldTimerManager().SetTimer(ElimTimer, [this] {ElimTimerFinished(); }, ElimDelay, false);
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	} 

	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress) 
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;

	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetCapsuleComponent(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 110.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false);
	}

	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void ABlasterCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr || OringinalMaterial == nullptr) return;

	switch (Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, OringinalMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		DissolveMaterialInstance = RedDissolveMatInst;
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	}
}

void ABlasterCharacter::SetSpawnPoint()
{
	if (HasAuthority() && BlasterPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<AActor*> PlayerStarts;
		
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts); // 특정 클래스(ATeamPlayerStart)를 가져와서 Tarray(PlayerStarts)를 반환.

		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			if (TeamStart && TeamStart->Team == BlasterPlayerState->GetTeam())
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}
		if (TeamPlayerStarts.Num() > 0)
		{
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(
				ChosenPlayerStart->GetActorLocation(),
				ChosenPlayerStart->GetActorRotation()
			);
		}
	}
}

void ABlasterCharacter::OnPlayerStateInitialized()
{
	BlasterPlayerState->AddToDefeat(0);
	BlasterPlayerState->AddToScore(0.f); // 게임이 시작되고 Score를 0으로 초기화.
	SetTeamColor(BlasterPlayerState->GetTeam());
	SetSpawnPoint();
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) 
		{
			Subsystem->AddMappingContext(CharacterMappingContext, 0);
			bInputsSet = true; //successful
		}
	}

	SpawnDefaultWeapon();

	UpdateHUDAmmo();

	UpdateHUDHealth();

	UpdateHUDShield();

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	if (Grenade)
	{
		Grenade->SetVisibility(false);
	}

	
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(Controller) : BlasterController;
	if (BlasterController)
	{
		BlasterController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(Controller) : BlasterController;
	if (BlasterController)
	{
		BlasterController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(Controller) : BlasterController;
	if (BlasterController && Combat && Combat->EquippedWeapon)
	{
		BlasterController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	UWorld* World = GetWorld();
	
	if (BlasterGameMode && World && !bElimed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);

		StartingWeapon->bDestroyWeapon = true;

		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector FowardDirecton = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(FowardDirecton, MovementVector.Y);

	const FVector RightDirectoin = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirectoin, MovementVector.X);
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisValue = Value.Get<FVector2D>();
	if (GetController())
	{
		AddControllerYawInput(LookAxisValue.X);
		AddControllerPitchInput(LookAxisValue.Y);
	}
}

void ABlasterCharacter::Jump(const FInputActionValue& Value)
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (GetController())
	{
		if (bIsCrouched)
		{
			UnCrouch();
		}
		else
			Super::Jump();

	}
}

void ABlasterCharacter::Equip(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;

	if (Combat)
	{
		if (Combat->bHoldingTheFlag) return;
		if(Combat->CombatState == ECombatState::ECS_Unoccupied) ServerEquip(); // 호출할때는 그냥, Server
		
		bool bSwap = Combat->ShouldSwapWeapon() &&
			!HasAuthority() &&
			Combat->CombatState == ECombatState::ECS_Unoccupied &&
			OverlappingWeapon == nullptr;
		
		if(bSwap)
		{
			PlaySwapWeaponMontage(); // Local Client
			Combat->CombatState = ECombatState::ECS_SwappingWeapon; // Local Client
			bFinishedSwappingWeapon = false;
		}
	}
}

void ABlasterCharacter::ServerEquip_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapon())
		{
			Combat->SwapWeapon(); 
		}
	}

}

void ABlasterCharacter::CrouchPressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay ) return;

	if (!bIsCrouched)
		Crouch();
	else
		UnCrouch();
}

void ABlasterCharacter::AimPressed()
{

	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(true);
	}

}

void ABlasterCharacter::AimReleased()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	float Speed = CalculateSpeed();
	bool IsFalling = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !IsFalling)
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 가만히 있는 상태에서 시점을 바꿀때 Yaw
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(StartAimRotation, CurrentAimRotation); //마지막으로 움직였을때 Yaw - 가만히있는 상태 Yaw 차이가 90 -90나면 고개를 돌리는 것.
		AO_Yaw = -1.f * DeltaAimRotation.Yaw; // 방향을 바꿔주기 위해 마이너스 붙임.
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || IsFalling)
	{
		bRotateRootBone = false;
		StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 움직일때 Yaw
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();

	
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch; // 이대로하면 언리얼에서 네트워킹할때 값을 압축하기 때문에(음수값을 양수값으로 바꿈) 오류가 생김 (마이너스로 안내려가고 360도로 감)
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// [270, 360) 를 [-90, 0)로 매핑
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch); // AO_Pitch값을 InRange값에서 OutRange값으로 Clamp해줌.
	}
	
	/* 간단하게 하려면
		AO_Pitch = GetBaseAimRotation().GetNormalized().Pitch;
	*/ 	
}

void ABlasterCharacter::FirePressed()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FirePressed(true);
	}
}

void ABlasterCharacter::FireReleased()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->FirePressed(false);
	}
}

void ABlasterCharacter::Reload()
{
	if (Combat && Combat->bHoldingTheFlag) return;
	if (bDisableGameplay) return;

	if (Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::ThrowGrenadePressed()
{


	if (Combat)
	{
		if (Combat->bHoldingTheFlag) return;
		Combat->ThrowGrenade();
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{

	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;

	if (bElimed || BlasterGameMode == nullptr) return;
	
	Damage = BlasterGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	float DamagedToHealth = Damage;

	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamagedToHealth = 0.f;
		}
		else
		{
			DamagedToHealth = FMath::Clamp(DamagedToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}

	Health = FMath::Clamp(Health - DamagedToHealth, 0.f, MaxHealth); // Health가 변경되면 Replicate될 것이므로 히트애니메이션을 재생.
	
	UpdateHUDHealth();

	UpdateHUDShield();
	
	PlayHitReactMontage(); // 서버용

	if (Health == 0.f)
	{
		if (BlasterGameMode)
		{
			BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(Controller) : BlasterController;
			ABlasterController* AttackerController = Cast<ABlasterController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterController, AttackerController);
		}
	}
}

void ABlasterCharacter::SimProxiesTurn() //NetRole이 ProxySimulated인 캐릭터에 적용.
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	bRotateRootBone = false; // 90도 움직일때마다 회전 애니메이션이 작동하지 않도록.

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	//Proxy용 제자리 회전 기능 
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	
	if (FMath::Abs(ProxyYaw) > TurnThresHold)
	{
		if (ProxyYaw > TurnThresHold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThresHold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::OnRep_Health(float LastHealth) //클라이언트용
{
	UpdateHUDHealth();

	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();

	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			OnPlayerStateInitialized();

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 6.f);	
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f); // 움직일때 Yaw
		}
	}
}



void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !bInputsSet && Controller)
	{
		BlasterController = !BlasterController ? Cast<ABlasterController>(Controller) : BlasterController;
		if (BlasterController)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(BlasterController->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(CharacterMappingContext, 0);
				bInputsSet = true; //successful
			}
		}
	}

	RotateInPlace(DeltaTime);

	HideCameraIfCharacterClose();

	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (Combat && Combat->bHoldingTheFlag)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (bDisableGameplay)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		bUseControllerRotationYaw = false;
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime; //TimeSince... 가 일정량에 도달하면 그 시간동안 복제를 하지 않았다는 뜻.
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Equip);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::AimPressed);
		EnhancedInputComponent->BindAction(AimReleaseAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::AimReleased);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::FirePressed);
		EnhancedInputComponent->BindAction(FireReleaseAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::FireReleased);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Reload);
		EnhancedInputComponent->BindAction(ThrowGrenadeAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::ThrowGrenadePressed);
	
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}

	if (BuffComp)
	{
		BuffComp->Character = this;
		
		BuffComp->SetInitialSpeed(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
		
		BuffComp->SetInitialJump(GetCharacterMovement()->JumpZVelocity);
	}

	if (LagCompensation)
	{
		LagCompensation->Character = this;
		
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterController>(Controller);
		}
		
	}
	
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* Instance = GetMesh()->GetAnimInstance();
	if (Instance && FireWeaponMontage)
	{
		Instance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		Instance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* Instance = GetMesh()->GetAnimInstance();
	if (Instance && HitReactMontage)
	{
		Instance->Montage_Play(HitReactMontage);
		FName SectionName(FName("FromFront"));
		Instance->Montage_JumpToSection(SectionName);

		FOnMontageEnded MontageEnd;
		MontageEnd.BindWeakLambda(this, [this](UAnimMontage* AnimMontage, bool bInterrupted)
		{
				if (bInterrupted)
				{
	
				}
				else
				{

				}

		});
		Instance->Montage_SetEndDelegate(MontageEnd, HitReactMontage);
	}


}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* Instance = GetMesh()->GetAnimInstance();
	if (Instance && ElimMontage)
	{
		Instance->Montage_Play(ElimMontage);
	}
	
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* Instance = GetMesh()->GetAnimInstance();
	if (Instance && ReloadMontage)
	{
		Instance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::ETW_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::ETW_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::ETW_SubMachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::ETW_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::ETW_SniperRifle:
			SectionName = FName("Sniper");
			break;
		case EWeaponType::ETW_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}

		Instance->Montage_JumpToSection(SectionName);

		FOnMontageEnded MontageEnd;
		MontageEnd.BindWeakLambda(this, [this](UAnimMontage* Animmontage, bool bInterrupted)
		{
			if (bInterrupted)
			{
				if (HasAuthority())
				{
					Combat->CombatState = ECombatState::ECS_Unoccupied;
				}
			}
			else
			{
				Combat->FinishReloading();
			}
		});
		Instance->Montage_SetEndDelegate(MontageEnd, ReloadMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwapWeaponMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapWeaponMontage)
	{
		AnimInstance->Montage_Play(SwapWeaponMontage);
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
	if (Combat == nullptr) return false;

	return Combat->bHoldingTheFlag;
}

bool ABlasterCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;

	return Combat->bLocallyReloading;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;

	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterPlayerState == nullptr) return ETeam::ET_NoTeam;
	return BlasterPlayerState->GetTeam();
}

void ABlasterCharacter::SetHoldingFlag(bool bHolding)
{
	if (Combat)
	{
		Combat->bHoldingTheFlag = bHolding;
	}
}
