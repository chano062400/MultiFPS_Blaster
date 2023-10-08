
#include "FlagZone.h"
#include "Components/SphereComponent.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/GameMode/CaptureTheFlagGameMode.h"
#include "Blaster/BlasterCharacter.h"

AFlagZone::AFlagZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Zone Sphere"));
	ZoneSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ZoneSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(ZoneSphere);
}

void AFlagZone::BeginPlay()
{
	Super::BeginPlay();
	
	ZoneSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	ZoneSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AFlagZone::OnSphereOverlap);
}

void AFlagZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFlag* OverlappingFlag = Cast<AFlag>(OtherActor);
	UE_LOG(LogTemp, Warning, TEXT("overlap!"));
	if (OverlappingFlag && OverlappingFlag->GetTeam() != Team)
	{
		ACaptureTheFlagGameMode* CTFGameMode = Cast<ACaptureTheFlagGameMode>(GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>());
		if (CTFGameMode)
		{
			CTFGameMode->FlagCaptured(OverlappingFlag, this);

			OverlappingFlag->ResetFlag();
		}
	}
}
