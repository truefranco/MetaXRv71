/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "MRUtilityKitGuardian.generated.h"

UCLASS(ClassGroup = MRUtilityKit, meta = (DisplayName = "MR Utility Kit Guardian Actor"))
class MRUTILITYKIT_API AMRUKGuardian : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * Procedural mesh that is generated from the anchor geometry and has the guardian material applied.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	UProceduralMeshComponent* GuardianMeshComponent;

	/**
	 * Attaches the procedural mesh component to this actor.
	 * @Param GuardianMesh The mesh to attach.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	void CreateGuardian(UProceduralMeshComponent* GuardianMesh);

public:
	AMRUKGuardian(const FObjectInitializer& ObjectInitializer);
};
