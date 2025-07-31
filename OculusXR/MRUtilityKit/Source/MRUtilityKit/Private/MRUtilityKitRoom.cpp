/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "MRUtilityKitRoom.h"
#include "MRUtilityKitAnchor.h"
#include "MRUtilityKitSerializationHelpers.h"
#include "MRUtilityKitSeatsComponent.h"
#include "MRUtilityKitSubsystem.h"
#include "MRUtilityKitBPLibrary.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#define LOCTEXT_NAMESPACE "MRUtilityKitRoom"

namespace
{
	float GetSeamlessFactor(float Perimeter, float StepSize)
	{
		float RoundedPerimeter = FMath::RoundHalfFromZero(Perimeter / StepSize);
		if (RoundedPerimeter <= 0.0f)
		{
			RoundedPerimeter = 1.0f;
		}
		return Perimeter / RoundedPerimeter;
	}
} // namespace

AMRUKRoom::AMRUKRoom(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create a scene component as root so we can attach spawned actors to it
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
}

void AMRUKRoom::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRoom();

	const auto Subsystem = GetGameInstance()->GetSubsystem<UMRUKSubsystem>();
	Subsystem->UnregisterRoom(this);
}

void AMRUKRoom::Destroyed()
{
	for (const auto Anchor : AllAnchors)
	{
		OnAnchorRemoved.Broadcast(Anchor);
		Anchor->Destroy();
	}

	Super::Destroyed();
}

void AMRUKRoom::LoadFromData(UMRUKRoomData* RoomData)
{
	check(RoomData);

	SpaceQueryResult = RoomData->SpaceQuery;
	RoomLayout = RoomData->RoomLayout;

	auto AnchorsToRemove = AllAnchors;

	AllAnchors.Empty();
	CeilingAnchor = nullptr;
	FloorAnchor = nullptr;
	WallAnchors.Empty();
	SeatAnchors.Empty();

	TArray<AMRUKAnchor*> AnchorsCreated;
	TArray<AMRUKAnchor*> AnchorsUpdated;

	for (const auto AnchorData : RoomData->AnchorsData)
	{
		auto AnchorFound = AnchorsToRemove.FindByPredicate([AnchorData](AMRUKAnchor* Anchor) {
			return Anchor && Anchor->SpaceQueryResult.UUID == AnchorData->SpaceQuery.UUID;
		});
		AMRUKAnchor* Anchor = nullptr;
		if (AnchorFound)
		{
			Anchor = *AnchorFound;
			UE_LOG(LogMRUK, Log, TEXT("Update existing anchor in room"));
			if (Anchor->LoadFromData(AnchorData))
			{
				AnchorsUpdated.Push(Anchor);
			}
			AnchorsToRemove.Remove(Anchor);
		}
		else
		{
			UE_LOG(LogMRUK, Log, TEXT("Spawn new anchor in room"));
			Anchor = SpawnAnchor();
			Anchor->LoadFromData(AnchorData);
			AnchorsCreated.Push(Anchor);
		}
		AddAnchorToRoom(Anchor);
	}

	UE_LOG(LogMRUK, Log, TEXT("Destroy %d old anchors"), AnchorsToRemove.Num());
	for (auto OldAnchor : AnchorsToRemove)
	{
		OnAnchorRemoved.Broadcast(OldAnchor);
		OldAnchor->Destroy();
	}

	InitializeRoom();

	for (auto Anchor : AnchorsUpdated)
	{
		OnAnchorUpdated.Broadcast(Anchor);
	}
	for (auto Anchor : AnchorsCreated)
	{
		OnAnchorCreated.Broadcast(Anchor);
	}
}

TSharedRef<FJsonObject> AMRUKRoom::JsonSerialize()
{
	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> AnchorsArray;
	for (const auto& Anchor : AllAnchors)
	{
		if (Anchor)
		{
			AnchorsArray.Add(MakeShareable(new FJsonValueObject(Anchor->JsonSerialize())));
		}
	}
	JsonObject->SetField(TEXT("UUID"), MRUKSerialize(SpaceQueryResult.UUID));
	JsonObject->SetField(TEXT("RoomLayout"), MRUKSerialize(RoomLayout));
	JsonObject->SetArrayField(TEXT("Anchors"), AnchorsArray);
	return JsonObject;
}

bool AMRUKRoom::Corresponds(UMRUKRoomData* RoomData) const
{
	if (!RoomData)
	{
		UE_LOG(LogMRUK, Warning, TEXT("Room query is null"));
		return false;
	}

	if (SpaceQueryResult.UUID == RoomData->SpaceQuery.UUID)
	{
		UE_LOG(LogMRUK, Log, TEXT("Rooms UUID equals"));
		return true;
	}

	for (auto Anchor : AllAnchors)
	{
		auto UUID = Anchor->SpaceQueryResult.UUID;
		auto Found = RoomData->AnchorsData.FindByPredicate([UUID](UMRUKAnchorData* AnchorData) {
			return UUID == AnchorData->SpaceQuery.UUID;
		});
		if (Found)
		{
			return true;
		}
	}

	UE_LOG(LogMRUK, Log, TEXT("Room is not equal"));
	return false;
}

AMRUKAnchor* AMRUKRoom::SpawnAnchor()
{
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActorSpawnParams.Owner = this;

	AMRUKAnchor* Anchor = GetWorld()->SpawnActor<AMRUKAnchor>(ActorSpawnParams);
	Anchor->Room = this;
	Anchor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

	return Anchor;
}

