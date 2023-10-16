
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UBuffComponent();
	
	friend class ABlasterCharacter;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


protected:
	virtual void BeginPlay() override;

	void HealRampUp(float DeltaTime);

	void ShieldRampUp(float DeltaTime);

private:	

	UPROPERTY()
		class ABlasterCharacter* Character;
		
	/* Heal Buff */


	bool bHealing = false;
	
	float HealingRate = 0.f;
	
	float AmountToHeal = 0.f;

	/* Speed Buff */

	FTimerHandle SpeedBuffTimer;

	float InitialBaseSpeed;
	
	float InitialCrouchSpeed;

	void ResetSpeed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBuffSpeed(float BaseSpeed, float CrouchSpeed);

	/* Jump Buff */

	FTimerHandle JumpBuffTimer;

	void ResetJump();

	UFUNCTION(NetMulticast, Reliable)
		void MulticastJumpBuff(float BuffJumpZVelocity);

	float InitialJump;

	/* Shield Buff */

	bool bReplenishingShield = false;

	float ReplenishRate = 0.f;

	float AmountToReplenish = 0.f;

public:

	void Heal(float HealAmount, float HealingTime);

	void RepleninshShield(float ReplenishAmount, float ReplenishingTime);

	void BuffSpeed(float BaseBuffSpeed, float CrouchBuffSpeed, float BuffTime);

	void SetInitialSpeed(float BaseSpeed, float CrouchSpeed);

	void SetInitialJump(float ZVelocity);

	void BuffJump(float BuffJumpZVelocity, float BuffTime);
};
