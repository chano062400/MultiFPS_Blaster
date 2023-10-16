// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown; // MatchTime이 끝나고 승자를 표시하고, Cooldown타이머를 시작함.
}

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	ABlasterGameMode();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

	virtual void OnMatchStateSet() override;

	UPROPERTY(EditAnywhere)
	float CountdownTime = 0.f;

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter, class ABlasterController* VictimController, ABlasterController* AttackerController);

	virtual void RequestRespawn(class ACharacter* ElimedCharacter, AController* ElimedController);

	void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving);

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	UPROPERTY(EditDefaultsOnly)
		float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
		float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
		float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

	bool bTeamsMatch = false;

};