void AMRUKRoom::AddAnchorToRoom(AMRUKAnchor* Anchor)
{
#if WITH_EDITOR
	if (Anchor->SemanticClassifications.Num() > 0)
	{
		FString Semantics = FString::Join(Anchor->SemanticClassifications, TEXT("-"));
		Anchor->SetActorLabel(Semantics);
	}
#endif

	if (Anchor->SpaceQueryResult.UUID == RoomLayout.FloorUuid)
	{
		FloorAnchor = Anchor;
	}
	if (Anchor->SpaceQueryResult.UUID == RoomLayout.CeilingUuid)
	{
		CeilingAnchor = Anchor;
	}
	if (RoomLayout.WallsUuid.Contains(Anchor->SpaceQueryResult.UUID))
	{
		WallAnchors.Push(Anchor);
	}
	if (Anchor->HasLabel(FMRUKLabels::GlobalMesh))
	{
		GlobalMeshAnchor = Anchor;
	}
	if (Anchor->HasLabel(FMRUKLabels::Couch))
	{
		SeatAnchors.Push(Anchor);
	}

	AllAnchors.Push(Anchor);
}

void AMRUKRoom::InitializeRoom()
{
	ComputeRoomBounds();
	ComputeAnchorHierarchy();
	ComputeSeats();
	ComputeRoomEdges();
	KeyWallAnchor = nullptr;
	AnchorMesh.CreateMesh(WallAnchors);
}

void AMRUKRoom::ComputeRoomBounds()
{
	RoomBounds.Init();

	for (auto Anchor : { FloorAnchor, CeilingAnchor })
	{
		if (Anchor)
		{
			auto Transform = Anchor->GetTransform();
			for (auto& Vertex : Anchor->PlaneBoundary2D)
			{
				auto Pos = Transform.TransformPosition(FVector(0.0f, Vertex.X, Vertex.Y));
				RoomBounds += Pos;
			}
		}
	}
}

void AMRUKRoom::ComputeAnchorHierarchy()
{
	// Reset anchor hierarchy
	for (auto Anchor : AllAnchors)
	{
		Anchor->ParentAnchor = nullptr;
		Anchor->ChildAnchors.Empty();
	}

	constexpr float OffsetTolerance = 1.0f;
	// Find things where are attached to walls such as doors, windows frames or wall art
	for (const auto& WallAnchor : WallAnchors)
	{
		if (!WallAnchor)
		{
			continue;
		}
		const auto& WallTransform = WallAnchor->GetTransform();
		const auto WallNormal = WallTransform.GetUnitAxis(EAxis::X);
		for (const auto& ChildAnchor : AllAnchors)
		{
			// Don't parent walls to themselves
			if (ChildAnchor == WallAnchor)
			{
				continue;
			}
			const auto& ChildTransform = ChildAnchor->GetTransform();
			const auto ChildNormal = ChildTransform.GetUnitAxis(EAxis::X);
			// Check that the two transforms face the same direction
			if (!FVector::Coincident(WallNormal, ChildNormal))
			{
				continue;
			}
			// Check that the position is close to the surface (they are a little bit offset
			// to prevent Z fighting so allow for that).
			auto LocalPos = WallTransform.InverseTransformPosition(ChildTransform.GetLocation());
			if (FMath::Abs(LocalPos.X) > OffsetTolerance)
			{
				continue;
			}
			// Check that the anchor is within the wall boundary
			if (!WallAnchor->IsPositionInBoundary(FVector2D(LocalPos.Y, LocalPos.Z)))
			{
				continue;
			}
			// We have a match
			ensureMsgf(!ChildAnchor->ParentAnchor, TEXT("This anchor already has a parent"));
			ChildAnchor->ParentAnchor = WallAnchor;
			WallAnchor->ChildAnchors.Push(ChildAnchor);
		}
	}

	// Find volumes on the floor
	if (FloorAnchor)
	{
		const auto& FloorTransform = FloorAnchor->GetTransform();
		const auto FloorNormal = FloorTransform.GetUnitAxis(EAxis::X);
		ensureMsgf(FVector::Coincident(FloorNormal, FVector::DownVector), TEXT("Floor normal should be pointing downwards"));
		auto FloorHeight = FloorTransform.GetLocation().Z;
		for (const auto& ChildAnchor : AllAnchors)
		{
			// Don't parent the floor to itself
			if (ChildAnchor == FloorAnchor)
			{
				continue;
			}
			const auto& ChildTransform = ChildAnchor->GetTransform();
			const auto ChildXAxis = ChildTransform.GetUnitAxis(EAxis::X);
			const auto& ChildVolumeBounds = ChildAnchor->VolumeBounds;
			// Only interested in scene volumes, the assumption is that all scene volumes have X axis pointing downwards
			if (!ChildVolumeBounds.IsValid || !FVector::Coincident(ChildXAxis, FVector::DownVector))
			{
				continue;
			}
			auto ChildBottom = ChildTransform.GetLocation().Z - ChildVolumeBounds.Max.X;
			// Check that the volume is on the floor
			if (FMath::Abs(FloorHeight - ChildBottom) > OffsetTolerance)
			{
				continue;
			}
			auto LocalPos = FloorTransform.InverseTransformPosition(ChildTransform.GetLocation());
			// Check that child anchor is within the bounds of the floor
			if (!FloorAnchor->IsPositionInBoundary(FVector2D(LocalPos.Y, LocalPos.Z)))
			{
				continue;
			}
			// We have a match
			ensureMsgf(!ChildAnchor->ParentAnchor, TEXT("This anchor already has a parent"));
			ChildAnchor->ParentAnchor = FloorAnchor;
			FloorAnchor->ChildAnchors.Push(ChildAnchor);
		}
	}

	// Find relationship between scene volumes
	for (const auto& ParentAnchor : AllAnchors)
	{
		if (!ParentAnchor)
		{
			continue;
		}
		const auto& ParentTransform = ParentAnchor->GetTransform();
		const auto ParentXAxis = ParentTransform.GetUnitAxis(EAxis::X);
		const auto& ParentVolumeBounds = ParentAnchor->VolumeBounds;
		// Only interested in scene volumes, the assumption is that all scene volumes have X axis pointing downwards
		if (!ParentVolumeBounds.IsValid || !FVector::Coincident(ParentXAxis, FVector::DownVector))
		{
			continue;
		}
		auto ParentTop = ParentTransform.GetLocation().Z - ParentVolumeBounds.Min.X;
		for (const auto& ChildAnchor : AllAnchors)
		{
			// Don't parent anchors to themselves
			if (ChildAnchor == ParentAnchor)
			{
				continue;
			}
			const auto& ChildTransform = ChildAnchor->GetTransform();
			const auto ChildXAxis = ChildTransform.GetUnitAxis(EAxis::X);
			const auto& ChildVolumeBounds = ChildAnchor->VolumeBounds;
			// Only interested in scene volumes, the assumption is that all scene volumes have X axis pointing downwards
			if (!ChildVolumeBounds.IsValid || !FVector::Coincident(ChildXAxis, FVector::DownVector))
			{
				continue;
			}
			auto ChildBottom = ChildTransform.GetLocation().Z - ChildVolumeBounds.Max.X;
			// Check that the two volumes are stack on top of each other
			if (FMath::Abs(ParentTop - ChildBottom) > OffsetTolerance)
			{
				continue;
			}
			// Check that at least one of the corners of the child volume is inside the bounds of the parent's volume
			// when projected onto the horizontal plane. This is to match the Scene Capture tool which requires the
			// user to defined stacked volumes by starting with one corner of the volume which must be on the parent's
			// volume.
			bool AnyCornerInside = false;
			for (int i = 0; i < 4; ++i)
			{
				// Get a different corner on each iteration of the loop (height is not important here)
				FVector ChildLocalPos(0.0f, i < 2 ? ChildVolumeBounds.Min.Y : ChildVolumeBounds.Max.Y, i % 2 == 0 ? ChildVolumeBounds.Min.Z : ChildVolumeBounds.Max.Z);
				auto LocalPos = ParentTransform.InverseTransformPosition(ChildTransform.TransformPosition(ChildLocalPos));
				// Check that child anchor is within the bounds of the parent on the horizontal plane
				if (LocalPos.Y >= ParentVolumeBounds.Min.Y && LocalPos.Y <= ParentVolumeBounds.Max.Y && LocalPos.Z >= ParentVolumeBounds.Min.Z && LocalPos.Z <= ParentVolumeBounds.Max.Z)
				{
					AnyCornerInside = true;
					break;
				}
			}
			if (!AnyCornerInside)
			{
				continue;
			}
			// We have a match
			ensureMsgf(!ChildAnchor->ParentAnchor, TEXT("This anchor already has a parent"));
			ChildAnchor->ParentAnchor = ParentAnchor;
			ParentAnchor->ChildAnchors.Push(ChildAnchor);
		}
	}
}

