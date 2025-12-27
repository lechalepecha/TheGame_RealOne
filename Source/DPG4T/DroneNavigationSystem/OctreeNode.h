// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeObject.h"
#include "OctreeNode.h"
/**
 * 
 */
class OctreeObject;
class OctreeNode;

class DPG4T_API OctreeNode// : public UObject
{
public:
	OctreeNode();
	OctreeNode(FBox bounds, float minNodeSize);
	~OctreeNode();

	TArray<OctreeObject> objects;

	int nextID;
	int id;

	void Divide(AActor* obj);
	FBox Bounds;
	
	TStaticArray<FBox, 8> childBounds;
	TArray<OctreeNode> children;

	bool isLeaf() { return children.IsEmpty(); };

	float minNodeSize;

private:
	void Divide(OctreeObject obj);
	void AddObj(OctreeObject octreeObj);
};
