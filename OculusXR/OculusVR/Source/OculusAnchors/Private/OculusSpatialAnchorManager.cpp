/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusSpatialAnchorManager.h"


namespace OculusAnchors
{
	bool FOculusSpatialAnchorManager::CreateSpatialAnchor(const FTransform& InTransform, uint64& OutRequestId)
	{
		EOculusResult::Type Result = CreateAnchor(InTransform, OutRequestId, FTransform::Identity);
		return UOculusFunctionLibrary::IsResultSuccess(Result);
	}
}