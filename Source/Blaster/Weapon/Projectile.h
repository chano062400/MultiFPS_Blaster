
#pragma once

#include "Kismet/GameplayStatics.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"


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

	// Only set this for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	// Doesn't matter for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
		class	UParticleSystem* ImpactParticle;

	UPROPERTY(EditAnywhere)
		class USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
		class UNiagaraSystem* TrailSystem;

	UPROPERTY()
		class UNiagaraComponent* TrailSystemComponent;

	void SpawnTrailSystem();

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
		float DestroyTime = 3.f;

	void DestroyTimerFinished();

	void StartDestroyTimer();

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* ProjectileMesh;

	virtual void ExplodeDamage();

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
	class UParticleSystemComponent* TracerComponent;

	


public:


};
