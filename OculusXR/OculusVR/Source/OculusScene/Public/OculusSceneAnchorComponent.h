/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "OculusAnchorComponent.h"
#include "OculusSceneAnchorComponent.generated.h"

UCLASS(meta = (DisplayName = "Oculus Scene Anchor Component", BlueprintSpawnableComponent))
class UOculusSceneAnchorComponent : public UOculusAnchorComponent
{
	GENERATED_BODY()

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UOculusSceneAnchorComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Oculus|Scene Anchor Component")
	TArray<FString> SemanticClassifications;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Oculus|Scene Anchor Component")
	FUInt64 RoomSpaceID = 0;
};
