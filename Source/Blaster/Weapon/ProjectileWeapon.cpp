// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h" 

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();

	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority()) // 서버
			{
				if (InstigatorPawn->IsLocallyControlled()) // 서버 - 호스트 - 유저 / Replicated된 발사체 사용, SSR X.
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

					SpawnedProjectile->bUseServerSideRewind = false;

					SpawnedProjectile->Damage = Damage;

					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else // Server, Not Locally Controlled - SSR(ServerSideRewind) 사용 , Replicated된 발사체 사용X
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

					SpawnedProjectile->bUseServerSideRewind = true;
				}

			}
			else // 클라이언트 , SSR O
			{
				if (InstigatorPawn->IsLocallyControlled()) // Replicated X , SSR O -> Local에선 발사체가 바로 보임.
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				}
				else // 클라이언트 , 로컬X / ReplicatedX , SSR X
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else // SSR X
		{
			if (InstigatorPawn->HasAuthority()) // SSR을 사용하지 않으므로 모든 서버 플레이어가 Replicated된 발사체를 사용.
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

				SpawnedProjectile->bUseServerSideRewind = false;

				SpawnedProjectile->Damage = Damage;

				SpawnedProjectile->HeadShotDamage = HeadShotDamage;

			}

		}
	}
}