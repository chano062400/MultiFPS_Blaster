// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "Blaster/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/PlayerController/BlasterController.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BlasterGameState)
	{
		for (auto PlayerState : BlasterGameState->PlayerArray)
		{
			ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState.Get());
			if(BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
				{
					BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	ABlasterPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ABlasterPlayerState>();
	ABlasterPlayerState* VictimPlayerState = Victim->GetPlayerState<ABlasterPlayerState>();

	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr) return BaseDamage;
	
	if (AttackerPlayerState == VictimPlayerState) return BaseDamage; // 자신 스스로를 공격.

	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam()) return 0.f;
	
	return BaseDamage;

}

void ATeamsGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterController* VictimController, ABlasterController* AttackerController)
{
	Super::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;

	if (BlasterGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BlasterGameState->BlueTeamScores();
		}

		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BlasterGameState->RedTeamScores();
		}
	}
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer) // 중간에 들어온 플레이어 팀 설정.
{
	Super::PostLogin(NewPlayer);

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BlasterGameState)
	{
		ABlasterPlayerState* BlasterPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BlasterGameState->BlueTeam.Num() >= BlasterGameState->RedTeam.Num())
			{
				BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
				BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting) // 플레이어가 나갈 경우 속해있던 팀에서 제거.
{
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BlasterPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>();
	if (BlasterPlayerState && BlasterGameState) 
	{
		if (BlasterGameState->RedTeam.Contains(BlasterPlayerState))
		{
			BlasterGameState->RedTeam.Remove(BlasterPlayerState);
		}
		
		if(BlasterGameState->BlueTeam.Contains(BlasterPlayerState))
		{
			BlasterGameState->BlueTeam.Remove(BlasterPlayerState);
		}
	}


}
