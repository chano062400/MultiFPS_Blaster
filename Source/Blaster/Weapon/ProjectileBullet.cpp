// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GamePlayStatics.h"
#include "Blaster/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterController.h"
#include "Blaster/Components/LagCompensationComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());

	if (OwnerCharacter)
	{
		ABlasterController* OwnerController = Cast<ABlasterController>(OwnerCharacter->Controller);
		if (OwnerController)
		{

			const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;

			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); //Destroy가 마지막에 되도록.
				return;
			}
			
			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
			if(bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(HitCharacter, TraceStart, InitialVelocity, OwnerController->GetServerTime() - OwnerController->SingleTripTime);
			}
			
		}
	}
	

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); //Destroy가 마지막에 되도록.
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	//FPredictProjectilePathParams PathParams;
	//PathParams.bTraceWithChannel = true; // 특정 충돌 채널을 통해 추적.
	//PathParams.bTraceWithCollision = true; 
	//PathParams.DrawDebugTime = 5.f;
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	//PathParams.LaunchVelocity =GetActorForwardVector() * InitialSpeed;
	//PathParams.MaxSimTime = 4.f;
	//PathParams.ProjectileRadius = 5.f;
	//PathParams.SimFrequency = 30.f; //frequency가 높을수록 모양이 정확해짐.
	//PathParams.StartLocation = GetActorLocation();
	//PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	//PathParams.ActorsToIgnore.Add(this);

	//FPredictProjectilePathResult PathResult;

	//UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}
