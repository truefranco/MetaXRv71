/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "OculusAnchorManager.h"

namespace OculusAnchors
{
	struct FOculusSpatialAnchorManager : FOculusAnchorManager
	{
		FOculusSpatialAnchorManager()
			: FOculusAnchorManager()
		{
		}

		static bool CreateSpatialAnchor(const FTransform &InTransform, uint64 &OutRequestId);
	};	
}
