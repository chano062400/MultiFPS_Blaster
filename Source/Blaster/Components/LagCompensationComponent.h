
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
		FVector Location;

	UPROPERTY()
		FRotator Rotation;

	UPROPERTY()
		FVector BoxExtent;

};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
		ABlasterCharacter* Character; // 샷건은 여러 명을 Hit 할 수 있기 때문에 기록함.
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadShots;
	
	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	friend class ABlasterCharacter;

	ULagCompensationComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SaveFramePackage(FFramePackage& Package);

	void ShowFramePackage(const FFramePackage& Package, FColor Color);

	//HitScan
	FServerSideRewindResult ServerSideRewind(class ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime);
	
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);
	//Projectile
	
	FServerSideRewindResult ProjectileServerSideRewind(
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	//Shotgun
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);

	UFUNCTION(Server, Reliable)
		void ShotgunServerScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters,
			const FVector_NetQuantize& TraceStart,
			const TArray<FVector_NetQuantize>& HitLocations,
			float HitTime);

	

	UPROPERTY()
		ABlasterCharacter* Character;

	UPROPERTY()
		class ABlasterController* Controller;

protected:
	virtual void BeginPlay() override;

	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime); //const 참조형식의 매개변수를 사용해서 메모리 재할당을 안함.

	
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);

	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);

	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);

	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	void SaveFramePackage();

	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime);

	//HitScan

	FServerSideRewindResult ConfirmHit(const FFramePackage& Package,
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart, 
		const FVector_NetQuantize& HitLocation);

	//Projectile

	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package,
		ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
		);

	//Shotgun 

	FShotgunServerSideRewindResult ShotgunConfirmHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations
	);

private:	
		
	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
		float MaxRecordTime = 4.f;
};
