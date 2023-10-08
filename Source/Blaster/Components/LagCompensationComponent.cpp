#include "LagCompensationComponent.h"
#include "Blaster/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Blaster.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage Package;
	SaveFramePackage(Package);
	ShowFramePackage(Package, FColor::Orange);
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo) //반복문에서 참조자 사용시 복사가 일어나지 않으나 변수값이 계속 변화함. <-> 포인터는 값 복사가 일어나고 변수값이 변하지 않음.
	{
		const FName& BoxInfoName = YoungerPair.Key; //OlderFrame이든 YoungerFrame이든 BoxName은 같음.
		
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName]; //OlderFrame에서 해당 BoxName에 맞는 Value값을 가져옴.
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName]; //YoungerFrame에서 해당 BoxName에 맞는 Value값.

		FBoxInformation InterpBoxInfo;

		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction); //보간
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction); //보간
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent; //박스 크기는 YoungerBox를 쓰든 OlderBox를 쓰든 상관 없음.

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;

	CacheBoxPositions(HitCharacter, CurrentFrame); // 현재 프레임의 HitCharacter에 대한 박스정보를 저장.'

	MoveBoxes(HitCharacter, Package); // 보간된 HItBox의 정보로 이동.
	
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision); // 메쉬에는 충돌이 일어나지 않도록 비활성화.

	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;

	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25;
	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);

		if (ConfirmHitResult.bBlockingHit) //HeadBox만 충돌이 일어나게 했으므로. 충돌이 일어난다면 무조건 HeadBox
		{
			if (ConfirmHitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
				if (Box)
				{
					DrawDebugBox(World, Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
				}
			}

			ResetHitBoxes(HitCharacter, CurrentFrame); // 처음에 저장해놓은 HitBox들의 정보로 리셋함.
			
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{true , true}; // 맞았고 헤드샷임.
		}
		else // HeadBox에 맞지 않았다면 나머지 박스들도 충돌이 일어날 수 있도록 함.
		{
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
 				}
			}

			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			if (ConfirmHitResult.bBlockingHit)
			{

				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(World, Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
					}
				}

				ResetHitBoxes(HitCharacter, CurrentFrame);

				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

				return FServerSideRewindResult{ true, false }; // 맞았지만 헤드샷은 아님
			}

		
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame); 

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);// 비활성화했던 메쉬를 다시 충돌 활성화 시킴

	return FServerSideRewindResult{ false, false };

}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;

	CacheBoxPositions(HitCharacter, CurrentFrame); // 현재 프레임의 HitCharacter에 대한 박스정보를 저장.'

	MoveBoxes(HitCharacter, Package); // 보간된 HItBox의 정보로 이동.

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision); // 메쉬에는 충돌이 일어나지 않도록 비활성화.

	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;	
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	FPredictProjectilePathResult PathResult;

	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if (PathResult.HitResult.bBlockingHit) // 헤드샷
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
			if (Box)
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
			}
		}

		ResetHitBoxes(HitCharacter, CurrentFrame); // 처음에 저장해놓은 HitBox들의 정보로 리셋함.

		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

		return FServerSideRewindResult{ true , true }; // 맞았고 헤드샷임.
	}
	else // 바디샷
	{
		for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

		if (PathResult.HitResult.bBlockingHit)
		{
			if (PathResult.HitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
				if (Box)
				{
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
				}
			}

			ResetHitBoxes(HitCharacter, CurrentFrame);

			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{ true, false }; // 맞았지만 헤드샷은 아님
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);// 비활성화했던 메쉬를 다시 충돌 활성화 시킴

	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
	}

	FShotgunServerSideRewindResult ShotgunResult;

	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : FramePackages) 
	{
		FFramePackage CurrentFrame;
		
		CurrentFrame.Character = Frame.Character;

		CacheBoxPositions(Frame.Character, CurrentFrame); // 현재 프레임의 HitCharacter에 대한 박스정보를 저장.'

		MoveBoxes(Frame.Character, Frame); // 보간된 HItBox의 정보로 이동.

		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision); // 메쉬에는 충돌이 일어나지 않도록 비활성화.

		CurrentFrames.Add(CurrentFrame);
	}

	for (auto& Frame : FramePackages)
	{
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	UWorld* World = GetWorld();
	//HeadShot 체크
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;

		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25;
		
		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());

			if (BlasterCharacter) // 헤드박스만 충돌이 가능하도록 했으므로 충돌이 일어난다면 헤드샷.
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(World, Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
					}
				}

				if (ShotgunResult.HeadShots.Contains(BlasterCharacter))
				{
					ShotgunResult.HeadShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.HeadShots.Emplace(BlasterCharacter, 1);
				}
			}

		}
	} 

	//헤드박스를 제외한 나머지 모든 박스들에 충돌이 일어나도록 함, 헤드박스는 충돌 비활성화.
	for (auto& Frame : FramePackages)
	{
		for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	//BodyShot 체크
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;

		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25;

		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());

			if (BlasterCharacter) // 헤드박스만 충돌이 가능하도록 했으므로 충돌이 일어난다면 헤드샷.
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(World, Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Blue, false, 8.f);
					}
				}

				if (ShotgunResult.BodyShots.Contains(BlasterCharacter))
				{
					ShotgunResult.BodyShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.BodyShots.Emplace(BlasterCharacter, 1);
				}
			}

		}
	}

	for (auto& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);

		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics); // 비활성화했던 메쉬를 다시 충돌 활성화 시킴
	}

	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage) // 박스 정보를 저장.
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;

			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();

			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}

	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	//for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	//{
	//	if (HitBoxPair.Value != nullptr)
	//	{
	//		HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location); // Pakcage의 HitBoxInfo의 해당 박스의 위치로 움직임.
	//		HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
	//		HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
	//	}
	//}
	for (TTuple<FName, UBoxComponent*>& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{

		if (HitBoxPair.Value != nullptr)
		{

			const FBoxInformation* BoxValue = Package.HitBoxInfo.Find(HitBoxPair.Key);

			if (BoxValue)
			{
				HitBoxPair.Value->SetWorldLocation(BoxValue->Location);
				HitBoxPair.Value->SetWorldRotation(BoxValue->Rotation);
				HitBoxPair.Value->SetBoxExtent(BoxValue->BoxExtent);
			}

		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	/*for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)  
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location); // Pakcage의 HitBoxInfo의 해당 박스의 위치로 움직임.
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	*/

	for (TTuple<FName, UBoxComponent*>& HitBoxPair : HitCharacter->HitCollisionBoxes) //ResetHitBoxes에서 Key가 Null인 경우가 있어 오류가 일어나서 Tuple로 변경하고, Find를 통해 Key값이 있는지 확인함.
	{
		if (HitBoxPair.Value != nullptr)
		{
			const FBoxInformation* BoxValue = Package.HitBoxInfo.Find(HitBoxPair.Key);

			if (BoxValue)
			{
				HitBoxPair.Value->SetWorldLocation(BoxValue->Location);
				HitBoxPair.Value->SetWorldRotation(BoxValue->Rotation);
				HitBoxPair.Value->SetBoxExtent(BoxValue->BoxExtent);
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || Character->HasAuthority()) return;


	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{   //Head 가장 최근 , Tail 가장 오래된  
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		while (HistoryLength > MaxRecordTime) // 최대 기록길이보다 더 길어진다면 삭제해야 함.
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail()); // 가장 오랜된 프레임부터 삭제.
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}// Tail을 없애고 Head를 새로 추가.
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);

		//ShowFramePackage(ThisFrame, FColor::Red); //4초 길이의 FramePackage의 박스 위치를 보여줌.
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package) // 모든 박스의 정보를 저장.
{
	Character = Character ? Cast<ABlasterCharacter>(GetOwner()) : Character;

	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds(); //서버에서만 기록하므로.
		Package.Character = Character;

		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}

}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, FColor Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameCheck = GetFrameToCheck(HitCharacter , HitTime);

	return ConfirmHit(FrameCheck, HitCharacter, TraceStart, HitLocation); 
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage FrameCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ProjectileConfirmHit(FrameCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	float TotalDamage = 0.f;

	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character == nullptr || Character->GetEquippedWeapon() == nullptr) continue;

		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetHeadShotDamage();
			
			TotalDamage += HeadShotDamage;
		}
			
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();

			TotalDamage += BodyShotDamage;
		}

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Damage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (Character && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		UGameplayStatics::ApplyDamage(
			HitCharacter, 
			Damage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn = HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;

	if (bReturn) return FFramePackage();
	
	FFramePackage FrameCheck;
	bool bShouldInterpolate = true;

	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time; // 0.1sec
	const float NewestHIstroyTime = History.GetHead()->GetValue().Time; // 3 sec

	if (OldestHistoryTime > HitTime) // 더 오래됨. ex) if(0.1 sec > 0.05sec)
	{
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		FrameCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHIstroyTime <= HitTime) // ex) if(3sec <= 3sec)
	{
		FrameCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger; // 아무것도 없는 처음 상태(head == tail)

	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode(); // Older를 한칸 이동  Tail->PrevNode ->->->->->  <-<-<-<-<-NextNode<-Head
		if (Older->GetValue().Time > HitTime) /* Older의 Time이(4) HitTime보다(3) 크다면 Younger도 한칸 이동(Older == Younger가 됨)
			<-> Older의 TIme이(2) HitTime(3)보다 작다면 이동하지 않고 Younger(2)과 Older(4)가 HitTime을 가두는 양쪽 범위가 됨.*/
		{
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime) //희박한 확률로 Older의 Time과 HitTiem이 같은 경우.
	{
		FrameCheck = Older->GetValue();
		bAllowAnyoneToDestroyMe = false;

	}

	if (bShouldInterpolate)
	{
		FrameCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime); // Older -> HitTime <- Younger  보간.
	}

	FrameCheck.Character = HitCharacter;

	return FrameCheck;
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FrameCheck;

	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FrameCheck.Add(GetFrameToCheck(HitCharacter, HitTime)); // 보간된 프레임 정보들을 넣음.
	}

	return ShotgunConfirmHit(FrameCheck, TraceStart, HitLocations);
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	SaveFramePackage();
}


