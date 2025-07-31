/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "MRUtilityKitBPLibrary.generated.h"

/**
 * Load the scene async from device.
 */
UCLASS()
class MRUTILITYKIT_API UMRUKLoadFromDevice : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMRUKLoaded);

	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit", meta = (WorldContext = "WorldContext", BlueprintInternalUseOnly = "true"))
	static UMRUKLoadFromDevice* LoadSceneFromDeviceAsync(const UObject* WorldContext);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FMRUKLoaded Success;

	UPROPERTY(BlueprintAssignable)
	FMRUKLoaded Failure;

private:
	UFUNCTION(CallInEditor)
	void OnSceneLoaded(bool Succeeded);

	TWeakObjectPtr<UWorld> World = nullptr;
};

UCLASS()
class MRUTILITYKIT_API UMRUKBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit", meta = (WorldContext = "WorldContext"))
	static bool LoadGlobalMeshFromDevice(FOculusSpaceQueryResult SpaceQuery, UProceduralMeshComponent* OutProceduralMesh, bool LoadCollision, const UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	static bool LoadGlobalMeshFromJsonString(const FString& JsonString, FOculusSpaceQueryResult SpaceQuery, UProceduralMeshComponent* OutProceduralMesh, bool LoadCollision);

	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	static void RecalculateProceduralMeshAndTangents(class UProceduralMeshComponent* Mesh);

	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	static bool IsUnrealEngineMetaFork();
};
