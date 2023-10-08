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
			if (InstigatorPawn->HasAuthority()) // ����
			{
				if (InstigatorPawn->IsLocallyControlled()) // ���� - ȣ��Ʈ - ���� / Replicated�� �߻�ü ���, SSR X.
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

					SpawnedProjectile->bUseServerSideRewind = false;

					SpawnedProjectile->Damage = Damage;

					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else // Server, Not Locally Controlled - SSR(ServerSideRewind) ��� , Replicated�� �߻�ü ���X
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

					SpawnedProjectile->bUseServerSideRewind = true;
				}

			}
			else // Ŭ���̾�Ʈ , SSR O
			{
				if (InstigatorPawn->IsLocallyControlled()) // Replicated X , SSR O -> Local���� �߻�ü�� �ٷ� ����.
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
				}
				else // Ŭ���̾�Ʈ , ����X / ReplicatedX , SSR X
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else // SSR X
		{
			if (InstigatorPawn->HasAuthority()) // SSR�� ������� �����Ƿ� ��� ���� �÷��̾ Replicated�� �߻�ü�� ���.
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);

				SpawnedProjectile->bUseServerSideRewind = false;

				SpawnedProjectile->Damage = Damage;

				SpawnedProjectile->HeadShotDamage = HeadShotDamage;

			}

		}
	}
}