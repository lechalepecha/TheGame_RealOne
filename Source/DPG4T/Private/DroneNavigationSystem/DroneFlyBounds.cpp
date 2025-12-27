// Fill out your copyright notice in the Description page of Project Settings.

#include "DroneNavigationSystem/DroneFlyBounds.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Async/Async.h"
#include "Kismet/KismetMathLibrary.h"
#include <DroneNavigationSystem/NavBoundsManager.h>

// Sets default values
ADroneFlyBounds::ADroneFlyBounds()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DefaultScene = CreateDefaultSubobject<USceneComponent>(FName("Default Scene"));

	CollisionMesh = CreateDefaultSubobject<UBoxComponent>(FName("BoxCollision"));
	CollisionMesh->AttachToComponent(DefaultScene, FAttachmentTransformRules::SnapToTargetIncludingScale);
}

void ADroneFlyBounds::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CollisionMesh->SetBoxExtent(DefaultScene->GetRelativeScale3D());

	FBoxSphereBounds BoxBounds = CollisionMesh->CalcBounds(CollisionMesh->GetComponentTransform());
	FVector BoxOrigin = BoxBounds.Origin;
	FVector BoxExtent = BoxBounds.BoxExtent;
	
	XMin = BoxOrigin.X - BoxExtent.X;
	XMax = BoxOrigin.X + BoxExtent.X;

	YMin = BoxOrigin.Y - BoxExtent.Y;
	YMax = BoxOrigin.Y + BoxExtent.Y;

	ZMin = BoxOrigin.Z - BoxExtent.Z;
	ZMax = BoxOrigin.Z + BoxExtent.Z;



}

// Called when the game starts or when spawned
void ADroneFlyBounds::BeginPlay()
{
	Super::BeginPlay();


	XArray = FillAxisArrays(XMin, XMax, LocationInterval);
	YArray = FillAxisArrays(YMin, YMax, LocationInterval);
	ZArray = FillAxisArrays(ZMin, ZMax, LocationInterval);
	

	FillDotLocations();

	FiXFilledDotLocations();

	GetWorld()->GetSubsystem<UNavBoundsManager>()->RegisterNavBounds(this);


	/*SelectBestLocation(0, DotLocations.Num() - 1);

	for (int i = 0; i < SelectedLocation.Num() - 1; i++)
	{
		DrawDebugLine(GetWorld(), DotLocations[SelectedLocation[i]].Location, DotLocations[SelectedLocation[i + 1]].Location, FColor::Red, false, 500.f);
	}*/
}


// Called every frame
void ADroneFlyBounds::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADroneFlyBounds::FillDotLocations()
{
	for (int i = 0; i < XArray.Num(); i++)
	{
		for (int j = 0; j < YArray.Num(); j++)
		{
			for (int k = 0; k < ZArray.Num(); k++)
			{
				DotLocations.Add(FDotLocationInfo(FVector(XArray[i], YArray[j], ZArray[k])));
				//DrawDebugSphere(GetWorld(), FVector(XArray[i], YArray[j], ZArray[k]), 10.f, 8, FColor::Blue, false, 500.f);

			}
		}
	}
}

