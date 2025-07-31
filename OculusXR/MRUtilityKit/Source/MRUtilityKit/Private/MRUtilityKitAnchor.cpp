/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "MRUtilityKitAnchor.h"
#include "MRUtilityKit.h"
#include "MRUtilityKitSubsystem.h"
#include "MRUtilityKitSerializationHelpers.h"
#include "MRUtilityKitSeatsComponent.h"
#include "MRUtilityKitRoom.h"
#include "OculusAnchorTypes.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "MRUKAnchor"

// #pragma optimize("", off)

namespace
{
	bool IsConvex(const FVector& PrevPoint, const FVector& CurrPoint, const FVector& NextPoint)
	{
		FVector Edge1 = PrevPoint - CurrPoint;
		FVector Edge2 = NextPoint - CurrPoint;

		float CrossProductZ = Edge1.Y * Edge2.Z - Edge1.Z * Edge2.Y;

		return CrossProductZ <= 0;
	}

	bool PointInTriangle(const FVector& A, const FVector& B, const FVector& C, const FVector& P)
	{
		FVector AB = B - A;
		FVector BC = C - B;
		FVector CA = A - C;

		FVector AP = P - A;
		FVector BP = P - B;
		FVector CP = P - C;

		float CrossProductZ1 = AB.Y * AP.Z - AB.Z * AP.Y;
		float CrossProductZ2 = BC.Y * BP.Z - BC.Z * BP.Y;
		float CrossProductZ3 = CA.Y * CP.Z - CA.Z * CP.Y;

		return (CrossProductZ1 >= 0) && (CrossProductZ2 >= 0) && (CrossProductZ3 >= 0);
	}

	bool IsEar(const TArray<FVector>& Vertices, const TArray<int32>& Indices, int32 PrevIndex, int32 CurrIndex, int32 NextIndex)
	{
		int32 NumPoints = Indices.Num();

		const FVector& PrevPoint = Vertices[PrevIndex];
		const FVector& CurrPoint = Vertices[CurrIndex];
		const FVector& NextPoint = Vertices[NextIndex];

		for (int32 i = 0; i < NumPoints; ++i)
		{
			int32 Index = Indices[i];
			if (Index != PrevIndex && Index != CurrIndex && Index != NextIndex)
			{
				FVector TestPoint = Vertices[Index];

				if (PointInTriangle(PrevPoint, CurrPoint, NextPoint, TestPoint))
				{
					return false;
				}
			}
		}

		return true;
	}

	// Ear clipping algorithm to triangulate the boundary
	TArray<int32> TriangulatePoints(const TArray<FVector>& Vertices)
	{
		TArray<int32> Indices;
		TArray<int32> Triangles;
		const int32 NumTriangles = FMath::Max(Vertices.Num() - 2, 0);
		Triangles.Reserve(3 * NumTriangles);

		Indices.Reserve(Vertices.Num());
		for (int i = 0; i < Vertices.Num(); ++i)
		{
			Indices.Push(i);
		}

		while (Indices.Num() > 3)
		{
			bool EarFound = false;

			for (int32 i = 0; i < Indices.Num(); ++i)
			{
				int32 PrevIndex = Indices[(i + Indices.Num() - 1) % Indices.Num()];
				int32 CurrIndex = Indices[i];
				int32 NextIndex = Indices[(i + 1) % Indices.Num()];

				if (IsConvex(Vertices[PrevIndex], Vertices[CurrIndex], Vertices[NextIndex]) && IsEar(Vertices, Indices, PrevIndex, CurrIndex, NextIndex))
				{
					Triangles.Add(PrevIndex);
					Triangles.Add(CurrIndex);
					Triangles.Add(NextIndex);

					Indices.RemoveAt(i);
					EarFound = true;
					break;
				}
			}

			if (!EarFound)
			{
				UE_LOG(LogMRUK, Error, TEXT("Failed to triangulate points."));
				break;
			}
		}

		if (Indices.Num() == 3)
		{
			Triangles.Append(Indices);
		}

		return Triangles;
	}

