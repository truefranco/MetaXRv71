/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "CoreMinimal.h"
#include "OculusAnchorComponent.h"
#include "OculusAnchorTypes.h"
#include "OculusFunctionLibrary.h"

DECLARE_DELEGATE_TwoParams(FOculusSpatialAnchorCreateDelegate, EOculusResult::Type /*Result*/, UOculusAnchorComponent* /*Anchor*/);
DECLARE_DELEGATE_TwoParams(FOculusAnchorEraseDelegate, EOculusResult::Type /*Result*/, FUUID /*AnchorUUID*/);
DECLARE_DELEGATE_FourParams(FOculusAnchorSetComponentStatusDelegate, EOculusResult::Type /*Result*/, uint64 /*Anchor*/, EOculusSpaceComponentType /*ComponentType*/, bool /*Enabled*/);
DECLARE_DELEGATE_TwoParams(FOculusAnchorSaveDelegate, EOculusResult::Type /*Result*/, UOculusAnchorComponent* /*Anchor*/);
DECLARE_DELEGATE_TwoParams(FOculusAnchorSaveListDelegate, EOculusResult::Type /*Result*/, const TArray<UOculusAnchorComponent*>& /*SavedAnchors*/);
DECLARE_DELEGATE_TwoParams(FOculusAnchorQueryDelegate, EOculusResult::Type /*Result*/, const TArray<FOculusSpaceQueryResult>& /*Results*/);
DECLARE_DELEGATE_ThreeParams(FOculusAnchorShareDelegate, EOculusResult::Type /*Result*/, const TArray<UOculusAnchorComponent*>& /*Anchors*/, const TArray<uint64>& /*Users*/);

namespace OculusAnchors
{

struct OCULUSANCHORS_API FOculusAnchors
{
	void Initialize();
	void Teardown();

	static FOculusAnchors* GetInstance();

	static bool CreateSpatialAnchor(const FTransform& InTransform, AActor* TargetActor, const FOculusSpatialAnchorCreateDelegate& ResultCallback, EOculusResult::Type& OutResult);
	static bool EraseAnchor(UOculusAnchorComponent* Anchor, const FOculusAnchorEraseDelegate& ResultCallback, EOculusResult::Type& OutResult);
	static bool DestroyAnchor(uint64 AnchorHandle, EOculusResult::Type& OutResult);

	static bool SetAnchorComponentStatus(UOculusAnchorComponent* Anchor, EOculusSpaceComponentType SpaceComponentType, bool Enable, float Timeout, const FOculusAnchorSetComponentStatusDelegate& ResultCallback, EOculusResult::Type& OutResult);
	static bool GetAnchorComponentStatus(UOculusAnchorComponent* Anchor, EOculusSpaceComponentType SpaceComponentType, bool& OutEnabled, bool& OutChangePending, EOculusResult::Type& OutResult);
	static bool GetAnchorSupportedComponents(UOculusAnchorComponent* Anchor, TArray<EOculusSpaceComponentType>& OutSupportedComponents, EOculusResult::Type& OutResult);

	static bool SetComponentStatus(uint64 Space, EOculusSpaceComponentType SpaceComponentType, bool Enable, float Timeout, const FOculusAnchorSetComponentStatusDelegate& ResultCallback, EOculusResult::Type& OutResult);
	static bool GetComponentStatus(uint64 AnchorHandle, EOculusSpaceComponentType SpaceComponentType, bool& OutEnabled, bool& OutChangePending, EOculusResult::Type& OutResult);
	static bool GetSupportedComponents(uint64 AnchorHandle, TArray<EOculusSpaceComponentType>& OutSupportedComponents, EOculusResult::Type& OutResult);
	
	static bool SaveAnchor(UOculusAnchorComponent* Anchor, EOculusSpaceStorageLocation StorageLocation, const FOculusAnchorSaveDelegate& ResultCallback, EOculusResult::Type& OutResult);
	static bool SaveAnchorList(const TArray<UOculusAnchorComponent*>& Anchors, EOculusSpaceStorageLocation StorageLocation, const FOculusAnchorSaveListDelegate& ResultCallback, EOculusResult::Type& OutResult);

	static bool QueryAnchors(const TArray<FUUID>& AnchorUUIDs, EOculusSpaceStorageLocation Location, const FOculusAnchorQueryDelegate& ResultCallback, EOculusResult::Type& OutResult);
	static bool QueryAnchorsAdvanced(const FOculusSpaceQueryInfo& QueryInfo, const FOculusAnchorQueryDelegate& ResultCallback, EOculusResult::Type& OutResult);

	static bool ShareAnchors(const TArray<UOculusAnchorComponent*>& Anchors, const TArray<uint64>& OculusUserIDs, const FOculusAnchorShareDelegate& ResultCallback, EOculusResult::Type& OutResult);

