#pragma once

UENUM(BlueprintType)
enum class ETeam : uint8
{
	ET_RedTeam UMETA(DisplayName = "Red Team"),
	ET_BlueTeam UMETA(DisplayNAme = "Blue Team"),
	ET_NoTeam UMETA(DisplayNAme = "No Team"),

	ET_MAX UMETA(DisplayNAme = "Default MAX"),
};