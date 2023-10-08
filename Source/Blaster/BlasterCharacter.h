// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/BlasterTypes/Team.h"
#include "BlasterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter ,public IInteractInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;

	void RotateInPlace(float DeltaTime);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override; 

	void PlayFireMontage(bool bAiming);

	void PlayHitReactMontage();

	void PlayElimMontage();

	void PlayReloadMontage();

	void PlayThrowGrenadeMontage();

	void PlaySwapWeaponMontage();

	virtual void OnRep_ReplicatedMovement() override;

	void DropOrDestroyWeapons();

	void Elim(bool bPlayerLeftGame);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();

	void UpdateHUDShield();

	void UpdateHUDAmmo();

	void SpawnDefaultWeapon();

	UPROPERTY()
		TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	bool bFinishedSwappingWeapon = false;

	UFUNCTION(Server, Reliable)
		void ServerLeaveGame();

	FOnLeftGame OnLeftGame;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();

	void SetTeamColor(ETeam Team);

	void SetSpawnPoint();

	void OnPlayerStateInitialized();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Input)
		class 	UInputMappingContext* CharacterMappingContext;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Input)
	class	UInputAction* MoveAction;

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Input)
		UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* EquipAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* AimReleaseAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* FireReleaseAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadonly, Category = Input)
		UInputAction* ThrowGrenadeAction;

	virtual void Move(const FInputActionValue& Value);

	virtual void Look(const FInputActionValue& Value);

	virtual void Jump(const FInputActionValue& Value) ;

	virtual void Equip(const FInputActionValue& Value);
	
	UFUNCTION(Server, Reliable)
		void ServerEquip();

	virtual void CrouchPressed();
	
	virtual void AimPressed();

	virtual void AimReleased();

	virtual void AimOffset(float DeltaTime);

	void CalculateAO_Pitch();

	virtual void FirePressed();
	 
	virtual void FireReleased();

	virtual void Reload();

	virtual void ThrowGrenadePressed();

	virtual void DropOrDestroyWeapon(AWeapon* Weapon);

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	void SimProxiesTurn();

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;
	
	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 100.f;
	
	UFUNCTION()
	void OnRep_Health(float LastHealth);
	
	UFUNCTION()
	void OnRep_Shield(float LastShield);

	void PollInit();

	/* Hit Boxes (Server Side Rewind)*/

	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;

	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;

	

private:

	UPROPERTY(EditAnywhere)
		UStaticMeshComponent* Grenade;

	UPROPERTY(VisibleAnywhere, Category = Camera)
		class USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, Category = Camera)
		class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverHeadWidget;

	UPROPERTY(ReplicatedUsing =  OnRep_OverlappiongWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappiongWeapon(AWeapon* PrevWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowprivateAccess = "true"))
		class UCombatComponent* Combat;
	
	UPROPERTY(VisibleAnywhere)
		class UBuffComponent* BuffComp;
	
	UPROPERTY(VisibleAnywhere)
		class ULagCompensationComponent* LagCompensation;

	float AO_Yaw;
	float AO_Pitch;
	float InterpAO_Yaw;
	FRotator StartAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "Combat")
		class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	 UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	 UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	 UAnimMontage* ReloadMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	 UAnimMontage* ThrowGrenadeMontage;
		
	UPROPERTY(EditAnywhere, Category = "Combat")
	 UAnimMontage* SwapWeaponMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThresHold = 200.f;

	bool bRotateRootBone;

	float TurnThresHold = 0.5f;
	
	FRotator ProxyRotationLastFrame;
	
	FRotator ProxyRotation;
	
	float ProxyYaw;
	
	float TimeSinceLastMovementReplication;
	
	float CalculateSpeed();

	UPROPERTY()
	class ABlasterController* BlasterController;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	bool bElimed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	bool bLeftGame = false;

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue); 

	void StartDissolve();

	UPROPERTY(VisibleAnywhere ,Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
		UMaterialInstance* RedDissolveMatInst;
	
	UPROPERTY(EditAnywhere, Category = Elim)
		UMaterialInstance* BlueDissolveMatInst;
	
	UPROPERTY(EditAnywhere, Category = Elim)
		UMaterialInstance* RedMaterial;
	
	UPROPERTY(EditAnywhere, Category = Elim)
		UMaterialInstance* BlueMaterial;
		
	UPROPERTY(EditAnywhere, Category = Elim)
		UMaterialInstance* OringinalMaterial;

	UPROPERTY(EditAnywhere, Category = Elim)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere, Category = Elim)
	class USoundCue* ElimBotSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;

	UPROPERTY()
	class UNiagaraComponent* CrownComponent;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	bool bInputsSet = false;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	
	bool IsWeaponEquipped();
	
	bool IsAiming();
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera; }
	
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	
	FORCEINLINE bool IsElimed() const { return bElimed; }
	
	FORCEINLINE float GetHealth() const { return Health; }

	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }

	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	FORCEINLINE float GetShield() const { return Shield; }

	FORCEINLINE float GetMaxShield() const { return MaxShield; }

	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }

	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }

	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return Grenade; }

	FORCEINLINE UBuffComponent* GetBuffComp() const { return BuffComp; }

	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }

	FORCEINLINE bool IsHoldingTheFlag() const;

	bool IsLocallyReloading();

	ECombatState GetCombatState() const;
	
	AWeapon* GetEquippedWeapon();
	
	FVector GetHitTarget() const;

	ETeam GetTeam();

	void SetHoldingFlag(bool bHolding);
};