void AMRUKRoom::ComputeSeats()
{
	for (const auto& SeatAnchor : SeatAnchors)
	{
		if (SeatAnchor)
		{
			auto SeatsComponent = SeatAnchor->FindComponentByClass<UMRUKSeatsComponent>();
			if (!SeatsComponent)
			{
				SeatsComponent = NewObject<UMRUKSeatsComponent>(SeatAnchor, TEXT("Seats"));
				SeatsComponent->RegisterComponent();
			}
			SeatsComponent->CalculateSeatPoses();
		}
	}
}

void AMRUKRoom::ComputeRoomEdges()
{
	if (!FloorAnchor)
	{
		UE_LOG(LogMRUK, Warning, TEXT("Floor anchor not set, can not compute room edges"));
		return;
	}

	const auto& FloorBoundary = FloorAnchor->PlaneBoundary2D;
	const auto& FloorTransform = FloorAnchor->GetActorTransform();

	RoomEdges.SetNum(FloorBoundary.Num(), true);
	for (int i = 0; i < RoomEdges.Num(); ++i)
	{
		const auto& BoundaryPoint = FloorBoundary[i];
		FVector Edge = FVector(0.0, BoundaryPoint.X, BoundaryPoint.Y);
		Edge = FloorTransform.TransformPosition(Edge);
		Edge.Z = 0.0;
		RoomEdges[i] = Edge;
	}
}

bool AMRUKRoom::IsPositionInRoom(const FVector& Position, bool TestVerticalBounds)
{
	if (!FloorAnchor)
	{
		return false;
	}

	if (!(TestVerticalBounds ? RoomBounds.IsInside(Position) : RoomBounds.IsInsideXY(Position)))
	{
		return false;
	}

	auto Transform = FloorAnchor->GetTransform();
	FVector LocalPos = Transform.InverseTransformPositionNoScale(Position);
	return FloorAnchor->IsPositionInBoundary(FVector2D(LocalPos.Y, LocalPos.Z));
}

bool AMRUKRoom::GenerateRandomPositionInRoom(FVector& OutPosition, float MinDistanceToSurface, bool AvoidVolumes)
{
	return GenerateRandomPositionInRoomFromStream(OutPosition, FRandomStream(NAME_None), MinDistanceToSurface, AvoidVolumes);
}