	/**
	 * In Unreal Engine, scale is always applied in the local space to avoid any skew. This means that if you have a component which has a 90 degree rotation and is scaled,
	 * or any of its children are scaled then the scale axes will not be applied as you would expect. This is can make it very awkward to work with when trying to scale the
	 * actors to fit within the scene volumes. To work around this problem, this code will attempt to adjust the scale axes recursively to match the expected behaviour.
	 * This will only work reliably if the rotations involved are 90 degrees, if they are not then it will pick the closest axis.
	 */
	void SetScaleRecursivelyAdjustingForRotation(USceneComponent* SceneComponent, const FVector& UnRotatedScale, const FQuat& AccumulatedRotation = FQuat::Identity, const FVector& ParentReciprocalScale = FVector::OneVector)
	{
		if (SceneComponent)
		{
			const auto RelativeRotation = SceneComponent->GetRelativeRotationCache().RotatorToQuat(SceneComponent->GetRelativeRotation());
			const auto Rotation = AccumulatedRotation * RelativeRotation;
			FVector RotatedXAxis = Rotation.GetAxisX();
			FVector RotatedYAxis = Rotation.GetAxisY();
			FVector RotatedZAxis = Rotation.GetAxisZ();
			FVector RotatedScale;
			if (FMath::Abs(RotatedXAxis.X) >= UE_INV_SQRT_2)
			{
				RotatedScale.X = UnRotatedScale.X;
			}
			else if (FMath::Abs(RotatedXAxis.Y) >= UE_INV_SQRT_2)
			{
				RotatedScale.X = UnRotatedScale.Y;
			}
			else
			{
				RotatedScale.X = UnRotatedScale.Z;
			}

			if (FMath::Abs(RotatedYAxis.X) >= UE_INV_SQRT_2)
			{
				RotatedScale.Y = UnRotatedScale.X;
			}
			else if (FMath::Abs(RotatedYAxis.Y) >= UE_INV_SQRT_2)
			{
				RotatedScale.Y = UnRotatedScale.Y;
			}
			else
			{
				RotatedScale.Y = UnRotatedScale.Z;
			}

			if (FMath::Abs(RotatedZAxis.X) >= UE_INV_SQRT_2)
			{
				RotatedScale.Z = UnRotatedScale.X;
			}
			else if (FMath::Abs(RotatedZAxis.Y) >= UE_INV_SQRT_2)
			{
				RotatedScale.Z = UnRotatedScale.Y;
			}
			else
			{
				RotatedScale.Z = UnRotatedScale.Z;
			}

			FVector OldScale = SceneComponent->GetRelativeScale3D();
			FVector NewScale = ParentReciprocalScale * RotatedScale * OldScale;
			SceneComponent->SetRelativeScale3D(NewScale);
			FVector NewParentReciprocalScale = ParentReciprocalScale * (OldScale / NewScale);
			for (auto Child : SceneComponent->GetAttachChildren())
			{
				if (Child)
				{
					SetScaleRecursivelyAdjustingForRotation(Child, UnRotatedScale, Rotation, NewParentReciprocalScale);
				}
			}
		}
	}
} // namespace

AMRUKAnchor::AMRUKAnchor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create a scene component as root so we can attach spawned actors to it
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
}

bool AMRUKAnchor::LoadFromData(UMRUKAnchorData* AnchorData)
{
	check(AnchorData);

	bool Changed = false;

	auto Seat = GetComponentByClass<UMRUKSeatsComponent>();
	if (Seat && !HasLabel(FMRUKLabels::Couch))
	{
		Seat->UnregisterComponent();
		Seat->DestroyComponent();
		Changed = true;
	}

	SpaceQueryResult = AnchorData->SpaceQuery;

	SetActorTransform(AnchorData->Transform, false, nullptr, ETeleportType::ResetPhysics);
	auto NewSemanticClassifications = AnchorData->SemanticClassifications;
	if (NewSemanticClassifications != SemanticClassifications)
	{
		Changed = true;
	}
	SemanticClassifications = NewSemanticClassifications;

	FString Semantics = FString::Join(SemanticClassifications, TEXT("-"));
	UE_LOG(LogMRUK, Log, TEXT("SpatialAnchor label is %s"), *Semantics);

	if (PlaneBounds != AnchorData->PlaneBounds)
	{
		Changed = true;
	}
	PlaneBounds = AnchorData->PlaneBounds;
	PlaneBoundary2D = AnchorData->PlaneBoundary2D;

	if (VolumeBounds == AnchorData->VolumeBounds)
	{
		Changed = false;
	}
	VolumeBounds = AnchorData->VolumeBounds;

	if (Changed)
	{
		if (ProceduralMeshComponent)
		{
			ProceduralMeshComponent->UnregisterComponent();
			ProceduralMeshComponent->DestroyComponent();
			ProceduralMeshComponent = nullptr;
		}

		if (CachedMesh.IsSet())
		{
			CachedMesh.GetValue().Clear();
		}
	}

	return Changed;
}

bool AMRUKAnchor::IsPositionInBoundary(const FVector2D& Position)
{
	if (PlaneBoundary2D.IsEmpty())
	{
		return false;
	}

	int Intersections = 0;

	for (int i = 1; i <= PlaneBoundary2D.Num(); i++)
	{
		FVector2D P1 = PlaneBoundary2D[i - 1];
		FVector2D P2 = PlaneBoundary2D[i % PlaneBoundary2D.Num()];
		if (Position.Y > FMath::Min(P1.Y, P2.Y) && Position.Y <= FMath::Max(P1.Y, P2.Y))
		{
			if (Position.X <= FMath::Max(P1.X, P2.X))
			{
				if (P1.Y != P2.Y)
				{
					auto Frac = (Position.Y - P1.Y) / (P2.Y - P1.Y);
					auto XIntersection = P1.X + Frac * (P2.X - P1.X);
					if (P1.X == P2.X || Position.X <= XIntersection)
					{
						Intersections++;
					}
				}
			}
		}
	}

	return Intersections % 2 == 1;
}

FVector AMRUKAnchor::GenerateRandomPositionOnPlane()
{
	return GenerateRandomPositionOnPlaneFromStream(FRandomStream(NAME_None));
}

