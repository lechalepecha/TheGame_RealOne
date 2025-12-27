// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeNode.h"
/**
 * 
 */
class OctreeNode;

class DPG4T_API Octree
{
public:
	Octree(TArray<AActor*> WorldObjects, float minNodeSize);
	Octree();
	~Octree();

	void CreateTree(TArray<AActor*> WorldObjects, float minNodeSize);

	FBox Bounds;
	//float minNodeSize;

	OctreeNode root;
	TArray<OctreeNode> emptyLeafs;


private:
	void CalculateBounds(TArray<AActor*> WorldObjects);
	void GetEmptyLeafs(OctreeNode node);
};
