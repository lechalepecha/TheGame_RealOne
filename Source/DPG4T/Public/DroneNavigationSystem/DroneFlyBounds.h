// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "vector"
#include "GameFramework/Actor.h"
#include "DotLocationInfo.h"
#include "FlyingNavigationInterface.h"
#include "DroneFlyBounds.generated.h"

class UBoxComponent;
class USceneComponent;

UCLASS(config = Game)
class DPG4T_API ADroneFlyBounds : public AActor, public IFlyingNavigationInterface
{
	GENERATED_BODY()


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionMesh = nullptr;


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USceneComponent* DefaultScene = nullptr;

	
public:	
	// Sets default values for this actor's properties
	ADroneFlyBounds();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	float LocationInterval{ 150.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	float XMin = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	float XMax = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	float YMin = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	float YMax = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	float ZMin = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	float ZMax = 0.f;

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	TArray<int> SelectedLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bounds, meta = (AllowPrivateAccess = "true"))
	TArray<FDotLocationInfo> DotLocations;


protected:

	TArray<float> XArray;
	TArray<float> YArray;
	TArray<float> ZArray;

	FDotLocationInfo CurrentDotLocation;
	


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay();

	virtual void OnConstruction(const FTransform& Transform) override;

	TArray<float> FillAxisArrays(float AxisMin, float AxisMax, float Interval);

	void FillDotLocations();
	void FiXFilledDotLocations();

	float FindHeuristics(FVector CurrentLocation, FVector EndLocation);
	float FindStepCost(FVector CurrentLocation, FVector NextLocation);

	void SelectBestLocation(int StartLocation, int EndLocation);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	int GetClosestLocation(FVector CurrentLocation);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void CalcTheRoute_Implementation(FVector CurrentLcoation, FVector EndLocation, APawn* Drone) override;
	
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void GetTheRouteInLocations_Implementation(const TArray<FVector>& RouteInLocations) override;
};
