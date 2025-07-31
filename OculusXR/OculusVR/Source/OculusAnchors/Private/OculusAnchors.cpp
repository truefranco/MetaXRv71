/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchors.h"
#include "OculusAnchorsModule.h"
#include "OculusAnchorDelegates.h"
#include "OculusHMDModule.h"
#include "OculusAnchorManager.h"
#include "OculusSpatialAnchorComponent.h"
#include "OculusAnchorBPFunctionLibrary.h"

namespace OculusAnchors
{

void FOculusAnchors::Initialize()
{
	DelegateHandleAnchorCreate = FOculusAnchorEventDelegates::OculusSpatialAnchorCreateComplete.AddRaw(this, &FOculusAnchors::HandleSpatialAnchorCreateComplete);
	DelegateHandleAnchorErase = FOculusAnchorEventDelegates::OculusSpaceEraseComplete.AddRaw(this, &FOculusAnchors::HandleAnchorEraseComplete);
	DelegateHandleSetComponentStatus = FOculusAnchorEventDelegates::OculusSpaceSetComponentStatusComplete.AddRaw(this, &FOculusAnchors::HandleSetComponentStatusComplete);
	DelegateHandleAnchorSave = FOculusAnchorEventDelegates::OculusSpaceSaveComplete.AddRaw(this, &FOculusAnchors::HandleAnchorSaveComplete);
	DelegateHandleAnchorSaveList = FOculusAnchorEventDelegates::OculusSpaceListSaveComplete.AddRaw(this, &FOculusAnchors::HandleAnchorSaveListComplete);
	DelegateHandleQueryResultsBegin = FOculusAnchorEventDelegates::OculusSpaceQueryResults.AddRaw(this, &FOculusAnchors::HandleAnchorQueryResultsBegin);
	DelegateHandleQueryResultElement = FOculusAnchorEventDelegates::OculusSpaceQueryResult.AddRaw(this, &FOculusAnchors::HandleAnchorQueryResultElement);
	DelegateHandleQueryComplete = FOculusAnchorEventDelegates::OculusSpaceQueryComplete.AddRaw(this, &FOculusAnchors::HandleAnchorQueryComplete);
	DelegateHandleAnchorShare = FOculusAnchorEventDelegates::OculusSpaceShareComplete.AddRaw(this, &FOculusAnchors::HandleAnchorSharingComplete);
}

void FOculusAnchors::Teardown()
{
	FOculusAnchorEventDelegates::OculusSpatialAnchorCreateComplete.Remove(DelegateHandleAnchorCreate);
	FOculusAnchorEventDelegates::OculusSpaceEraseComplete.Remove(DelegateHandleAnchorErase);
	FOculusAnchorEventDelegates::OculusSpaceSetComponentStatusComplete.Remove(DelegateHandleSetComponentStatus);
	FOculusAnchorEventDelegates::OculusSpaceSaveComplete.Remove(DelegateHandleAnchorSave);
	FOculusAnchorEventDelegates::OculusSpaceListSaveComplete.Remove(DelegateHandleAnchorSaveList);
	FOculusAnchorEventDelegates::OculusSpaceQueryResults.Remove(DelegateHandleQueryResultsBegin);
	FOculusAnchorEventDelegates::OculusSpaceQueryResult.Remove(DelegateHandleQueryResultElement);
	FOculusAnchorEventDelegates::OculusSpaceQueryComplete.Remove(DelegateHandleQueryComplete);
	FOculusAnchorEventDelegates::OculusSpaceShareComplete.Remove(DelegateHandleAnchorShare);
}

FOculusAnchors* FOculusAnchors::GetInstance()
{
	return FOculusAnchorsModule::GetOculusAnchors();
}

bool FOculusAnchors::CreateSpatialAnchor(const FTransform& InTransform, AActor* TargetActor, const FOculusSpatialAnchorCreateDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid actor provided when attempting to create a spatial anchor."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
		return false;
	}

