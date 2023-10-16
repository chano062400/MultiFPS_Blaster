// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletShells.generated.h"

class USoundBase;

UCLASS()
class BLASTER_API ABulletShells : public AActor
{
	GENERATED_BODY()
	
public:	

	ABulletShells();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BulletMesh;

	UPROPERTY(EditAnywhere)
	float BulletEjectImpulse;

	UPROPERTY(EditAnywhere)
	USoundBase* BulletSound;

	 FTimerHandle BulletDestroyTimer;
};
