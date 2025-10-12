// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "vector"
#include "Engine/UserDefinedStruct.h"
#include "DotLocationInfo.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FDotLocationInfo {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<bool> NeighbourAccessability;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int> NeighbourLinearIndex;
};


UCLASS(BlueprintType)
class DPG4T_API UDotLocation : public UUserDefinedStruct
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDotLocationInfo LocationInformation;
};