void ADroneFlyBounds::FiXFilledDotLocations()
{
	static const FIntVector NeighborOffsets[26] = {
		{-1,-1,-1}, {-1,-1,0}, {-1,-1,1},
		{-1,0,-1},  {-1,0,0},  {-1,0,1},
		{-1,1,-1},  {-1,1,0},  {-1,1,1},
		{0,-1,-1},  {0,-1,0},  {0,-1,1},
		{0,0,-1},   {0,0,1},
		{0,1,-1},   {0,1,0},   {0,1,1},
		{1,-1,-1},  {1,-1,0},  {1,-1,1},
		{1,0,-1},   {1,0,0},   {1,0,1},
		{1,1,-1},   {1,1,0},   {1,1,1}
	};

	for (int i = 0; i <= XArray.Num() - 1; i++)
	{
		int Xindex = i;
		for (int j = 0; j <= YArray.Num() - 1; j++)
		{
			int Yindex = j;
			for (int k = 0; k <= ZArray.Num() - 1; k++)
			{
				int Zindex = k;

				int CurrentLinearIndex =0;
				int NeighbourLinearIndex=0;

				TArray<bool> Accessability;
				TArray<int> NeighboursIndexes;

				for (int s = 0; s <= 25; s++)
				{
					int tempX = Xindex + NeighborOffsets[s].X;
					int tempY = Yindex + NeighborOffsets[s].Y;
					int tempZ = Zindex + NeighborOffsets[s].Z;
					
					if (tempX >= 0 && tempX <= XArray.Num() - 1 && tempY >= 0 && tempY <= YArray.Num() - 1 && tempZ >= 0 && tempZ <= ZArray.Num() - 1)
					{
						CurrentLinearIndex = (Xindex * (ZArray.Num() * YArray.Num())) + (Yindex * ZArray.Num()) + Zindex;
						NeighbourLinearIndex = (tempX * (ZArray.Num() * YArray.Num())) + (tempY * ZArray.Num()) + tempZ;
						
						FCollisionQueryParams Params = FCollisionQueryParams();
						Params.AddIgnoredActor(this);
						FHitResult HitResult;
						

						bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, DotLocations[CurrentLinearIndex].Location, DotLocations[NeighbourLinearIndex].Location, ECollisionChannel::ECC_Visibility, Params);
						
						Accessability.Add(!bHit);
						NeighboursIndexes.Add(NeighbourLinearIndex);
					
					}
				}

				if (CurrentLinearIndex < DotLocations.Num() && CurrentLinearIndex >= 0)
				{
					DotLocations[CurrentLinearIndex].NeighbourAccessability = Accessability;
					DotLocations[CurrentLinearIndex].NeighbourLinearIndex = NeighboursIndexes;
				}
				
			}
		}
	}

	
}

float ADroneFlyBounds::FindHeuristics(FVector CurrentLocation, FVector EndLocation)
{
	float XX = UKismetMathLibrary::Square(CurrentLocation.X - EndLocation.X);
	float YY = UKismetMathLibrary::Square(CurrentLocation.Y - EndLocation.Y);
	float ZZ = UKismetMathLibrary::Square(CurrentLocation.Z - EndLocation.Z);

	float Result = UKismetMathLibrary::Sqrt(XX + YY + ZZ);

	return Result;
}

float ADroneFlyBounds::FindStepCost(FVector CurrentLocation, FVector NextLocation)
{
	float XX = UKismetMathLibrary::Square(NextLocation.X - CurrentLocation.X);
	float YY = UKismetMathLibrary::Square(NextLocation.Y - CurrentLocation.Y);
	float ZZ = UKismetMathLibrary::Square(NextLocation.Z - CurrentLocation.Z);

	float Result = UKismetMathLibrary::Sqrt(XX + YY + ZZ);

	return Result;
}

TArray<int> ADroneFlyBounds::SelectBestLocation(int StartLoc, int EndLoc, TArray<FDotLocationInfo> LocationsCopy)
{
	int32 StartIdx = StartLoc;
	int32 EndIdx = EndLoc;

	TSet<int32> ClosedSet;
	TSet<int32> OpenSet;
	OpenSet.Add(StartIdx);

	TMap<int32, int32> CameFrom;
	TMap<int32, float> GScore;
	for (int32 i = 0; i < LocationsCopy.Num(); ++i) GScore.Add(i, FLT_MAX);
	GScore[StartIdx] = 0;
	TArray<int> SelectedLocations;

	TMap<int32, float> FScore;
	for (int32 i = 0; i < LocationsCopy.Num(); ++i)
		FScore.Add(i, FLT_MAX);
	FScore[StartIdx] = FindHeuristics(LocationsCopy[StartIdx].Location, LocationsCopy[EndIdx].Location);

	while (OpenSet.Num() > 0)
	{
		// Найти узел с минимальным FScore в open set
		int32 Current = -1;
		float MinF = FLT_MAX;
		for (int32 idx : OpenSet)
		{
			if (FScore[idx] < MinF)
			{
				MinF = FScore[idx];
				Current = idx;
			}
		}
		if (Current == EndIdx)
		{
			// Восстановить путь через CameFrom
			SelectedLocations.Empty();
			int Step = EndIdx;
			while (CameFrom.Contains(Step))
			{
				SelectedLocations.Insert(Step, 0);
				Step = CameFrom[Step];
			}
			SelectedLocations.Insert(StartIdx, 0);
			return SelectedLocations;
		}
		OpenSet.Remove(Current);
		ClosedSet.Add(Current);

		auto& CurDot = LocationsCopy[Current];
		for (int32 n = 0; n < CurDot.NeighbourLinearIndex.Num(); ++n)
		{
			int32 Neigh = CurDot.NeighbourLinearIndex[n];
			if (!CurDot.NeighbourAccessability[n]) continue;
			if (ClosedSet.Contains(Neigh)) continue;

			float TentativeG = GScore[Current]
				+ FindStepCost(CurDot.Location, LocationsCopy[Neigh].Location);

				if (!OpenSet.Contains(Neigh))
					OpenSet.Add(Neigh);
				else if (TentativeG >= GScore[Neigh])
					continue;

				CameFrom.Add(Neigh, Current);
				GScore[Neigh] = TentativeG;
				FScore[Neigh] = GScore[Neigh]
					+ FindHeuristics(LocationsCopy[Neigh].Location, LocationsCopy[EndIdx].Location);
		}
	}
	// Если сюда дошли — пути нет
	SelectedLocations.Empty();
	return SelectedLocations;
}

