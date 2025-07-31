/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "CoreMinimal.h"
#include "OculusAnchorComponent.h"
#include "OculusHMDPrivate.h"

namespace OculusAnchors
{
	struct OCULUSANCHORS_API FOculusAnchorManager
	{
		static EOculusResult::Type CreateAnchor(const FTransform& InTransform, uint64& OutRequestId, const FTransform& CameraTransform);
		static EOculusResult::Type DestroySpace(uint64 Space);
		static EOculusResult::Type SetSpaceComponentStatus(uint64 Space, EOculusSpaceComponentType SpaceComponentType, bool Enable,float Timeout, uint64& OutRequestId);
		static EOculusResult::Type GetSpaceComponentStatus(uint64 Space, EOculusSpaceComponentType SpaceComponentType, bool &OutEnabled, bool &OutChangePending);
		static EOculusResult::Type GetSupportedAnchorComponents(uint64 Handle, TArray<EOculusSpaceComponentType>& OutSupportedTypes);
		static EOculusResult::Type SaveAnchor(uint64 Space, EOculusSpaceStorageLocation StorageLocation, EOculusSpaceStoragePersistenceMode StoragePersistenceMode, uint64& OutRequestId);
		static EOculusResult::Type SaveAnchorList(const TArray<uint64>& Spaces, EOculusSpaceStorageLocation StorageLocation, uint64& OutRequestId);
		static EOculusResult::Type EraseAnchor(uint64 AnchorHandle, EOculusSpaceStorageLocation StorageLocation, uint64& OutRequestId);
		static EOculusResult::Type QuerySpaces(const FOculusSpaceQueryInfo& QueryInfo, uint64& OutRequestId);
		static EOculusResult::Type ShareSpaces(const TArray<uint64>& Spaces, const TArray<uint64>& UserIds, uint64& OutRequestId);
		static EOculusResult::Type GetSpaceContainerUUIDs(uint64 Space, TArray<FUUID>& OutUUIDs);
		static EOculusResult::Type GetSpaceScenePlane(uint64 Space, FVector& OutPos, FVector& OutSize);
		static EOculusResult::Type GetSpaceSceneVolume(uint64 Space, FVector& OutPos, FVector& OutSize);
		static EOculusResult::Type GetSpaceSemanticClassification(uint64 Space, TArray<FString>& OutSemanticClassification);
		static EOculusResult::Type GetSpaceContainer(uint64 Space, TArray<FUUID>& OutContainerUuids);
		static EOculusResult::Type GetSpaceBoundary2D(uint64 Space, TArray<FVector2D>& OutVertices);

		static void OnPollEvent(ovrpEventDataBuffer* EventDataBuffer, bool& EventPollResult);
	};
}