FVector AMRUKAnchor::GenerateRandomPositionOnPlaneFromStream(const FRandomStream& RandomStream)
{
	if (PlaneBoundary2D.IsEmpty())
	{
		return FVector::ZeroVector;
	}

	// Cache the mesh so that if the function is called multiple times it will re-use the previously triangulated mesh.
	if (!CachedMesh.IsSet())
	{
		TriangulatedMeshCache Mesh;
		const int32 NumVertices = PlaneBoundary2D.Num();
		Mesh.Vertices.Reserve(NumVertices);

		for (int i = 0; i < PlaneBoundary2D.Num(); ++i)
		{
			auto Vertex = PlaneBoundary2D[i];
			Mesh.Vertices.Push(FVector(0, Vertex.X, Vertex.Y));
		}
		Mesh.Triangles = TriangulatePoints(Mesh.Vertices);

		// Compute the area of each triangle and the total surface area of the mesh
		Mesh.Areas.Reserve(Mesh.Triangles.Num() / 3);
		Mesh.TotalArea = 0.0f;
		for (int i = 0; i < Mesh.Triangles.Num(); i += 3)
		{
			auto i0 = Mesh.Triangles[i];
			auto i1 = Mesh.Triangles[i + 1];
			auto i2 = Mesh.Triangles[i + 2];
			auto V0 = Mesh.Vertices[i0];
			auto V1 = Mesh.Vertices[i1];
			auto V2 = Mesh.Vertices[i2];
			auto Cross = FVector::CrossProduct(V1 - V0, V2 - V0);
			float Area = Cross.Size() * 0.5f;
			Mesh.TotalArea += Area;
			Mesh.Areas.Add(Area);
		}
		CachedMesh.Emplace(MoveTemp(Mesh));
	}

	const auto& Mesh = CachedMesh.GetValue();

	// Pick a random triangle weighted by surface area (triangles with larger surface
	// area have more chance of being chosen)
	auto Rand = RandomStream.FRandRange(0.0f, Mesh.TotalArea);
	int TriangleIndex = 0;
	for (; TriangleIndex < Mesh.Areas.Num() - 1; ++TriangleIndex)
	{
		Rand -= Mesh.Areas[TriangleIndex];
		if (Rand <= 0.0f)
		{
			break;
		}
	}

	// Get the vertices of the chosen triangle
	auto i0 = Mesh.Triangles[TriangleIndex * 3];
	auto i1 = Mesh.Triangles[TriangleIndex * 3 + 1];
	auto i2 = Mesh.Triangles[TriangleIndex * 3 + 2];
	auto V0 = Mesh.Vertices[i0];
	auto V1 = Mesh.Vertices[i1];
	auto V2 = Mesh.Vertices[i2];

	// Calculate a random point on that triangle
	float U = RandomStream.FRandRange(0.0f, 1.0f);
	float V = RandomStream.FRandRange(0.0f, 1.0f);
	if (U + V > 1.0f)
	{
		if (U > V)
		{
			U = 1.0f - U;
		}
		else
		{
			V = 1.0f - V;
		}
	}
	return V0 + U * (V1 - V0) + V * (V2 - V0);
}

bool AMRUKAnchor::Raycast(const FVector& Origin, const FVector& Direction, float MaxDist, FMRUKHit& OutHit)
{
	// If this anchor is the global mesh test against it
	if (this == Room->GlobalMeshAnchor)
	{
		FHitResult GlobalMeshOutHit{};
		float Dist = MaxDist;
		if (MaxDist <= 0.0)
		{
			Dist = 100 * 1024; // 1024 m should cover every scene
		}
		if (ActorLineTraceSingle(GlobalMeshOutHit, Origin, Origin + Direction * Dist, ECollisionChannel::ECC_WorldDynamic, FCollisionQueryParams::DefaultQueryParam))
		{
			OutHit.HitPosition = GlobalMeshOutHit.Location;
			OutHit.HitNormal = GlobalMeshOutHit.Normal;
			OutHit.HitDistance = GlobalMeshOutHit.Distance;
			return true;
		}
		return false;
	}

	auto Transform = GetTransform();
	// Transform the ray into local space
	auto InverseTransform = Transform.Inverse();
	const auto OriginLocal = InverseTransform.TransformPositionNoScale(Origin);
	const auto DirectionLocal = InverseTransform.TransformVectorNoScale(Direction);
	FRay LocalRay = FRay(OriginLocal, DirectionLocal);
	bool FoundHit = false;

	// If this anchor has a plane, hit test against it
	if (PlaneBounds.bIsValid && RayCastPlane(LocalRay, MaxDist, OutHit))
	{
		// Update max dist for the volume raycast
		MaxDist = OutHit.HitDistance;
		FoundHit = true;
	}
	// If this anchor has a volume, hit test against it
	if (VolumeBounds.IsValid && RayCastVolume(LocalRay, MaxDist, OutHit))
	{
		MaxDist = OutHit.HitDistance;
		FoundHit = true;
	}

	return FoundHit;
}

