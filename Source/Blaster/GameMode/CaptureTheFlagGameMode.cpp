#include "CaptureTheFlagGameMode.h"
#include "BlasterGameMode.h"
#include "Blaster/Weapon/Flag.h"
#include "Blaster/CaptureTheFlag/FlagZone.h"
#include "Blaster/GameState/BlasterGameState.h"

void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterController* VictimController, ABlasterController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);


}

void ACaptureTheFlagGameMode::FlagCaptured(AFlag* Flag, AFlagZone* FlagZone)
{
	bool bValidCaputre = Flag->GetTeam() != FlagZone->Team;
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState);

	if (BlasterGameState)
	{
		if(FlagZone->Team == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
		}
		
		if (FlagZone->Team == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
		}
	}
}
