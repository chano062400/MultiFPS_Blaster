// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrossHairsCenter;
	UTexture2D* CrossHairsLeft;
	UTexture2D* CrossHairsRight;
	UTexture2D* CrossHairsTop;
	UTexture2D* CrossHairsBottom;
	float CrossHairSpread;
	FLinearColor CrossHairColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
public:

	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = "Announcement")
		TSubclassOf<class UUserWidget> AnnouncementClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY()
		class UAnnouncement* Announcement;

	void AddCharacterOverlay();

	void AddAnnouncement();

	void AddElimeAnnouncement(FString AttackerName, FString VictimName);

protected:

	virtual void BeginPlay() override;



private:

	FHUDPackage HUDPackage;

	void DrawCrossHair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrossHairColor);

	UPROPERTY(EditAnywhere)
	float CrossHairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;

	UPROPERTY()
	class APlayerController* OwningPlayer;

	UPROPERTY(EditAnywhere)
	float ElimAnnouncementTime = 1.5f;

	UFUNCTION()
	void ElimAnnouncementFinished(UElimAnnouncement* MsgToRemove);

	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessages;

public:

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