bool AMRUKAnchor::RaycastAll(const FVector& Origin, const FVector& Direction, float MaxDist, TArray<FMRUKHit>& OutHits)
{
	if (this == Room->GlobalMeshAnchor)
	{
		FHitResult GlobalMeshOutHit{};
		float Dist = MaxDist;
		if (MaxDist <= 0.0)
		{
			Dist = 100 * 1024; // 1024 m should cover every scene
		}
		if (ActorLineTraceSingle(GlobalMeshOutHit, Origin, Origin + Direction * Dist, ECollisionChannel::ECC_WorldDynamic, FCollisionQueryParams::DefaultQueryParam))
		{
			FMRUKHit Hit{};
			Hit.HitPosition = GlobalMeshOutHit.Location;
			Hit.HitNormal = GlobalMeshOutHit.Normal;
			Hit.HitDistance = GlobalMeshOutHit.Distance;
			OutHits.Push(Hit);
			return true;
		}
		return false;
	}

	auto Transform = GetTransform();
	// Transform the ray into local space
	auto InverseTransform = Transform.Inverse();
	const auto OriginLocal = InverseTransform.TransformPositionNoScale(Origin);
	const auto DirectionLocal = InverseTransform.TransformVectorNoScale(Direction);
	FRay LocalRay = FRay(OriginLocal, DirectionLocal);
	bool FoundHit = false;
	// If this anchor has a plane, hit test against it
	FMRUKHit Hit;
	if (PlaneBounds.bIsValid && RayCastPlane(LocalRay, MaxDist, Hit))
	{
		OutHits.Push(Hit);
		FoundHit = true;
	}
	// If this anchor has a volume, hit test against it
	if (VolumeBounds.IsValid && RayCastVolume(LocalRay, MaxDist, Hit))
	{
		OutHits.Push(Hit);
		FoundHit = true;
	}

	return FoundHit;
}

void AMRUKAnchor::AttachProceduralMesh(bool GenerateCollision, UMaterialInterface* ProceduralMaterial)
{
	AttachProceduralMesh({}, GenerateCollision, ProceduralMaterial);
}

void AMRUKAnchor::AttachProceduralMesh(TArray<FMRUKPlaneUV> PlaneUVAdjustments, bool GenerateCollision, UMaterialInterface* ProceduralMaterial)
{
	if (ProceduralMeshComponent)
	{
		// Procedural mesh already attached
		return;
	}

	ProceduralMeshComponent = NewObject<UProceduralMeshComponent>(this, TEXT("ProceduralMesh"));
	ProceduralMeshComponent->SetupAttachment(RootComponent);
	ProceduralMeshComponent->RegisterComponent();

	GenerateProceduralAnchorMesh(*ProceduralMeshComponent, PlaneUVAdjustments, false, GenerateCollision);

	for (int32 SectionIndex = 0; SectionIndex < ProceduralMeshComponent->GetNumSections(); ++SectionIndex)
	{
		ProceduralMeshComponent->SetMaterial(SectionIndex, ProceduralMaterial);
	}
}

