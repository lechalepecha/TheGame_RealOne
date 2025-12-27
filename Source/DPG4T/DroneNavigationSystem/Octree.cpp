// Fill out your copyright notice in the Description page of Project Settings.


#include "Octree.h"

Octree::Octree(TArray<AActor*> WorldObjects, float minNodeSize)
{
	
	CalculateBounds(WorldObjects);

	CreateTree(WorldObjects, minNodeSize);
	
	GetEmptyLeafs(root);
}

Octree::Octree()
{
}

void Octree::CreateTree(TArray<AActor*> WorldObjects, float minNodeSize) 
{
	/*Async(EAsyncExecution::ThreadPool, [this, minNodeSize]()
		{*/
			root = OctreeNode(Bounds, minNodeSize);
		//});
	//ParallelFor(WorldObjects.Num(), [&](int32 Index) { root.Divide(WorldObjects[Index]); });
	for (AActor* obj : WorldObjects)
	{
		root.Divide(obj);
	}
}

Octree::~Octree()
{
}

void Octree::CalculateBounds(TArray<AActor*> WorldObjects)
{
	FBox TotalBox;
	for (AActor* WorldObject : WorldObjects)
	{
		if (WorldObject != nullptr)
		{
			FVector Origin;
			FVector BoxExtent;
			WorldObject->GetActorBounds(false, Origin, BoxExtent);


			FBox Box = FBox::BuildAABB(Origin, BoxExtent);
			TotalBox += Box;
		}
	}
	TotalBox = FBox::BuildAABB(TotalBox.GetCenter(), TotalBox.GetExtent());
	Bounds = TotalBox;
}

void Octree::GetEmptyLeafs(OctreeNode node)
{

}
