
#pragma once

#include "Kismet/GameplayStatics.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UParticleSystemComponent;
class UParticleSystem;
class USoundBase;
class UBoxComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	virtual void Destroyed() override;
	
	/* ServerSideRewind */

	UPROPERTY(EditAnywhere)
	bool bUseServerSideRewind = false;

	FVector_NetQuantize TraceStart;

	FVector_NetQuantize100 InitialVelocity; // 일반벡터보단 작지만 그냥 NetQuantize보단 정밀함.

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	// 수류탄, 로켓만 설정.
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	// 수류탄, 로켓X
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual void ExplodeDamage();

	void SpawnTrailSystem();

	void DestroyTimerFinished();

	void StartDestroyTimer();

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticle;

	UPROPERTY(EditAnywhere)
	USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;

	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;
	
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

public:	
	
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer; 
	 
	UPROPERTY()
	UParticleSystemComponent* TracerComponent;

};