	UWorld* World = TargetActor->GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to retrieve World Context while creating spatial anchor."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
		return false;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to retrieve Player Controller while creating spatial anchor"));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
		return false;
	}

	APlayerCameraManager* PlayerCameraManager = PlayerController->PlayerCameraManager;
	FTransform MainCameraTransform = FTransform::Identity;
	if (IsValid(PlayerCameraManager))
	{
		MainCameraTransform.SetLocation(PlayerCameraManager->GetCameraLocation());
		MainCameraTransform.SetRotation(FQuat(PlayerCameraManager->GetCameraRotation()));
	}

	UOculusAnchorComponent* Anchor = Cast<UOculusAnchorComponent>(TargetActor->GetComponentByClass(UOculusAnchorComponent::StaticClass()));
	if (IsValid(Anchor) && Anchor->HasValidHandle())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Actor targeted to create anchor already has an anchor component with a valid handle."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
		return false;
	}

	uint64 RequestId = 0;
	EOculusResult::Type Result = FOculusAnchorManager::CreateAnchor(InTransform, RequestId, MainCameraTransform);
	bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(Result);

	if (bAsyncStartSuccess)
	{
		CreateAnchorBinding AnchorData;
		AnchorData.RequestId = RequestId;
		AnchorData.Actor = TargetActor;
		AnchorData.Binding = ResultCallback;

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->CreateSpatialAnchorBindings.Add(RequestId, AnchorData);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async create spatial anchor."));
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
	}

	return bAsyncStartSuccess;
}

bool FOculusAnchors::EraseAnchor(UOculusAnchorComponent* Anchor, const FOculusAnchorEraseDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	if (!IsValid(Anchor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid anchor provided when attempting to erase an anchor."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, FUUID());
		return false;
	}

	if (!Anchor->HasValidHandle())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Cannot erase anchor with invalid handle."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, FUUID());
		return false;
	}

	if (!Anchor->IsStoredAtLocation(EOculusSpaceStorageLocation::Local))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Only local anchors can be erased."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, FUUID());
		return false;
	}

	uint64 RequestId = 0;

	// Erase only supports local anchors
	EOculusResult::Type Result = FOculusAnchorManager::EraseAnchor(Anchor->GetHandle(), EOculusSpaceStorageLocation::Local, RequestId);
	bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(Result);

	if (bAsyncStartSuccess)
	{
		EraseAnchorBinding EraseData;
		EraseData.RequestId = RequestId;
		EraseData.Binding = ResultCallback;
		EraseData.Anchor = Anchor;

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->EraseAnchorBindings.Add(RequestId, EraseData);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async erase spatial anchor."));
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, FUUID());
	}

	return bAsyncStartSuccess;
}

bool FOculusAnchors::DestroyAnchor(uint64 AnchorHandle, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::DestroySpace(AnchorHandle);

	return UOculusFunctionLibrary::IsResultSuccess(OutResult);
}

bool FOculusAnchors::SetAnchorComponentStatus(UOculusAnchorComponent* Anchor, EOculusSpaceComponentType SpaceComponentType, bool Enable, float Timeout, const FOculusAnchorSetComponentStatusDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	if (!IsValid(Anchor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid anchor provided when attempting to set anchor component status."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, FUInt64(), EOculusSpaceComponentType::Undefined, false);
		return false;
	}

	if (!Anchor->HasValidHandle())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Anchor provided to set anchor component status has invalid handle."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, FUInt64(), EOculusSpaceComponentType::Undefined, false);
		return false;
	}

	uint64 RequestId = 0;
	OutResult = FOculusAnchorManager::SetSpaceComponentStatus(Anchor->GetHandle(), SpaceComponentType, Enable, Timeout, RequestId);
	bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(OutResult);

	if (bAsyncStartSuccess)
	{
		SetComponentStatusBinding SetComponentStatusData;
		SetComponentStatusData.RequestId = RequestId;
		SetComponentStatusData.Binding = ResultCallback;
		SetComponentStatusData.AnchorHandle = Anchor->GetHandle();

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->SetComponentStatusBindings.Add(RequestId, SetComponentStatusData);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async call to set anchor component status."));
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, FUInt64(), EOculusSpaceComponentType::Undefined, false);
	}

	return true;
}

