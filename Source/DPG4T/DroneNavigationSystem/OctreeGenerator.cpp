// Fill out your copyright notice in the Description page of Project Settings.


#include "OctreeGenerator.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AOctreeGenerator::AOctreeGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AOctreeGenerator::BeginPlay()
{
	Super::BeginPlay();

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("Nodable"), WorldObjects);
	
	Oc = Octree(WorldObjects, minNodeSize);
	
	if (!Oc.root.isLeaf())
	{
		UE_LOG(LogTemp, Error, TEXT("Entered beginplay of OctreeGen"));

		Rekur(Oc.root);
	}

	DrawDebugBox(GetWorld(), Oc.Bounds.GetCenter(), Oc.Bounds.GetExtent(), FColor::Green, false, 15.f);
}



// Called every frame
void AOctreeGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AOctreeGenerator::Rekur(OctreeNode node)
{

	/*ParallelFor(node.children.Num(), [&](int32 Idx)
		{
			DrawDebugBox(GetWorld(), node.Bounds.GetCenter(), node.Bounds.GetExtent() * 0.9, FColor::Red, false, 15.f);

			if (!node.children[Idx].isLeaf())
			{
				Rekur(node.children[Idx]);
			}
		});*/
	for (OctreeNode child : node.children)
	{
		// UE_LOG(LogTemplateCharacter, Error, TEXT("Entered function rekur"));
		//DrawDebugBox(GetWorld(), node.Bounds.GetCenter(), node.Bounds.GetExtent() * 0.9, FColor::Red, false, 15.f);

		if (!child.isLeaf())
		{

			//OctreeNode Node = node;
			Async(EAsyncExecution::ThreadPool, [this, child]()
				{
					Rekur(child);
				});
		}
	}
}