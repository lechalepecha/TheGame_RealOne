// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EAbilityType.h"
#include "AbilityData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FAbilityData {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAbilityType AbilityType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* AbilityMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityTimerRollBack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText AbilityDescription;
};

UCLASS(BlueprintType)
class DPG4T_API UAbilityAsset : public UPrimaryDataAsset {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAbilityData AbilityDataItems;
};