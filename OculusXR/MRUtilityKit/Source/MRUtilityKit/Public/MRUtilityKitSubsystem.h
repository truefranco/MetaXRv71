/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Dom/JsonObject.h"
#include "OculusRoomLayoutManagerComponent.h"
#include "MRUtilityKitRoom.h"
#include "MRUtilityKit.h"
#include "MRUtilityKitData.h"
#include "MRUtilityKitAnchorActorSpawner.h"
#include "MRUtilityKitSubsystem.generated.h"

/**
 * The scene toolkit subsystem allows to load and access the scene data.
 */
UCLASS(ClassGroup = MRUtilityKit)
class MRUTILITYKIT_API UMRUKSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoaded, bool, Success);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureComplete, bool, Success);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomCreated, AMRUKRoom*, Room);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomUpdated, AMRUKRoom*, Room);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomRemoved, AMRUKRoom*, Room);

	/**
	 * The status of the scene loading. When loading from device this is an asynchronous process
	 * so will be in the Busy state until it moves to Complete or Failed.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	EMRUKInitStatus SceneLoadStatus = EMRUKInitStatus::None;

	/**
	 * An event that will trigger when a scene is loaded either from Device or from JSON.
	 * The Success parameter indicates whether the scene was loaded successfully or not.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnLoaded OnSceneLoaded;

	/**
	 * An event that gets fired after a room has been created.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnRoomCreated OnRoomCreated;

	/**
	 * An event that gets fired after a room has been updated.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnRoomUpdated OnRoomUpdated;

	/**
	 * An event that gets fired when a room gets removed.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnRoomRemoved OnRoomRemoved;

	/**
	 * An event that will trigger when the capture flow completed.
	 * The Success parameter indicates whether the scene was captured successfully or not.
	 */
	UPROPERTY(BlueprintAssignable, Category = "MR Utility Kit")
	FOnCaptureComplete OnCaptureComplete;

	/**
	 * Contains a list of rooms that are tracked by the scene toolkit subsystem.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	TArray<AMRUKRoom*> Rooms;

	/**
	 * When world locking is enabled the position of the VR Pawn will be adjusted each frame to ensure
	 * the room anchors are where they should be relative to the camera position. This is necessary to
	 * ensure the position of the virtual objects in the world do not get out of sync with the real world.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	bool EnableWorldLock = true;

	/**
	 * Cast a ray and return the closest hit anchor in the scene.
	 * @Param Origin    Origin The origin of the ray.
	 * @Param Direction Direction The direction of the ray.
	 * @Param MaxDist   The maximum distance the ray should travel.
	 * @Param OutHit    The closest hit.
	 * @Return          The anchor that the ray hit
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit", meta = (AutoCreateRefTerm = "LabelFilter"))
	AMRUKAnchor* Raycast(const FVector& Origin, const FVector& Direction, float MaxDist, const FMRUKLabelFilter& LabelFilter, FMRUKHit& OutHit);

	/**
	 * Cast a ray and collect hits against the volumes and plane bounds in every room in the scene.
     * The order of the hits in the array is not specified.
	 * @Param Origin     Origin The origin of the ray.
	 * @Param Direction  Direction The direction of the ray.
	 * @Param MaxDist    The maximum distance the ray should travel.
	 * @Param OutHits    The hits the ray collected.
	 * @Param OutAnchors The anchors that were hit. Each anchor in this array corresponds to a entry at the same position in OutHits.
	 * @Return           Whether the ray hit anything
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit", meta = (AutoCreateRefTerm = "LabelFilter"))
	bool RaycastAll(const FVector& Origin, const FVector& Direction, float MaxDist, const FMRUKLabelFilter& LabelFilter, TArray<FMRUKHit>& OutHits, TArray<AMRUKAnchor*>& OutAnchors);

	/**
	 * Return the room that the user is currently in. (NOTE: This is not yet fully implemented and only returns the first room).
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	AMRUKRoom* GetCurrentRoom() const;

	/**
	 * Save all rooms and anchors to JSON. This JSON representation can than later be used by
	 * LoadSceneFromJsonString() to load the scene again.
	 * @Return the JSON string.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	FString SaveSceneToJsonString();

	/**
	 * Load rooms and anchors from a JSON representation.
	 * If the scene is already loaded the scene will be updated with the changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	void LoadSceneFromJsonString(const FString& String);

	/**
	 * Load rooms and anchors from the device.
	 * If the scene is already loaded the scene will be updated with the changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	void LoadSceneFromDevice(int MaxQueries = 64);

	/**
	 * Removes and clears every room.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	void ClearScene();

	/**
	 *  Get the position on the surface that is closest to the given position with respect to the distance in all rooms.
	 *  @param WorldPosition      The position in world space from which the closest surface point should be found.
	 *  @param OutSurfacePosition The closest position on the closest surface if any. Otherwise zero.
	 *  @param LabelFilter        The label filter can be used to include/exclude certain labels from the search.
	 *  @param MaxDistance        The distance to which a closest surface position should be searched. Everything below or equal to zero will be treated as infinity.
	 *  @return                   The Anchor on which the closest surface position was found or a null pointer otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit", meta = (AutoCreateRefTerm = "LabelFilter"))
	AMRUKAnchor* TryGetClosestSurfacePosition(const FVector& WorldPosition, FVector& OutSurfacePosition, const FMRUKLabelFilter& LabelFilter, float MaxDistance = 0.0);

	/**
	 *	Checks if the given position is on or inside of any scene volume in the rooms.
	 *  All rooms will be checked and the first anchors scene volume that has the point on or inside it will be returned.
	 *	@param WorldPosition      The position in world space to check
	 *	@param TestVerticalBounds Whether the vertical bounds should be checked or not
	 *	@return					  The anchor the WorldPosition is in. A null pointer otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	AMRUKAnchor* IsPositionInSceneVolume(const FVector& WorldPosition, bool TestVerticalBounds = true, float Tolerance = 0.0);

	/**
	 * Spawn meshes on the position of the anchors of each room.
	 * The actors should have Z as up Y as right and X as forward.
	 * The pivot point should be in the bottom center.
	 * @Param ActorClasses A map wich tells to spawn which actor to a given label.
	 * @Param RandomStream A random generator to choose randomly between actor classes if there a multiple for one label.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	TArray<AActor*> SpawnInterior(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, UMaterialInterface* ProceduralMaterial = nullptr);

	/**
	 * Spawn meshes on the position of the anchors of each room from a random stream.
	 * The actors should have Z as up Y as right and X as forward.
	 * The pivot point should be in the bottom center.
	 * @Param ActorClasses A map wich tells to spawn which actor to a given label.
	 * @Param RandomStream A random generator to choose randomly between actor classes if there a multiple for one label.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	TArray<AActor*> SpawnInteriorFromStream(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, const FRandomStream& RandomStream, UMaterialInterface* ProceduralMaterial = nullptr);

	/**
	 * Launch the scene capture. After a successful capture the scene should be updated.
	 * @Return Whether the capture was successful.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool LaunchSceneCapture();

public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	TSharedRef<FJsonObject> JsonSerialize();
	void UnregisterRoom(AMRUKRoom* Room);
	// Calculate the bounds of an Actor class and return it, the result is saved in a cache for faster lookup.
	FBox GetActorClassBounds(TSubclassOf<AActor> Actor);
	UOculusRoomLayoutManagerComponent* GetRoomLayoutManager();

private:
	UOculusRoomLayoutManagerComponent* RoomLayoutManager = nullptr;

	AMRUKRoom* SpawnRoom();

	void FinishedLoading(bool success);

	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual ETickableTickType GetTickableTickType() const override { return (HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMRUKSubsystem, STATGROUP_Tickables); }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	// ~FTickableGameObject interface

	UFUNCTION()
	void SceneDataLoadedComplete(bool Success);
	UFUNCTION()
	void UpdatedSceneDataLoadedComplete(bool Success);
	UFUNCTION()
	void SceneCaptureComplete(FUInt64 RequestId, bool bSuccess);

	UPROPERTY()
	UMRUKSceneData* SceneData = nullptr;

	TMap<TSubclassOf<AActor>, FBox> ActorClassBoundsCache;

	AActor* RoomLayoutManagerActor = nullptr;
};
