

#include "BulletShells.h"
#include "Kismet/GamePlayStatics.h"

ABulletShells::ABulletShells()
{
	PrimaryActorTick.bCanEverTick = false;

	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	SetRootComponent(BulletMesh);
	BulletMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	BulletMesh->SetSimulatePhysics(true);
	BulletMesh->SetEnableGravity(true);
	BulletMesh->SetNotifyRigidBodyCollision(true);
	BulletEjectImpulse = 5.f;
}

void ABulletShells::BeginPlay()
{
	Super::BeginPlay();
	
	BulletMesh->AddImpulse(GetActorForwardVector() * BulletEjectImpulse);

	BulletMesh->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
}

void ABulletShells::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (BulletSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BulletSound, GetActorLocation());
	}

	GetWorldTimerManager().SetTimer(BulletDestroyTimer, [this] {Destroy();}, 5.f, false);
	

}

