

#include "BlasterController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Components/CombatComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/BlasterPlayerState.h"
#include "Components/Image.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blaster/HUD/ReturnToMainMenu.h"
#include "Blaster/BlasterTypes/Announcement.h"
#include "Blaster/GameState/BlasterGameState.h"

void ABlasterController::BroadCastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ABlasterController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim) // 클라이언트에게 표시될 문구 설정.
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if(Attacker && Victim && Self)
	{
		BlasterHUD = BlasterHUD == nullptr ? GetHUD<ABlasterHUD>() : BlasterHUD;
		if (BlasterHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				BlasterHUD->AddElimeAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			
			if (Victim == Self && Attacker != Self)
			{
				BlasterHUD->AddElimeAnnouncement(Attacker->GetPlayerName(), "You");
				return;
			}
			
			if (Attacker == Victim && Attacker == Self)
			{
				BlasterHUD->AddElimeAnnouncement("You" , "You");
				return;
			}

			if (Attacker == Victim && Attacker != Self)
			{
				BlasterHUD->AddElimeAnnouncement(Attacker->GetPlayerName(), "Themselves");
				return;
			}

			BlasterHUD->AddElimeAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
			return;

		}
	}
}

void ABlasterController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(CharacterMappingContext, 0);
	}
}

void ABlasterController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterController, MatchState);
	DOREPLIFETIME(ABlasterController, bShowTeamScore);
}

void ABlasterController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay && 
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText;
	
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar &&
		BlasterHUD->CharacterOverlay->ShieldText;

	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterController::SetHUDScore(float ScoreAmount)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount;

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(FString::Printf(TEXT(" %d"), FMath::FloorToInt(ScoreAmount))));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = ScoreAmount;
	}
}

void ABlasterController::HideTeamScore() // 개인전이면 TeamScore X
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValidHUD = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->BlueTeamScore &&
		BlasterHUD->CharacterOverlay->RedTeamScore &&
		BlasterHUD->CharacterOverlay->TeamScoreSpacer;
		
	if (bValidHUD)
	{
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->TeamScoreSpacer->SetText(FText());
	}
}

void ABlasterController::InitTeamScore() // 팀전이라면 TeamScore O
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	

	bool bValidHUD = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->BlueTeamScore &&
		BlasterHUD->CharacterOverlay->RedTeamScore;

	FString Zero("0");
	FString Spacer("|");

	if(bValidHUD)
	{
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->CharacterOverlay->TeamScoreSpacer->SetText(FText::FromString(Spacer));
	}
}

void ABlasterController::SetHUDRedTeamScore(int32 RedScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValidHUD = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->RedTeamScore;

	if (bValidHUD)
	{
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), RedScore)));

	}
}

void ABlasterController::SetHUDBlueTeamScore(int32 BlueScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bValidHUD = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->BlueTeamScore;

	if (bValidHUD)
	{
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(FString::Printf(TEXT("%d"), BlueScore)));

	}
}

void ABlasterController::SetHUDDefeat(int32 Defeat)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatAmount;

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->DefeatAmount->SetText(FText::FromString(FString::Printf(TEXT(" %d"), Defeat)));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeat;
	}
}

void ABlasterController::SetHUDWeaponAmmo(int32 Ammo) // 무기 들었을때 처음 한번 탄약 개수 업데이트.
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(FString::Printf(TEXT(" %d"), Ammo)));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(FString::Printf(TEXT(" %d"), CarriedAmmo)));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = CarriedAmmo;
	}
}

void ABlasterController::SetHUDMatchCountdown(float CountdownTime) //초기 설정한 매치시간 보여줌.
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - (Minutes * 60.f);

		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(FString::Printf(TEXT(" %02d:%02d"), Minutes,Seconds)));
	}
}

void ABlasterController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTime;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - (Minutes * 60.f);

		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(FString::Printf(TEXT(" %02d:%02d"), Minutes, Seconds)));
	}
}

void ABlasterController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->GrenadeText;

	if (bHUDValid)
	{
	
		BlasterHUD->CharacterOverlay->GrenadeText->SetText(FText::FromString(FString::Printf(TEXT(" %d"), Grenades)));
	}
	else
	{
		HUDGrenades = Grenades;
	}
}

void ABlasterController::SetHUDTime() // 시간이 줄어드는 걸 보여줌
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime  - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft) // 1초가 지날때마다 업데이트
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}

	}

	CountdownInt = SecondsLeft;
}

void ABlasterController::PollInit() // 처음 초기화.
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeShield)
				{
					UE_LOG(LogTemp, Warning, TEXT("SetHUDShield"));
					SetHUDShield(HUDShield, HUDMaxShield);
				}
				if(bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);

				if(bInitializeDefeats) SetHUDDefeat(HUDDefeats);
				
				if(bInitializeScore) SetHUDScore(HUDScore);

				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);

				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				
				if (BlasterCharacter && BlasterCharacter->GetCombat() && bInitializeGrenades)
				{
					SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency) // 설정한 동기화 주기마다 동기화.
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrquency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerState->GetCompressedPing() * 4: %d"), PlayerState->GetCompressedPing() * 4);
			if (PlayerState->GetCompressedPing() * 4 > HighPingThresHold) //GetCompressedPing() - 압축된 Ping 값(Ping = PingInMS / 4)의 리터럴 값을 가져옵니다.
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}

	bool bHighPingAnimationPlaying = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingAnimation &&
		BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation);

	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasterController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr) return;

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(QuitAction, ETriggerEvent::Triggered, this, &ABlasterController::ShowReturnToMainMenu);
	}
}

