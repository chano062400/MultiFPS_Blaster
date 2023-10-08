// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Blaster/Components/CombatComponent.h"
#include "Blaster/BlasterCharacter.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->GetCombat()->PickupAmmo(WeaponType, AmmoAmount);
	}

	Destroy();
}