bool AMRUKRoom::GenerateRandomPositionInRoomFromStream(FVector& OutPosition, const FRandomStream& RandomStream, float MinDistanceToSurface, bool AvoidVolumes)
{
	if (!FloorAnchor)
	{
		return false;
	}
	if (MinDistanceToSurface > RoomBounds.GetExtent().GetMin())
	{
		// We can exit early here as we know it's not possible to generate a position in the room that satisfies
		// the MinDistanceToSurface requirement
		return false;
	}
	FVector Position;
	constexpr int MaxIterations = 1000;
	// Bail after MaxIteration tries to avoid infinite loop in case MinDistanceToSurface is too large
	// and we can't find a position which does not intersect with the walls and volumes
	for (int i = 0; i < MaxIterations; ++i)
	{
		if (MinDistanceToSurface > 0.0f)
		{
			// If MinDistanceToSurface is large then it can be more efficient to randomly generate points within
			// the shrunking bounds of the room
			Position.X = RandomStream.FRandRange(RoomBounds.Min.X + MinDistanceToSurface, RoomBounds.Max.X - MinDistanceToSurface);
			Position.Y = RandomStream.FRandRange(RoomBounds.Min.Y + MinDistanceToSurface, RoomBounds.Max.Y - MinDistanceToSurface);
			Position.Z = RandomStream.FRandRange(RoomBounds.Min.Z + MinDistanceToSurface, RoomBounds.Max.Z - MinDistanceToSurface);
			if (!IsPositionInRoom(Position))
			{
				// Reject points that are outside the room
				continue;
			}
			FVector SurfacePos;
			float SurfaceDistance;
			FMRUKLabelFilter Filter;
			Filter.IncludedLabels = { FMRUKLabels::WallFace };
			if (TryGetClosestSurfacePosition(Position, SurfacePos, SurfaceDistance, Filter, MinDistanceToSurface))
			{
				// Reject points that are too close to the walls
				continue;
			}
		}
		else
		{
			Position = FloorAnchor->GenerateRandomPositionOnPlaneFromStream(RandomStream);
			Position = FloorAnchor->GetTransform().TransformPosition(Position);
			Position.Z = RandomStream.FRandRange(RoomBounds.Min.Z + MinDistanceToSurface, RoomBounds.Max.Z - MinDistanceToSurface);
		}
		if (AvoidVolumes && IsPositionInSceneVolume(Position, true, MinDistanceToSurface))
		{
			// Reject points inside volumes if avoid volumes has been enabled
			continue;
		}
		OutPosition = Position;
		return true;
	}
	return false;
}

AMRUKAnchor* AMRUKRoom::Raycast(const FVector& Origin, const FVector& Direction, float MaxDist, const FMRUKLabelFilter& LabelFilter, FMRUKHit& OutHit)
{
	AMRUKAnchor* HitComponent = nullptr;
	for (const auto& Anchor : AllAnchors)
	{
		if (!Anchor || !Anchor->PassesLabelFilter(LabelFilter))
		{
			continue;
		}
		FMRUKHit HitResult;
		if (Anchor->Raycast(Origin, Direction, MaxDist, HitResult))
		{
			// Prevent further hits which are further away from being found
			MaxDist = HitResult.HitDistance;
			OutHit = HitResult;
			HitComponent = Anchor;
		}
	}
	return HitComponent;
}

bool AMRUKRoom::RaycastAll(const FVector& Origin, const FVector& Direction, float MaxDist, const FMRUKLabelFilter& LabelFilter, TArray<FMRUKHit>& OutHits, TArray<AMRUKAnchor*>& OutAnchors)
{
	bool HitAnything = false;
	for (const auto& Anchor : AllAnchors)
	{
		if (!Anchor || !Anchor->PassesLabelFilter(LabelFilter))
		{
			continue;
		}
		if (Anchor->RaycastAll(Origin, Direction, MaxDist, OutHits))
		{
			HitAnything = true;
			// For each element in OutHits we want an equivalent entry in OutAnchors with the same index
			// which represents which anchor was hit.
			while (OutHits.Num() > OutAnchors.Num())
			{
				OutAnchors.Push(Anchor);
			}
		}
	}
	return HitAnything;
}

void AMRUKRoom::ClearRoom()
{
	RoomLayout = FOculusRoomLayout();
	SpaceQueryResult = FOculusSpaceQueryResult();
	RoomBounds.Init();
	for (auto Anchor : AllAnchors)
	{
		if (Anchor)
		{
			Anchor->Destroy();
		}
	}
	AllAnchors.Empty();
	WallAnchors.Empty();
	SeatAnchors.Empty();
	FloorAnchor = nullptr;
	CeilingAnchor = nullptr;
	KeyWallAnchor = nullptr;
}

bool AMRUKRoom::DoesRoomHave(const TArray<FString>& Labels)
{
	if (Labels.IsEmpty())
	{
		return true;
	}

	TArray<FString> RemainingLabels = Labels;

	for (const auto& Anchor : AllAnchors)
	{
		for (const auto& AnchorLabel : Anchor->SemanticClassifications)
		{
			const auto AnchorLabelIndex = RemainingLabels.Find(AnchorLabel);
			if (AnchorLabelIndex != INDEX_NONE)
			{
				RemainingLabels.RemoveAt(AnchorLabelIndex);
				if (RemainingLabels.IsEmpty())
				{
					return true;
				}
			}
		}
	}

	return false;
}

AMRUKAnchor* AMRUKRoom::TryGetClosestSurfacePosition(const FVector& WorldPosition, FVector& OutSurfacePosition, float& OutSurfaceDistance, const FMRUKLabelFilter& LabelFilter, float MaxDistance)
{
	if (MaxDistance <= 0.0)
	{
		MaxDistance = FLT_MAX;
	}
	OutSurfacePosition = FVector::ZeroVector;
	AMRUKAnchor* ClosestAnchor = nullptr;

	for (const auto Anchor : AllAnchors)
	{
		if (!Anchor || !Anchor->PassesLabelFilter(LabelFilter))
		{
			continue;
		}

		FVector SurfacePos{};
		const auto Distance = Anchor->GetClosestSurfacePosition(WorldPosition, SurfacePos);
		if (Distance < MaxDistance)
		{
			MaxDistance = Distance;
			OutSurfacePosition = SurfacePos;
			ClosestAnchor = Anchor;
		}
	}

	OutSurfaceDistance = MaxDistance;
	return ClosestAnchor;
}

AMRUKAnchor* AMRUKRoom::IsPositionInSceneVolume(const FVector& WorldPosition, bool TestVerticalBounds, float Tolerance)
{
	for (const auto& Anchor : AllAnchors)
	{
		if (!Anchor)
		{
			continue;
		}

		if (Anchor->IsPositionInVolumeBounds(WorldPosition, TestVerticalBounds, Tolerance))
		{
			return Anchor;
		}
	}

	return nullptr;
}

