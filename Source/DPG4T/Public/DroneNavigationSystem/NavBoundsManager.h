// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DroneFlyBounds.h"
#include "Subsystems/WorldSubsystem.h"
#include "NavBoundsManager.generated.h"

/**
 * 
 */
class ADroneFlyBounds;

UCLASS(config = Game)
class DPG4T_API UNavBoundsManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:

    UPROPERTY()
    
    
    TArray<ADroneFlyBounds*> RegisteredBounds;

    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void RegisterNavBounds(ADroneFlyBounds* Bounds)
    {
        RegisteredBounds.AddUnique(Bounds);
    }

    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void UnregisterNavBounds(ADroneFlyBounds* Bounds)
    {
        RegisteredBounds.Remove(Bounds);
    }

    UFUNCTION(BlueprintCallable, Category = "Navigation")
    ADroneFlyBounds* FindBestNavBounds(const FVector& Location)
    {
        ADroneFlyBounds* BestBounds = nullptr;
        float MinDistance = FLT_MAX;

        for (ADroneFlyBounds* Bounds : RegisteredBounds)
        {
            if (!IsValid(Bounds)) continue;

            float Distance = FVector::Dist(Location, Bounds->GetActorLocation());
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                BestBounds = Bounds;
            }
        }

        return BestBounds;
    }
	
};