bool FOculusAnchors::GetAnchorComponentStatus(UOculusAnchorComponent* Anchor, EOculusSpaceComponentType SpaceComponentType, bool& OutEnabled, bool& OutChangePending, EOculusResult::Type& OutResult)
{
	if (!IsValid(Anchor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid anchor provided when attempting to get space component status."));
		OutResult = EOculusResult::Failure;
		return false;
	}

	if (!Anchor->HasValidHandle())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Anchor provided to get space component status has invalid handle."));
		OutResult = EOculusResult::Failure;
		return false;
	}

	return GetComponentStatus(Anchor->GetHandle(), SpaceComponentType, OutEnabled, OutChangePending, OutResult);
}

bool FOculusAnchors::GetAnchorSupportedComponents(UOculusAnchorComponent* Anchor, TArray<EOculusSpaceComponentType>& OutSupportedComponents, EOculusResult::Type& OutResult)
{
	if (!IsValid(Anchor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid anchor provided when attempting to get space component status."));
		OutResult = EOculusResult::Failure;
		return false;
	}

	if (!Anchor->HasValidHandle())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Anchor provided to get space component status has invalid handle."));
		OutResult = EOculusResult::Failure;
		return false;
	}

	return GetSupportedComponents(Anchor->GetHandle(), OutSupportedComponents, OutResult);
}

bool FOculusAnchors::GetComponentStatus(uint64 AnchorHandle, EOculusSpaceComponentType SpaceComponentType, bool& OutEnabled, bool& OutChangePending, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::GetSpaceComponentStatus(AnchorHandle, SpaceComponentType, OutEnabled, OutChangePending);
	return UOculusFunctionLibrary::IsResultSuccess(OutResult);
}

bool FOculusAnchors::GetSupportedComponents(uint64 AnchorHandle, TArray<EOculusSpaceComponentType>& OutSupportedComponents, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::GetSupportedAnchorComponents(AnchorHandle, OutSupportedComponents);
	return UOculusFunctionLibrary::IsResultSuccess(OutResult);
}

bool FOculusAnchors::SetComponentStatus(uint64 Space, EOculusSpaceComponentType SpaceComponentType, bool Enable, float Timeout, const FOculusAnchorSetComponentStatusDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	uint64 RequestId = 0;
	OutResult = FOculusAnchorManager::SetSpaceComponentStatus(Space, SpaceComponentType, Enable, Timeout, RequestId);
	bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(OutResult);

	if (bAsyncStartSuccess)
	{
		SetComponentStatusBinding SetComponentStatusData;
		SetComponentStatusData.RequestId = RequestId;
		SetComponentStatusData.Binding = ResultCallback;
		SetComponentStatusData.AnchorHandle = Space;

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->SetComponentStatusBindings.Add(RequestId, SetComponentStatusData);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async call to set anchor component status."));
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, Space, SpaceComponentType, Enable);
	}

	return true;
}
bool FOculusAnchors::SaveAnchor(UOculusAnchorComponent* Anchor, EOculusSpaceStorageLocation StorageLocation, const FOculusAnchorSaveDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	if (!IsValid(Anchor))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid anchor provided when attempting to save anchor."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
		return false;
	}

	if (!Anchor->HasValidHandle())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Anchor provided to save anchor has invalid handle."));
		OutResult = EOculusResult::Failure;
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
		return false;
	}

	uint64 RequestId = 0;
	EOculusResult::Type Result = FOculusAnchorManager::SaveAnchor(Anchor->GetHandle(), StorageLocation, EOculusSpaceStoragePersistenceMode::Indefinite, RequestId);
	bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(Result);

	if (bAsyncStartSuccess)
	{
		SaveAnchorBinding SaveAnchorData;
		SaveAnchorData.RequestId = RequestId;
		SaveAnchorData.Binding = ResultCallback;
		SaveAnchorData.Location = StorageLocation;
		SaveAnchorData.Anchor = Anchor;

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->AnchorSaveBindings.Add(RequestId, SaveAnchorData);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async call to save anchor."));
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, nullptr);
	}

	return bAsyncStartSuccess;
}

