// Fill out your copyright notice in the Description page of Project Settings.

#include "DroneNavigationSystem/DroneFlyBounds.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

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

	SelectBestLocation();

	for (int i = 0; i < SelectedLocation.Num() - 1; i++)
	{
		DrawDebugLine(GetWorld(), DotLocations[SelectedLocation[i]].Location, DotLocations[SelectedLocation[i + 1]].Location, FColor::Red, false, 500.f);
	}
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

void ADroneFlyBounds::SelectBestLocation()
{
	int32 StartIdx = 0;
	int32 EndIdx = DotLocations.Num() - 1;

	// Инициализация
	TSet<int32> ClosedSet;
	TSet<int32> OpenSet;
	OpenSet.Add(StartIdx);

	TMap<int32, int32> CameFrom;
	TMap<int32, float> GScore;
	for (int32 i = 0; i < DotLocations.Num(); ++i) GScore.Add(i, FLT_MAX);
	GScore[StartIdx] = 0;

	TMap<int32, float> FScore;
	for (int32 i = 0; i < DotLocations.Num(); ++i)
		FScore.Add(i, FLT_MAX);
	FScore[StartIdx] = FindHeuristics(DotLocations[StartIdx].Location, DotLocations[EndIdx].Location);

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
			SelectedLocation.Empty();
			int Step = EndIdx;
			while (CameFrom.Contains(Step))
			{
				SelectedLocation.Insert(Step, 0);
				Step = CameFrom[Step];
			}
			SelectedLocation.Insert(StartIdx, 0);
			return;
		}
		OpenSet.Remove(Current);
		ClosedSet.Add(Current);

		auto& CurDot = DotLocations[Current];
		for (int32 n = 0; n < CurDot.NeighbourLinearIndex.Num(); ++n)
		{
			int32 Neigh = CurDot.NeighbourLinearIndex[n];
			if (!CurDot.NeighbourAccessability[n]) continue;
			if (ClosedSet.Contains(Neigh)) continue;

			float TentativeG = GScore[Current]
				+ FindStepCost(CurDot.Location, DotLocations[Neigh].Location);

				if (!OpenSet.Contains(Neigh))
					OpenSet.Add(Neigh);
				else if (TentativeG >= GScore[Neigh])
					continue;

				CameFrom.Add(Neigh, Current);
				GScore[Neigh] = TentativeG;
				FScore[Neigh] = GScore[Neigh]
					+ FindHeuristics(DotLocations[Neigh].Location, DotLocations[EndIdx].Location);
		}
	}
	// Если сюда дошли — пути нет
	//SelectedLocation.Empty();
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