void AMRUKAnchor::GenerateProceduralAnchorMesh(UProceduralMeshComponent& ProceduralMesh, const TArray<FMRUKPlaneUV>& PlaneUVAdjustments, bool PreferVolume, bool GenerateCollision, double Offset)
{
	int SectionIndex = 0;
	if (VolumeBounds.IsValid)
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;	   // Currently unused
		TArray<FProcMeshTangent> Tangents; // Currently unused
		constexpr int32 NumVertices = 24;
		constexpr int32 NumTriangles = 12;
		Vertices.Reserve(NumVertices);
		Triangles.Reserve(3 * NumTriangles);
		Normals.Reserve(NumVertices);
		UVs.Reserve(NumVertices);

		FBox VolumeBoundsOffset(VolumeBounds.Min - Offset, VolumeBounds.Max + Offset);
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				FVector Normal = FVector::ZeroVector;
				if (j == 0)
				{
					Normal[i] = -1.0f;
				}
				else
				{
					Normal[i] = 1.0f;
				}
				auto BaseIndex = Vertices.Num();
				FVector Vertex;
				Vertex[i] = VolumeBoundsOffset[j][i];
				for (int k = 0; k < 2; k++)
				{
					for (int l = 0; l < 2; l++)
					{
						Vertex[(i + 1) % 3] = VolumeBoundsOffset[k][(i + 1) % 3];
						Vertex[(i + 2) % 3] = VolumeBoundsOffset[l][(i + 2) % 3];
						Vertices.Push(Vertex);
						Normals.Push(Normal);
						// The 4 side faces of the cube should have their 0, 0 at the top left corner
						// when viewed from the outside.
						// The top face should have UVs that are consistent with planes to avoid Z fighting
						// in case a plane and volume overlap (e.g. in the case of the desk).
						FVector2D UV;
						switch (i)
						{
							case 0:
								UV = FVector2D(1 - k, 1 - l);
								break;
							case 1:
								UV = FVector2D(k, l);
								break;
							case 2:
								UV = FVector2D(1 - l, k);
								break;
						}
						if (j == 0)
						{
							UV.X = 1 - UV.X;
						}
						UVs.Push(UV);
					}
				}
				if (j == 1)
				{
					Triangles.Push(BaseIndex);
					Triangles.Push(BaseIndex + 1);
					Triangles.Push(BaseIndex + 2);
					Triangles.Push(BaseIndex + 2);
					Triangles.Push(BaseIndex + 1);
					Triangles.Push(BaseIndex + 3);
				}
				else
				{
					Triangles.Push(BaseIndex);
					Triangles.Push(BaseIndex + 2);
					Triangles.Push(BaseIndex + 1);
					Triangles.Push(BaseIndex + 1);
					Triangles.Push(BaseIndex + 2);
					Triangles.Push(BaseIndex + 3);
				}
			}
		}
		ProceduralMesh.CreateMeshSection_LinearColor(SectionIndex++, Vertices, Triangles, Normals, UVs, Colors, Tangents, GenerateCollision);
	}
	if (PlaneBounds.bIsValid && !(VolumeBounds.IsValid && PreferVolume))
	{
		TArray<FVector> Vertices;
		TArray<FVector> Normals;
		TArray<FVector2D> UV0s;
		TArray<FVector2D> UV1s;
		TArray<FVector2D> UV2s;
		TArray<FVector2D> UV3s;
		TArray<FLinearColor> Colors; // Currently unused
		TArray<FProcMeshTangent> Tangents;
		const int32 NumVertices = PlaneBoundary2D.Num();
		Vertices.Reserve(NumVertices);
		Normals.Reserve(NumVertices);
		UV0s.Reserve(NumVertices);
		UV1s.Reserve(NumVertices);
		UV2s.Reserve(NumVertices);
		UV3s.Reserve(NumVertices);
		Tangents.Reserve(NumVertices);

		static const FVector Normal = -FVector::XAxisVector;
		const FVector NormalOffset = Normal * Offset;
		auto BoundsSize = PlaneBounds.GetSize();
		for (const auto& PlaneBoundaryVertex : PlaneBoundary2D)
		{
			const FVector Vertex = FVector(0, PlaneBoundaryVertex.X, PlaneBoundaryVertex.Y) + NormalOffset;
			Vertices.Push(Vertex);
			Normals.Push(Normal);
			Tangents.Push(FProcMeshTangent(-FVector::YAxisVector, false));
			auto U = (PlaneBoundaryVertex.X - PlaneBounds.Min.X) / BoundsSize.X;
			auto V = 1 - (PlaneBoundaryVertex.Y - PlaneBounds.Min.Y) / BoundsSize.Y;
			if (PlaneUVAdjustments.Num() == 0)
			{
				UV0s.Push(FVector2D(U, V));
			}
			if (PlaneUVAdjustments.Num() >= 1)
			{
				UV0s.Push(FVector2D(U, V) * PlaneUVAdjustments[0].Scale + PlaneUVAdjustments[0].Offset);
			}
			if (PlaneUVAdjustments.Num() >= 2)
			{
				UV1s.Push(FVector2D(U, V) * PlaneUVAdjustments[1].Scale + PlaneUVAdjustments[1].Offset);
			}
			if (PlaneUVAdjustments.Num() >= 3)
			{
				UV2s.Push(FVector2D(U, V) * PlaneUVAdjustments[2].Scale + PlaneUVAdjustments[2].Offset);
			}
			if (PlaneUVAdjustments.Num() >= 4)
			{
				UV3s.Push(FVector2D(U, V) * PlaneUVAdjustments[3].Scale + PlaneUVAdjustments[3].Offset);
			}
		}
		TArray<int32> Triangles = TriangulatePoints(Vertices);
		ProceduralMesh.CreateMeshSection_LinearColor(SectionIndex++, Vertices, Triangles, Normals, UV0s, UV1s, UV2s, UV3s, Colors, Tangents, GenerateCollision);
	}
}

bool AMRUKAnchor::HasLabel(const FString& Label) const
{
	return SemanticClassifications.Contains(Label);
}

bool AMRUKAnchor::HasAnyLabel(const TArray<FString>& Labels) const
{
	for (const auto& Label : Labels)
	{
		if (HasLabel(Label))
		{
			return true;
		}
	}
	return false;
}

bool AMRUKAnchor::PassesLabelFilter(const FMRUKLabelFilter& LabelFilter) const
{
	return LabelFilter.PassesFilter(SemanticClassifications);
}

float AMRUKAnchor::GetClosestSurfacePosition(const FVector& TestPosition, FVector& OutSurfacePosition)
{
	const auto& Transform = GetActorTransform();
	const auto TestPositionLocal = Transform.InverseTransformPosition(TestPosition);

	double ClosestDistance = DBL_MAX;
	FVector ClosestPoint = FVector::ZeroVector;

	if (PlaneBounds.bIsValid)
	{
		const auto BestPoint2D = PlaneBounds.GetClosestPointTo(FVector2D(TestPositionLocal.Y, TestPositionLocal.Z));
		FVector BestPoint(0.0, BestPoint2D.X, BestPoint2D.Y);
		const auto Distance = FVector::Distance(BestPoint, TestPositionLocal);
		if (Distance < ClosestDistance)
		{
			ClosestPoint = BestPoint;
			ClosestDistance = Distance;
		}
	}
	if (VolumeBounds.IsValid)
	{
		const auto BestPoint = VolumeBounds.GetClosestPointTo(TestPositionLocal);
		const auto Distance = FVector::Distance(BestPoint, TestPositionLocal);
		if (Distance < ClosestDistance)
		{
			ClosestPoint = BestPoint;
			ClosestDistance = Distance;
		}
	}

	OutSurfacePosition = Transform.TransformPosition(ClosestPoint);
	return (float)ClosestDistance;
}

