/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusSceneAnchorComponent.h"

#include "Engine/StaticMeshActor.h"


UOculusSceneAnchorComponent::UOculusSceneAnchorComponent(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	bUpdateHeadSpaceTransform = false;
}

void UOculusSceneAnchorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetHandle().Value == 0)
	{
		return;
	}
}