AMRUKAnchor* AMRUKRoom::TryGetClosestSeatPose(const FVector& RayOrigin, const FVector& RayDirection, FTransform& OutSeatTransform)
{
	FTransform ClosestPose{};
	AMRUKAnchor* ClosestAnchor = nullptr;
	double ClosestDot = DBL_MIN;
	
	for (const auto& SeatAnchor : SeatAnchors)
	{
		if (!SeatAnchor)
		{
			continue;
		}
		const auto SeatsComponent = SeatAnchor->FindComponentByClass<UMRUKSeatsComponent>();
		if (!SeatsComponent)
		{
			continue;
		}

		for (const auto& SeatPose : SeatsComponent->SeatPoses)
		{
			const auto VecToSeat = (SeatPose.GetLocation() - RayOrigin).GetSafeNormal();
			const auto ThisDot = FVector::DotProduct(RayDirection, VecToSeat);
			if (ThisDot <= ClosestDot)
			{
				continue;
			}
			ClosestDot = ThisDot;
			ClosestPose = SeatPose;
			ClosestAnchor = SeatAnchor;
		}
	}

	OutSeatTransform = ClosestPose;
	return ClosestAnchor;
}

TArray<AMRUKAnchor*> AMRUKRoom::GetAnchorsByLabel(const FString& Label) const
{
	TArray<AMRUKAnchor*> Anchors;

	for (const auto& Anchor : AllAnchors)
	{
		if (Anchor && Anchor->HasLabel(Label))
		{
			Anchors.Push(Anchor);
		}
	}
	return Anchors;
}

AMRUKAnchor* AMRUKRoom::GetFirstAnchorByLabel(const FString& Label) const
{
	const auto Anchors = GetAnchorsByLabel(Label);
	if (Anchors.IsEmpty())
	{
		return nullptr;
	}
	return Anchors[0];
}

AMRUKAnchor* AMRUKRoom::GetBestPoseFromRaycast(const FVector& RayOrigin, const FVector& RayDirection, float MaxDist, const FMRUKLabelFilter& LabelFilter, FTransform& OutPose, EMRUKPositioningMethod PositioningMethod)
{
	FTransform BestPose{};

	FMRUKHit Hit{};
	const auto HitAnchor = Raycast(RayOrigin, RayDirection, MaxDist, LabelFilter, Hit);
	if (!HitAnchor)
	{
		return nullptr;
	}

	FVector PosePosition = Hit.HitPosition;
	FVector PoseUp = FVector::UpVector;
	// By default, use the surface normal for pose forward
	// Caution: Make sure all the cases of this being "up" are caught below
	FVector PoseForward = Hit.HitNormal;

	constexpr double ParallelTolerance = 0.999;
	
	if (!HitAnchor->VolumeBounds.IsValid && FVector::DotProduct(Hit.HitNormal, PoseUp) >= ParallelTolerance)
	{
		// HitNormal and PoseUp are parallel. E.g. Walls and floors.
		PoseForward = FVector{ RayOrigin.X - Hit.HitPosition.X, RayOrigin.Y - Hit.HitPosition.Y, 0.0 }.GetSafeNormal();
	}
	else if (HitAnchor->VolumeBounds.IsValid)
	{
		// This is a volume object, and the ray has hit the top surface
		if (FVector::DotProduct(Hit.HitNormal, (FVector::UpVector)) >= ParallelTolerance)
		{
			const auto& Transform = HitAnchor->GetActorTransform();

			switch (PositioningMethod)
			{
				case EMRUKPositioningMethod::Center:
				{
					const auto HitLocalPos = Transform.InverseTransformPosition(Hit.HitPosition);

					double ShortestDistance = DBL_MAX;
					FVector Forward = FVector::ZeroVector;

					auto Dist = FMath::Abs(HitLocalPos.Y - HitAnchor->VolumeBounds.Min.Y);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						Forward = -HitAnchor->GetActorRightVector();
					}
					Dist = FMath::Abs(HitLocalPos.Y - HitAnchor->VolumeBounds.Max.Y);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						Forward = HitAnchor->GetActorRightVector();
					}
					Dist = FMath::Abs(HitLocalPos.Z - HitAnchor->VolumeBounds.Min.Z);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						Forward = -HitAnchor->GetActorUpVector();
					}
					Dist = FMath::Abs(HitLocalPos.Z - HitAnchor->VolumeBounds.Max.Z);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						Forward = HitAnchor->GetActorUpVector();
					}

					PoseForward = Forward;
					PosePosition = Transform.TransformPosition(FVector::ZeroVector);
				}
				break;
				case EMRUKPositioningMethod::Edge:
				{
					const auto HitLocalPos = Transform.InverseTransformPosition(Hit.HitPosition);

					double ShortestDistance = DBL_MAX;
					FVector PoseLocal = FVector::ZeroVector;

					auto Dist = FMath::Abs(HitLocalPos.Y - HitAnchor->VolumeBounds.Min.Y);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						PoseForward = -HitAnchor->GetActorRightVector();
						PoseLocal = { 0.0, HitAnchor->VolumeBounds.Min.Y, HitLocalPos.Z };
					}
					Dist = FMath::Abs(HitLocalPos.Y - HitAnchor->VolumeBounds.Max.Y);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						PoseForward = HitAnchor->GetActorRightVector();
						PoseLocal = { 0.0, HitAnchor->VolumeBounds.Max.Y, HitLocalPos.Z };
					}
					Dist = FMath::Abs(HitLocalPos.Z - HitAnchor->VolumeBounds.Min.Z);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						PoseForward = -HitAnchor->GetActorUpVector();
						PoseLocal = { 0.0, HitLocalPos.Y, HitAnchor->VolumeBounds.Min.Z };
					}
					Dist = FMath::Abs(HitLocalPos.Z - HitAnchor->VolumeBounds.Max.Z);
					if (Dist < ShortestDistance)
					{
						ShortestDistance = Dist;
						PoseForward = HitAnchor->GetActorUpVector();
						PoseLocal = { 0.0, HitLocalPos.Y, HitAnchor->VolumeBounds.Max.Z };
					}
					PosePosition = Transform.TransformPosition(PoseLocal);
				}
				break;
				default:
				{
					const auto HitLocalPos = Transform.InverseTransformPosition(Hit.HitPosition);
					PosePosition = Transform.TransformPosition({ 0.0, HitLocalPos.Y, HitLocalPos.Z });
					PoseForward = FVector{ RayOrigin.X - Hit.HitPosition.X, RayOrigin.Y - Hit.HitPosition.Y, 0.0 }.GetSafeNormal();
				}
				break;
			}
		}
	}

	BestPose.SetLocation(PosePosition);
	BestPose.SetRotation(UKismetMathLibrary::MakeRotFromXZ(PoseForward, PoseUp).Quaternion());
	OutPose = BestPose;

	return HitAnchor;
}