void ABlasterController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr)return;
	
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}

	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}

}

void ABlasterController::OnRep_ShowTeamScore()
{
	if (bShowTeamScore)
	{
		InitTeamScore();
	}
	else
	{
		HideTeamScore();
	}
}

void ABlasterController::ServerCheckMatchState_Implementation()
{
	BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));

	if (BlasterGameMode)
	{
		WarmupTime = BlasterGameMode->WarmupTime;
		MatchTime = BlasterGameMode->MatchTime;
		LevelStartingTime = BlasterGameMode->LevelStartingTime;
		MatchState = BlasterGameMode->GetMatchState();
		CooldownTime = BlasterGameMode->CooldownTime;
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, LevelStartingTime, CooldownTime);

	}
}

void ABlasterController::ClientJoinMidGame_Implementation(FName StateMatch, float Warmup, float Match, float StartingTime, float Cooldown) // 클라이언트가 중간에 게임에 들어왔을때 UI 동기화.
{
	MatchState = StateMatch;
	WarmupTime = Warmup;
	MatchTime = Match;
	LevelStartingTime = StartingTime;
	CooldownTime = Cooldown;
	OnMatchStateSet(MatchState);
	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds(); // 클라이언트에 넘길 서버의 시간.
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt); // 서버까지 메시지가 오는데 걸린 시간과 서버의 시간을 클라이언트에 넘겨줌.
}

void ABlasterController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceiveClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // 서버에 메시지를 보내는데 걸린 시간.
	SingleTripTime = RoundTripTime * 0.5f;
	float CurrentServerTime = TimeServerReceiveClientRequest + SingleTripTime; // 클라이언트에서 설정할 서버시간 = 받은 서버시간 + 왕복시간의 1/2(서버에서 클라이언트에 메시지 보내는 시간)

	ClientServerDelta = CurrentServerTime - (GetWorld()->GetTimeSeconds()); // 서버와 클라이언트의 시간 차이 = 계산한 서버 시간 - 클라이언트의 시간 
}

void ABlasterController::OnPossess(APawn* InPawn) // 캐릭터가 죽었다가 다시 살아났을 때 체력을 다시 업데이트.
{
	Super::OnPossess(InPawn);

	BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	CheckTimeSync(DeltaTime);

	PollInit(); // 오버레이 변수가 초기화되지 않았다면 초기화해서 화면에 나타나게 함.

	CheckPing(DeltaTime);
}


float ABlasterController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds(); //서버는 서버시간만 반환.
	return GetWorld()->GetTimeSeconds() + ClientServerDelta; // 클라이언트 시간 + 서버시간과의 차이 = 서버의 시간.
}

void ABlasterController::ReceivedPlayer() // 최대한 빨리 서버 시간과 동기화. / 이 PlayerController의 뷰포트/넷 연결이 이 플레이어 컨트롤러와 연결된 후에 호출됩니다.
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
	}
	else if(MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}


void ABlasterController::HandleMatchHasStarted(bool bTeamsMatch)
{
	if(HasAuthority()) bShowTeamScore = bTeamsMatch; // TeamMatch라면 TeamScore를 보이도록.

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if(BlasterHUD->CharacterOverlay == nullptr)BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}

		if (!HasAuthority()) return;

		if (bShowTeamScore)
		{
			InitTeamScore();
		}
		else
		{
			HideTeamScore();
		}
	}
}

void ABlasterController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->AnnouncementText &&
			BlasterHUD->Announcement->InfoText;

		if(bHUDValid)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				
				FString InfoTextString = bShowTeamScore ? GetTeamsInfoText(BlasterGameState) : GetInfoText(TopPlayers);

				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}

			
		}
	}

	BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->bDisableGameplay = true;
		BlasterCharacter->GetCombat()->FirePressed(false); //발사를 누른채 쿨다운으로 들어가면 계속 발사가 될 것이므로 거짓으로 해야함.
	}
}


FString ABlasterController::GetInfoText(TArray<class ABlasterPlayerState*>& Players)
{

	ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();

	if (BlasterPlayerState == nullptr) return FString();
	
	FString InfoTextString;
	
	if (Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == BlasterPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("Winner : \n%s"), *Players[0]->GetPlayerName());
	}
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayerTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		for (auto TiedPlayer : Players)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}
	return InfoTextString;
}

FString ABlasterController::GetTeamsInfoText(ABlasterGameState* BlasterGameState)
{
	if (BlasterGameState == nullptr) return FString();

	FString InfoTextString;

	const int32 RedTeamScore = BlasterGameState->RedTeamScore;

	const int32 BlueTeamScore = BlasterGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"),*Announcement::TeamsTiedForTheWin);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if (BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::RedTeam, RedTeamScore));

	}

	return InfoTextString;
}

void ABlasterController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast <ABlasterHUD>(GetHUD()) : BlasterHUD;
	
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation;

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation, 0.f, 5);
	}
}

void ABlasterController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast <ABlasterHUD>(GetHUD()) : BlasterHUD;

	bool bHUDValid = BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation;

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnimation))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

