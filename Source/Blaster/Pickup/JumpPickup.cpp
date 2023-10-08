#include "JumpPickup.h"
#include "Blaster/BlasterCharacter.h"
#include "Blaster/Components/BuffComponent.h"


void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABlasterCharacter* Character = Cast<ABlasterCharacter>(OtherActor);
	if (Character)
	{
		UBuffComponent* BuffComp = Character->GetBuffComp();
		if (BuffComp)
		{
			BuffComp->BuffJump(JumpBuff, JumpBuffTime);
		}
	}

	Destroy();
}
