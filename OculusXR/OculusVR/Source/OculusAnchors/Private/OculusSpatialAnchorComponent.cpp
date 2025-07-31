/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusSpatialAnchorComponent.h"

DEFINE_LOG_CATEGORY(LogOculusSpatialAnchor);

UOculusSpatialAnchorComponent::UOculusSpatialAnchorComponent(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
}

bool UOculusSpatialAnchorComponent::Create(const FTransform& NewAnchorTransform, AActor* OwningActor, const FOculusSpatialAnchorCreateDelegate& Callback)
{
	EOculusResult::Type AnchorResult;
	return OculusAnchors::FOculusAnchors::CreateSpatialAnchor(NewAnchorTransform, OwningActor, Callback, AnchorResult);
}

bool UOculusSpatialAnchorComponent::Erase(const FOculusAnchorEraseDelegate& Callback)
{
	EOculusResult::Type AnchorResult;
	return OculusAnchors::FOculusAnchors::EraseAnchor(this, Callback, AnchorResult);
}

bool UOculusSpatialAnchorComponent::Save(EOculusSpaceStorageLocation Location, const FOculusAnchorSaveDelegate& Callback)
{
	EOculusResult::Type AnchorResult;
	return OculusAnchors::FOculusAnchors::SaveAnchor(this, Location, Callback, AnchorResult);
}