bool AMRUKAnchor::IsPositionInVolumeBounds(const FVector& Position, bool TestVerticalBounds, float Tolerance)
{
	if (!VolumeBounds.IsValid)
	{
		return false;
	}

	const auto& LocalPosition = GetActorTransform().InverseTransformPosition(Position);

	return ((TestVerticalBounds ? ((LocalPosition.X >= VolumeBounds.Min.X - Tolerance) && (LocalPosition.X <= VolumeBounds.Max.X + Tolerance)) : true)
		&& (LocalPosition.Y >= VolumeBounds.Min.Y - Tolerance) && (LocalPosition.Y <= VolumeBounds.Max.Y + Tolerance)
		&& (LocalPosition.Z >= VolumeBounds.Min.Z - Tolerance) && (LocalPosition.Z <= VolumeBounds.Max.Z + Tolerance));
}

FVector AMRUKAnchor::GetFacingDirection() const
{
	if (Room == nullptr)
	{
		return {};
	}

	if (!VolumeBounds.IsValid)
	{
		return GetActorForwardVector();
	}

	return GetDirectionAwayFromClosestWall();
}

FVector AMRUKAnchor::GetDirectionAwayFromClosestWall(int* OutCardinalAxisIndex, const TArray<int> ExcludedAxes) const
{
	double ClosestWallDistance = DBL_MAX;
	FVector AwayFromWall{};

	for (int i = 0; i < 4; ++i)
	{
		if (ExcludedAxes.Contains(i))
		{
			continue;
		}
		// Shoot a ray along the cardinal directions
		// The "Up" (i.e. Z axis) for anchors typically points away from the facing direction, but it depends
		// entirely on how the user defined the volume in scene capture.
		const auto CardinalAxis = (FQuat::MakeFromEuler({ 0.0f, 0.0f, 90.0f * i }).RotateVector(GetActorUpVector()));

		for (const auto& WallAnchor : Room->WallAnchors)
		{
			if (!WallAnchor)
			{
				continue;
			}
			FMRUKHit Hit{};
			if (!WallAnchor->Raycast(GetActorLocation(), CardinalAxis, 0.0, Hit))
			{
				continue;
			}
			const auto DistToWall = FVector::Distance(Hit.HitPosition, GetActorLocation());
			if (DistToWall < ClosestWallDistance)
			{
				ClosestWallDistance = DistToWall;
				AwayFromWall = -CardinalAxis;
				if (OutCardinalAxisIndex)
				{
					*OutCardinalAxisIndex = i;
				}
			}
		}
	}

	return AwayFromWall;
}

