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
	struct FOculusRoomLayoutManager
	{
		static bool RequestSceneCapture(uint64& OutRequestID);
		static bool GetSpaceRoomLayout(const uint64 Space, const uint32 MaxWallsCapacity,
									   FUUID &OutCeilingUuid, FUUID &OutFloorUuid, TArray<FUUID>& OutWallsUuid);
		static bool GetSpaceTriangleMesh(uint64 Space, TArray<FVector>& Vertices, TArray<int32>& Triangles);
		
		static void OnPollEvent(ovrpEventDataBuffer* EventDataBuffer, bool& EventPollResult);
	};
}


