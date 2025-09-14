// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EAbilityType.generated.h"
/**
 * 
 */
UENUM(BlueprintType)
enum class EAbilityType : uint8
{
	None,
	ForcePush,
	SingleStun,
	MegaPunch,
	Flame,
	Lash,
	LashAOE
};
