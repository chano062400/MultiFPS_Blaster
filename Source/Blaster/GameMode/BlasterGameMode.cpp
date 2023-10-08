// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Blaster/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true; //게임을 바로 시작하지 않고 게임시작을 호출하기 전까지 대기시간을 가짐 - 폰이 바로 생성되지 않고 레벨에서 움직일 수만 있음.
	bTeamsMatch = false;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterController* BlasterController = Cast<ABlasterController>(*It);
		if (BlasterController)
		{
			BlasterController->OnMatchStateSet(MatchState, bTeamsMatch);// BlasterGameMode인지 TeamsGameMode인지에 따라 false / true로 달라짐.
		}
	}
}


void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime; // 맵에 들어가고 나서의 시간을 재야함. 
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}

	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterController* VictimController, ABlasterController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		TArray<ABlasterPlayerState*> PlayersCurrentInTheLead;
		for (auto& LeadPlayer : BlasterGameState->TopScoringPlayers)
		{
			PlayersCurrentInTheLead.Add(LeadPlayer);
		}

		AttackerPlayerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);

		if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))  // 최고점자 리스트에 있다면 왕관을 생성.
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead(); 
			}
		}

		for (int32 i = 0; i < PlayersCurrentInTheLead.Num(); i++) // 최고점을 빼앗겼다면 왕관을 없앰.
		{
			if (!PlayersCurrentInTheLead.Contains(PlayersCurrentInTheLead[i])) // 처음에 추가했떤 최고점자들 리스트에 없다면 
			{
				ABlasterCharacter* Loser = Cast<ABlasterCharacter>(PlayersCurrentInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		VictimPlayerState->AddToDefeat(1);
	}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim(false); // 나간게 아니라 Damage에 입어서 Elim된 것이므로 false
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterController* BlasterPlayer = Cast<ABlasterController>(*It);
		if (BlasterPlayer && AttackerPlayerState && VictimPlayerState)
		{
			BlasterPlayer->BroadCastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimedCharacter, AController* ElimedController)
{
	if (ElimedCharacter)
	{
		ElimedCharacter->Reset();
		ElimedCharacter->Destroy();
	}

	if (ElimedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimedController, PlayerStarts[Selection]);
	}
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving)) //최고 득점자 플레이어라면 
	{
		BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving); // 목록에서 지움.
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn());
	if (BlasterCharacter)
	{
		BlasterCharacter->Elim(true);
	}
}

float ABlasterGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}

