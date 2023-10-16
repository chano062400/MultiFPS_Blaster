

#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "Kismet/GamePlayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Sound\SoundCue.h"
#include "Blaster/Animation/BlasterAnimInstance.h"
#include "Blaster/Weapon/Projectile.h"
#include "Blaster/Weapon/Shotgun.h"
#include "Blaster/Weapon/Flag.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimingWalkSpeed = 450.f;
	
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);//소유 클라이언트에게만 복제하게 함 -> 전송 데이터 최소화.
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
	DOREPLIFETIME(UCombatComponent, bHoldingTheFlag);
}

void UCombatComponent::EquipWeapon(AWeapon* Weapon) //EquipWeapon -> SetWeaponState -> OnRep_SetWeaponState
{
	if (Character == nullptr || Weapon == nullptr) return;
	
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (Weapon->GetWeaponType() == EWeaponType::ETW_Flag)
	{
		Character->Crouch();
		
		bHoldingTheFlag = true;

		Weapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
		AttachFlagToLeftHand(Weapon);
		
		Weapon->SetOwner(Character);

		TheFlag = Cast<AFlag>(Weapon);
	}
	else
	{
		if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
		{
			EquipSecondaryWeapon(Weapon);
		}
		else
		{
			EquipPrimaryWeapon(Weapon);
		}

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}

	
}

void UCombatComponent::SwapWeapon() // Server에서 실행.
{
	if (CombatState != ECombatState::ECS_Unoccupied || Character == nullptr) return;

	Character->PlaySwapWeaponMontage(); //Server
	Character->bFinishedSwappingWeapon = false;
	CombatState = ECombatState::ECS_SwappingWeapon; //Replicate -> OnRep_CombatState

	if (SecondaryWeapon) SecondaryWeapon->EnableCustomDepth(false);
	
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToRightHand(EquippedWeapon);

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetInstigator(Character);
	
	EquippedWeapon->SetHUDAmmo();

	UpdateCarriedAmmo();
	
	PlayEquipWeaponSound(WeaponToEquip);

	ReloadEmptyWeapon();


}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;

	SecondaryWeapon = WeaponToEquip;

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	
	AttachActorToBackPack(WeaponToEquip);

	PlayEquipWeaponSound(WeaponToEquip);
	
	SecondaryWeapon->SetOwner(Character);
	SecondaryWeapon->SetInstigator(Character);
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::PlayEquipWeaponSound(AWeapon* WeaponToEquip)
{
	if (Character && WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->EquipSound,
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType())) // 장착한 무기의 무기타입이 TMap에 있다면
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // CarriedAmmo가 업데이트 -> OnRep_CarriedAmmo
	}

	Controller = Controller == nullptr ? Cast<ABlasterController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr ||ActorToAttach == nullptr) return;

	const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (RightHandSocket)
	{
		RightHandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr ) return;

	bool bUsePistolSocket = EquippedWeapon->GetWeaponType() == EWeaponType::ETW_Pistol || EquippedWeapon->GetWeaponType() == EWeaponType::ETW_SubMachineGun;
	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	const USkeletalMeshSocket* LeftHandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (LeftHandSocket)
	{
		LeftHandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachFlagToLeftHand(AWeapon* Flag)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || Flag == nullptr) return;

	const USkeletalMeshSocket* FlagSocket = Character->GetMesh()->GetSocketByName(FName("FlagSocket"));

	if (FlagSocket)
	{
		FlagSocket->AttachActor(Flag, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackPack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;

	const USkeletalMeshSocket* BackPackSocket = Character->GetMesh()->GetSocketByName(FName("BackPackSocket"));
	
	if (BackPackSocket)
	{
		BackPackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		
		UpdateCarriedAmmo();
	}
	
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetCamera())
		{
			DefaultFOV = Character->GetCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority()) // 서버에서
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return; 

	bAiming = bIsAiming;
	ServerAiming(bIsAiming); // RPC
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimingWalkSpeed : BaseWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::ETW_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}

	if(Character->IsLocallyControlled()) bAimPressed = bIsAiming;
}

void UCombatComponent::ServerAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimingWalkSpeed : BaseWalkSpeed;
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::ETW_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		
		AttachActorToRightHand(EquippedWeapon);

		PlayEquipWeaponSound(EquippedWeapon);

		EquippedWeapon->SetHUDAmmo();

		EquippedWeapon->EnableCustomDepth(false);

		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

		AttachActorToBackPack(SecondaryWeapon);

		PlayEquipWeaponSound(SecondaryWeapon);
	}
}