void AnchorComponentsToReferences(const TArray<UOculusAnchorComponent*>& Anchors, TArray<uint64>& Handles, TArray<TWeakObjectPtr<UOculusAnchorComponent>>& AnchorPtrs)
{
	Handles.Empty();
	AnchorPtrs.Empty();

	for (auto& AnchorInstance : Anchors)
	{
		if (!IsValid(AnchorInstance))
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("Invalid anchor provided when attempting to process anchor list."));
			continue;
		}

		if (!AnchorInstance->HasValidHandle())
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("Anchor provided to anchor list has invalid handle."));
			continue;
		}

		Handles.Add(AnchorInstance->GetHandle().GetValue());
		AnchorPtrs.Add(AnchorInstance);
	}
}

bool FOculusAnchors::SaveAnchorList(const TArray<UOculusAnchorComponent*>& Anchors, EOculusSpaceStorageLocation StorageLocation, const FOculusAnchorSaveListDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	TArray<uint64> Handles;
	TArray<TWeakObjectPtr<UOculusAnchorComponent>> SavedAnchors;

	AnchorComponentsToReferences(Anchors, Handles, SavedAnchors);

	uint64 RequestId = 0;
	OutResult = FOculusAnchorManager::SaveAnchorList(Handles, StorageLocation, RequestId);
	bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(OutResult);

	if (bAsyncStartSuccess)
	{
		SaveAnchorListBinding SaveAnchorListData;
		SaveAnchorListData.RequestId = RequestId;
		SaveAnchorListData.Binding = ResultCallback;
		SaveAnchorListData.Location = StorageLocation;
		SaveAnchorListData.SavedAnchors = SavedAnchors;

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->AnchorSaveListBindings.Add(RequestId, SaveAnchorListData);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async call to save anchor list."));
		ResultCallback.ExecuteIfBound(OutResult, TArray<UOculusAnchorComponent*>());
	}

	return bAsyncStartSuccess;
}

bool FOculusAnchors::QueryAnchors(const TArray<FUUID>& AnchorUUIDs, EOculusSpaceStorageLocation Location, const FOculusAnchorQueryDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	FOculusSpaceQueryInfo QueryInfo;
	QueryInfo.FilterType = EOculusSpaceQueryFilterType::FilterByIds;
	QueryInfo.IDFilter = AnchorUUIDs;
	QueryInfo.Location = Location;
	QueryInfo.MaxQuerySpaces = AnchorUUIDs.Num();

	return QueryAnchorsAdvanced(QueryInfo, ResultCallback, OutResult);
}

bool FOculusAnchors::QueryAnchorsAdvanced(const FOculusSpaceQueryInfo& QueryInfo, const FOculusAnchorQueryDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	uint64 RequestId = 0;
	OutResult = FOculusAnchorManager::QuerySpaces(QueryInfo, RequestId);
	//bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(OutResult);
	bool bAsyncStartSuccess = UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OutResult);
	if (bAsyncStartSuccess)
	{
		AnchorQueryBinding QueryResults;
		QueryResults.RequestId = RequestId;
		QueryResults.Binding = ResultCallback;
		QueryResults.Location = QueryInfo.Location;

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->AnchorQueryBindings.Add(RequestId, QueryResults);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async call to query anchors."));
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, TArray<FOculusSpaceQueryResult>());
	}

	return bAsyncStartSuccess;
}

