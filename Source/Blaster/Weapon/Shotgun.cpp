// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Components/LagCompensationComponent.h"
#include "Blaster/PlayerController/BlasterController.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets)
{
	AWeapon::Fire(FVector());

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()); // 발사 출발점.

		const FVector Start = SocketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> HitMap;
		TMap<ABlasterCharacter*, uint32> HeadShotHitMap;
		for (const FVector_NetQuantize& HitTarget : HitTargets)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter)
			{
				const bool bHeadShot = FireHit.BoneName.ToString() == FString("head");

				if (bHeadShot) // 헤드샷 
				{
					if (HeadShotHitMap.Contains(BlasterCharacter))
					{
						HeadShotHitMap[BlasterCharacter]++;
					}
					else
					{
						HeadShotHitMap.Emplace(BlasterCharacter, 1);
					}
				}
				else // 바디샷
				{
					if (HitMap.Contains(BlasterCharacter))
					{
						HitMap[BlasterCharacter]++;
					}
					else
					{
						HitMap.Emplace(BlasterCharacter, 1);
					}
				}
				
			}

			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()

				);
			}

			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					0.5f,
					FMath::RandRange(-0.5f, 0.5f)
				);
			}
		}

		TMap<ABlasterCharacter*, float> DamageMap;// 총 데미지 합을 넣을 Map
		TArray<ABlasterCharacter*> HitCharacters;
		for (auto HitPairs : HitMap) // 바디샷 데미지 계산
		{
			if (HitPairs.Key)
			{
				DamageMap.Emplace(HitPairs.Key, HitPairs.Value * Damage);

				HitCharacters.AddUnique(HitPairs.Key);
			}
		}

		for (auto HeadShotHitPairs : HeadShotHitMap) // 헤드샷 데미지 계산까지 총합
		{
			if (HeadShotHitPairs.Key)
			{
				if (DamageMap.Contains(HeadShotHitPairs.Key))
				{
					DamageMap[HeadShotHitPairs.Key] += HeadShotHitPairs.Value * HeadShotDamage;
				}
				else
				{
					DamageMap.Emplace(HeadShotHitPairs.Key, HeadShotHitPairs.Value * HeadShotDamage);	
				}
				HitCharacters.AddUnique(HeadShotHitPairs.Key);
			}
		}

		for (auto DamagePair : DamageMap)
		{
			if (DamagePair.Key && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled(); //둘중 하나라도 해당하면 서버가 데미지를 입어야 함.
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						DamagePair.Key, 
						DamagePair.Value, // 계산한 총 데미지 합
						InstigatorController,
						this,
						UDamageType::StaticClass()
					);
				}
			}
		}

		
		if (!HasAuthority() && bUseServerSideRewind)
		{
			BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
			BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterController>(InstigatorController) : BlasterOwnerController;

			if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetLagCompensation() && BlasterOwnerController && BlasterOwnerCharacter->IsLocallyControlled())
			{
				BlasterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
					HitCharacters,
					Start,
					HitTargets,
					BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime
				);
			}
		}

	}

}

void AShotgun::ShotgunTraceEndScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()); // 발사 출발점.

	const FVector TraceStart = SocketTransform.GetLocation();


	const FVector ToTargetNormailize = (HitTarget - TraceStart).GetSafeNormal(); //단위벡터로 만듦.
	const FVector SphereCenter = TraceStart + ToTargetNormailize * DistanceToSphere; //도착지점 구체 중심.
	
	

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SphereRadius); // 산탄
		const FVector EndLoc = SphereCenter + RandVec; // 산탄된 탄 위치.
		 FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);
	}
}
