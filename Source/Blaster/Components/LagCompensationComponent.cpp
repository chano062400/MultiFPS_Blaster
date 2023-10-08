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

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo) //�ݺ������� ������ ���� ���簡 �Ͼ�� ������ �������� ��� ��ȭ��. <-> �����ʹ� �� ���簡 �Ͼ�� �������� ������ ����.
	{
		const FName& BoxInfoName = YoungerPair.Key; //OlderFrame�̵� YoungerFrame�̵� BoxName�� ����.
		
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName]; //OlderFrame���� �ش� BoxName�� �´� Value���� ������.
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName]; //YoungerFrame���� �ش� BoxName�� �´� Value��.

		FBoxInformation InterpBoxInfo;

		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction); //����
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction); //����
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent; //�ڽ� ũ��� YoungerBox�� ���� OlderBox�� ���� ��� ����.

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;

	CacheBoxPositions(HitCharacter, CurrentFrame); // ���� �������� HitCharacter�� ���� �ڽ������� ����.'

	MoveBoxes(HitCharacter, Package); // ������ HItBox�� ������ �̵�.
	
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision); // �޽����� �浹�� �Ͼ�� �ʵ��� ��Ȱ��ȭ.

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

		if (ConfirmHitResult.bBlockingHit) //HeadBox�� �浹�� �Ͼ�� �����Ƿ�. �浹�� �Ͼ�ٸ� ������ HeadBox
		{
			if (ConfirmHitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
				if (Box)
				{
					DrawDebugBox(World, Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
				}
			}

			ResetHitBoxes(HitCharacter, CurrentFrame); // ó���� �����س��� HitBox���� ������ ������.
			
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

			return FServerSideRewindResult{true , true}; // �¾Ұ� ��弦��.
		}
		else // HeadBox�� ���� �ʾҴٸ� ������ �ڽ��鵵 �浹�� �Ͼ �� �ֵ��� ��.
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

				return FServerSideRewindResult{ true, false }; // �¾����� ��弦�� �ƴ�
			}

		
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame); 

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);// ��Ȱ��ȭ�ߴ� �޽��� �ٽ� �浹 Ȱ��ȭ ��Ŵ

	return FServerSideRewindResult{ false, false };

}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;

	CacheBoxPositions(HitCharacter, CurrentFrame); // ���� �������� HitCharacter�� ���� �ڽ������� ����.'

	MoveBoxes(HitCharacter, Package); // ������ HItBox�� ������ �̵�.

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision); // �޽����� �浹�� �Ͼ�� �ʵ��� ��Ȱ��ȭ.

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

	if (PathResult.HitResult.bBlockingHit) // ��弦
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
			if (Box)
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
			}
		}

		ResetHitBoxes(HitCharacter, CurrentFrame); // ó���� �����س��� HitBox���� ������ ������.

		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

		return FServerSideRewindResult{ true , true }; // �¾Ұ� ��弦��.
	}
	else // �ٵ�
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

			return FServerSideRewindResult{ true, false }; // �¾����� ��弦�� �ƴ�
		}
	}

	ResetHitBoxes(HitCharacter, CurrentFrame);

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);// ��Ȱ��ȭ�ߴ� �޽��� �ٽ� �浹 Ȱ��ȭ ��Ŵ

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

		CacheBoxPositions(Frame.Character, CurrentFrame); // ���� �������� HitCharacter�� ���� �ڽ������� ����.'

		MoveBoxes(Frame.Character, Frame); // ������ HItBox�� ������ �̵�.

		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision); // �޽����� �浹�� �Ͼ�� �ʵ��� ��Ȱ��ȭ.

		CurrentFrames.Add(CurrentFrame);
	}

	for (auto& Frame : FramePackages)
	{
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	UWorld* World = GetWorld();
	//HeadShot üũ
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

			if (BlasterCharacter) // ���ڽ��� �浹�� �����ϵ��� �����Ƿ� �浹�� �Ͼ�ٸ� ��弦.
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

	//���ڽ��� ������ ������ ��� �ڽ��鿡 �浹�� �Ͼ���� ��, ���ڽ��� �浹 ��Ȱ��ȭ.
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

	//BodyShot üũ
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

			if (BlasterCharacter) // ���ڽ��� �浹�� �����ϵ��� �����Ƿ� �浹�� �Ͼ�ٸ� ��弦.
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

		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics); // ��Ȱ��ȭ�ߴ� �޽��� �ٽ� �浹 Ȱ��ȭ ��Ŵ
	}

	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage) // �ڽ� ������ ����.
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
	//		HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location); // Pakcage�� HitBoxInfo�� �ش� �ڽ��� ��ġ�� ������.
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
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location); // Pakcage�� HitBoxInfo�� �ش� �ڽ��� ��ġ�� ������.
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	*/

	for (TTuple<FName, UBoxComponent*>& HitBoxPair : HitCharacter->HitCollisionBoxes) //ResetHitBoxes���� Key�� Null�� ��찡 �־� ������ �Ͼ�� Tuple�� �����ϰ�, Find�� ���� Key���� �ִ��� Ȯ����.
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
	{   //Head ���� �ֱ� , Tail ���� ������  
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		while (HistoryLength > MaxRecordTime) // �ִ� ��ϱ��̺��� �� ������ٸ� �����ؾ� ��.
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail()); // ���� ������ �����Ӻ��� ����.
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}// Tail�� ���ְ� Head�� ���� �߰�.
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);

		//ShowFramePackage(ThisFrame, FColor::Red); //4�� ������ FramePackage�� �ڽ� ��ġ�� ������.
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package) // ��� �ڽ��� ������ ����.
{
	Character = Character ? Cast<ABlasterCharacter>(GetOwner()) : Character;

	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds(); //���������� ����ϹǷ�.
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

	if (OldestHistoryTime > HitTime) // �� ������. ex) if(0.1 sec > 0.05sec)
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
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger; // �ƹ��͵� ���� ó�� ����(head == tail)

	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode(); // Older�� ��ĭ �̵�  Tail->PrevNode ->->->->->  <-<-<-<-<-NextNode<-Head
		if (Older->GetValue().Time > HitTime) /* Older�� Time��(4) HitTime����(3) ũ�ٸ� Younger�� ��ĭ �̵�(Older == Younger�� ��)
			<-> Older�� TIme��(2) HitTime(3)���� �۴ٸ� �̵����� �ʰ� Younger(2)�� Older(4)�� HitTime�� ���δ� ���� ������ ��.*/
		{
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime) //����� Ȯ���� Older�� Time�� HitTiem�� ���� ���.
	{
		FrameCheck = Older->GetValue();
		bAllowAnyoneToDestroyMe = false;

	}

	if (bShouldInterpolate)
	{
		FrameCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime); // Older -> HitTime <- Younger  ����.
	}

	FrameCheck.Character = HitCharacter;

	return FrameCheck;
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FrameCheck;

	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FrameCheck.Add(GetFrameToCheck(HitCharacter, HitTime)); // ������ ������ �������� ����.
	}

	return ShotgunConfirmHit(FrameCheck, TraceStart, HitLocations);
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	SaveFramePackage();
}