bool FOculusAnchors::ShareAnchors(const TArray<UOculusAnchorComponent*>& Anchors, const TArray<uint64>& OculusUserIDs, const FOculusAnchorShareDelegate& ResultCallback, EOculusResult::Type& OutResult)
{
	TArray<uint64> Handles;
	TArray<TWeakObjectPtr<UOculusAnchorComponent>> SharedAnchors;

	AnchorComponentsToReferences(Anchors, Handles, SharedAnchors);

	uint64 RequestId = 0;
	OutResult = FOculusAnchorManager::ShareSpaces(Handles, OculusUserIDs, RequestId);
	bool bAsyncStartSuccess = UOculusFunctionLibrary::IsResultSuccess(OutResult);

	if (bAsyncStartSuccess)
	{
		ShareAnchorsBinding ShareAnchorsData;
		ShareAnchorsData.RequestId = RequestId;
		ShareAnchorsData.Binding = ResultCallback;
		ShareAnchorsData.SharedAnchors = SharedAnchors;
		ShareAnchorsData.OculusUserIds = OculusUserIDs;

		FOculusAnchors* SDKInstance = GetInstance();
		SDKInstance->ShareAnchorsBindings.Add(RequestId, ShareAnchorsData);
	}
	else
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to start async call to share anchor."));
		ResultCallback.ExecuteIfBound(EOculusResult::Failure, TArray<UOculusAnchorComponent*>(), TArray<uint64>());
	}

	return bAsyncStartSuccess;
}

bool FOculusAnchors::GetSpaceContainerUUIDs(uint64 Space, TArray<FUUID>& OutUUIDs, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::GetSpaceContainerUUIDs(Space, OutUUIDs);
	return UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OutResult);
}

bool FOculusAnchors::GetSpaceScenePlane(uint64 Space, FVector& OutPos, FVector& OutSize, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::GetSpaceScenePlane(Space, OutPos, OutSize);
	return UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OutResult);
}

bool FOculusAnchors::GetSpaceSceneVolume(uint64 Space, FVector& OutPos, FVector& OutSize, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::GetSpaceSceneVolume(Space, OutPos, OutSize);
	return UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OutResult);
}

bool FOculusAnchors::GetSpaceSemanticClassification(uint64 Space, TArray<FString>& OutSemanticClassifications, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::GetSpaceSemanticClassification(Space, OutSemanticClassifications);
	return UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OutResult);
}

bool FOculusAnchors::GetSpaceBoundary2D(uint64 Space, TArray<FVector2D>& OutVertices, EOculusResult::Type& OutResult)
{
	OutResult = FOculusAnchorManager::GetSpaceBoundary2D(Space, OutVertices);
	return UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(OutResult);
}

void FOculusAnchors::HandleSpatialAnchorCreateComplete(FUInt64 RequestId, int Result, FUInt64 Space, FUUID UUID)
{
	CreateAnchorBinding* AnchorDataPtr = CreateSpatialAnchorBindings.Find(RequestId.GetValue());
	if (AnchorDataPtr == nullptr)
	{
		UE_LOG(LogOculusAnchors, Error, TEXT("Couldn't find anchor data binding for create spatial anchor! Request: %llu"), RequestId.GetValue());
		return;
	}

	if (!OVRP_SUCCESS(Result))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to create Spatial Anchor. Request: %llu  --  Result: %d"), RequestId.GetValue(), Result);
		AnchorDataPtr->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), nullptr);
		CreateSpatialAnchorBindings.Remove(RequestId.GetValue());
		return;
	}

	if (!AnchorDataPtr->Actor.IsValid())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Actor has been invalidated while creating actor. Request: %llu"), RequestId.GetValue());

		// Clean up the orphaned space
		EOculusResult::Type AnchorResult;
		FOculusAnchors::DestroyAnchor(Space, AnchorResult);

		AnchorDataPtr->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), nullptr);
		CreateSpatialAnchorBindings.Remove(RequestId.GetValue());
		return;
	}

	AActor* TargetActor = AnchorDataPtr->Actor.Get();

	UOculusSpatialAnchorComponent* SpatialAnchorComponent = TargetActor->FindComponentByClass<UOculusSpatialAnchorComponent>();
	if (SpatialAnchorComponent == nullptr)
	{
		SpatialAnchorComponent = Cast<UOculusSpatialAnchorComponent>(TargetActor->AddComponentByClass(UOculusSpatialAnchorComponent::StaticClass(), false, FTransform::Identity, false));
	}

	SpatialAnchorComponent->SetHandle(Space);
	SpatialAnchorComponent->SetUUID(UUID);

	uint64 tempOut;
	FOculusAnchorManager::SetSpaceComponentStatus(Space, EOculusSpaceComponentType::Locatable, true, 0.0f, tempOut);
	FOculusAnchorManager::SetSpaceComponentStatus(Space, EOculusSpaceComponentType::Sharable, true, 0.0f, tempOut);
	FOculusAnchorManager::SetSpaceComponentStatus(Space, EOculusSpaceComponentType::Storable, true, 0.0f, tempOut);

	AnchorDataPtr->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SpatialAnchorComponent);
	CreateSpatialAnchorBindings.Remove(RequestId.GetValue());
}

