/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "OculusAnchorTypes.h"
#include "OculusAnchorComponent.h"
#include "OculusAnchorComponents.h"
#include "OculusFunctionLibrary.h"
#include "OculusAnchorLatentActions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOculus_LatentAction_CreateSpatialAnchor_Success, UOculusAnchorComponent*, Anchor, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_CreateSpatialAnchor_Failure, EOculusResult::Type, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOculus_LatentAction_EraseAnchor_Success, AActor*, Actor, FUUID, UUID, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_EraseAnchor_Failure, EOculusResult::Type, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOculus_LatentAction_SaveAnchor_Success, UOculusAnchorComponent*, Anchor, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_SaveAnchor_Failure, EOculusResult::Type, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOculus_LatentAction_SaveAnchorList_Success, const TArray<UOculusAnchorComponent*>&, Anchors, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_SaveAnchorList_Failure, EOculusResult::Type, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOculus_LatentAction_QueryAnchors_Success, const TArray<FOculusSpaceQueryResult>&, QueryResults, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_QueryAnchors_Failure, EOculusResult::Type, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOculus_LatentAction_SetComponentStatus_Success, UOculusAnchorComponent*, Anchor, EOculusSpaceComponentType, ComponentType, bool, Enabled, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_SetComponentStatus_Failure, EOculusResult::Type, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOculus_LatentAction_SetAnchorComponentStatus_Success, UOculusBaseAnchorComponent*, Component, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_SetAnchorComponentStatus_Failure, EOculusResult::Type, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOculus_LatentAction_ShareAnchors_Success, const TArray<UOculusAnchorComponent*>&, SharedAnchors, const TArray<FString>&, UserIds, EOculusResult::Type, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOculus_LatentAction_ShareAnchors_Failure, EOculusResult::Type, Result);

//
// Create Anchor
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_CreateSpatialAnchor : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_CreateSpatialAnchor* OculusAsyncCreateSpatialAnchor(AActor* TargetActor, const FTransform& AnchorTransform);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_CreateSpatialAnchor_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_CreateSpatialAnchor_Failure Failure;

	// Target actor
	UPROPERTY(Transient)
	AActor* TargetActor;

	FTransform AnchorTransform;

private:
	void HandleCreateComplete(EOculusResult::Type CreateResult, UOculusAnchorComponent* Anchor);
};


//
// Erase Anchor
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_EraseAnchor : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_EraseAnchor* OculusAsyncEraseAnchor(AActor* TargetActor);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_EraseAnchor_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_EraseAnchor_Failure Failure;

	// Target actor
	UPROPERTY(Transient)
	AActor* TargetActor;

	FUInt64 DeleteRequestId;

private:
	void HandleEraseAnchorComplete(EOculusResult::Type EraseResult, FUUID UUID);
};


//
// Save Anchor
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_SaveAnchor : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_SaveAnchor* OculusAsyncSaveAnchor(AActor* TargetActor, EOculusSpaceStorageLocation StorageLocation);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SaveAnchor_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SaveAnchor_Failure Failure;

	// Target actor
	UPROPERTY(Transient)
	AActor* TargetActor;

	EOculusSpaceStorageLocation StorageLocation;

private:
	void HandleSaveAnchorComplete(EOculusResult::Type SaveResult, UOculusAnchorComponent* Anchor);
};


//
// Save Anchors List
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_SaveAnchorsList : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_SaveAnchorsList* OculusAsyncSaveAnchorsList(const TArray<AActor*>& TargetActors, EOculusSpaceStorageLocation StorageLocation);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SaveAnchorList_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SaveAnchorList_Failure Failure;

	UPROPERTY(Transient)
	TArray<UOculusAnchorComponent*> TargetAnchors;

	EOculusSpaceStorageLocation StorageLocation;

private:
	void HandleSaveAnchorsListComplete(EOculusResult::Type SaveResult, const TArray<UOculusAnchorComponent*>& SavedSpaces);
};


