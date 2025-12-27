// Fill out your copyright notice in the Description page of Project Settings.


#include "OctreeObject.h"

OctreeObject::OctreeObject()
{
}

OctreeObject::OctreeObject(AActor* obj)
{
	FVector Origin;
	FVector BoxExtent;
	obj->GetActorBounds(false, Origin, BoxExtent);

	FBox Box = FBox::BuildAABB(Origin, BoxExtent);

	Bounds = Box;
}

OctreeObject::~OctreeObject()
{
}

bool OctreeObject::Intersects(FBox BoundsToCheck)
{
	return Bounds.Intersect(BoundsToCheck);
}