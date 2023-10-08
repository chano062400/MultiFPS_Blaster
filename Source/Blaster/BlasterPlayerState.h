// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterTypes/Team.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	virtual void OnRep_Score() override; // 부모클래스에서 UFUNCTION()

	UFUNCTION()
	virtual void OnRep_Defeat();

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	void AddToScore(float ScoreAmount);

	void AddToDefeat(int32 DefeatAmount);
private:
	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY()
	class ABlasterController* BlasterController;

	UPROPERTY(ReplicatedUsing = OnRep_Defeat)
	int32 Defeat;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION()
		void OnRep_Team();

public:

	FORCEINLINE ETeam GetTeam() { return Team; }
	void SetTeam(ETeam TeamToSet);
};