//
// Query Anchors
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_QueryAnchors : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_QueryAnchors* OculusAsyncQueryAnchors(EOculusSpaceStorageLocation Location, const TArray<FUUID>& UUIDs, int32 MaxAnchors);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_QueryAnchors* OculusAsyncQueryAnchorsAdvanced(const FOculusSpaceQueryInfo& QueryInfo);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_QueryAnchors_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_QueryAnchors_Failure Failure;

	FOculusSpaceQueryInfo QueryInfo;
	TArray<FOculusSpaceQueryResult> QueryResults;

private:
	void HandleQueryAnchorsResults(EOculusResult::Type QueryResult, const TArray<FOculusSpaceQueryResult>& Results);
};


//
// Set Component Status
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_SetAnchorComponentStatus : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_SetAnchorComponentStatus* OculusAsyncSetAnchorComponentStatus(AActor* TargetActor, EOculusSpaceComponentType ComponentType, bool bEnabled);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SetComponentStatus_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SetComponentStatus_Failure Failure;

	// Target actor
	UPROPERTY(Transient)
	AActor* TargetActor;

	UPROPERTY(Transient)
	UOculusAnchorComponent* TargetAnchorComponent;

	EOculusSpaceComponentType ComponentType;
	bool bEnabled;

private:
	void HandleSetComponentStatusComplete(EOculusResult::Type SetStatusResult, uint64 AnchorHandle, EOculusSpaceComponentType SpaceComponentType, bool bResultEnabled);
};

//
// Set Anchor Component Status
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_SetComponentStatus : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_SetComponentStatus* OculusAsyncSetComponentStatus(UOculusBaseAnchorComponent* Component, bool bEnabled);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SetAnchorComponentStatus_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_SetAnchorComponentStatus_Failure Failure;

	// Target actor
	UPROPERTY(Transient)
	UOculusBaseAnchorComponent* Component;
	bool bEnabled;

private:
	void HandleSetComponentStatusComplete(EOculusResult::Type SetStatusResult, uint64 AnchorHandle, EOculusSpaceComponentType SpaceComponentType, bool bResultEnabled);
};

//
// Share Anchors
//
UCLASS()
class OCULUSANCHORS_API UOculusAsyncAction_ShareAnchors : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UOculusAsyncAction_ShareAnchors* OculusAsyncShareAnchors(const TArray<AActor*>& TargetActors, const TArray<FString>& ToShareWithIds);

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_ShareAnchors_Success Success;

	UPROPERTY(BlueprintAssignable)
	FOculus_LatentAction_ShareAnchors_Failure Failure;

	// Target Spaces
	UPROPERTY(Transient)
	TArray<UOculusAnchorComponent*> TargetAnchors;

	// Users to share with
	TArray<uint64> ToShareWithIds;

	FUInt64 ShareSpacesRequestId;

private:
	void HandleShareAnchorsComplete(EOculusResult::Type ShareResult, const TArray<UOculusAnchorComponent*>& TargetAnchors, const TArray<uint64>& OculusUserIds);
};

UCLASS()
class OCULUSANCHORS_API UOculusAnchorLaunchCaptureFlow : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOculusAnchorCaptureFlowFinished);

	UFUNCTION(BlueprintCallable, Category = "OculusVR|SpatialAnchor", meta = (WorldContext = "WorldContext", BlueprintInternalUseOnly = "true"))
	static UOculusAnchorLaunchCaptureFlow* LaunchCaptureFlowAsync(const UObject* WorldContext);

	void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FOculusAnchorCaptureFlowFinished Success;

	UPROPERTY(BlueprintAssignable)
	FOculusAnchorCaptureFlowFinished Failure;

private:
	uint64 Request = 0;

	UFUNCTION(CallInEditor)
	void OnCaptureFinish(FUInt64 RequestId, bool bSuccess);
};