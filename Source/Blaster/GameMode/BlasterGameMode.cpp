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
	bDelayedStart = true; //������ �ٷ� �������� �ʰ� ���ӽ����� ȣ���ϱ� ������ ���ð��� ���� - ���� �ٷ� �������� �ʰ� �������� ������ ���� ����.
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
			BlasterController->OnMatchStateSet(MatchState, bTeamsMatch);// BlasterGameMode���� TeamsGameMode������ ���� false / true�� �޶���.
		}
	}
}


void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime; // �ʿ� ���� ������ �ð��� �����. 
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

		if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))  // �ְ����� ����Ʈ�� �ִٸ� �հ��� ����.
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead(); 
			}
		}

		for (int32 i = 0; i < PlayersCurrentInTheLead.Num(); i++) // �ְ����� ���Ѱ�ٸ� �հ��� ����.
		{
			if (!PlayersCurrentInTheLead.Contains(PlayersCurrentInTheLead[i])) // ó���� �߰��߶� �ְ����ڵ� ����Ʈ�� ���ٸ� 
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
		EliminatedCharacter->Elim(false); // ������ �ƴ϶� Damage�� �Ծ Elim�� ���̹Ƿ� false
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
	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving)) //�ְ� ������ �÷��̾��� 
	{
		BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving); // ��Ͽ��� ����.
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

