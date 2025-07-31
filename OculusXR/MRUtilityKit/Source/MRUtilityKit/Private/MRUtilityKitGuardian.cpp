/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "MRUtilityKitGuardian.h"

AMRUKGuardian::AMRUKGuardian(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create a scene component as root so we can attach spawned actors to it
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
}

void AMRUKGuardian::CreateGuardian(UProceduralMeshComponent* GuardianMesh)
{
	GuardianMesh->SetupAttachment(RootComponent);
	GuardianMesh->RegisterComponent();
	GuardianMeshComponent = GuardianMesh;
}
