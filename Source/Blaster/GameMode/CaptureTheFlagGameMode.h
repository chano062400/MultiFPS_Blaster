
#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CaptureTheFlagGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ACaptureTheFlagGameMode : public ATeamsGameMode
{
	GENERATED_BODY()
	
public:

	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter, class ABlasterController* VictimController, ABlasterController* AttackerController)override;

	void FlagCaptured(class AFlag* Flag, class AFlagZone* FlagZone);
};
