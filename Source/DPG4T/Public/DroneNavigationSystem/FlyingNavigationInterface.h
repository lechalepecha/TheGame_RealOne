// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FlyingNavigationInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFlyingNavigationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DPG4T_API IFlyingNavigationInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Navigation")
	void GetTheRouteInLocations(const TArray<FVector>& RouteInLocations);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Navigation")
	void CalcTheRoute(FVector CurrentLcoation, FVector EndLocation, APawn* Drone);

};