AActor* AMRUKAnchor::SpawnInterior(const TSubclassOf<class AActor>& ActorClass, bool MatchAspectRatio, bool CalculateFacingDirection, EMRUKSpawnerScalingMode ScalingMode)
{
	FActorSpawnParameters Params{};
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	Interior = GetWorld()->SpawnActor(ActorClass, nullptr, Params);
	auto InteriorRoot = Interior->GetRootComponent();
	if (!InteriorRoot)
	{
		UE_LOG(LogMRUK, Error, TEXT("SpawnInterior Spawned actor does not have a root component."));
		return nullptr;
	}
	InteriorRoot->SetMobility(EComponentMobility::Movable);
	Interior->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	Interior->SetActorRelativeScale3D(FVector::OneVector);

	const auto ChildLocalBounds = Interior->CalculateComponentsBoundingBoxInLocalSpace(true);
	FQuat Rotation = FQuat::Identity;
	FVector Offset = FVector::ZeroVector;
	FVector Scale = FVector::OneVector;

	if (VolumeBounds.IsValid)
	{
		int CardinalAxisIndex = 0;
		if (CalculateFacingDirection && !MatchAspectRatio)
		{
			// Pick rotation that is pointing away from the closest wall
			// If we are also matching the aspect ratio then we only have a choice
			// between 2 directions and first need to figure out what those 2 directions
			// are before doing the ray casting.
			GetDirectionAwayFromClosestWall(&CardinalAxisIndex);
		}
		Rotation = FQuat::MakeFromEuler(FVector(90, -(CardinalAxisIndex + 1) * 90, 90));

		FBox ChildBounds = ChildLocalBounds.TransformBy(FTransform(Rotation));
		const FVector ChildSize1 = ChildBounds.GetSize();

		Scale = VolumeBounds.GetSize() / ChildSize1;

		if (MatchAspectRatio)
		{
			FVector ChildSize2 = ChildSize1;
			Swap(ChildSize2.Y, ChildSize2.Z);
			FVector Scale2 = VolumeBounds.GetSize() / ChildSize2;

			float Distortion1 = FMath::Max(Scale.Y, Scale.Z) / FMath::Min(Scale.Y, Scale.Z);
			float Distortion2 = FMath::Max(Scale2.Y, Scale2.Z) / FMath::Min(Scale2.Y, Scale2.Z);

			bool FlipToMatchAspectRatio = Distortion1 > Distortion2;
			if (FlipToMatchAspectRatio)
			{
				CardinalAxisIndex = 1;
				Scale = Scale2;
			}
			if (CalculateFacingDirection)
			{
				GetDirectionAwayFromClosestWall(&CardinalAxisIndex, FlipToMatchAspectRatio ? TArray<int>{ 0, 2 } : TArray<int>{ 1, 3 });
			}
			if (CardinalAxisIndex != 0)
			{
				// Update the rotation and child bounds if necessary
				Rotation = FQuat::MakeFromEuler(FVector(90, -(CardinalAxisIndex + 1) * 90, 90));
				ChildBounds = ChildLocalBounds.TransformBy(FTransform(Rotation));
			}
		}

		switch (ScalingMode)
		{
			case EMRUKSpawnerScalingMode::UniformScaling:
				Scale.X = Scale.Y = Scale.Z = FMath::Min3(Scale.X, Scale.Y, Scale.Z);
				break;
			case EMRUKSpawnerScalingMode::UniformXYScale:
				Scale.Y = Scale.Z = FMath::Min(Scale.Y, Scale.Z);
				break;
			case EMRUKSpawnerScalingMode::NoScaling:
				Scale = FVector::OneVector;
				break;
		}

		// Calculate the offset between the base of the two bounding boxes. Note that the anchor is on the
		// top of the volume and the X axis points downwards. So the base is at Max.X.
		FVector VolumeBase = FVector(VolumeBounds.Max.X, 0.5 * (VolumeBounds.Min.Y + VolumeBounds.Max.Y), 0.5 * (VolumeBounds.Min.Z + VolumeBounds.Max.Z));
		FVector ChildBase = FVector(ChildBounds.Max.X, 0.5 * (ChildBounds.Min.Y + ChildBounds.Max.Y), 0.5 * (ChildBounds.Min.Z + ChildBounds.Max.Z));

		Offset = VolumeBase - ChildBase * Scale;
	}
	else if (PlaneBounds.bIsValid)
	{
		const auto XAxis = GetTransform().GetUnitAxis(EAxis::X);
		// Adjust the rotation so that Z always points up. This enables assets to be authored in a more natural
		// way and show up in the scene as expected.
		if (XAxis.Z <= -UE_INV_SQRT_2)
		{
			// This is a floor or other surface facing upwards
			Rotation = FQuat::MakeFromEuler(FVector(0, 90, 0));
		}
		else if (XAxis.Z >= UE_INV_SQRT_2)
		{
			// This is ceiling or other surface facing downwards.
			Rotation = FQuat::MakeFromEuler(FVector(0, -90, 0));
		}
		else
		{
			// This is a wall or other upright surface.
			Rotation = FQuat::MakeFromEuler(FVector(0, 0, 180));
		}

		const auto ChildBounds = ChildLocalBounds.TransformBy(FTransform(Rotation));
		const auto ChildBounds2D = FBox2D(FVector2D(ChildBounds.Min.Y, ChildBounds.Min.Z), FVector2D(ChildBounds.Max.Y, ChildBounds.Max.Z));
		auto Scale2D = PlaneBounds.GetSize() / ChildBounds2D.GetSize();

		switch (ScalingMode)
		{
			case EMRUKSpawnerScalingMode::UniformScaling:
			case EMRUKSpawnerScalingMode::UniformXYScale:
				Scale2D.X = Scale2D.Y = FMath::Min(Scale2D.X, Scale2D.Y);
				break;
			case EMRUKSpawnerScalingMode::NoScaling:
				Scale2D = FVector2D::UnitVector;
				break;
		}

		const auto Offset2D = PlaneBounds.GetCenter() - ChildBounds2D.GetCenter() * Scale2D;

		Offset = FVector(0.0, Offset2D.X, Offset2D.Y);
		Scale = FVector(0.5 * (Scale2D.X + Scale2D.Y), Scale2D.X, Scale2D.Y);
	}
	Interior->SetActorRelativeRotation(Rotation);
	Interior->SetActorRelativeLocation(Offset);
	SetScaleRecursivelyAdjustingForRotation(InteriorRoot, Scale);

	return Interior;
}