void UCombatComponent::FirePressed(bool bPressed)
{
	bFirePressed = bPressed;

	if (bFirePressed)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0) return;

	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}
	if (Character && !Character->HasAuthority())
	{
		ServerThrowGrenade();
	}

	if (Character && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (Grenades == 0) return;

	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::TraceCrossHair(FHitResult& TraceHitResult)
{
	FVector2D ViewPortSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize);
	}

	FVector2D CrossHairLocation(ViewPortSize.X / 2.f , ViewPortSize.Y / 2.f);
	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrossHairLocation,
		CrossHairWorldPosition,
		CrossHairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrossHairWorldPosition;

		if (Character) //라인트레이스가 캐릭터보다 앞에서 이루어지도록 함.
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrossHairWorldDirection * (DistanceToCharacter + 100.f);
			/*DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red, false);*/
		}

		FVector End = Start + CrossHairWorldDirection * 80000.f;


		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (!TraceHitResult.bBlockingHit) // 조준하는 곳이 너무 멀리있으면 방향이 이상해지는걸 수정.
		{
			TraceHitResult.ImpactPoint = End;
		}

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractInterface>())//캐릭터가 인터페이스를 상속해서 캐릭터가 GetActor일 경우에만 빨간색으로 변함.)
		{
			HUDPackage.CrossHairColor = FColor::Red;
		}
		else
		{
			HUDPackage.CrossHairColor = FColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrossHairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;
	
	Controller = Controller == nullptr ? Cast<ABlasterController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrossHairsCenter = EquippedWeapon->CrossHairsCenter;
				HUDPackage.CrossHairsLeft = EquippedWeapon->CrossHairsLeft;
				HUDPackage.CrossHairsRight = EquippedWeapon->CrossHairsRight;
				HUDPackage.CrossHairsTop = EquippedWeapon->CrossHairsTop;
				HUDPackage.CrossHairsBottom = EquippedWeapon->CrossHairsBottom;
			}
			else
			{
				HUDPackage.CrossHairsCenter = nullptr;
				HUDPackage.CrossHairsLeft = nullptr;
				HUDPackage.CrossHairsRight = nullptr;
				HUDPackage.CrossHairsTop = nullptr;
				HUDPackage.CrossHairsBottom = nullptr;
			}

			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrossHairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());


			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrossHairInAirFactor = FMath::FInterpTo(CrossHairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrossHairInAirFactor = FMath::FInterpTo(CrossHairInAirFactor, 0.f , DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, 0.6f, DeltaTime, 30.f);
			}
			else
			{
				CrossHairAimFactor = FMath::FInterpTo(CrossHairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrossHairShootFactor = FMath::FInterpTo(CrossHairShootFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrossHairSpread = 0.5f + CrossHairVelocityFactor + CrossHairInAirFactor - CrossHairAimFactor + CrossHairShootFactor;
				
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}



void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull() && !bLocallyReloading)
	{
		ServerReload();

		HandleReload(); // 지연이 일어나지 않게 바로 리로드를 실행.

		bLocallyReloading = true;

	}
}


void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;

	bLocallyReloading = false;

	if (Character->HasAuthority())
	{
		UpdateAmmoValues();
		CombatState = ECombatState::ECS_Unoccupied;
	}

	if (bFirePressed)
	{
		Fire();
	}
	
}

void UCombatComponent::FInishSwapWeapon()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	if (Character) Character->bFinishedSwappingWeapon = true;
	if (SecondaryWeapon) SecondaryWeapon->EnableCustomDepth(true);
}

void UCombatComponent::FinishSwapAttachWeapon()
{
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	AttachActorToRightHand(EquippedWeapon);

	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipWeaponSound(EquippedWeapon);

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);

	AttachActorToBackPack(SecondaryWeapon);

}

void UCombatComponent::ServerReload_Implementation() //서버 CombatState
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	CombatState = ECombatState::ECS_Reloading;
	
	if(!Character->IsLocallyControlled()) HandleReload(); // Local에선 이미 리로드가 실행되고 있으므로 2번 실행하지 않기 위해.
	
}

void UCombatComponent::HandleReload()
{
	if (Character)
	{
		Character->PlayReloadMontage();
	}
	
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMaxAmmo() - EquippedWeapon->GetAmmo(); // 재장전 할 수 있는 수.

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried); // 재장전 해야하는 최소 수. 만약 재장전 할 수 있는 수보다 갖고 있는 탄약 수가 적다면 갖고 있는 모든 탄약을 재장전. 
		return FMath::Clamp(RoomInMag, 0, Least);
	}

	return 0;
}

void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimPressed;
	}
}