FORCEINLINE int ClampIndex(int idx, int maxIdx)
{
	return FMath::Clamp(idx, 0, maxIdx);
}

FORCEINLINE int NearestIndex1D(float Value, const TArray<float> Arr)
{
	int Best = 0;
	float BestD2 = FMath::Square(Value - Arr[0]);
	for (int i = 1; i < Arr.Num(); ++i)
	{
		float D2 = FMath::Square(Value - Arr[i]);
		if (D2 < BestD2)
		{
			BestD2 = D2;
			Best = i;
		}
	}
	return Best;
}


int ADroneFlyBounds::GetClosestLocation(FVector CurrentLocation)
{

	float best = FLT_MAX;
	int bestInt = 0;
	for (int i = 0; i < DotLocations.Num(); i++)
	{
		float bestTemp = FVector::DistSquared(DotLocations[i].Location, CurrentLocation);

		if (bestTemp < best)
		{
			best = bestTemp;
			bestInt = i;
		}
	}

	return bestInt;
}

void ADroneFlyBounds::CalcTheRoute_Implementation(FVector CurrentLocation, FVector EndLocation, APawn* Drone)
{

	TArray<FDotLocationInfo> LocationsCopy = DotLocations;
	TWeakObjectPtr<APawn> WeakDrone(Drone);

	int StartIndex = GetClosestLocation(CurrentLocation);
	int EndIndex = GetClosestLocation(EndLocation);

	Async(EAsyncExecution::ThreadPool, [this, LocationsCopy, WeakDrone, StartIndex, EndIndex, CurrentLocation, EndLocation]()
	{
		TArray<int> SelectedLocation = SelectBestLocation(StartIndex, EndIndex, LocationsCopy);

		TArray<FVector> RouteInLocations;
		for (int i = 0; i < SelectedLocation.Num(); i++)
		{
			RouteInLocations.Add(LocationsCopy[SelectedLocation[i]].Location);
		}
		RouteInLocations[0] = CurrentLocation;
		RouteInLocations[SelectedLocation.Num()-1] = EndLocation;
		
		Async(EAsyncExecution::TaskGraphMainThread, [WeakDrone, RouteInLocations]() 
		{
			if (APawn* ValidDrone = WeakDrone.Get())
			{
				IFlyingNavigationInterface::Execute_GetTheRouteInLocations(ValidDrone, RouteInLocations);
			}
		});


		// IFlyingNavigationInterface::Execute_GetTheRouteInLocations(WeakDrone, RouteInLocations);
	});

}

void ADroneFlyBounds::GetTheRouteInLocations_Implementation(const TArray<FVector>& RouteInLocations)
{
	//UE_LOG(LogTemp, Warning, TEXT("Бессмысленная реализация GetTheRouteInLocations вызвана. Количество точек маршрута: %d"), RouteInLocations.Num());
	//int jopa = RouteInLocations.Num();
}



TArray<float> ADroneFlyBounds::FillAxisArrays(float AxisMin, float AxisMax, float Interval)
{
	TArray<float> Out;
	while (AxisMin < AxisMax)
	{
		Out.Add(AxisMin);

		AxisMin += Interval;
	}

	return Out;
}