TSharedRef<FJsonObject> AMRUKAnchor::JsonSerialize()
{
	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetField(TEXT("UUID"), MRUKSerialize(SpaceQueryResult.UUID));
	JsonObject->SetField(TEXT("SemanticClassifications"), MRUKSerialize(SemanticClassifications));
	JsonObject->SetField(TEXT("Transform"), MRUKSerialize((double&)GetTransform()));
	if (PlaneBounds.bIsValid)
	{
		JsonObject->SetField(TEXT("PlaneBounds"), MRUKSerialize((double&)PlaneBounds));
	}
	if (!PlaneBoundary2D.IsEmpty())
	{
		JsonObject->SetField(TEXT("PlaneBoundary2D"), MRUKSerialize(PlaneBoundary2D));
	}
	if (VolumeBounds.IsValid)
	{
		JsonObject->SetField(TEXT("VolumeBounds"), MRUKSerialize((double&)VolumeBounds));
	}

	if (this == Room->GlobalMeshAnchor)
	{
		TArray<UProceduralMeshComponent*> ProcMeshComponents;
		GetComponents<UProceduralMeshComponent>(ProcMeshComponents);
		for (const auto& Component : ProcMeshComponents)
		{
			auto ProcMeshComponent = Cast<UProceduralMeshComponent>(Component);
			if (ProcMeshComponent && ProcMeshComponent->ComponentHasTag("GlobalMesh"))
			{
				ensure(ProcMeshComponent->GetNumSections() == 1);

				auto GlobalMeshJson = MakeShared<FJsonObject>();
				GlobalMeshJson->SetField(TEXT("UUID"), MRUKSerialize(SpaceQueryResult.UUID));

				auto ProcMeshSection = ProcMeshComponent->GetProcMeshSection(0);

				TArray<TSharedPtr<FJsonValue>> PositionsJson;
				for (const auto& Vertex : ProcMeshSection->ProcVertexBuffer)
				{
					PositionsJson.Add(MRUKSerialize((double&)Vertex.Position));
				}
				GlobalMeshJson->SetArrayField(TEXT("Positions"), PositionsJson);

				TArray<TSharedPtr<FJsonValue>> IndicesJson;
				for (const auto& Index : ProcMeshSection->ProcIndexBuffer)
				{
					IndicesJson.Add(MakeShared<FJsonValueNumber>(Index));
				}
				GlobalMeshJson->SetArrayField(TEXT("Indices"), IndicesJson);

				JsonObject->SetObjectField(TEXT("GlobalMesh"), GlobalMeshJson);
			}
		}
	}

	return JsonObject;
}

bool AMRUKAnchor::RayCastPlane(const FRay& LocalRay, float MaxDist, FMRUKHit& OutHit)
{
	// If the ray is behind or parallel to the anchor's plane then ignore it
	if (LocalRay.Direction.X >= SMALL_NUMBER)
	{
		// Distance to the plane from the ray origin along the ray's direction
		float Dist = -LocalRay.Origin.X / LocalRay.Direction.X;
		// If the distance is negative or less than the maximum distance then ignore it
		if (Dist >= 0.0f && (MaxDist <= 0 || Dist < MaxDist))
		{
			FVector HitPos = LocalRay.PointAt(Dist);
			// Ensure the hit is within the plane extends and within the boundary
			FVector2D Pos2D(HitPos.Y, HitPos.Z);
			if (PlaneBounds.IsInside(Pos2D) && IsPositionInBoundary(Pos2D))
			{
				// Transform the result back into world space
				auto Transform = GetTransform();
				OutHit.HitPosition = Transform.TransformPositionNoScale(HitPos);
				OutHit.HitNormal = Transform.TransformVectorNoScale(-FVector::XAxisVector);
				OutHit.HitDistance = Dist;
				return true;
			}
		}
	}
	return false;
}

bool AMRUKAnchor::RayCastVolume(const FRay& LocalRay, float MaxDist, FMRUKHit& OutHit)
{
	// Use the slab method to determine if the ray intersects with the bounding box
	// https://education.siggraph.org/static/HyperGraph/raytrace/rtinter3.htm
	float DistNear = -BIG_NUMBER, DistFar = BIG_NUMBER;
	int HitAxis = 0;
	for (int i = 0; i < 3; ++i)
	{
		if (FMath::Abs(LocalRay.Direction.Component(i)) >= SMALL_NUMBER)
		{
			// Distance to the plane from the ray origin along the ray's direction
			float Dist1 = (VolumeBounds.Min.Component(i) - LocalRay.Origin.Component(i)) / LocalRay.Direction.Component(i);
			float Dist2 = (VolumeBounds.Max.Component(i) - LocalRay.Origin.Component(i)) / LocalRay.Direction.Component(i);

			if (Dist1 > Dist2)
			{
				std::swap(Dist1, Dist2);
			}
			if (Dist1 > DistNear)
			{
				DistNear = Dist1;
				HitAxis = i;
			}
			if (Dist2 < DistFar)
			{
				DistFar = Dist2;
			}
		}
		else
		{
			// In this case there is no intersection because the ray is parallel to the plane
			// Check that it is within bounds
			if (LocalRay.Origin.Component(i) < VolumeBounds.Min.Component(i) || LocalRay.Origin.Component(i) > VolumeBounds.Max.Component(i))
			{
				// No intersection, set DistNear to a large number
				DistNear = BIG_NUMBER;
				break;
			}
		}
	}
	if (DistNear >= 0 && DistNear <= DistFar && (MaxDist <= 0 || DistNear < MaxDist))
	{
		FVector HitPos = LocalRay.PointAt(DistNear);
		FVector HitNormal = FVector::ZeroVector;
		HitNormal.Component(HitAxis) = LocalRay.Direction.Component(HitAxis) > 0 ? -1 : 1;
		// Transform the result back into world space
		auto Transform = GetTransform();
		OutHit.HitPosition = Transform.TransformPositionNoScale(HitPos);
		OutHit.HitNormal = Transform.TransformVectorNoScale(HitNormal);
		OutHit.HitDistance = DistNear;
		return true;
	}
	return false;
}

void AMRUKAnchor::TriangulatedMeshCache::Clear()
{
	Vertices.Empty();
	Triangles.Empty();
	Areas.Empty();
	TotalArea = 0.0f;
}

// #pragma optimize("", on)

#undef LOCTEXT_NAMESPACE
