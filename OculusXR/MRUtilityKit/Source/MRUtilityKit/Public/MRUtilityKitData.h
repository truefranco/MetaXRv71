/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "GameFramework/Actor.h"
#include "OculusRoomLayoutManagerComponent.h"
#include "Dom/JsonValue.h"
#include "MRUtilityKitData.generated.h"

/**
 * Actor to help finding the localization of actors.
 * It gets a list of all anchor queries that should be localized
 * and checks every tick if the anchor localization is there.
 * When the localization is complete, it will emit the event OnComplete.
 *
 * NOTE: Normally this should be a async task. However, the anchor data
 * can only be queried in game thread.
 */
UCLASS(ClassGroup = MRUtilityKit)
class MRUTILITYKIT_API AMRUKLocalizer : public AActor
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComplete, bool, Success);

	/**
	 * Event that gets fired when all anchors have been localized.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnComplete OnComplete;

	TArray<class UMRUKAnchorData*> AnchorsData;

	AMRUKLocalizer();

	void Tick(float DeltaTime) override;
};

/**
 * Load anchor data from device.
 */
UCLASS(ClassGroup = MRUtilityKit)
class MRUTILITYKIT_API UMRUKAnchorData : public UObject
{
	GENERATED_BODY()

public:
	FOculusSpaceQueryResult SpaceQuery;
	FTransform Transform;

	FBox2D PlaneBounds;
	FBox VolumeBounds;

	TArray<FString> SemanticClassifications;
	TArray<FVector2D> PlaneBoundary2D;

	bool NeedAnchorLocalization = false;

	void LoadFromDevice(FOculusSpaceQueryResult SpaceQuery, int32 MaxQueries = 64);
	void LoadFromJson(const FJsonValue& Value);
};

/**
 * Load room data from device.
 * When all room data has been loaded, the OnComplete event will be fired.
 */
UCLASS(ClassGroup = MRUtilityKit)
class MRUTILITYKIT_API UMRUKRoomData : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComplete, bool, Success);

	/**
	 * Event that gets fired after all room data has been loaded.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnComplete OnComplete;

	FOculusSpaceQueryResult SpaceQuery;
	FOculusRoomLayout RoomLayout;

	UPROPERTY()
	TArray<UMRUKAnchorData*> AnchorsData;

	AMRUKLocalizer* LocalizationActor = nullptr;

	void LoadFromDevice(FOculusSpaceQueryResult SpaceQuery, int32 MaxQueries = 64);
	void LoadFromJson(const FJsonValue& Value);

private:
	void FinishQuery(bool Success);
	void RoomQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults, const FUInt64 RoomSpaceID);
	UFUNCTION()
	void AnchorsInitialized(bool Success);
};

/**
 * Load scene data from device.
 * When all scene data has been loaded, the OnComplete event will be fired.
 */
UCLASS(ClassGroup = MRUtilityKit)
class MRUTILITYKIT_API UMRUKSceneData : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComplete, bool, Success);

	/**
	 * Event that gets fired after all scene data has been loaded.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnComplete OnComplete;

	UPROPERTY()
	TArray<UMRUKRoomData*> RoomsData;

	void LoadFromDevice(int32 MaxQueries = 64);
	void LoadFromJson(const FString& Json);

private:
	int32 NumRoomsLeftToInitialize = 0;
	bool AnyRoomFailed = false;

	void FinishQuery(bool Success);
	void SceneDataLoadedComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults);
	UFUNCTION()
	void RoomQueryComplete(bool Success);
};
