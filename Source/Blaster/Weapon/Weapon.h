// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Weapon.generated.h"

class UTexture2D;
class USoundCue;
class USphereComponent;
class UWidgetComponent;
class UAnimationAsset;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),


	EWS_MAX UMETA(DisplayName = "MAX"),
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "HitScan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_MAX UMETA(DisplayName = "Default MAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	

	AWeapon();

	virtual void Tick(float DeltaTime) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:

	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();

	virtual void OnEquipped();

	virtual void OnDropped();

	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void  OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void  OnEndSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
		void OnPingTooHigh(bool bPingTooHigh);

	UFUNCTION()
		void OnRep_WeaponState();

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
		float DistanceToSphere = 800.f; // 도착지점 구체까지 거리.

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
		float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere)
		float Damage = 20.f;

	UPROPERTY(EditAnywhere)
		float HeadShotDamage = 40.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	UPROPERTY()
		class ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY()
		class ABlasterController* BlasterOwnerController;

private:

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	void SpendRound();

	UPROPERTY(EditAnywhere, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	USphereComponent* WeaponSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category= "Weapon")
	UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABulletShells> BulletClass;

	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere)
	float Ammo;

	int32 Sequence = 0; //처리되지 않은 서버 요청의 수.(Ammo)

	UPROPERTY(EditAnywhere)
	float MaxAmmo;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere)
	ETeam Team;

public:

	void SetWeaponState(EWeaponState State);
	
	FORCEINLINE USphereComponent* GetWeaponSphere() const { return WeaponSphere; }
	
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	
	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

	FORCEINLINE int32 GetAmmo() const { return Ammo; }

	FORCEINLINE int32 GetMaxAmmo() const { return MaxAmmo; }

	FORCEINLINE float GetDamage() const { return Damage; }

	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }

	FORCEINLINE ETeam GetTeam() const { return Team; }
	
	virtual void OnRep_Owner() override;

	void SetHUDAmmo();

	void ShowPickupWidget(bool bShowWidget);

	virtual void Fire(const FVector& HitTarget);

	virtual void Dropped();

	void AddAmmo(int32 AmmoToAdd);

	void EnableCustomDepth(bool Enable);

	bool IsEmpty();

	bool IsFull();

	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFireType FireType;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	FVector TraceEndScatter(const FVector& HitTarget);

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrossHairsCenter;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrossHairsLeft;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrossHairsRight;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrossHairsTop;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrossHairsBottom;

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	USoundCue* EquipSound;


};
