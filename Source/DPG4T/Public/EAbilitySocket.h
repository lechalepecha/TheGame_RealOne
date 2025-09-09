// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EAbilitySocket.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EAbilitySocket : uint8
{
	None,
	FirstUpdate,
	SecondUpdate
	
};