AMRUKAnchor* AMRUKRoom::GetKeyWall(float Tolerance)
{
	if (KeyWallAnchor)
	{
		return KeyWallAnchor;
	}

	TArray<AMRUKAnchor*> SortedWalls = WallAnchors;
	SortedWalls.Sort([](const AMRUKAnchor& a, const AMRUKAnchor& b) { return a.PlaneBounds.GetExtent().X < b.PlaneBounds.GetExtent().X; });
	// Find the first one with no other walls behind it.
	// SortedWalls is sorted from shortest side to longest
	for (int i = SortedWalls.Num() - 1; i >= 0; --i)
	{
		const auto WallAnchor = SortedWalls[i];

		bool NoPointsBehind = true;

		// Loop through the other corners, making sure none is behind the wall in question
		for (const auto& RoomEdge : RoomEdges)
		{
			auto VecToCorner = RoomEdge - WallAnchor->GetActorLocation();
			// Due to anchor precision, we use a tolerance value.
			// For example, an adjacent wall edge may be just behind the wall, leading to a false result
			VecToCorner -= WallAnchor->GetActorForwardVector() * Tolerance;
			
			NoPointsBehind &= (FVector::DotProduct(- WallAnchor->GetActorForwardVector(), (VecToCorner))) >= 0.0;
			if (!NoPointsBehind)
			{
				break;
			}
		}

		if (NoPointsBehind)
		{
			KeyWallAnchor = WallAnchor;
			return WallAnchor;
		}
	}

	return nullptr;
}

AMRUKAnchor* AMRUKRoom::GetLargestSurface(const FString& Label) const
{
	AMRUKAnchor* LargestSurfaceAnchor = nullptr;
	double LargestSurfaceArea = 0.0;
	const auto LabelUpper = Label.ToUpper();

	for (const auto Anchor : AllAnchors)
	{
		if (!Anchor || !Anchor->HasLabel(Label))
		{
			continue;
		}

		double ThisSurfaceArea = 0.0;
		if (Anchor->PlaneBounds.bIsValid)
		{
			ThisSurfaceArea = Anchor->PlaneBounds.GetArea();
		}
		else if (Anchor->VolumeBounds.IsValid)
		{
			const auto VolumeSize = Anchor->VolumeBounds.GetSize();
			ThisSurfaceArea = VolumeSize.Y * VolumeSize.Z;
		}

		if (ThisSurfaceArea > LargestSurfaceArea)
		{
			LargestSurfaceArea = ThisSurfaceArea;
			LargestSurfaceAnchor = Anchor;
		}
	}

	return LargestSurfaceAnchor;
}

void AMRUKRoom::AttachProceduralMeshToWalls(UMaterialInterface* ProceduralMaterial)
{
	AttachProceduralMeshToWalls({}, ProceduralMaterial);
}

void AMRUKRoom::ComputeWallMeshUVAdjustments(const TArray<FMRUKTexCoordModes>& WallTextureCoordinateModes, TArray<FMRUKAnchorWithPlaneUVs>& OutAnchorsWithPlaneUVs)
{
	TArray<AMRUKAnchor*> ConnectedWalls = ComputeConnectedWalls();
	float Perimeter = 0.0f;
	for (const auto& WallAnchor : ConnectedWalls)
	{
		Perimeter += WallAnchor->PlaneBounds.GetSize().X;
	}
	float WorldToMeters = GetWorldSettings()->WorldToMeters;
	float WallHeight = RoomBounds.GetSize().Z;
	float SeamlessWorldToMeters = GetSeamlessFactor(Perimeter, WorldToMeters);
	float UOffset = 0.0f;
	const TArray<FMRUKTexCoordModes>& TexCoordModes = WallTextureCoordinateModes.IsEmpty() ? TArray<FMRUKTexCoordModes>{ FMRUKTexCoordModes{} } : WallTextureCoordinateModes;
	for (auto& WallAnchor : ConnectedWalls)
	{
		auto WallWidth = WallAnchor->PlaneBounds.GetSize().X;
		TArray<FMRUKPlaneUV> PlaneUVAdjustments;
		for (auto TexCoordMode : TexCoordModes)
		{
			float DenominatorX;
			float DenominatorY;
			// Determine the scaling in the V direction first, if this is set to maintain aspect
			// ratio we need to come back to it after U scaling has been determined.
			switch (TexCoordMode.V)
			{
				// Default to stretch in case maintain aspect ratio is set for both axes
				default:
				case EMRUKCoordModeV::Stretch:
					DenominatorY = WallHeight;
					break;
				case EMRUKCoordModeV::Metric:
					DenominatorY = WorldToMeters;
					break;
			}
			switch (TexCoordMode.U)
			{
				default:
				case EMRUKCoordModeU::Stretch:
					DenominatorX = Perimeter;
					break;
				case EMRUKCoordModeU::Metric:
					DenominatorX = WorldToMeters;
					break;
				case EMRUKCoordModeU::MetricSeamless:
					DenominatorX = SeamlessWorldToMeters;
					break;
				case EMRUKCoordModeU::MaintainAspectRatio:
					DenominatorX = DenominatorY;
					break;
				case EMRUKCoordModeU::MaintainAspectRatioSeamless:
					DenominatorX = GetSeamlessFactor(Perimeter, DenominatorY);
					break;
			}
			// Do another pass on V in case it has maintain aspect ratio set
			if (TexCoordMode.V == EMRUKCoordModeV::MaintainAspectRatio)
			{
				DenominatorY = DenominatorX;
			}

			FVector2D Offset(UOffset / DenominatorX, 0);
			FVector2D Scale(WallWidth / DenominatorX, WallHeight / DenominatorY);
			PlaneUVAdjustments.Push({ Offset, Scale });
		}
		OutAnchorsWithPlaneUVs.Push({ WallAnchor, PlaneUVAdjustments });
		UOffset += WallWidth;
	}
}

