/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "MRUtilityKitAnchorMesh.h"
#include "GameFramework/Pawn.h"
#include "OculusAnchorBPFunctionLibrary.h"

FMRUKAnchorMesh::FMRUKAnchorMesh()
{
}

// Helper function to check if a point is inside the circumcircle of a triangle
bool FMRUKAnchorMesh::IsInsideCircumcircle(const FTriangle& Triangle, const FVector2D& P) const
{
	const FVector2D& P1 = Nodes[Triangle.P1].Position;
	const FVector2D& P2 = Nodes[Triangle.P2].Position;
	const FVector2D& P3 = Nodes[Triangle.P3].Position;

	FVector2D A = P1 - P;
	FVector2D B = P2 - P;
	FVector2D C = P3 - P;

	float Det = A.SizeSquared() * FVector2D::CrossProduct(B, C) + B.SizeSquared() * FVector2D::CrossProduct(C, A) + C.SizeSquared() * FVector2D::CrossProduct(A, B);

	// Clockwise orientation indicates the point is inside the circumcircle
	return Det < 0;
}

// Function to find the closest triangle to a given point
FMRUKAnchorMesh::FTriangle FMRUKAnchorMesh::FindClosestTriangle(const FVector2D& TargetPoint, FVector& OutBarycentric) const
{
	float MinDistanceSquared = FLT_MAX;
	int32 ClosestTriangleIndex = -1;

	for (int32 i = 0; i < Triangles.Num(); ++i)
	{
		const FVector2D& P1 = Nodes[Triangles[i].P1].Position;
		const FVector2D& P2 = Nodes[Triangles[i].P2].Position;
		const FVector2D& P3 = Nodes[Triangles[i].P3].Position;

		FVector CurrentBarycentric = CalculateBarycentricCoordinates(P1, P2, P3, TargetPoint);

		FVector2D Pos = CurrentBarycentric.X * P1 + CurrentBarycentric.Y * P2 + CurrentBarycentric.Z * P3;

		float DistanceSquared = FVector2D::DistSquared(Pos, TargetPoint);

		if (DistanceSquared < MinDistanceSquared)
		{
			MinDistanceSquared = DistanceSquared;
			ClosestTriangleIndex = i;
			OutBarycentric = CurrentBarycentric;
		}
	}

	if (ClosestTriangleIndex == -1)
	{
		return {};
	}

	return Triangles[ClosestTriangleIndex];
}

// Helper function to add edge to the convex hull or remove it if it is already present
void FMRUKAnchorMesh::AddEdgeToConvexHull(TArray<FEdge>& Edges, FEdge NewEdge)
{
	// If the edge is already in the list then we are going to remove it
	for (int i = 0; i < Edges.Num(); ++i)
	{
		const auto& Edge = Edges[i];
		if ((Edge.P1 == NewEdge.P1 && Edge.P2 == NewEdge.P2) || (Edge.P1 == NewEdge.P2 && Edge.P2 == NewEdge.P1))
		{
			Edges.RemoveAt(i);
			return;
		}
	}
	// Otherwise add it
	Edges.Add(NewEdge);
}

// Function to calculate the barycentric coordinates of a point in a triangle
FVector FMRUKAnchorMesh::CalculateBarycentricCoordinates(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, const FVector2D& P)
{
	auto TotalArea = 0.5 * FVector2D::CrossProduct(P2 - P1, P3 - P1);

	// Calculate areas of sub-triangles formed by point P and the triangle vertices
	auto Area1 = 0.5 * FVector2D::CrossProduct(P2 - P, P3 - P);
	auto Area2 = 0.5 * FVector2D::CrossProduct(P - P1, P3 - P1);
	auto Area3 = TotalArea - Area1 - Area2;

	FVector Barycentric;
	// Calculate barycentric coordinates
	Barycentric.X = Area1 / TotalArea;
	Barycentric.Y = Area2 / TotalArea;
	Barycentric.Z = Area3 / TotalArea;

	if (Barycentric.X < 0)
	{
		auto Edge = P3 - P2;
		auto t = FVector2D::DotProduct(P - P2, Edge) / Edge.SizeSquared();
		t = FMath::Clamp(t, 0.0f, 1.0f);
		Barycentric.X = 0.0;
		Barycentric.Y = 1.0 - t;
		Barycentric.Z = t;
	}
	else if (Barycentric.Y < 0)
	{
		auto Edge = P1 - P3;
		auto t = FVector2D::DotProduct(P - P3, Edge) / Edge.SizeSquared();
		t = FMath::Clamp(t, 0.0f, 1.0f);
		Barycentric.X = t;
		Barycentric.Y = 0.0;
		Barycentric.Z = 1.0 - t;
	}
	else if (Barycentric.Z < 0)
	{
		auto Edge = P2 - P1;
		auto t = FVector2D::DotProduct(P - P1, Edge) / Edge.SizeSquared();
		t = FMath::Clamp(t, 0.0f, 1.0f);
		Barycentric.X = 1.0 - t;
		Barycentric.Y = t;
		Barycentric.Z = 0.0;
	}

	return Barycentric;
}

