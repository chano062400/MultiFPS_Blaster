// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "ElimAnnouncement.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

		float SpreadScale = CrossHairSpreadMax * HUDPackage.CrossHairSpread;

		if (HUDPackage.CrossHairsCenter)
		{
			FVector2D Spread(0.f, 0.f);
			DrawCrossHair(HUDPackage.CrossHairsCenter, ViewportCenter, Spread, HUDPackage.CrossHairColor);
		}
		if (HUDPackage.CrossHairsLeft)
		{
			FVector2D Spread(-SpreadScale, 0.f);
			DrawCrossHair(HUDPackage.CrossHairsLeft, ViewportCenter, Spread, HUDPackage.CrossHairColor);
		}
		if (HUDPackage.CrossHairsRight)
		{
			FVector2D Spread(SpreadScale, 0.f);
			DrawCrossHair(HUDPackage.CrossHairsRight, ViewportCenter, Spread, HUDPackage.CrossHairColor);
		}
		if (HUDPackage.CrossHairsTop)
		{
			FVector2D Spread(0.f, -SpreadScale);
			DrawCrossHair(HUDPackage.CrossHairsTop, ViewportCenter, Spread, HUDPackage.CrossHairColor);
		}
		if (HUDPackage.CrossHairsBottom)
		{
			FVector2D Spread(0.f, SpreadScale);
			DrawCrossHair(HUDPackage.CrossHairsBottom, ViewportCenter, Spread, HUDPackage.CrossHairColor);
		}
	}
}

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();

}

void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void ABlasterHUD::AddElimeAnnouncement(FString AttackerName, FString VictimName)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if (OwningPlayer && ElimAnnouncementClass)
	{
		UElimAnnouncement* ElimAnnouncementWIdget = CreateWidget<UElimAnnouncement>(OwningPlayer, ElimAnnouncementClass);
		if (ElimAnnouncementWIdget)
		{
			ElimAnnouncementWIdget->SetAnnouncementText(AttackerName, VictimName);
			ElimAnnouncementWIdget->AddToViewport();

			for (auto Msg : ElimMessages)
			{
				if (Msg && Msg->AnnouncementBox)
				{
					UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnouncementBox); //CanvasPanel에 있는 AnnouncementBox의 정보를 받아옴.
					if (CanvasSlot)
					{
						FVector2D Position = CanvasSlot->GetPosition(); //Announcement의 위치
						FVector2D NewPosition(Position.X, Position.Y - CanvasSlot->GetSize().Y); // 위로 올릴 위치(AnnouncementBox의 Y의 길이만큼 올림)
						CanvasSlot->SetPosition(NewPosition);
					}
				}
			}


			ElimMessages.Add(ElimAnnouncementWIdget);

			FTimerHandle ElimMsgTimer;
			FTimerDelegate ElimMsgDelegate;
			ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementFinished"), ElimAnnouncementWIdget);
			GetWorldTimerManager().SetTimer(ElimMsgTimer,
				ElimMsgDelegate,
				ElimAnnouncementTime,
				false);
		}
	}
}

void ABlasterHUD::ElimAnnouncementFinished(UElimAnnouncement* MsgToRemove)
{
	if (MsgToRemove)
	{
		MsgToRemove->RemoveFromParent();
	}
}

void ABlasterHUD::DrawCrossHair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrossHairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint(ViewportCenter.X - (TextureWidth / 2.f) + Spread.X, ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y);

	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrossHairColor
	);
}