void FOculusAnchors::HandleAnchorEraseComplete(FUInt64 RequestId, int Result, FUUID UUID, EOculusSpaceStorageLocation Location)
{
	EraseAnchorBinding* EraseDataPtr = EraseAnchorBindings.Find(RequestId.GetValue());
	if (EraseDataPtr == nullptr)
	{
		UE_LOG(LogOculusAnchors, Error, TEXT("Couldn't find binding for space erase! Request: %llu"), RequestId.GetValue());
		return;
	}

	if (!OVRP_SUCCESS(Result))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to erase Spatial Anchor. Request: %llu  --  Result: %d"), RequestId.GetValue(), Result);
		EraseDataPtr->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), UUID);
		EraseAnchorBindings.Remove(RequestId.GetValue());
		return;
	}

	if (EraseDataPtr->Anchor.IsValid())
	{
		// Since you can only erase local anchors, just unset local anchor storage
		EraseDataPtr->Anchor->SetStoredLocation(EOculusSpaceStorageLocation::Local, false);
	}

	EraseDataPtr->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), UUID);
	EraseAnchorBindings.Remove(RequestId.GetValue());
}

void FOculusAnchors::HandleSetComponentStatusComplete(FUInt64 RequestId, int Result, FUInt64 Space, FUUID UUID, EOculusSpaceComponentType ComponentType, bool Enabled)
{
	SetComponentStatusBinding* SetStatusBinding = SetComponentStatusBindings.Find(RequestId.GetValue());

	if (SetStatusBinding == nullptr)
	{
		UE_LOG(LogOculusAnchors, Verbose, TEXT("Couldn't find binding for set component status! Request: %llu"), RequestId.GetValue());
		return;
	}

	if (SetStatusBinding != nullptr)
	{
		SetStatusBinding->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SetStatusBinding->AnchorHandle, ComponentType, Enabled);
		SetComponentStatusBindings.Remove(RequestId.GetValue());
		return;
	}

	SetStatusBinding->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SetStatusBinding->AnchorHandle, ComponentType, Enabled);
	SetComponentStatusBindings.Remove(RequestId.GetValue());
}

void FOculusAnchors::HandleAnchorSaveComplete(FUInt64 RequestId, FUInt64 Space, bool Success, int Result, FUUID UUID)
{
	SaveAnchorBinding* SaveAnchorData = AnchorSaveBindings.Find(RequestId.GetValue());
	if (SaveAnchorData == nullptr)
	{
		UE_LOG(LogOculusAnchors, Error, TEXT("Couldn't find binding for save anchor! Request: %llu"), RequestId.GetValue());
		return;
	}

	if (!OVRP_SUCCESS(Result))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to save Spatial Anchor. Request: %llu  --  Result: %d  --  Space: %llu"), RequestId.GetValue(), Result, Space.GetValue());
		SaveAnchorData->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SaveAnchorData->Anchor.Get());
		AnchorSaveBindings.Remove(RequestId.GetValue());
		return;
	}

	if (SaveAnchorData->Anchor.IsValid())
	{
		SaveAnchorData->Anchor->SetStoredLocation(SaveAnchorData->Location, true);
	}

	SaveAnchorData->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SaveAnchorData->Anchor.Get());
	AnchorSaveBindings.Remove(RequestId.GetValue());
}

