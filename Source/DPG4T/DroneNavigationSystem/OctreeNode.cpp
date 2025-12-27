// Fill out your copyright notice in the Description page of Project Settings.


#include "OctreeNode.h"

OctreeNode::OctreeNode()
{

}

OctreeNode::OctreeNode(FBox bounds, float minNodeSize)
{
	id = nextID++;
	this->Bounds = bounds;
	this->minNodeSize = minNodeSize;

	FVector newSize = Bounds.GetSize()*0.25f; //halved size
	FVector centerOffset = Bounds.GetSize() * 0.25f; //quarter offset
	FVector parentCenter = Bounds.GetCenter();

	ParallelFor(8, [&](int32 Idx)
		{
			FVector childCenter = parentCenter;

			childCenter.X += centerOffset.X * ((Idx & 1) == 0 ? 1 : -1);
			childCenter.Y += centerOffset.Y * ((Idx & 2) == 0 ? 1 : -1);
			childCenter.Z += centerOffset.Z * ((Idx & 4) == 0 ? 1 : -1);

			childBounds[Idx] = FBox::BuildAABB(childCenter, newSize);
		});

	/*for (int i = 0; i < 8; i++)
	{
		FVector childCenter = parentCenter;

		childCenter.X += centerOffset.X * ((i & 1) == 0 ? 1 : -1);
		childCenter.Y += centerOffset.Y * ((i & 2) == 0 ? 1 : -1);
		childCenter.Z += centerOffset.Z * ((i & 4) == 0 ? 1 : -1);

		childBounds[i] = FBox::BuildAABB(childCenter, newSize);

		
	}*/
}

OctreeNode::~OctreeNode()
{
}

void OctreeNode::Divide(AActor* obj)
{
	
		Divide(OctreeObject(obj));
	
}

void OctreeNode::Divide(OctreeObject obj)
{
	if (Bounds.GetExtent().X <= minNodeSize)
	{
		AddObj(obj);
		return;
	}
	
	bool intersectedChild = false;

	/*ParallelFor(8, [&](int32 Idx)
		{
			children.Add(OctreeNode(childBounds[Idx], minNodeSize));

			if (obj.Intersects(childBounds[Idx]))
			{
				children[Idx].Divide(obj);
				intersectedChild = true;
			}
		});*/
	for (int i = 0; i < 8; i++)
	{
		children.Add(OctreeNode(childBounds[i], minNodeSize));

		if (obj.Intersects(childBounds[i]))
		{
			children[i].Divide(obj);
			intersectedChild = true;
		}
	}
	
	if (!intersectedChild)
	{
		AddObj(obj);
	}
}

void OctreeNode::AddObj(OctreeObject octreeObj)
{
	objects.Add(octreeObj);
}