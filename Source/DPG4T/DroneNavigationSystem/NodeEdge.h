// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Node.h"

/**
 * 
 */

class Node;

class DPG4T_API NodeEdge
{
public:
	NodeEdge();
	~NodeEdge();

	Node a, b;

	NodeEdge(Node a, Node b);

	bool Equal();
};