void FOculusAnchors::HandleAnchorSaveListComplete(FUInt64 RequestId, int Result)
{
	SaveAnchorListBinding* SaveListData = AnchorSaveListBindings.Find(RequestId.GetValue());
	if (SaveListData == nullptr)
	{
		UE_LOG(LogOculusAnchors, Error, TEXT("Couldn't find binding for save anchor list! Request: %llu"), RequestId.GetValue());
		return;
	}

	// Get all anchors
	TArray<UOculusAnchorComponent*> SavedAnchors;
	for (auto& WeakAnchor : SaveListData->SavedAnchors)
	{
		if (WeakAnchor.IsValid())
		{
			SavedAnchors.Add(WeakAnchor.Get());
		}
	}

	// Failed to save
	if (!OVRP_SUCCESS(Result))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to save Spatial Anchors. Request: %llu  --  Result: %d"), RequestId.GetValue(), Result);
		SaveListData->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SavedAnchors);
		AnchorSaveListBindings.Remove(RequestId.GetValue());
		return;
	}

	// Set new storage location
	for (auto& SavedAnchor : SavedAnchors)
	{
		SavedAnchor->SetStoredLocation(SaveListData->Location, true);
	}

	SaveListData->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SavedAnchors);
	AnchorSaveListBindings.Remove(RequestId.GetValue());
}


void FOculusAnchors::HandleAnchorQueryResultsBegin(FUInt64 RequestId)
{
	// no op
}

void FOculusAnchors::HandleAnchorQueryResultElement(FUInt64 RequestId, FUInt64 Space, FUUID UUID)
{
	AnchorQueryBinding* ResultPtr = AnchorQueryBindings.Find(RequestId.GetValue());
	if (ResultPtr)
	{
		uint64 tempOut;
		TArray<EOculusSpaceComponentType> supportedTypes;
		FOculusAnchorManager::GetSupportedAnchorComponents(Space, supportedTypes);

		if (supportedTypes.Contains(EOculusSpaceComponentType::Locatable))
		{
			FOculusAnchorManager::SetSpaceComponentStatus(Space, EOculusSpaceComponentType::Locatable, true, 0.0f, tempOut);
		}

		if (supportedTypes.Contains(EOculusSpaceComponentType::Sharable))
		{
			FOculusAnchorManager::SetSpaceComponentStatus(Space, EOculusSpaceComponentType::Sharable, true, 0.0f, tempOut);
		}

		if (supportedTypes.Contains(EOculusSpaceComponentType::Storable))
		{
			FOculusAnchorManager::SetSpaceComponentStatus(Space, EOculusSpaceComponentType::Storable, true, 0.0f, tempOut);
		}
		
        ResultPtr->Results.Add(FOculusSpaceQueryResult(Space, UUID, ResultPtr->Location));
	}
}

void FOculusAnchors::HandleAnchorQueryComplete(FUInt64 RequestId, int Result)
{
	AnchorQueryBinding* ResultPtr = AnchorQueryBindings.Find(RequestId.GetValue());
	if (ResultPtr)
	{
		ResultPtr->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), ResultPtr->Results);
		AnchorQueryBindings.Remove(RequestId.GetValue());
	}
}

void FOculusAnchors::HandleAnchorSharingComplete(FUInt64 RequestId, int Result)
{
	ShareAnchorsBinding* ShareAnchorsData = ShareAnchorsBindings.Find(RequestId);
	if (ShareAnchorsData == nullptr)
	{
		UE_LOG(LogOculusAnchors, Error, TEXT("Couldn't find binding for share anchors! Request: %llu"), RequestId.GetValue());
		return;
	}

	TArray<UOculusAnchorComponent*> SharedAnchors;
	for (auto& WeakAnchor : ShareAnchorsData->SharedAnchors)
	{
		SharedAnchors.Add(WeakAnchor.Get());
	}

	ShareAnchorsData->Binding.ExecuteIfBound(static_cast<EOculusResult::Type>(Result), SharedAnchors, ShareAnchorsData->OculusUserIds);
	ShareAnchorsBindings.Remove(RequestId.GetValue());
}

}