/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "GameFramework/Actor.h"
#include "MRUtilityKit.h"
#include "MRUtilityKitAnchorActorSpawner.generated.h"

/**
 * Spawns meshes on anchor positions.
 */
UCLASS(ClassGroup = MRUtilityKit, meta = (DisplayName = "MR Utility Kit Anchor Actor Spawner"))
class MRUTILITYKIT_API AMRUKAnchorActorSpawner : public AActor
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteriorSpawned);

	/**
	 * Event that gets fired when the interior spawner finished spawning actors.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnInteriorSpawned OnActorsSpawned;

	/**
	 * Seed to use for the random generator that decideds wich actor class to 
	 * spawn if there a given multiple for a label.
	 * negative values will have the effect to initialize the random generator
	 * to a random seed.
	 */
	UPROPERTY(EditAnywhere, Category = "MR Utility Kit")
	int AnchorRandomSpawnSeed = -1;

	/**
	 * Whether SpawnInterior() should be called automatically after the scene 
	 * toolkit has been initialized.
	 */
	UPROPERTY(EditAnywhere, Category = "MR Utility Kit")
	bool SpawnOnStart = true;

	/**
	 * Material to use when falling back to procedural material.
	 */
	UPROPERTY(EditAnywhere, Category = "MR Utility Kit")
	UMaterialInterface* ProceduralMaterial = nullptr;

	/**
	 * A map of Actor classes to spawn for the given label.
	 */
	UPROPERTY(EditAnywhere, Category = "MR Utility Kit")
	TMap<FString, FMRUKSpawnGroup> SpawnGroups{
		{ FMRUKLabels::Bed, {} },
		{ FMRUKLabels::Ceiling, {} },
		{ FMRUKLabels::Couch, {} },
		{ FMRUKLabels::DoorFrame, {} },
		{ FMRUKLabels::Floor, {} },
		{ FMRUKLabels::Lamp, {} },
		{ FMRUKLabels::Plant, {} },
		{ FMRUKLabels::Screen, {} },
		{ FMRUKLabels::Storage, {} },
		{ FMRUKLabels::Table, {} },
		{ FMRUKLabels::WallArt, {} },
		{ FMRUKLabels::WallFace, {} },
		{ FMRUKLabels::WindowFrame, {} },
		{ FMRUKLabels::Other, {} },
	};

	/**
	 * The spawned interior actors.
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "MR Utility Kit")
	TArray<AActor*> InteriorActors;

	/**
	 * Spawns the meshes for the given labels above on the anchor positions in each room.
	 * There might be multiple actor classes for a give label. If thats the case a actor class will be chosen radomly. 
	 * The seed for this random generator can be set by AnchorRandomSpawnSeed.
	 * This function will be called automatically after the scene toolkit initialized unless
	 * the option SpawnOnStart is set to false.
	 * If there is no actor class specified for a label then a procedural mesh matching the anchors volume and plane
	 * will be generated.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	void SpawnInterior();

protected:
	void BeginPlay() override;

	UFUNCTION()
	void OnSceneLoaded(bool Success);

private:
	int32 LastSeed = -1;
};
