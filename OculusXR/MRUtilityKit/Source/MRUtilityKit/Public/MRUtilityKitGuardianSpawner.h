/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "GameFramework/Actor.h"
#include "MRUtilityKitGuardian.h"
#include "MRUtilityKitGuardianSpawner.generated.h"

/**
 * Show a guardian if the player gets close to any furniture or walls.
 */
UCLASS(ClassGroup = MRUtilityKit, meta = (DisplayName = "MR Utility Kit Guardian"))
class MRUTILITYKIT_API AMRUKGuardianSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMRUKGuardianSpawner();

	/**
	 * Whether SpawnGuardian() should be called automatically after the scene 
	 * toolkit has been initialized.
	 */
	UPROPERTY(EditAnywhere, Category = "MR Utility Kit")
	bool SpawnOnStart = true;

	/**
	 * How close the camera needs to come to a surface before the guardian appears.
	 */
	UPROPERTY(EditAnywhere, Category = "MR Utility Kit")
	double GuardianDistance = 0.75;

	/**
	 * Whether the fading value should be calculated for the shader or not.
	 If fading is not needed this can save performance.
	 */
	UPROPERTY(EditAnywhere, Category = "MR Utility Kit")
	bool EnableFade = true;

	/**
	 * Spawn the guardian. This will get called automatically after the scene toolkit has 
	 * been initialized if SpawnOnStart is set to true.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	void SpawnGuardian();

	/**
	 * Set the guardian material to a different one.
	 * @Param Material The guardian material.
	 */
	UFUNCTION(BlueprintSetter, Category = "MR Utility Kit")
	void SetGuardianMaterial(UMaterialInstance* Material);

	/**
	 * Set the density of the grid.
	 * @Param Density The grid density.
	 */
	UFUNCTION(BlueprintSetter, Category = "MR Utility Kit")
	void SetGridDensity(float Density);

public:
	void Tick(float DeltaSeconds) override;

protected:
	/**
	 * The material to use for the guardian. It needs to have a scalar parameter Fade 
	 * and a vector parameter WallScale. If this material is not set a default one
	 * will be used.
	 */
	UPROPERTY(EditAnywhere, BlueprintSetter = SetGuardianMaterial, Category = "MR Utility Kit")
	UMaterialInstance* GuardianMaterial = nullptr;

	/**
	 * How dense the grid should be.
	 */
	UPROPERTY(EditAnywhere, BlueprintSetter = SetGridDensity, Category = "MR Utility Kit")
	float GridDensity = 2.0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MR Utility Kit")
	TArray<AMRUKGuardian*> GuardianActors;

	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UMaterialInstanceDynamic* DynamicGuardianMaterial = nullptr;
	void DestroyGuardians();
	UFUNCTION()
	void SceneLoaded(bool Success);
};
