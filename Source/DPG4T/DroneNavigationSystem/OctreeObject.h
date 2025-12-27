// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class DPG4T_API OctreeObject
{
public:
	OctreeObject();	
	OctreeObject(AActor* obj);

	~OctreeObject();

	FBox Bounds;

	bool Intersects(FBox BoundsToCheck);
};
