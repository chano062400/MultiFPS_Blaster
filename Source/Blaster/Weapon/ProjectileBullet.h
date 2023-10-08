// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()
	
public:
	AProjectileBullet();

#if WITH_EDITOR //조건부 컴파일. 런타임이 아닌 컴파일에 확인됨.
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& Event) override;
#endif

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void BeginPlay() override;
};
