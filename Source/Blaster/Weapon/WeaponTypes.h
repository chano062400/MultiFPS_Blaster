#pragma once

#define TRACE_LENGTH 80000.f
#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252


UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	ETW_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	ETW_Pistol UMETA(DisplayName = "Pistol"),
	ETW_SubMachineGun UMETA(DisplayName = "SubMachine Gun"),
	ETW_Shotgun UMETA(DisplayName = "Shotgun"),
	ETW_SniperRifle UMETA(DisplayName = "Sniper Rifle"),
	ETW_GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
	ETW_Flag UMETA(DisplayName = "Flag"),
	
	
	EWT_MAX UMETA(DisplayName = "Default Max")
	
};