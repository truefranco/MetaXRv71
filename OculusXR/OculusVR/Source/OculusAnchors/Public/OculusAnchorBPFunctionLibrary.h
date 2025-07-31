/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "OculusAnchorTypes.h"
#include "OculusFunctionLibrary.h"
#include "OculusAnchorComponents.h"
#include "OculusAnchorBPFunctionLibrary.generated.h"

//Helper
UCLASS()
class OCULUSANCHORS_API UOculusAnchorBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Spawn Oculus Anchor Actor", WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"), Category = "Oculus|SpatialAnchor")
	static AActor* SpawnActorWithAnchorHandle(UObject* WorldContextObject, FUInt64 Handle, FUUID UUID, EOculusSpaceStorageLocation AnchorLocation, UClass* ActorClass, AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingMethod);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Spawn Oculus Anchor Actor From Query", WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"), Category = "Oculus|SpatialAnchor")
	static AActor* SpawnActorWithAnchorQueryResults(UObject* WorldContextObject, const FOculusSpaceQueryResult& QueryResult, UClass* ActorClass, AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionHandlingMethod);

	UFUNCTION(BlueprintCallable, Category = "Oculus|SpatialAnchor")
	static bool GetAnchorComponentStatus(AActor* TargetActor, EOculusSpaceComponentType ComponentType, bool& bIsEnabled);

	UFUNCTION(BlueprintCallable, Category = "Oculus|SpatialAnchor")
	static bool GetAnchorTransformByHandle(const FUInt64& Handle, FTransform& OutTransform);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "FUInt64 To String", CompactNodeTitle = "->", BlueprintAutocast), Category = "Oculus|SpatialAnchor")
	static FString AnchorHandleToString(const FUInt64 Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "FUUID To String", CompactNodeTitle = "->", BlueprintAutocast), Category = "Oculus|SpatialAnchor")
	static FString AnchorUUIDToString(const FUUID& Value);

	UFUNCTION(BlueprintCallable, Category = "Oculus|SpatialAnchor")
	static FUUID StringToAnchorUUID(const FString& Value);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "FUInt64 equal", CompactNodeTitle = "==", Keywords = "equal", BlueprintAutocast), Category = "Oculus|SpatialAnchor")
	static bool IsEqual_FUInt64(const FUInt64 Left, const FUInt64 Right) { return Left == Right; };

	UFUNCTION(BlueprintPure, meta = (DisplayName = "FUUID equal", CompactNodeTitle = "==", Keywords = "equal", BlueprintAutocast), Category = "Oculus|SpatialAnchor")
	static bool IsEqual_FUUID(const FUUID& Left, const FUUID& Right) { return Left.IsEqual(Right); };

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	static bool IsAnchorResultSuccess(EOculusResult::Type result);

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	static const UOculusBaseAnchorComponent* GetAnchorComponent(const FOculusSpaceQueryResult& QueryResult, EOculusSpaceComponentType ComponentType, UObject* Outer);

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	static bool GetRoomLayout(FUInt64 Space, FOculusRoomLayout& RoomLayoutOut, int32 MaxWallsCapacity = 64);
};