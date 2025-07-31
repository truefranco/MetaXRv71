// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "OculusRoomLayoutManagerComponent.h"
#include "OculusAnchorComponent.h"
#include "OculusSceneAnchorComponent.h"
#include "OculusSceneActor.generated.h"


/** ELaunchCaptureFlowWhenMissingScene
* Used to dictate whether the actor should launch the Capture Flow application when a scene is not detected on the device.
* The Actor will check if a scene capture is either non-existent or invalid (ie. missing walls/ceiling/floor) before checking if Capture Flow 
* should be launched.
* 
* NEVER:	will never launch Flow Capture.
* ONCE:		will only launch it once.  If the actor still doesn't detect that a scene was captured, it will not launch Capture Flow again.
* ALWAYS:	will always re-launch Flow Capture if a scene was not detected on the device.
*/
UENUM(BlueprintType)
enum ELaunchCaptureFlowWhenMissingScene
{
	NEVER	UMETA(DisplayName = "Never"),
	ONCE	UMETA(DisplayName = "Once"),
	ALWAYS	UMETA(DisplayName = "Always")
};

/** FSpawnedSceneAnchorProperties
* Properties/Components that a spawned scene anchor will use.
*/
USTRUCT(BlueprintType)
struct FSpawnedSceneAnchorProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Spawned Scene Anchor Properties")
	TSoftClassPtr<UOculusSceneAnchorComponent> ActorComponent = TSoftClassPtr<UOculusSceneAnchorComponent>(FSoftClassPath(UOculusSceneAnchorComponent::StaticClass()));

	UPROPERTY(EditAnywhere, Category = "Spawned Scene Anchor Properties")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, Category = "Spawned Scene Anchor Properties")
	bool ForceParallelToFloor = false;

	UPROPERTY(EditAnywhere, Category = "Spawned Scene Anchor Properties")
	FVector AddOffset = FVector::ZeroVector;
};


/**
* AOculusSceneActor
* The purpose of this actor is to be able to spawn "scene anchor" actors.
* 
* Each actor type (based on their semantic label) can be configured to be spawned with a specific mesh and actor component.
* 
* Overall, it provides a simple interface to be able to quickly get a captured scene from Capture Flow populated at runtime.
* It also provides a basic and flexible template to making use of the OculusAnchorSDK and UOculusRoomLayoutManagerComponent 
* to drive the actor's logic.  This removes the need for the developer to implement a system from scratch that makes use of 
* the native methods and components.
*
* TLDR: 
* - This actor populates a captured scene (created in Capture Flow) by spawning child actors with predefined actor and mesh components.
* - Can be used as is, or can be derived or modified as needed depending on the application's needs.
*/
UCLASS(ClassGroup = OculusScene)
class OCULUSSCENE_API AOculusSceneActor : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	void LaunchCaptureFlow();

	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	bool IsScenePopulated();

	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	bool IsRoomLayoutValid();

	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	void PopulateScene();

	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	void ClearScene();

	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	void SetVisibilityToAllSceneAnchors(const bool bIsVisible);

	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	void SetVisibilityToSceneAnchorsBySemanticLabel(const FString SemanticLabel, const bool bIsVisible);

	UFUNCTION(BlueprintCallable, Category = "Oculus|Scene Actor")
	TArray<AActor*> GetActorsBySemanticLabel(const FString SemanticLabel);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Scene Actor")
	bool bEnsureRoomIsValid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Scene Actor")
	TEnumAsByte<ELaunchCaptureFlowWhenMissingScene> LauchCaptureFlowWhenMissingScene = ELaunchCaptureFlowWhenMissingScene::ALWAYS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Scene Actor", meta = (UIMin = 1, ClampMin = 1, UIMax = 1024, ClampMax = 1024))
	int32 MaxQueries = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Scene Actor")
	bool bVerboseLog = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Scene Actor")
	bool bPopulateSceneOnBeginPlay = true;

	// If true then when the scene model is loaded we will only attempt to populate the room the user is standing in.
	// Otherwise all rooms and all scene anchors will be loaded.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Scene Actor")
	bool bActiveRoomOnly = true;

	UPROPERTY(EditAnywhere, Category = "Oculus|Scene Actor")
	TMap<FString, FSpawnedSceneAnchorProperties> ScenePlaneSpawnedSceneAnchorProperties;

	UPROPERTY(EditAnywhere, Category = "Oculus|Scene Actor")
	TMap<FString, FSpawnedSceneAnchorProperties> SceneVolumeSpawnedSceneAnchorProperties;

public:
	AOculusSceneActor(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaTime) override;


private:
	EOculusResult::Type QueryAllRooms();
	void RoomLayoutQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults);

	EOculusResult::Type QueryRoomUUIDs(const FUInt64 RoomSpaceID, const TArray<FUUID>& RoomUUIDs);
	void SceneRoomQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults, const FUInt64 RoomSpaceID);

	void StartSingleRoomQuery(FUInt64 RoomSpaceID, FOculusRoomLayout RoomLayout);
	EOculusResult::Type QueryFloorForActiveRoom(FUInt64 RoomSpaceID, FOculusRoomLayout RoomLayout);
	void ActiveRoomFloorQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults, FUInt64 RoomSpaceID, FOculusRoomLayout RoomLayout);
	bool PointInPolygon2D(FVector2D PointToTest, const TArray<FVector2D>& PolyVerts) const;

	void GetSemanticClassifications(uint64 Space, TArray<FString>& OutSemanticLabels) const;

	void SceneCaptureComplete_Handler(FUInt64 RequestId, bool bResult);

	// Launches Capture Flow if (based on LauchCaptureFlowWhenMissingScene member value)
	void LaunchCaptureFlowIfNeeded();

	// Resets states of the Actor
	void ResetStates();

	// Validates UUID
	bool IsValidUuid(const FUUID& Uuid);

	// Spawns a scene anchor
	//bool SpawnSceneAnchor(const FUInt64& Space, const FVector& BoundedSize, const TArray<FString>& SemanticClassifications, const EOculusSpaceComponentType AnchorComponentType);
	AActor* SpawnActorWithSceneComponent(const FUInt64& Space, const FUInt64& RoomSpaceID, const TArray<FString>& SemanticClassifications, UClass* sceneAnchorComponentInstanceClass);

	// Spawns a scene anchor
	AActor* SpawnOrUpdateSceneAnchor(AActor* Anchor, const FUInt64& Space, const FUInt64& RoomSpaceID, const FVector& BoundedPos, const FVector& BoundedSize, const TArray<FString>& SemanticClassifications, const EOculusSpaceComponentType AnchorComponentType);

	// Components for room layout and spatial anchors functionalities
	UOculusRoomLayoutManagerComponent* RoomLayoutManagerComponent = nullptr;

	class UOculusSceneGlobalMeshComponent* SceneGlobalMeshComponent = nullptr;

	// Whether Capture Flow was already launched once
	bool bCaptureFlowWasLaunched;

	// Whether last room layout was valid
	bool bRoomLayoutIsValid;

	// Whether we found a captured scene
	bool bFoundCapturedScene;

	UPROPERTY(Transient)
	TMap<FUInt64, FOculusRoomLayout> RoomLayouts;
};
