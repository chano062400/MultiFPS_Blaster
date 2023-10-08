// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
*
*/
UCLASS()
class BLASTER_API ABlasterController : public APlayerController
{
	GENERATED_BODY()
public:

	void SetHUDHealth(float Health, float MaxHealth);
	
	void SetHUDShield(float Shield, float MaxShield);

	void SetHUDScore(float ScoreAmount);

	void HideTeamScore();

	void InitTeamScore();

	void SetHUDRedTeamScore(int32 RedScore);

	void SetHUDBlueTeamScore(int32 BlueScore);

	void SetHUDDefeat(int32 Defeat);

	void SetHUDWeaponAmmo(int32 Ammo);

	void SetHUDCarriedAmmo(int32 CarriedAmmo);

	void SetHUDMatchCountdown(float CountdownTime);

	void SetHUDAnnouncementCountdown(float CountdownTime);

	void SetHUDGrenades(int32 Grenades);

	virtual void OnPossess(APawn* InPawn) override;

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetServerTime(); //����� �����ð� ��ȯ.

	virtual void ReceivedPlayer() override;

	void OnMatchStateSet(FName State, bool bTeamsMatch = false);

	void HandleMatchHasStarted(bool bTeamsMatch = false);

	void HandleCooldown();

	void HighPingWarning();

	void StopHighPingWarning();

	float SingleTripTime = 0.f;

	FHighPingDelegate HighPingDelegate;

	void BroadCastElim(APlayerState* Attacker, APlayerState* Victim);

	FString GetInfoText(TArray<class ABlasterPlayerState*>& Players);

	FString GetTeamsInfoText(class ABlasterGameState* BlasterGameState);

protected:

	virtual void BeginPlay() override;

	void SetHUDTime();

	void PollInit();

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Client, Reliable)
		void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceiveClientRequest);

	float ClientServerDelta = 0.f; //Ŭ���̾�Ʈ�� ������ �ð� ����.

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
		void ClientJoinMidGame(FName StateMatch, float Warmup, float Match, float StartingTime, float Cooldown);

	void CheckPing(float DeltaTime);

	virtual void SetupInputComponent() override;

	void ShowReturnToMainMenu();

	UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Input)
		class UInputMappingContext* CharacterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
		class UInputAction* QuitAction;

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScore)
		bool bShowTeamScore = false;

	UFUNCTION()
	void OnRep_ShowTeamScore();

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	bool bReturnToMainMenuOpen = false; 

	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY()
		class ABlasterGameMode* BlasterGameMode;

	float LevelStartingTime = 0.f;

	float MatchTime = 0.f;

	float WarmupTime = 0.f;

	float CooldownTime = 0.f;

	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;


	float HUDHealth;

	bool bInitializeHealth = false;

	float HUDMaxHealth;
	
	float HUDShield;

	bool bInitializeShield = false;

	float HUDMaxShield;

	float HUDScore;
	
	bool bInitializeScore = false;

	int32 HUDDefeats;
	
	bool bInitializeDefeats = false;

	float HUDCarriedAmmo;

	bool bInitializeCarriedAmmo = false;

	float HUDWeaponAmmo;

	bool bInitializeWeaponAmmo = false;

	int32 HUDGrenades;

	bool bInitializeGrenades = false;

	float HighPingRunningTime = 0.f; 

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	UPROPERTY(EditAnywhere)
	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrquency = 20.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	UPROPERTY(EditAnywhere)
	float HighPingThresHold = 50.f;
};