/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchorComponents.h"

#include "OculusAnchorBPFunctionLibrary.h"
#include "OculusAnchorManager.h"
#include "OculusRoomLayoutManager.h"
#include "OculusSpatialAnchorComponent.h"

bool UOculusBaseAnchorComponent::IsComponentEnabled() const
{
	bool OutEnabled;
	bool OutChangePending;

	auto OutResult = OculusAnchors::FOculusAnchorManager::GetSpaceComponentStatus(Space, Type, OutEnabled, OutChangePending);
	return UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OutResult) && OutEnabled;
}

EOculusSpaceComponentType UOculusBaseAnchorComponent::GetType() const
{
	return Type;
}

uint64 UOculusBaseAnchorComponent::GetSpace() const
{
	return Space;
}

bool UOculusLocatableAnchorComponent::GetTransform(FTransform& outTransform) const
{
	ensure(IsComponentEnabled());

	if (!UOculusAnchorBPFunctionLibrary::GetAnchorTransformByHandle(Space, outTransform))
	{
		UE_LOG(LogOculusSpatialAnchor, Warning, TEXT("Fetching transform failed."));
		return false;
	}
	return true;
}

bool UOculusPlaneAnchorComponent::GetPositionAndSize(FVector& outPosition, FVector& outSize) const
{
	ensure(IsComponentEnabled());

	if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OculusAnchors::FOculusAnchorManager::GetSpaceScenePlane(Space, outPosition, outSize)))
	{
		UE_LOG(LogOculusSpatialAnchor, Warning, TEXT("Fetching scene plane failed."));
		return false;
	}

	return true;
}

bool UOculusVolumeAnchorComponent::GetPositionAndSize(FVector& outPosition, FVector& outSize) const
{
	ensure(IsComponentEnabled());

	if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OculusAnchors::FOculusAnchorManager::GetSpaceSceneVolume(Space, outPosition, outSize)))
	{
		UE_LOG(LogOculusSpatialAnchor, Warning, TEXT("Fetching scene plane failed."));
		return false;
	}

	return true;
}

bool UOculusSemanticClassificationAnchorComponent::GetSemanticClassifications(TArray<FString>& outClassifications) const
{
	ensure(IsComponentEnabled());

	if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OculusAnchors::FOculusAnchorManager::GetSpaceSemanticClassification(Space, outClassifications)))
	{
		UE_LOG(LogOculusSpatialAnchor, Warning, TEXT("Fetching scene volume failed."));
		return false;
	}

	return true;
}

bool UOculusXRRoomLayoutAnchorComponent::GetRoomLayout(FUUID& outFloorUUID, FUUID& outCeilingUUID, TArray<FUUID>& outWallsUUIDs) const
{
	ensure(IsComponentEnabled());

	if (!OculusAnchors::FOculusRoomLayoutManager::GetSpaceRoomLayout(Space, 64, outCeilingUUID, outFloorUUID, outWallsUUIDs))
	{
		UE_LOG(LogOculusSpatialAnchor, Warning, TEXT("Fetching room layout failed."));
		return false;
	}

	return true;
}

bool UOculusSpaceContainerAnchorComponent::GetUUIDs(TArray<FUUID>& outUUIDs) const
{
	ensure(IsComponentEnabled());

	if (!OculusAnchors::FOculusAnchorManager::GetSpaceContainer(Space, outUUIDs))
	{
		UE_LOG(LogOculusSpatialAnchor, Warning, TEXT("Fetching container uuids failed."));
		return false;
	}

	return true;
}
