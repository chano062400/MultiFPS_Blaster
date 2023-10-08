// Fill out your copyright notice in the Description page of Project Settings.


#include "OverHeadWidget.h"
#include"Components/TextBlock.h"

void UOverHeadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverHeadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;
	switch (RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy: // 조종하고 있는 것.
		Role = FString("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy: // 조종X
		Role = FString("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}
	FString RemoteRoleString = FString::Printf(TEXT("Local Role is %s"), *Role);
	SetDisplayText(RemoteRoleString);
}

void UOverHeadWidget::NativeDestruct()
{
	RemoveFromParent();

	Super::NativeDestruct();
}