void FMRUKAnchorMesh::CreateMesh(const TArray<AMRUKAnchor*>& Anchors)
{
	Nodes.Empty();
	Triangles.Empty();

	Nodes.Reserve(Anchors.Num());
	FBox2D Bounds;
	for (const auto& Anchor : Anchors)
	{
		if (!Anchor)
		{
			continue;
		}
		const auto AnchorPosition = Anchor->GetActorLocation();
		FVector2D Position2D(AnchorPosition.X, AnchorPosition.Y);
		Nodes.Add(FNode{ Anchor, Position2D });
		Bounds += Position2D;
	}

	if (Nodes.IsEmpty())
	{
		return;
	}

	// Delaunay triangulation using Bowyer-Watson algorithm

	// Add a super-triangle that contains all points to ensure a convex hull
	float MaxSize = Bounds.GetSize().GetMax();
	auto Center = Bounds.GetCenter();

	FVector2D P1(Center.X - 20 * MaxSize, Center.Y - MaxSize);
	FVector2D P2(Center.X, Center.Y + 20 * MaxSize);
	FVector2D P3(Center.X + 20 * MaxSize, Center.Y - MaxSize);

	int32 NumPoints = Nodes.Num();

	Triangles.Add(FTriangle{ NumPoints, NumPoints + 1, NumPoints + 2 });

	Nodes.Add({ nullptr, P1 });
	Nodes.Add({ nullptr, P2 });
	Nodes.Add({ nullptr, P3 });

	// Incremental insertion of points
	TArray<FEdge> Edges;
	for (int32 i = 0; i < NumPoints; ++i)
	{
		Edges.Reset();
		for (int32 j = Triangles.Num() - 1; j >= 0; --j)
		{
			const auto& Triangle = Triangles[j];
			if (IsInsideCircumcircle(Triangle, Nodes[i].Position))
			{
				AddEdgeToConvexHull(Edges, FEdge{ Triangle.P1, Triangle.P2 });
				AddEdgeToConvexHull(Edges, FEdge{ Triangle.P2, Triangle.P3 });
				AddEdgeToConvexHull(Edges, FEdge{ Triangle.P3, Triangle.P1 });
				Triangles.RemoveAt(j);
			}
		}

		// Create new Triangles and update edges
		for (const FEdge& edge : Edges)
		{
			Triangles.Add(FTriangle{ edge.P1, edge.P2, i });
		}
	}

	// Remove the super-triangle nodes
	Nodes.SetNum(NumPoints);

	// Remove Triangles that include the super-triangle vertices
	Triangles.RemoveAll([NumPoints](const FTriangle& t) {
		return t.P1 >= NumPoints || t.P2 >= NumPoints || t.P3 >= NumPoints;
	});
}

void FMRUKAnchorMesh::UpdateWorldLock(APawn* Pawn, const FVector& HeadWorldPosition)
{
	if (Nodes.IsEmpty() || Triangles.IsEmpty())
	{
		return;
	}
	const auto& PawnTransform = Pawn->GetActorTransform();
	auto PawnInverseTransform = PawnTransform.Inverse();
	auto HeadPosition2D = FVector2D(HeadWorldPosition.X, HeadWorldPosition.Y);

	FVector Barycentric;

	FTriangle Triangle = FindClosestTriangle(HeadPosition2D, Barycentric);

	FVector WeightedOffset = FVector::ZeroVector;
	FVector2D WeightedYawVector(0.0f, 0.0f);
	
	for (const auto& [TriangleIndex, Weight] : 
		{ TTuple<int32, float>{ Triangle.P1, Barycentric.X }, 
		  TTuple<int32, float>{ Triangle.P2, Barycentric.Y }, 
		  TTuple<int32, float>{ Triangle.P3, Barycentric.Z } })
	{
		const auto& Anchor = Nodes[TriangleIndex].Anchor;

		FTransform AnchorTransform;
		if (Anchor->SpaceQueryResult.Space && UOculusAnchorBPFunctionLibrary::GetAnchorTransformByHandle(Anchor->SpaceQueryResult.Space, AnchorTransform))
		{
			// Convert the Anchor Transform back into tracking space. Ideally we would have a version of
			// UOculusXRAnchorBPFunctionLibrary::GetAnchorTransformByHandle
			// which does not transform from Tracking To World space so we don't need to undo it here.
			AnchorTransform *= PawnInverseTransform;

			const FTransform& Transform = Anchor->GetActorTransform();
			FTransform Adjustment = AnchorTransform.Inverse() * Transform;

			WeightedOffset += Adjustment.GetTranslation() * Weight;

			// Convert angles to 2D unit vectors and sum them up, weighted by their respective weights.
			float RadianAngle = FMath::DegreesToRadians(Adjustment.Rotator().Yaw);
			FVector2D AngleVector(FMath::Cos(RadianAngle), FMath::Sin(RadianAngle));
			WeightedYawVector += AngleVector * Weight;
		}
	}

	float WeightedYaw = FMath::RadiansToDegrees(FMath::Atan2(WeightedYawVector.Y, WeightedYawVector.X));
	Pawn->SetActorLocationAndRotation(WeightedOffset, FRotator(0.0, WeightedYaw, 0.0));
}
