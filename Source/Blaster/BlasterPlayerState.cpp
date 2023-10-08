// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"
#include "Blaster/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterController.h"
#include "Net/UnrealNetwork.h"
#include "BlasterTypes/Team.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeat);
	DOREPLIFETIME(ABlasterPlayerState, Team);
}

void ABlasterPlayerState::OnRep_Score() // 클라이언트만 복제 
{
	Super::OnRep_Score();

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(BlasterCharacter->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	 
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(BlasterCharacter->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToDefeat(int32 DefeatAmount)
{
	Defeat += DefeatAmount;

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(BlasterCharacter->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDDefeat(Defeat);
		}
	}
}

void ABlasterPlayerState::OnRep_Team()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterCharacter->SetTeamColor(Team);
	}
}

void ABlasterPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterCharacter->SetTeamColor(Team);
	}
}


void ABlasterPlayerState::OnRep_Defeat()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterController>(BlasterCharacter->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDDefeat(Defeat);
		}
	}
}

