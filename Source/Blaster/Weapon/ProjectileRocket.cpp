// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->InitialSpeed = InitialSpeed;
	RocketMovementComponent->MaxSpeed = InitialSpeed;
}

void AProjectileRocket::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	if (Event.GetPropertyName() == TEXT("InitialSpeed"))
	{
		RocketMovementComponent->InitialSpeed = InitialSpeed;
		RocketMovementComponent->MaxSpeed = InitialSpeed;
	}
}

void AProjectileRocket::Destroyed()
{

}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) // Ŭ���̾�Ʈ������ �ǰ�ȿ��.
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	SpawnTrailSystem();
	
	if (ProjectileLoopSound && LoopingSoundAttenuation)
	{
		ProjectileLoopSoundComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoopSound,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*) nullptr,
			false
		);
	}

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true; // Ư�� �浹 ä���� ���� ����.
	PathParams.bTraceWithCollision = true;
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PathParams.LaunchVelocity = GetActorForwardVector() * InitialSpeed;
	PathParams.MaxSimTime = 4.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.SimFrequency = 30.f; //frequency�� �������� ����� ��Ȯ����.
	PathParams.StartLocation = GetActorLocation();
	PathParams.TraceChannel = ECollisionChannel::ECC_Visibility;
	PathParams.ActorsToIgnore.Add(this);

	FPredictProjectilePathResult PathResult;

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	if (OtherActor == GetOwner()) //���ڸ��� �������� ĳ���Ͱ� �¾��� ��.
	{
		return;
	}

	ExplodeDamage();

	StartDestroyTimer();

	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}

	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}

	if (ProjectileLoopSoundComponent && ProjectileLoopSoundComponent->IsPlaying())
	{
		ProjectileLoopSoundComponent->Stop();
	}
}

