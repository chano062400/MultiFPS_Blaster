// Fill out your copyright notice in the Description page of Project Settings.


#include "ShieldPickup.h"
#include "Blaster/BlasterCharacter.h"
#include "Blaster/Components/BuffComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"


void AShieldPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* Character = Cast<ABlasterCharacter>(OtherActor);
	if (Character)
	{
		UBuffComponent* BuffComp = Character->GetBuffComp();
		if (BuffComp)
		{
			BuffComp->RepleninshShield(ShieldReplenishAmount, ShieldReplenishingTime);
		}
	}

	Destroy();
}
