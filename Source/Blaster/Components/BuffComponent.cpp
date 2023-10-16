

#include "BuffComponent.h"
#include "Blaster/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}


void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	
	HealingRate = HealAmount / HealingTime; //�����Ӹ��� ġ���ؾ��ϴ� ��
	
	AmountToHeal += HealAmount; // �� ġ���ؾ��ϴ� ��
}

void UBuffComponent::RepleninshShield(float ReplenishAmount, float ReplenishingTime)
{
	bReplenishingShield = true;

	ReplenishRate = ReplenishAmount / ReplenishingTime;

	AmountToReplenish += ReplenishAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || Character == nullptr || Character->IsElimed()) return;

	const float HealThisFrame = HealingRate * DeltaTime;

	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));

	Character->UpdateHUDHealth(); // ������. Ŭ���̾�Ʈ�� OnRep_Health()�� ���� ������Ʈ ��.

	AmountToHeal -= HealThisFrame; //AmountToHeal�� 0�� �Ǹ� ���̻� ġ�� X

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	if (!bReplenishingShield || Character == nullptr || Character->IsElimed()) return;

	const float ReplenishThisFrame = ReplenishRate * DeltaTime;

	Character->SetShield(FMath::Clamp(Character->GetShield() + ReplenishThisFrame, 0.f, Character->GetMaxShield()));

	Character->UpdateHUDShield(); // ������. Ŭ���̾�Ʈ�� OnRep_Shield()�� ���� ������Ʈ ��.

	AmountToReplenish -= ReplenishThisFrame; //AmountToReplenish�� 0�� �Ǹ� ���̻� ġ�� X

	if (AmountToReplenish <= 0.f || Character->GetShield() >= Character->GetMaxShield())
	{
		bReplenishingShield = false;
		AmountToReplenish = 0.f;
	}
}

void UBuffComponent::BuffSpeed(float BaseBuffSpeed, float CrouchBuffSpeed, float BuffTime)
{
	if (Character == nullptr) return;
	
	Character->GetWorldTimerManager().SetTimer(SpeedBuffTimer, [this] {UBuffComponent::ResetSpeed();}, BuffTime, false );

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseBuffSpeed;

		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchBuffSpeed;
	}

	MulticastBuffSpeed(BaseBuffSpeed, CrouchBuffSpeed);
}

void UBuffComponent::SetInitialSpeed(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;

	InitialCrouchSpeed = CrouchSpeed;
}



void UBuffComponent::ResetSpeed()
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;

		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	}
	
	MulticastBuffSpeed(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::MulticastBuffSpeed_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;

		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

void UBuffComponent::SetInitialJump(float ZVelocity)
{
	InitialJump = ZVelocity;
}

void UBuffComponent::ResetJump()
{
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = InitialJump;
	}

	MulticastJumpBuff(InitialJump);
}

void UBuffComponent::BuffJump(float BuffJumpZVelocity, float BuffTime)
{
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr ) return;

	Character->GetWorldTimerManager().SetTimer(JumpBuffTimer, [this] {UBuffComponent::ResetJump(); }, BuffTime, false);

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpZVelocity;
	}

	MulticastJumpBuff(BuffJumpZVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float BuffJumpZVelocity)
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpZVelocity;
	}
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);

	ShieldRampUp(DeltaTime);
}