UProceduralMeshComponent* AMRUKRoom::GetOrCreateGlobalMeshProceduralMeshComponent(bool& OutExistedAlready)
{
	// Try to find the global mesh procedural mesh component if it already exists
	TArray<UProceduralMeshComponent*> ProcMeshComponents;
	GetComponents<UProceduralMeshComponent>(ProcMeshComponents);
	for (const auto& ProcMeshComponent : ProcMeshComponents)
	{
		if (ProcMeshComponent->ComponentHasTag("GlobalMesh"))
		{
			OutExistedAlready = true;
			return ProcMeshComponent;
		}
	}

	// Create the procedural mesh component if it doesn't exist already
	auto ProceduralMesh = NewObject<UProceduralMeshComponent>(GlobalMeshAnchor, TEXT("GlobalMesh"));
	ProceduralMesh->ComponentTags.Add("GlobalMesh");
	ProceduralMesh->RegisterComponent();
	OutExistedAlready = false;
	return ProceduralMesh;
}

void AMRUKRoom::SetupGlobalMeshProceduralMeshComponent(UProceduralMeshComponent& ProcMeshComponent, bool ExistedAlready, UMaterialInterface* Material)
{
	ProcMeshComponent.SetMaterial(0, Material);
	if (!ExistedAlready)
	{
		ProcMeshComponent.SetCollisionProfileName(TEXT("BlockAll"));
		GlobalMeshAnchor->AddOwnedComponent(GlobalMeshAnchor->GetRootComponent());
		ProcMeshComponent.AttachToComponent(GlobalMeshAnchor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		ProcMeshComponent.SetRelativeScale3D(FVector(GetWorld()->GetWorldSettings()->WorldToMeters));
	}
}

bool AMRUKRoom::LoadGlobalMeshFromDevice(UMaterialInterface* Material)
{
	if (!GlobalMeshAnchor)
	{
		UE_LOG(LogMRUK, Warning, TEXT("This room doesn't have a global mesh anchor"));
		return false;
	}

	bool ProcMeshExisted = false;
	UProceduralMeshComponent* ProcMesh = GetOrCreateGlobalMeshProceduralMeshComponent(ProcMeshExisted);

	if (!UMRUKBPLibrary::LoadGlobalMeshFromDevice(GlobalMeshAnchor->SpaceQueryResult, ProcMesh, true, GetWorld()))
	{
		UE_LOG(LogMRUK, Warning, TEXT("Could not load Global Mesh from device"));
		ProcMesh->DestroyComponent();
		return false;
	}

	SetupGlobalMeshProceduralMeshComponent(*ProcMesh, ProcMeshExisted, Material);

	return true;
}

bool AMRUKRoom::LoadGlobalMeshFromJsonString(const FString& JsonString, UMaterialInterface* Material)
{
	if (!GlobalMeshAnchor)
	{
		UE_LOG(LogMRUK, Warning, TEXT("This room doesn't have a global mesh anchor"));
		return false;
	}

	bool ProcMeshExisted = false;
	UProceduralMeshComponent* ProcMesh = GetOrCreateGlobalMeshProceduralMeshComponent(ProcMeshExisted);

	if (!UMRUKBPLibrary::LoadGlobalMeshFromJsonString(JsonString, SpaceQueryResult, ProcMesh, true))
	{
		UE_LOG(LogMRUK, Warning, TEXT("Could not load Global Mesh from JSON"));
		ProcMesh->DestroyComponent();
		return false;
	}

	SetupGlobalMeshProceduralMeshComponent(*ProcMesh, ProcMeshExisted, Material);

	return true;
}

void AMRUKRoom::AttachProceduralMeshToWalls(const TArray<FMRUKTexCoordModes>& WallTextureCoordinateModes, UMaterialInterface* ProceduralMaterial)
{
	TArray<FMRUKAnchorWithPlaneUVs> AnchorsWithPlaneUVs;
	ComputeWallMeshUVAdjustments(WallTextureCoordinateModes, AnchorsWithPlaneUVs);
	for (const auto& AnchorWithPlaneUVs : AnchorsWithPlaneUVs)
	{
		AnchorWithPlaneUVs.Anchor->AttachProceduralMesh(AnchorWithPlaneUVs.PlaneUVs, true, ProceduralMaterial);
	}
}

TArray<AMRUKAnchor*> AMRUKRoom::ComputeConnectedWalls() const
{
	if (WallAnchors.IsEmpty())
	{
		return {};
	}

	TArray<AMRUKAnchor*> ConnectedWalls;
	TArray<AMRUKAnchor*> RemainingWalls = WallAnchors;

	for (int i = RemainingWalls.Num() - 1; i >= 0; --i)
	{
		if (RemainingWalls[i] == nullptr)
		{
			RemainingWalls.RemoveAt(i);
		}
	}

	ConnectedWalls.Reserve(WallAnchors.Num());
	ConnectedWalls.Push(RemainingWalls.Last());
	RemainingWalls.Pop();

	while (!RemainingWalls.IsEmpty())
	{
		auto PrevWall = ConnectedWalls.Last();
		FVector LocalMaxEdge(0, PrevWall->PlaneBounds.Max.X, 0);
		auto MaxEdge = PrevWall->GetTransform().TransformPosition(LocalMaxEdge);
		int ClosestIndex = 0;
		float ClosestDist = 3.402823466e+38F;
		for (int i = 0; i < RemainingWalls.Num(); i++)
		{
			const auto& WallAnchor = RemainingWalls[i];
			FVector LocalMinEdge(0, WallAnchor->PlaneBounds.Min.X, 0);
			auto MinEdge = WallAnchor->GetTransform().TransformPosition(LocalMinEdge);
			float Dist = FVector::Dist2D(MaxEdge, MinEdge);
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				ClosestIndex = i;
			}
		}
		ConnectedWalls.Push(RemainingWalls[ClosestIndex]);
		RemainingWalls.RemoveAt(ClosestIndex);
	}

	return ConnectedWalls;
}

