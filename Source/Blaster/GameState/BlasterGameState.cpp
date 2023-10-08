// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/BlasterPlayerState.h"
#include "Blaster/PlayerController/BlasterController.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasterGameState, RedTeamScore);
	DOREPLIFETIME(ABlasterGameState, BlueTeamScore);

}

void ABlasterGameState::UpdateTopScore(ABlasterPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer); //이미 있는 플레이어라면 또 추가 X -> AddUnique
	}
	else if(ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasterGameState::OnRep_RedTeamScore()
{
	ABlasterController* BlasterController = Cast<ABlasterController>(GetWorld()->GetFirstPlayerController());
	if (BlasterController)
	{
		BlasterController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::OnRep_BlueTeamScore()
{
	ABlasterController* BlasterController = Cast<ABlasterController>(GetWorld()->GetFirstPlayerController());
	if (BlasterController)
	{
		BlasterController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABlasterGameState::RedTeamScores()
{
	++RedTeamScore;

	ABlasterController* BlasterController = Cast<ABlasterController>(GetWorld()->GetFirstPlayerController());
	if (BlasterController)
	{
		BlasterController->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABlasterGameState::BlueTeamScores()
{
	++BlueTeamScore;

	ABlasterController* BlasterController = Cast<ABlasterController>(GetWorld()->GetFirstPlayerController());
	if (BlasterController)
	{
		BlasterController->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
