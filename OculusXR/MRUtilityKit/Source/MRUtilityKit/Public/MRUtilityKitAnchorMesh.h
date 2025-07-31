/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "MRUtilityKit.h"
#include "MRUtilityKitAnchor.h"

class MRUTILITYKIT_API FMRUKAnchorMesh
{
public:
	FMRUKAnchorMesh();

	void CreateMesh(const TArray<AMRUKAnchor*>& Anchors);

	void UpdateWorldLock(APawn* Pawn, const FVector& HeadWorldPosition);

private:
	friend class FMRUKSpec;

	// Node structure to represent nodes in the mesh
	struct FNode
	{
		AMRUKAnchor* Anchor;
		FVector2D Position;
	};

	// Edge structure to represent edges between points
	struct FEdge
	{
		int32 P1; // Index of the first point
		int32 P2; // Index of the second point
	};

	// Triangle structure to represent triangles formed by points
	struct FTriangle
	{
		int32 P1; // Index of the first point
		int32 P2; // Index of the second point
		int32 P3; // Index of the third point
	};

	bool IsInsideCircumcircle(const FTriangle& Triangle, const FVector2D& P) const;
	FTriangle FindClosestTriangle(const FVector2D& TargetPoint, FVector& OutBarycentric) const;
	static void AddEdgeToConvexHull(TArray<FEdge>& Edges, FEdge NewEdge);
	static FVector CalculateBarycentricCoordinates(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, const FVector2D& P);

	TArray<FNode> Nodes;
	TArray<FTriangle> Triangles;
};