TArray<AActor*> AMRUKRoom::SpawnInterior(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, UMaterialInterface* ProceduralMaterial)
{
	return SpawnInteriorFromStream(SpawnGroups, FRandomStream(NAME_None), ProceduralMaterial);
}

TArray<AActor*> AMRUKRoom::SpawnInteriorFromStream(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, const FRandomStream& RandomStream, UMaterialInterface* ProceduralMaterial)
{
	TArray<AActor*> InteriorActors;

	float WorldToMeters = GetWorldSettings()->WorldToMeters;
	auto WallFace = SpawnGroups.Find(FMRUKLabels::WallFace);
	if (!WallFace || (WallFace->Actors.IsEmpty() && WallFace->FallbackToProcedural))
	{
		// If no wall mesh is given we want to spawn the walls procedural to make seemles UVs
		AttachProceduralMeshToWalls(ProceduralMaterial);
	}
	auto Floor = SpawnGroups.Find(FMRUKLabels::Floor);
	if (FloorAnchor && (!Floor || (Floor->Actors.IsEmpty() && Floor->FallbackToProcedural)))
	{
		// Use metric scaling to match walls
		FVector2D Scale = FloorAnchor->PlaneBounds.GetSize() / WorldToMeters;
		FloorAnchor->AttachProceduralMesh({ { FVector2D::ZeroVector, Scale } }, true, ProceduralMaterial);
	}
	auto Ceiling = SpawnGroups.Find(FMRUKLabels::Ceiling);
	if (CeilingAnchor && (!Ceiling || (Ceiling->Actors.IsEmpty() && Ceiling->FallbackToProcedural)))
	{
		// Use metric scaling to match walls
		FVector2D Scale = CeilingAnchor->PlaneBounds.GetSize() / WorldToMeters;
		CeilingAnchor->AttachProceduralMesh({ { FVector2D::ZeroVector, Scale } }, true, ProceduralMaterial);
	}
	const auto Subsystem = GetGameInstance()->GetSubsystem<UMRUKSubsystem>();

	for (const auto& Anchor : AllAnchors)
	{
		if (!Anchor)
		{
			continue;
		}
		if (Anchor->SemanticClassifications.IsEmpty())
		{
			Anchor->AttachProceduralMesh();
			continue;
		}

		bool SpawnProceduralMesh = true;
		for (const auto& SemanticClassification : Anchor->SemanticClassifications)
		{
			const auto SpawnGroup = SpawnGroups.Find(SemanticClassification);
			if (!SpawnGroup)
			{
				continue;
			}
			if (SpawnGroup->Actors.IsEmpty())
			{
				if (!SpawnGroup->FallbackToProcedural)
				{
					SpawnProceduralMesh = false;
				}
				continue;
			}

			SpawnProceduralMesh = false;

			int Index = 0;
			if (SpawnGroup->Actors.Num() > 1)
			{
				if (SpawnGroup->SelectionMode == EMRUKSpawnerSelectionMode::Random)
				{
					Index = RandomStream.RandRange(0, SpawnGroup->Actors.Num() - 1);
				}
				else if (SpawnGroup->SelectionMode == EMRUKSpawnerSelectionMode::ClosestSize)
				{
					if (Anchor->VolumeBounds.IsValid)
					{
						float AnchorSize = FMath::Pow(Anchor->VolumeBounds.GetVolume(), 1.0 / 3.0);
						float ClosestSizeDifference = 3.4e+38f;
						for (int i = 0; i < SpawnGroup->Actors.Num(); ++i)
						{
							const auto& SpawnActor = SpawnGroup->Actors[i];
							auto Bounds = Subsystem->GetActorClassBounds(SpawnActor.Actor);
							if (Bounds.IsValid)
							{
								float SpawnActorSize = FMath::Pow(Bounds.GetVolume(), 1.0 / 3.0);
								float SizeDifference = FMath::Abs(AnchorSize - SpawnActorSize);
								if (SizeDifference < ClosestSizeDifference)
								{
									ClosestSizeDifference = SizeDifference;
									Index = i;
								}
							}
						}
					}
				}
			}

			const auto& SpawnActor = SpawnGroup->Actors[Index];
			if (SpawnActor.Actor)
			{
				auto InteriorActor = Anchor->SpawnInterior(SpawnActor.Actor, SpawnActor.MatchAspectRatio, SpawnActor.CalculateFacingDirection, SpawnActor.ScalingMode);
				InteriorActors.Push(InteriorActor);
			}
			else
			{
				UE_LOG(LogMRUK, Error, TEXT("Actor is nullptr for label %s."), *SemanticClassification);
			}
			break;
		}

		if (SpawnProceduralMesh)
		{
			Anchor->AttachProceduralMesh(true, ProceduralMaterial);
		}
	}

	return InteriorActors;
}

bool AMRUKRoom::IsWallAnchor(AMRUKAnchor* Anchor) const
{
	return WallAnchors.Contains(Anchor);
}

void AMRUKRoom::UpdateWorldLock(APawn* Pawn, const FVector& HeadWorldPosition)
{
	AnchorMesh.UpdateWorldLock(Pawn, HeadWorldPosition);
}

#undef LOCTEXT_NAMESPACE