	static bool GetSpaceContainerUUIDs(uint64 Space, TArray<FUUID>& OutUUIDs, EOculusResult::Type& OutResult);
	static bool GetSpaceScenePlane(uint64 Space, FVector& OutPos, FVector& OutSize, EOculusResult::Type& OutResult);
	static bool GetSpaceSceneVolume(uint64 Space, FVector& OutPos, FVector& OutSize, EOculusResult::Type& OutResult);
	static bool GetSpaceSemanticClassification(uint64 Space, TArray<FString>& OutSemanticClassifications, EOculusResult::Type& OutResult);
	static bool GetSpaceBoundary2D(uint64 Space, TArray<FVector2D>& OutVertices, EOculusResult::Type& OutResult);

private:
	void HandleSpatialAnchorCreateComplete(FUInt64 RequestId, int Result, FUInt64 Space, FUUID UUID);
	void HandleAnchorEraseComplete(FUInt64 RequestId, int Result, FUUID UUID, EOculusSpaceStorageLocation Location);

	void HandleSetComponentStatusComplete(FUInt64 RequestId, int Result, FUInt64 Space, FUUID UUID, EOculusSpaceComponentType ComponentType, bool Enabled);

	void HandleAnchorSaveComplete(FUInt64 RequestId, FUInt64 Space, bool Success, int Result, FUUID UUID);
	void HandleAnchorSaveListComplete(FUInt64 RequestId, int Result);

	void HandleAnchorQueryResultsBegin(FUInt64 RequestId);
	void HandleAnchorQueryResultElement(FUInt64 RequestId, FUInt64 Space, FUUID UUID);
	void HandleAnchorQueryComplete(FUInt64 RequestId, int Result);

	void HandleAnchorSharingComplete(FUInt64 RequestId, int Result);

	struct EraseAnchorBinding
	{
		FUInt64 RequestId;
		FOculusAnchorEraseDelegate Binding;
		TWeakObjectPtr<UOculusAnchorComponent> Anchor;
	};

	struct SetComponentStatusBinding
	{
		FUInt64 RequestId;
		FOculusAnchorSetComponentStatusDelegate Binding;
		uint64 AnchorHandle;
	};

	struct CreateAnchorBinding
	{
		FUInt64 RequestId;
		FOculusSpatialAnchorCreateDelegate Binding;
		TWeakObjectPtr<AActor> Actor;
	};

	struct SaveAnchorBinding
	{
		FUInt64 RequestId;
		FOculusAnchorSaveDelegate Binding;
		EOculusSpaceStorageLocation Location;
		TWeakObjectPtr<UOculusAnchorComponent> Anchor;
	};

	struct SaveAnchorListBinding
	{
		FUInt64 RequestId;
		FOculusAnchorSaveListDelegate Binding;
		EOculusSpaceStorageLocation Location;
		TArray<TWeakObjectPtr<UOculusAnchorComponent>> SavedAnchors;
	};

	struct AnchorQueryBinding
	{
		FUInt64 RequestId;
		FOculusAnchorQueryDelegate Binding;
		EOculusSpaceStorageLocation Location;
		TArray<FOculusSpaceQueryResult> Results;
	};

	struct ShareAnchorsBinding
	{
		FUInt64 RequestId;
		FOculusAnchorShareDelegate Binding;
		TArray<TWeakObjectPtr<UOculusAnchorComponent>> SharedAnchors;
		TArray<uint64> OculusUserIds;
	};

	// Delegate bindings
	TMap<uint64, CreateAnchorBinding>		CreateSpatialAnchorBindings;
	TMap<uint64, EraseAnchorBinding>		EraseAnchorBindings;
	TMap<uint64, SetComponentStatusBinding>	SetComponentStatusBindings;
	TMap<uint64, SaveAnchorBinding>			AnchorSaveBindings;
	TMap<uint64, SaveAnchorListBinding>		AnchorSaveListBindings;
	TMap<uint64, AnchorQueryBinding>		AnchorQueryBindings;
	TMap<uint64, ShareAnchorsBinding>		ShareAnchorsBindings;

	// Delegate handles
	FDelegateHandle DelegateHandleAnchorCreate;
	FDelegateHandle DelegateHandleAnchorErase;
	FDelegateHandle DelegateHandleSetComponentStatus;
	FDelegateHandle DelegateHandleAnchorSave;
	FDelegateHandle DelegateHandleAnchorSaveList;
	FDelegateHandle DelegateHandleQueryResultsBegin;
	FDelegateHandle DelegateHandleQueryResultElement;
	FDelegateHandle DelegateHandleQueryComplete;
	FDelegateHandle DelegateHandleAnchorShare;
};

}