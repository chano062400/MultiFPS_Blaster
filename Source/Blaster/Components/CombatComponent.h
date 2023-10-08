// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

	
public:	
	UCombatComponent();

	friend class ABlasterCharacter;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void FInishSwapWeapon();

	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapon();

	void FirePressed(bool bPressed);

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
		void LaunchGrenade();

	UFUNCTION(Server, Reliable)
		void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);


protected:
	virtual void BeginPlay() override;

	void EquipWeapon(class AWeapon* Weapon);

	void SwapWeapon();

	void ReloadEmptyWeapon();

	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);

	void UpdateCarriedAmmo();

	void AttachActorToRightHand(AActor* ActorToAttach);

	void AttachActorToLeftHand(AActor* ActorToAttach);

	void AttachFlagToLeftHand(AWeapon* Flag);

	void AttachActorToBackPack(AActor* ActorToAttach);

	void DropEquippedWeapon();

	void Reload();

	void ShowAttachedGrenade(bool bShowGrenade);

	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);

	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)  //RPC
	void ServerAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
		void OnRep_SecondaryWeapon();

	void Fire();

	void FireProjectileWeapon();

	void FireHitScanWeapon();

	void FireShotgun();

	void LocalFire(const FVector_NetQuantize& TraceHitTarget);

	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(Server, Reliable, WithValidation) // 클라이언트에서 호출돼서 서버에서 실행됨.
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);

	UFUNCTION(NetMulticast, Reliable) // 서버에서 호출되고 서버,클라이언트 모두 실행됨 
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
		void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	void TraceCrossHair(FHitResult& TraceHitResult);

	void SetHUDCrossHairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();

	int32 AmountToReload();

	bool bLocallyReloading = false;

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterController* Controller;

	UPROPERTY()
	class ABlasterHUD* HUD;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;

	bool bAimPressed = false;
		
	UFUNCTION()
	void OnRep_Aiming();

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimingWalkSpeed;

	bool bFirePressed;

	float CrossHairVelocityFactor;
	float CrossHairInAirFactor;
	float CrossHairAimFactor;
	float CrossHairShootFactor;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category= Combat)
		float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere)
		float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	FTimerHandle FireTimer;

	
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	//캐릭터가 가지고 있는 현재 장착한 무기의 탄약 수. / Weapon-WeaponAmmo - 탄창 하나에 있는 탄약 수.
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
		int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UPROPERTY(EditAnywhere)
	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
		int32 MaxCarriedAmmo = 500;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 100;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 10;

	UPROPERTY(EditAnywhere)
		int32 StartingPistolAmmo = 30;

	UPROPERTY(EditAnywhere)
		int32 StartingSMGAmmo = 50;

	UPROPERTY(EditAnywhere)
		int32 StartingShotgunAmmo = 15;

	UPROPERTY(EditAnywhere)
		int32 StartingSniperAmmo = 20;

	UPROPERTY(EditAnywhere)
		int32 StartingGrenadeLauncherAmmo = 10;
 
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState, VisibleAnywhere)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

	void UpdateShotgunAmmoValues();

	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;

	UFUNCTION()
	void OnRep_Grenades();

	UPROPERTY(EditAnywhere)
		int32 MaxGrenades = 4;

	void UpdateHUDGrenades();

	UPROPERTY(ReplicatedUsing = OnRep_bHoldingTheFlag, VisibleAnywhere)
	bool bHoldingTheFlag = false;

	UFUNCTION()
	void OnRep_bHoldingTheFlag();

	UPROPERTY()
	class AFlag* TheFlag;

public:

	FORCEINLINE int32 GetGrenades()  const {return Grenades; }

	bool ShouldSwapWeapon();
};
