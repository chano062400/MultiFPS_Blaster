// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "Blaster/BlasterCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterTypes/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());

	if (BlasterCharacter)
	{
		BlasterCharacterMovement = BlasterCharacter->GetCharacterMovement();
	}
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacterMovement)
	{
		Speed = UKismetMathLibrary::VSizeXY(BlasterCharacterMovement->Velocity);
		IsFalling = BlasterCharacterMovement->IsFalling();
		IsAccelerating = BlasterCharacterMovement->GetCurrentAcceleration().Size() > 0.f ? true : false;
		bWeaponEquipped = BlasterCharacter->IsWeaponEquipped() ? true : false;
		EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
		bIsCrouched = BlasterCharacter->bIsCrouched;
		bAiming = BlasterCharacter->IsAiming(); 
		TurningInPlace = BlasterCharacter->GetTurningInPlace();
		bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
		bElimed = BlasterCharacter->IsElimed();
		bHoldingTheFlag = BlasterCharacter->IsHoldingTheFlag();
		

		//YawOffset
		FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
		FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
		DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 15.f);
		YawOffset = DeltaRotation.Yaw;

		//Lean
		CharacterRotationLastFrame = CharacterRotation;
		CharacterRotation = BlasterCharacter->GetActorRotation();
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
		const float Target = Delta.Yaw / DeltaTime;
		const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
		Lean = FMath::Clamp(Interp, -90.f, 90.f);

		AO_Yaw = BlasterCharacter->GetAO_Yaw();
		AO_Pitch = BlasterCharacter->GetAO_Pitch();

		if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh()) // 왼손의 위치를 LeftHandSocket 소켓의 위치로 바꿈.(소켓의 위치는 hand_r 본의 위치를 기준으로 바뀜)
		{
			LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
			FVector OutPosition;
			FRotator OutRotation;
			BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
			LeftHandTransform.SetLocation(OutPosition);
			LeftHandTransform.SetRotation(FQuat(OutRotation));

			if (BlasterCharacter->IsLocallyControlled())
			{
				bLocallyControlled = true;
				FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));//기본 X축 방향이 반대라서;
				RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
			}
			

			//총구 방향과 발사 방향 오차 비교
			FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
			FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
			DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);

		}

		bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
		bool bFABRIKOverride = BlasterCharacter->IsLocallyControlled() &&
			BlasterCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade && 
			BlasterCharacter->bFinishedSwappingWeapon;

		if (bFABRIKOverride)
		{
			bUseFABRIK = !BlasterCharacter->IsLocallyReloading();
		}

		bUseAimOffset = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->bDisableGameplay;
		bTransformRightHand = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->bDisableGameplay;

	}
} 
