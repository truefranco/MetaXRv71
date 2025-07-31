/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "OculusAnchorComponent.h"
#include "OculusAnchors.h"
#include "OculusSpatialAnchorComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOculusSpatialAnchor, Log, All);

UCLASS(meta = (DisplayName = "Oculus Spatial Anchor Component", BlueprintSpawnableComponent))
class OCULUSANCHORS_API UOculusSpatialAnchorComponent : public UOculusAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusSpatialAnchorComponent(const FObjectInitializer& ObjectInitializer);

	static bool Create(const FTransform& NewAnchorTransform, AActor* OwningActor, const FOculusSpatialAnchorCreateDelegate& Callback);

	bool Erase(const FOculusAnchorEraseDelegate& Callback);
	bool Save(EOculusSpaceStorageLocation Location, const FOculusAnchorSaveDelegate& Callback);

private:
};