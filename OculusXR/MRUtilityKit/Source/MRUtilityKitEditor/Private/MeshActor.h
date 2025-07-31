/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "GameFramework/Actor.h"
#include "MeshActor.generated.h"

UCLASS()
class AMeshActor : public AActor
{
	GENERATED_BODY()

public:
	UStaticMeshComponent* Mesh;

public:
	AMeshActor();
};
