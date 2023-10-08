

#include "PickupSpawnPoint.h"
#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	
	StartSpawnPickupTimer((AActor*)nullptr);
}

void APickupSpawnPoint::SpawnPickup()
{
	int PickupClassesNum = PickupClasses.Num();
	if (PickupClassesNum > 0)
	{
		int Rand = FMath::RandRange(0, PickupClassesNum - 1);

		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Rand], GetActorTransform());

		if (HasAuthority() && SpawnedPickup)
		{
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}

}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority()) 
	{
		SpawnPickup();
	}

}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnTimeMin, SpawnTimeMax);

	GetWorldTimerManager().SetTimer(SpawnPickupTimer, [this] {APickupSpawnPoint::SpawnPickupTimerFinished(); }, SpawnTime, false);

}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