void UCombatComponent::OnRep_CombatState() // 클라이언트 CombatState - LocalX
{
	switch (CombatState)
	{
		case ECombatState::ECS_Reloading:
			if(Character && !Character->IsLocallyControlled()) HandleReload(); // Local에선 이미 리로드가 실행되고 있으므로 2번 실행하지 않기 위해.
			break;
		case ECombatState::ECS_Unoccupied:
			if (bFirePressed)
			{
				Fire();
			}
			break;
		case ECombatState::ECS_ThrowingGrenade:
			if (Character && !Character->IsLocallyControlled()) //조종하고 있는 캐릭터라면 두번 재생되기 때문에 제외.
			{
				Character->PlayThrowGrenadeMontage();
				AttachActorToLeftHand(EquippedWeapon);
				ShowAttachedGrenade(true);
			}
			break;
		case ECombatState::ECS_SwappingWeapon:
			if (Character && !Character->IsLocallyControlled())
			{
				Character->PlaySwapWeaponMontage();
			}
			break;
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	
	int32 ReloadAmount = AmountToReload();

	CombatState = ECombatState::ECS_Reloading;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // 재장전 한 탄약 수 만큼 갖고있는 탄약 수에서 뺌.
	}

	Controller = Controller == nullptr ? Cast<ABlasterController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo); // 서버 HUD 업데이트
	}

	EquippedWeapon->AddAmmo(ReloadAmount); // 재장전 한 수만큼 탄약을 더해줌.

}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()]; // 재장전 한 탄약 수 만큼 갖고있는 탄약 수에서 뺌.
	}

	Controller = Controller == nullptr ? Cast<ABlasterController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo); // 서버 HUD 업데이트
	}

	EquippedWeapon->AddAmmo(1); // 재장전 한 수만큼 탄약을 더해줌.
	
	bCanFire = true;
	
	if (EquippedWeapon->IsFull() && Character->GetReloadMontage() || CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}

}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<ABlasterController>(Character->Controller) : Controller;
	if (Controller)
	{	
		Controller->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::OnRep_bHoldingTheFlag()
{
	if (bHoldingTheFlag && Character && Character->IsLocallyControlled())
	{
		Character->Crouch();
	}

}

bool UCombatComponent::ShouldSwapWeapon()
{
	return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr);
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	
	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget); //HitTarget은 로컬로 제어하는 캐릭터에서만 유효.
	}
}


void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target) //FVector_NetQuantize - 트래픽절약을 위해 사용.
{
	if (Character && Character->HasAuthority() && GrenadeClass && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(GrenadeClass, StartingLocation, ToTarget.Rotation(), SpawnParams);
		}
	}
}


void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetCamera())
	{
		Character->GetCamera()->SetFieldOfView(CurrentFOV);
	}


}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(FireTimer, [this] {UCombatComponent::FireTimerFinished(); }, EquippedWeapon->FireDelay, false);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	
	bCanFire = true;
	if (bFirePressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	
	ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;

	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::ETW_Shotgun) return true;

	if (bLocallyReloading) return false;

	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::ETW_Shotgun &&
		CarriedAmmo == 0;

	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::ETW_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::ETW_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::ETW_SubMachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::ETW_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::ETW_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::ETW_GrenadeLauncher, StartingGrenadeLauncherAmmo);

}


void UCombatComponent::Fire()
{

	if (CanFire())
	{
		
		if (EquippedWeapon)
		{
			bCanFire = false;
			CrossHairShootFactor = 0.75f;

			switch (EquippedWeapon->FireType)
			{
			case EFireType::EFT_HitScan:
				FireHitScanWeapon();
				break;
			case EFireType::EFT_Projectile:
				FireProjectileWeapon();
				break;
			case EFireType::EFT_Shotgun:
				FireShotgun();
				break;
			}
		}
		StartFireTimer();
	} 
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character) 
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndScatter(HitTarget) : HitTarget;

		if(!Character->HasAuthority()) LocalFire(HitTarget); // 핑이 높아도 LocalController에선 지연이 안 일어나도록 따로 Local에서 처리. 피격은 지연이 일어남.

		ServerFire(HitTarget, EquippedWeapon->FireDelay);

	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character) // 서버와 클라이언트에서 산란위치를 같게 하기위해서, 발사하면서 산란이 일어나게 X -> 산란기능을 통해 HitTaret을 설정한 후 발사.
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndScatter(HitTarget) : HitTarget;

		if (!Character->HasAuthority()) LocalFire(HitTarget); // 핑이 높아도 LocalController에선 지연이 안 일어나도록 따로 Local에서 처리. 피격은 지연이 일어남.

		ServerFire(HitTarget, EquippedWeapon->FireDelay);

	}
}

void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	
	if (Shotgun && Character)
	{ 
		TArray<FVector_NetQuantize> HitTargets;

		Shotgun->ShotgunTraceEndScatter(HitTarget, HitTargets);

		if (!Character->HasAuthority()) ShotgunLocalFire(HitTargets); // 핑이 높아도 LocalController에선 지연이 안 일어나도록 따로 Local에서 처리. 피격은 지연이 일어남.

		ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);

	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;

	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);

	if (Shotgun == nullptr || Character == nullptr) return;

	if (Character && CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		bLocallyReloading = false;

		Character->PlayFireMontage(bAiming);
		
		Shotgun->FireShotgun(TraceHitTargets);

		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}

bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return; 
	
	LocalFire(TraceHitTarget);
}

void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}

bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;

	ShotgunLocalFire(TraceHitTargets);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceCrossHair(HitResult);
		HitTarget = HitResult.ImpactPoint;
		
		SetHUDCrossHairs(DeltaTime);
		InterpFOV(DeltaTime);

	}
}

