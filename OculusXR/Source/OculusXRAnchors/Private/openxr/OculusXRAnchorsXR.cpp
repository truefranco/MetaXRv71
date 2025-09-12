// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRAnchorsXR.h"
#include "OpenXRCore.h"
#include "OpenXRHMD.h"
#include "IOpenXRHMDModule.h"
#include "OpenXR/OculusXROpenXRUtilities.h"
#include "OculusXRAnchorsModule.h"
#include "OculusXRHMDPrivate.h"
#include "OculusXRAnchorDelegates.h"
#include "OculusXRAnchorsUtil.h"

#define LOCTEXT_NAMESPACE "OculusXRAnchors"

namespace XRAnchors
{
	PFN_xrCreateSpatialAnchorFB xrCreateSpatialAnchorFB = nullptr;
	PFN_xrSetSpaceComponentStatusFB xrSetSpaceComponentStatusFB = nullptr;
	PFN_xrGetSpaceComponentStatusFB xrGetSpaceComponentStatusFB = nullptr;
	PFN_xrEnumerateSpaceSupportedComponentsFB xrEnumerateSpaceSupportedComponentsFB = nullptr;
	PFN_xrGetSpaceUuidFB xrGetSpaceUuidFB = nullptr;

	PFN_xrGetSpaceContainerFB xrGetSpaceContainerFB = nullptr;

	PFN_xrQuerySpacesFB xrQuerySpacesFB = nullptr;
	PFN_xrRetrieveSpaceQueryResultsFB xrRetrieveSpaceQueryResultsFB = nullptr;

	PFN_xrShareSpacesFB xrShareSpacesFB = nullptr;

	PFN_xrShareSpacesMETA xrShareSpacesMETA = nullptr;

	PFN_xrSaveSpaceFB xrSaveSpaceFB = nullptr;
	PFN_xrEraseSpaceFB xrEraseSpaceFB = nullptr;

	PFN_xrSaveSpaceListFB xrSaveSpaceListFB = nullptr;

	PFN_xrCreateSpaceUserFB xrCreateSpaceUserFB = nullptr;
	PFN_xrDestroySpaceUserFB xrDestroySpaceUserFB = nullptr;
	PFN_xrGetSpaceUserIdFB xrGetSpaceUserIdFB = nullptr;

	PFN_xrSaveSpacesMETA xrSaveSpacesMETA = nullptr;
	PFN_xrEraseSpacesMETA xrEraseSpacesMETA = nullptr;

	PFN_xrDiscoverSpacesMETA xrDiscoverSpacesMETA = nullptr;
	PFN_xrRetrieveSpaceDiscoveryResultsMETA xrRetrieveSpaceDiscoveryResultsMETA = nullptr;

	FAnchorsXR::FAnchorsXR()
		: bExtAnchorsEnabled(false)
		, bExtContainerEnabled(false)
		, bExtQueryEnabled(false)
		, bExtSharingEnabled(false)
		, bExtStorageEnabled(false)
		, bExtStorageBatchEnabled(false)
		, bExtUserEnabled(false)
		, bExtDiscoveryEnabled(false)
		, bExtPersistenceEnabled(false)
		, bExtSharingMetaEnabled(false)
		, bExtGroupSharingEnabled(false)
		, OpenXRHMD(nullptr)
	{
	}

	FAnchorsXR::~FAnchorsXR()
	{
	}

	void FAnchorsXR::RegisterAsOpenXRExtension()
	{
		RegisterOpenXRExtensionModularFeature();
	}

	bool FAnchorsXR::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME);
		return true;
	}

	bool FAnchorsXR::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_FB_SPATIAL_ENTITY_CONTAINER_EXTENSION_NAME);
		OutExtensions.Add(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME);
		OutExtensions.Add(XR_FB_SPATIAL_ENTITY_SHARING_EXTENSION_NAME);
		OutExtensions.Add(XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME);
		OutExtensions.Add(XR_FB_SPATIAL_ENTITY_STORAGE_BATCH_EXTENSION_NAME);
		OutExtensions.Add(XR_FB_SPATIAL_ENTITY_USER_EXTENSION_NAME);
		OutExtensions.Add(XR_META_SPATIAL_ENTITY_DISCOVERY_EXTENSION_NAME);
		OutExtensions.Add(XR_META_SPATIAL_ENTITY_PERSISTENCE_EXTENSION_NAME);
		OutExtensions.Add(XR_META_SPATIAL_ENTITY_SHARING_EXTENSION_NAME);
		OutExtensions.Add(XR_META_SPATIAL_ENTITY_GROUP_SHARING_EXTENSION_NAME);
		return true;
	}

	const void* FAnchorsXR::OnCreateInstance(class IOpenXRHMDModule* InModule, const void* InNext)
	{
		if (InModule != nullptr)
		{
			bExtAnchorsEnabled = InModule->IsExtensionEnabled(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME);
			bExtContainerEnabled = InModule->IsExtensionEnabled(XR_FB_SPATIAL_ENTITY_CONTAINER_EXTENSION_NAME);
			bExtQueryEnabled = InModule->IsExtensionEnabled(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME);
			bExtSharingEnabled = InModule->IsExtensionEnabled(XR_FB_SPATIAL_ENTITY_SHARING_EXTENSION_NAME);
			bExtStorageEnabled = InModule->IsExtensionEnabled(XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME);
			bExtStorageBatchEnabled = InModule->IsExtensionEnabled(XR_FB_SPATIAL_ENTITY_STORAGE_BATCH_EXTENSION_NAME);
			bExtUserEnabled = InModule->IsExtensionEnabled(XR_FB_SPATIAL_ENTITY_USER_EXTENSION_NAME);
			bExtDiscoveryEnabled = InModule->IsExtensionEnabled(XR_META_SPATIAL_ENTITY_DISCOVERY_EXTENSION_NAME);
			bExtPersistenceEnabled = InModule->IsExtensionEnabled(XR_META_SPATIAL_ENTITY_PERSISTENCE_EXTENSION_NAME);
			bExtSharingMetaEnabled = InModule->IsExtensionEnabled(XR_META_SPATIAL_ENTITY_SHARING_EXTENSION_NAME);
			bExtSharingMetaEnabled = InModule->IsExtensionEnabled(XR_META_SPATIAL_ENTITY_GROUP_SHARING_EXTENSION_NAME);

			UE_LOG(LogOculusXRAnchors, Log, TEXT("[Anchors] Extensions available"));
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Spatial Entity: %hs"), bExtAnchorsEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Container:		%hs"), bExtContainerEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Query:			%hs"), bExtQueryEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Sharing:		%hs"), bExtSharingEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Storage:		%hs"), bExtStorageEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Storage Batch:  %hs"), bExtStorageBatchEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Users:			%hs"), bExtUserEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Discovery:		%hs"), bExtDiscoveryEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Persistence:	%hs"), bExtPersistenceEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Sharing Meta:	%hs"), bExtSharingMetaEnabled ? "ENABLED" : "DISABLED");
			UE_LOG(LogOculusXRAnchors, Log, TEXT("			Group Sharing:	%hs"), bExtGroupSharingEnabled ? "ENABLED" : "DISABLED");
		}

		return InNext;
	}

	const void* FAnchorsXR::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		InitOpenXRFunctions(InInstance);

		OpenXRHMD = (FOpenXRHMD*)GEngine->XRSystem.Get();

		return InNext;
	}

	void FAnchorsXR::OnDestroySession(XrSession InSession)
	{
		OpenXRHMD = nullptr;
	}

	void FAnchorsXR::OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader)
	{
		if (OpenXRHMD == nullptr)
		{
			UE_LOG(LogOculusXRAnchors, Log, TEXT("[FAnchorsXR::OnEvent] Receieved event but no HMD was present."));
			return;
		}

		if (InHeader->type == XR_TYPE_EVENT_DATA_SPATIAL_ANCHOR_CREATE_COMPLETE_FB)
		{
			const XrEventDataSpatialAnchorCreateCompleteFB* const event =
				reinterpret_cast<const XrEventDataSpatialAnchorCreateCompleteFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpatialAnchorCreateCompleteFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Space:     %llu"), event->space);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Uuid:      %s"), *FOculusXRUUID(event->uuid.data).ToString());

			FOculusXRAnchorEventDelegates::OculusSpatialAnchorCreateComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result),
				(uint64)event->space,
				event->uuid.data);
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_SET_STATUS_COMPLETE_FB)
		{
			const XrEventDataSpaceSetStatusCompleteFB* const event =
				reinterpret_cast<const XrEventDataSpaceSetStatusCompleteFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceSetStatusCompleteFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Space:     %llu"), event->space);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Uuid:      %s"), *FOculusXRUUID(event->uuid.data).ToString());
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Type:      %d"), event->componentType);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Enabled:   %d"), event->enabled);

			FOculusXRAnchorEventDelegates::OculusSpaceSetComponentStatusComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result),
				(uint64)event->space,
				event->uuid.data,
				OculusXRAnchors::ToComponentType(event->componentType),
				(bool)event->enabled);
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_SAVE_COMPLETE_FB)
		{
			const XrEventDataSpaceSaveCompleteFB* const event =
				reinterpret_cast<const XrEventDataSpaceSaveCompleteFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceSaveCompleteFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Space:     %llu"), event->space);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Uuid:      %s"), *FOculusXRUUID(event->uuid.data).ToString());
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Location:  %d"), event->location);

			FOculusXRAnchorEventDelegates::OculusSpaceSaveComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result),
				XR_SUCCEEDED(event->result),
				OculusXRAnchors::GetResultFromXrResult(event->result),
				event->uuid.data);
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_LIST_SAVE_COMPLETE_FB)
		{
			const XrEventDataSpaceListSaveCompleteFB* const event =
				reinterpret_cast<const XrEventDataSpaceListSaveCompleteFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceListSaveCompleteFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);

			FOculusXRAnchorEventDelegates::OculusSpaceListSaveComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result));
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACES_SAVE_RESULT_META)
		{
			const XrEventDataSpacesSaveResultMETA* const event =
				reinterpret_cast<const XrEventDataSpacesSaveResultMETA*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpacesSaveResultMETA"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);

			FOculusXRAnchorEventDelegates::OculusAnchorsSaveComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result));
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_ERASE_COMPLETE_FB)
		{
			const XrEventDataSpaceEraseCompleteFB* const event =
				reinterpret_cast<const XrEventDataSpaceEraseCompleteFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceEraseCompleteFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Space:     %llu"), event->space);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Uuid:      %s"), *FOculusXRUUID(event->uuid.data).ToString());
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Location:  %d"), event->location);

			FOculusXRAnchorEventDelegates::OculusSpaceEraseComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result),
				event->uuid.data,
				OculusXRAnchors::ToStorageLocation(event->location));
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACES_ERASE_RESULT_META)
		{
			const XrEventDataSpacesEraseResultMETA* const event =
				reinterpret_cast<const XrEventDataSpacesEraseResultMETA*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpacesEraseResultMETA"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);

			FOculusXRAnchorEventDelegates::OculusAnchorsEraseComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result));
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_SHARE_COMPLETE_FB)
		{
			const XrEventDataSpaceShareCompleteFB* const event =
				reinterpret_cast<const XrEventDataSpaceShareCompleteFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceShareCompleteFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);

			FOculusXRAnchorEventDelegates::OculusSpaceShareComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result));
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB)
		{
			const XrEventDataSpaceQueryCompleteFB* const event =
				reinterpret_cast<const XrEventDataSpaceQueryCompleteFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceQueryCompleteFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);

			FOculusXRAnchorEventDelegates::OculusSpaceQueryComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result));
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB)
		{
			const XrEventDataSpaceQueryResultsAvailableFB* const event =
				reinterpret_cast<const XrEventDataSpaceQueryResultsAvailableFB*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceQueryResultsAvailableFB"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);

			// Initial query for the number of elements
			XrSpaceQueryResultsFB queryResults{ XR_TYPE_SPACE_QUERY_RESULTS_FB, nullptr };
			queryResults.resultCapacityInput = 0;
			queryResults.resultCountOutput = 0;
			queryResults.results = nullptr;

			auto getCapacityResult = xrRetrieveSpaceQueryResultsFB(OpenXRHMD->GetSession(), event->requestId, &queryResults);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[XrEventDataSpaceQueryResultsAvailableFB] -- Capacity"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Capacity: %d"), queryResults.resultCountOutput);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), getCapacityResult);

			// Get the data payload
			TArray<XrSpaceQueryResultFB> resultArray;
			resultArray.SetNum(queryResults.resultCountOutput);
			queryResults.resultCapacityInput = queryResults.resultCountOutput;
			queryResults.results = resultArray.GetData();

			auto getDataResult = xrRetrieveSpaceQueryResultsFB(OpenXRHMD->GetSession(), event->requestId, &queryResults);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[XrEventDataSpaceQueryResultsAvailableFB] -- Data Retrieval"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), getDataResult);

			FOculusXRAnchorEventDelegates::OculusSpaceQueryResults.Broadcast(event->requestId);
			for (const auto& it : resultArray)
			{
				UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Anchor(%llu / %s)"),
					it.space, *FOculusXRUUID(it.uuid.data).ToString());

				FOculusXRAnchorEventDelegates::OculusSpaceQueryResult.Broadcast(
					event->requestId,
					(uint64)it.space,
					it.uuid.data);
			}
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_DISCOVERY_COMPLETE_META)
		{
			const XrEventDataSpaceDiscoveryCompleteMETA* const event =
				reinterpret_cast<const XrEventDataSpaceDiscoveryCompleteMETA*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceDiscoveryCompleteMETA"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);

			FOculusXRAnchorEventDelegates::OculusAnchorsDiscoverComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result));
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SPACE_DISCOVERY_RESULTS_AVAILABLE_META)
		{
			const XrEventDataSpaceDiscoveryResultsAvailableMETA* const event =
				reinterpret_cast<const XrEventDataSpaceDiscoveryResultsAvailableMETA*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataSpaceDiscoveryResultsAvailableMETA"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);

			// Initial query for the number of elements
			XrSpaceDiscoveryResultsMETA discoverResults{ XR_TYPE_SPACE_DISCOVERY_RESULTS_META, nullptr };
			discoverResults.resultCapacityInput = 0;
			discoverResults.resultCountOutput = 0;
			discoverResults.results = nullptr;

			auto getCapacityResult = xrRetrieveSpaceDiscoveryResultsMETA(OpenXRHMD->GetSession(), event->requestId, &discoverResults);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[XrEventDataSpaceDiscoveryResultsAvailableMETA] -- Capacity"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Capacity: %d"), discoverResults.resultCountOutput);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), getCapacityResult);

			if (XR_FAILED(getCapacityResult))
			{
				return;
			}

			// Get the data payload
			TArray<XrSpaceDiscoveryResultMETA> resultArray;
			resultArray.SetNum(discoverResults.resultCountOutput);
			discoverResults.resultCapacityInput = discoverResults.resultCountOutput;
			discoverResults.results = resultArray.GetData();

			auto getDataResult = xrRetrieveSpaceDiscoveryResultsMETA(OpenXRHMD->GetSession(), event->requestId, &discoverResults);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[XrEventDataSpaceDiscoveryResultsAvailableMETA] -- Data Retrieval"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), getDataResult);

			TArray<FOculusXRAnchorsDiscoverResult> outputArray;
			for (const auto& it : resultArray)
			{
				UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Anchor(%llu / %s)"),
					it.space, *FOculusXRUUID(it.uuid.data).ToString());
				outputArray.Add(FOculusXRAnchorsDiscoverResult((uint64)it.space, it.uuid.data));
			}

			if (XR_FAILED(getDataResult))
			{
				return;
			}

			FOculusXRAnchorEventDelegates::OculusAnchorsDiscoverResults.Broadcast(
				event->requestId,
				outputArray);
		}
		else if (InHeader->type == XR_TYPE_EVENT_DATA_SHARE_SPACES_COMPLETE_META)
		{
			const XrEventDataShareSpacesCompleteMETA* const event =
				reinterpret_cast<const XrEventDataShareSpacesCompleteMETA*>(InHeader);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[FAnchorsXR::OnEvent] XrEventDataShareSpacesCompleteMETA"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						RequestId: %llu"), event->requestId);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("						Result:    %d"), event->result);

			FOculusXRAnchorEventDelegates::OculusSpaceShareComplete.Broadcast(
				event->requestId,
				OculusXRAnchors::GetResultFromXrResult(event->result));
		}
	}

	XrResult FAnchorsXR::CreateSpatialAnchor(const FTransform& InTransform, uint64& OutRequestId, const FTransform& CameraTransform)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[CreateSpatialAnchor] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsAnchorExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[CreateSpatialAnchor] Spatial entity extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		const FTransform TrackingToWorld = OpenXRHMD->GetTrackingToWorldTransform();

		// convert to tracking space
		const FQuat TrackingSpaceOrientation = TrackingToWorld.Inverse().TransformRotation(InTransform.Rotator().Quaternion());
		const FVector TrackingSpacePosition = TrackingToWorld.Inverse().TransformPosition(InTransform.GetLocation());

		const OculusXRHMD::FPose TrackingSpacePose(TrackingSpaceOrientation, TrackingSpacePosition);
		OculusXRHMD::FPose ConversionPose = TrackingSpacePose;

#if WITH_EDITOR
		// Link only head space position update
		FVector OutHeadPosition;
		FQuat OutHeadOrientation;
		const bool bGetPose = OpenXRHMD->GetCurrentPose(OpenXRHMD->HMDDeviceId, OutHeadOrientation, OutHeadPosition);
		if (!bGetPose)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("FOculusAnchorManager::CreateAnchor failed to get current headset pose."));
			return XR_ERROR_RUNTIME_FAILURE;
		}

		OculusXRHMD::FPose HeadPose(OutHeadOrientation, OutHeadPosition);

		OculusXRHMD::FPose MainCameraPose(CameraTransform.GetRotation(), CameraTransform.GetLocation());
		OculusXRHMD::FPose PoseInHeadSpace = MainCameraPose.Inverse() * TrackingSpacePose;

		// To world space pose
		OculusXRHMD::FPose WorldPose = HeadPose * PoseInHeadSpace;

		ConversionPose = WorldPose;
#endif

		XrPosef xrPose;
		xrPose.orientation = ToXrQuat(OpenXRHMD->GetBaseOrientation() * ConversionPose.Orientation);
		xrPose.position = ToXrVector(OpenXRHMD->GetBaseOrientation().RotateVector(ConversionPose.Position) / OpenXRHMD->GetWorldToMetersScale() + OpenXRHMD->GetBasePosition());

		XrSpatialAnchorCreateInfoFB createInfo;
		createInfo.type = XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_FB;
		createInfo.next = nullptr;
		createInfo.time = OpenXRHMD->GetDisplayTime();
		createInfo.space = OpenXRHMD->GetTrackingSpace();
		createInfo.poseInSpace = xrPose;

		auto result = xrCreateSpatialAnchorFB(OpenXRHMD->GetSession(), &createInfo, (uint64_t*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[CreateSpatialAnchor] Spatial entity creation failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::DestroySpatialAnchor(uint64 AnchorHandle)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DestroySpatialAnchor] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsAnchorExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DestroySpatialAnchor] Spatial entity extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		return xrDestroySpace((XrSpace)AnchorHandle);
	}

	XrResult FAnchorsXR::TryGetAnchorTransform(uint64 AnchorHandle, FTransform& OutTransform, FOculusXRAnchorLocationFlags& OutLocationFlags, EOculusXRAnchorSpace Space)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[TryGetAnchorTransform] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsAnchorExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[TryGetAnchorTransform] Spatial entity extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		OutTransform = FTransform::Identity;
		OutLocationFlags = FOculusXRAnchorLocationFlags(0);

		XrSpaceLocation location = { XR_TYPE_SPACE_LOCATION, nullptr };
		auto result = xrLocateSpace((XrSpace)AnchorHandle, OpenXRHMD->GetTrackingSpace(), OpenXRHMD->GetDisplayTime(), &location);
		if (XR_SUCCEEDED(result))
		{
			OutLocationFlags = FOculusXRAnchorLocationFlags(location.locationFlags);
			if (OutLocationFlags.IsValid())
			{
				float worldToMeters = OpenXRHMD->GetWorldToMetersScale();

				FVector basePosition = OpenXRHMD->GetBasePosition();
				FVector inPosition(-location.pose.position.z, location.pose.position.x, location.pose.position.y);
				FQuat baseOrientation = OpenXRHMD->GetBaseOrientation();
				FQuat inOrientation(-location.pose.orientation.z, location.pose.orientation.x, location.pose.orientation.y, -location.pose.orientation.w);

				FVector outPosition = (inPosition - basePosition) * worldToMeters;
				outPosition = baseOrientation.Inverse().RotateVector(outPosition);

				FQuat outOrientation = baseOrientation.Inverse() * inOrientation;
				outOrientation.Normalize();

				switch (Space)
				{
					case EOculusXRAnchorSpace::World:
					{
						const FTransform trackingToWorld = OpenXRHMD->GetTrackingToWorldTransform();
						OutTransform.SetLocation(trackingToWorld.TransformPosition(outPosition));
						OutTransform.SetRotation(FRotator(trackingToWorld.TransformRotation(outOrientation)).Quaternion());
					}
					break;

					case EOculusXRAnchorSpace::Tracking:
					{
						OutTransform.SetLocation(outPosition);
						OutTransform.SetRotation(FRotator(outOrientation).Quaternion());
					}
					break;
				}
			}
		}

		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[TryGetAnchorTransform] Get transform failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::SetAnchorComponentStatus(uint64 AnchorHandle, EOculusXRSpaceComponentType ComponentType, bool Enable, float Timeout, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SetAnchorComponentStatus] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsAnchorExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SetAnchorComponentStatus] Spatial entity extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		// Early out if the change is a no-op
		bool isEnabled, changePending;
		auto getStatusResult = GetAnchorComponentStatus(AnchorHandle, ComponentType, isEnabled, changePending);
		bool isStatusChangingOrSame = ((isEnabled == Enable) && !changePending) || ((isEnabled != Enable) && changePending);
		if (XR_SUCCEEDED(getStatusResult) && isStatusChangingOrSame)
		{
			return XR_SUCCESS;
		}

		XrSpaceComponentStatusSetInfoFB setInfo{ XR_TYPE_SPACE_COMPONENT_STATUS_SET_INFO_FB, nullptr };
		setInfo.componentType = OculusXRAnchors::ToComponentType(ComponentType);
		setInfo.enabled = Enable ? 1 : 0;
		setInfo.timeout = Timeout;

		auto result = xrSetSpaceComponentStatusFB((XrSpace)AnchorHandle, &setInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SetAnchorComponentStatus] Set space component status failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::GetAnchorComponentStatus(uint64 AnchorHandle, EOculusXRSpaceComponentType ComponentType, bool& OutEnabled, bool& OutChangePending)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetAnchorComponentStatus] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsAnchorExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetAnchorComponentStatus] Spatial entity extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpaceComponentStatusFB status{ XR_TYPE_SPACE_COMPONENT_STATUS_FB, nullptr };
		auto result = xrGetSpaceComponentStatusFB((XrSpace)AnchorHandle, OculusXRAnchors::ToComponentType(ComponentType), &status);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetAnchorComponentStatus] Get space component status failed. Result: %d"), result);
		}
		else
		{
			OutEnabled = (bool)status.enabled;
			OutChangePending = (bool)status.changePending;
		}

		return result;
	}

	XrResult FAnchorsXR::GetSupportedAnchorComponents(uint64 AnchorHandle, TArray<EOculusXRSpaceComponentType>& OutSupportedTypes)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetSupportedAnchorComponents] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsAnchorExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetSupportedAnchorComponents] Spatial entity extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		uint32 input = 0;
		uint32 output = 0;

		auto getCapacityResult = xrEnumerateSpaceSupportedComponentsFB((XrSpace)AnchorHandle, input, &output, nullptr);
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[GetSupportedAnchorComponents] -- Capacity"));
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Capacity: %d"), output);
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), getCapacityResult);

		if (XR_FAILED(getCapacityResult))
		{
			return getCapacityResult;
		}

		input = output;
		TArray<XrSpaceComponentTypeFB> components;
		components.SetNum(output);

		auto dataResult = xrEnumerateSpaceSupportedComponentsFB((XrSpace)AnchorHandle, input, &output, components.GetData());
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[GetSupportedAnchorComponents] -- Data Retrieval"));
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), dataResult);
		for (auto& it : components)
		{
			auto ueComponentType = OculusXRAnchors::ToComponentType(it);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Component Type: %s"), *OculusXRAnchors::ToString(ueComponentType));
			OutSupportedTypes.Add(ueComponentType);
		}

		return dataResult;
	}

	XrResult FAnchorsXR::GetAnchorContainerUUIDs(uint64 AnchorHandle, TArray<FOculusXRUUID>& OutUUIDs)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetAnchorContainerUUIDs] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsContainerExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetAnchorContainerUUIDs] Spatial entity container extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpaceContainerFB spaceContainer = { XR_TYPE_SPACE_CONTAINER_FB, nullptr };
		spaceContainer.uuidCountOutput = 0;
		spaceContainer.uuidCapacityInput = 0;
		spaceContainer.uuids = nullptr;

		auto getCapacityResult = xrGetSpaceContainerFB(OpenXRHMD->GetSession(), (XrSpace)AnchorHandle, &spaceContainer);
		if (XR_FAILED(getCapacityResult))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetAnchorContainerUUIDs] -- Failed to get space container capacity. Result: %d"), getCapacityResult);
			return getCapacityResult;
		}

		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[GetAnchorContainerUUIDs] -- Capacity"));
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Capacity: %d"), spaceContainer.uuidCountOutput);
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), getCapacityResult);

		spaceContainer.uuidCapacityInput = spaceContainer.uuidCountOutput;
		TArray<XrUuidEXT> uuids;
		uuids.SetNum(spaceContainer.uuidCapacityInput);
		spaceContainer.uuids = uuids.GetData();

		auto dataResult = xrGetSpaceContainerFB(OpenXRHMD->GetSession(), (XrSpace)AnchorHandle, &spaceContainer);
		if (XR_FAILED(dataResult))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetAnchorContainerUUIDs] -- Failed to get space container data. Result: %d"), dataResult);
			return dataResult;
		}

		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[GetAnchorContainerUUIDs] -- Data Retrieval"));
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    Result: %d"), dataResult);
		for (auto& it : uuids)
		{
			FOculusXRUUID ueUuid(it.data);
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("    UUID: %s"), *ueUuid.ToString());
			OutUUIDs.Add(ueUuid);
		}

		return dataResult;
	}

	XrResult FAnchorsXR::SaveAnchor(uint64 AnchorHandle, EOculusXRSpaceStorageLocation StorageLocation, EOculusXRSpaceStoragePersistenceMode StoragePersistenceMode, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchor] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsStorageExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchor] Spatial entity storage extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!AnchorHandle)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchor] Supplied anchor handle is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpaceSaveInfoFB spaceSaveInfo = { XR_TYPE_SPACE_SAVE_INFO_FB, nullptr };
		spaceSaveInfo.space = (XrSpace)AnchorHandle;
		spaceSaveInfo.location = OculusXRAnchors::ToStorageLocation(StorageLocation);
		spaceSaveInfo.persistenceMode = XR_SPACE_PERSISTENCE_MODE_INDEFINITE_FB; // Only one persistence mode so far, this is hard coded

		auto result = xrSaveSpaceFB(OpenXRHMD->GetSession(), &spaceSaveInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchor] Save anchor failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::SaveAnchorList(const TArray<uint64>& AnchorHandles, EOculusXRSpaceStorageLocation StorageLocation, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchorList] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsStorageBatchExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchorList] Spatial entity storage batch extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (AnchorHandles.Num() == 0)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchorList] You must supply more than zero anchors to save anchor list."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpaceListSaveInfoFB spaceListSaveInfo = { XR_TYPE_SPACE_LIST_SAVE_INFO_FB, nullptr };
		spaceListSaveInfo.location = OculusXRAnchors::ToStorageLocation(StorageLocation);
		spaceListSaveInfo.spaces = (XrSpace*)AnchorHandles.GetData();
		spaceListSaveInfo.spaceCount = AnchorHandles.Num();

		auto result = xrSaveSpaceListFB(OpenXRHMD->GetSession(), &spaceListSaveInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchorList] Save anchor list failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::SaveAnchors(const TArray<uint64>& AnchorHandles, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchors] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsPersistenceExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchors] Spatial entity persistence extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (AnchorHandles.Num() == 0)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchors] You must supply more than zero anchors to save anchors."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpacesSaveInfoMETA spacesSaveInfo = { XR_TYPE_SPACES_SAVE_INFO_META, nullptr };
		spacesSaveInfo.spaces = (XrSpace*)AnchorHandles.GetData();
		spacesSaveInfo.spaceCount = AnchorHandles.Num();

		auto result = xrSaveSpacesMETA(OpenXRHMD->GetSession(), &spacesSaveInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[SaveAnchors] Save anchors failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::DiscoverAnchors(const FOculusXRSpaceDiscoveryInfo& DiscoveryInfo, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DiscoverAnchors] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsDiscoveryExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DiscoverAnchors] Spatial entity discovery extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		const auto filterCount = DiscoveryInfo.Filters.Num();
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[DiscoverAnchors] FilterCount: %d"), filterCount);

		union FilterUnion
		{
			XrSpaceFilterUuidMETA uuidFilter;
			XrSpaceFilterComponentMETA componentFilter;
		};

		TArray<XrUuidEXT> uuidBuffer;

		// This vector provides temporary storage for each filter
		TArray<FilterUnion> filterStorage;
		filterStorage.SetNum(filterCount);

		// This vector is the array of points to filters in filterStorage (the input to OpenXR)
		TArray<const XrSpaceFilterBaseHeaderMETA*> filterArray;
		filterArray.SetNum(filterCount);

		for (int32 i = 0; i < filterCount; ++i)
		{
			const auto& filter = DiscoveryInfo.Filters[i];
			FilterUnion xrFilter = {};

			UOculusXRSpaceDiscoveryIdsFilter* IdFilter = Cast<UOculusXRSpaceDiscoveryIdsFilter>(DiscoveryInfo.Filters[i]);
			UOculusXRSpaceDiscoveryComponentsFilter* ComponentFilter = Cast<UOculusXRSpaceDiscoveryComponentsFilter>(DiscoveryInfo.Filters[i]);

			if (IsValid(IdFilter))
			{
				if (IdFilter->Uuids.Num() == 0)
				{
					UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DiscoverAnchors] Uuid filter is empty when attempting to filter by uuid."));
					return XR_ERROR_VALIDATION_FAILURE;
				}

				UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[DiscoverAnchors] UUID Filter:"));
				for (auto& it : IdFilter->Uuids)
				{
					UE_LOG(LogOculusXRAnchors, Verbose, TEXT("	%s"), *it.ToString());
				}

				Algo::Transform(IdFilter->Uuids, uuidBuffer, [](const FOculusXRUUID& In) { return OculusXRAnchors::ToUuid(In); });

				xrFilter.uuidFilter = {
					XR_TYPE_SPACE_FILTER_UUID_META,
					nullptr,
					static_cast<uint32>(uuidBuffer.Num()),
					reinterpret_cast<const XrUuidEXT*>(uuidBuffer.GetData()),
				};
			}
			else if (IsValid(ComponentFilter))
			{
				UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[DiscoverAnchors] Component Filter:"));
				UE_LOG(LogOculusXRAnchors, Verbose, TEXT("	%s"), *OculusXRAnchors::ToString(ComponentFilter->ComponentType));

				xrFilter.componentFilter = {
					XR_TYPE_SPACE_FILTER_COMPONENT_META,
					nullptr,
					OculusXRAnchors::ToComponentType(ComponentFilter->ComponentType),
				};
			}
			else
			{
				UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DiscoverAnchors] Unknown filter type."));
			}

			filterStorage[i] = xrFilter;
			filterArray[i] = reinterpret_cast<const XrSpaceFilterBaseHeaderMETA*>(&filterStorage[i]);
		}

		XrSpaceDiscoveryInfoMETA xrDiscoveryInfo = { XR_TYPE_SPACE_DISCOVERY_INFO_META };
		xrDiscoveryInfo.filterCount = static_cast<uint32_t>(filterArray.Num());
		xrDiscoveryInfo.filters = filterArray.GetData();

		auto result = xrDiscoverSpacesMETA(OpenXRHMD->GetSession(), &xrDiscoveryInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DiscoverAnchors] Discover anchors failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::QueryAnchors(const FOculusXRSpaceQueryInfo& QueryInfo, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[QueryAnchors] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsQueryExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[QueryAnchors] Spatial entity query extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpaceUuidFilterInfoFB uuidFilter;
		TArray<XrUuidEXT> uuidsBuffer;
		XrSpaceComponentFilterInfoFB componentTypeFilter;
		XrSpaceGroupUuidFilterInfoMETA groupUuidFilter;
		XrSpaceStorageLocationFilterInfoFB storageLocation;

		XrSpaceQueryInfoFB actionQuery = { XR_TYPE_SPACE_QUERY_INFO_FB, nullptr };
		actionQuery.next = nullptr;
		actionQuery.excludeFilter = nullptr;
		actionQuery.maxResultCount = QueryInfo.MaxQuerySpaces;
		actionQuery.timeout = OculusXR::ToXrDuration(QueryInfo.Timeout);
		actionQuery.queryAction = XR_SPACE_QUERY_ACTION_LOAD_FB;
		actionQuery.excludeFilter = nullptr;

		XrSpaceFilterInfoBaseHeaderFB* filter = nullptr;
		if (QueryInfo.FilterType == EOculusXRSpaceQueryFilterType::FilterByIds)
		{
			if (QueryInfo.IDFilter.Num() == 0)
			{
				UE_LOG(LogOculusXRAnchors, Warning, TEXT("[QueryAnchors] Uuid filter is empty when attempting to filter by uuid."));
				return XR_ERROR_VALIDATION_FAILURE;
			}

			// Add UUID filter
			uuidFilter = { XR_TYPE_SPACE_UUID_FILTER_INFO_FB };
			uuidFilter.next = nullptr;
			Algo::Transform(QueryInfo.IDFilter, uuidsBuffer, [](const FOculusXRUUID& In) { return OculusXRAnchors::ToUuid(In); });
			uuidFilter.uuids = uuidsBuffer.GetData();
			uuidFilter.uuidCount = uuidsBuffer.Num();
			filter = (XrSpaceFilterInfoBaseHeaderFB*)(&uuidFilter);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[QueryAnchors] UUID Filter:"));
			for (auto& it : QueryInfo.IDFilter)
			{
				UE_LOG(LogOculusXRAnchors, Verbose, TEXT("	%s"), *it.ToString());
			}
		}
		else if (QueryInfo.FilterType == EOculusXRSpaceQueryFilterType::FilterByComponentType)
		{
			if (QueryInfo.ComponentFilter.Num() != 0)
			{
				UE_LOG(LogOculusXRAnchors, Warning, TEXT("[QueryAnchors] You must supply only one component when attempting to filter by component."));
				return XR_ERROR_VALIDATION_FAILURE;
			}

			// Add component filter
			componentTypeFilter = { XR_TYPE_SPACE_COMPONENT_FILTER_INFO_FB };
			componentTypeFilter.next = nullptr;
			componentTypeFilter.componentType = OculusXRAnchors::ToComponentType(QueryInfo.ComponentFilter[0]);
			filter = (XrSpaceFilterInfoBaseHeaderFB*)(&componentTypeFilter);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[QueryAnchors] Component Filter:"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("	%s"), *OculusXRAnchors::ToString(QueryInfo.ComponentFilter[0]));
		}
		else if (QueryInfo.FilterType == EOculusXRSpaceQueryFilterType::FilterByGroup)
		{
			// Add component filter
			groupUuidFilter = { XR_TYPE_SPACE_GROUP_UUID_FILTER_INFO_META };
			groupUuidFilter.next = nullptr;
			groupUuidFilter.groupUuid = OculusXRAnchors::ToUuid(QueryInfo.GroupUUIDFilter);
			filter = (XrSpaceFilterInfoBaseHeaderFB*)(&groupUuidFilter);

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[QueryAnchors] Group Filter:"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("	%s"), *OculusXRAnchors::ToString(QueryInfo.ComponentFilter[0]));
		}
		else
		{
			actionQuery.filter = nullptr;
		}

		// if storage is enabled add the storage location to the query
		if (IsStorageExtensionSupported() && filter != nullptr)
		{
			storageLocation = { XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB };
			storageLocation.next = nullptr;
			storageLocation.location = OculusXRAnchors::ToStorageLocation(QueryInfo.Location);
			filter->next = &storageLocation;

			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("[QueryAnchors] Location Filter:"));
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("	%s"), *OculusXRAnchors::ToString(QueryInfo.Location));
		}

		actionQuery.filter = filter;

		auto result = xrQuerySpacesFB(OpenXRHMD->GetSession(), (XrSpaceQueryInfoBaseHeaderFB*)&actionQuery, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[QueryAnchors] Share anchors failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::ShareAnchors(const TArray<uint64>& AnchorHandles, const TArray<uint64>& UserIds, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[ShareAnchors] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsSharingExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[ShareAnchors] Spatial entity sharing extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (AnchorHandles.Num() == 0)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[ShareAnchors] You must supply more than zero anchors to share anchors."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (UserIds.Num() == 0)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[ShareAnchors] You must supply more than zero users to share anchors."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		TArray<XrSpaceUserFB> users;
		Algo::Transform(UserIds, users, [this](const uint64& In) {
			XrSpaceUserFB out;
			CreateSpaceUser(In, (SpaceUser&)out);
			return out;
		});

		XrSpaceShareInfoFB shareInfo = { XR_TYPE_SPACE_SHARE_INFO_FB, nullptr };
		shareInfo.spaces = (XrSpace*)AnchorHandles.GetData();
		shareInfo.spaceCount = AnchorHandles.Num();
		shareInfo.users = users.GetData();
		shareInfo.userCount = users.Num();

		auto result = xrShareSpacesFB(OpenXRHMD->GetSession(), &shareInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[ShareAnchors] Share anchors failed. Result: %d"), result);
		}

		for (auto& it : users)
		{
			DestroySpaceUser((SpaceUser)it);
		}

		return result;
	}

	XrResult FAnchorsXR::ShareAnchorsWithGroups(const TArray<uint64>& AnchorHandles, const TArray<FOculusXRUUID>& Groups, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[ShareAnchorsWithGroups] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsSharingMetaExtensionSupported() || !IsGroupSharingExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[ShareAnchorsWithGroups] Spatial entity sharing (Meta) extensions are unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		for (auto& it : Groups)
		{
			if (it == FOculusXRUUID::Zero)
			{
				UE_LOG(LogOculusXRAnchors, Error, TEXT("Anchor sharing failed. One or more group targets provided had a UUID value of zero."));
				return XR_ERROR_VALIDATION_FAILURE;
			}
		}

		XrShareSpacesInfoMETA xrInfo = { XR_TYPE_SHARE_SPACES_INFO_META, nullptr };
		xrInfo.spaces = (XrSpace*)AnchorHandles.GetData();
		xrInfo.spaceCount = AnchorHandles.Num();

		XrShareSpacesRecipientGroupsMETA groupRecipientInfo = { XR_TYPE_SHARE_SPACES_RECIPIENT_GROUPS_META, nullptr };
		TArray<XrUuidEXT> groupUuids;
		Algo::Transform(Groups, groupUuids, [](const FOculusXRUUID& In) { return OculusXRAnchors::ToUuid(In); });

		groupRecipientInfo.groupCount = groupUuids.Num();
		groupRecipientInfo.groups = groupUuids.GetData();
		xrInfo.recipientInfo = reinterpret_cast<const XrShareSpacesRecipientBaseHeaderMETA*>(&groupRecipientInfo);

		auto result = xrShareSpacesMETA(OpenXRHMD->GetSession(), &xrInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		return result;
	}

	XrResult FAnchorsXR::CreateSpaceUser(uint64 SpaceUserId, SpaceUser& OutUser)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[CreateSpaceUser] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsUserExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[CreateSpaceUser] Spatial entity user extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!SpaceUserId)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[CreateSpaceUser] Invalid space user id."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpaceUserCreateInfoFB userCreateInfo = { XR_TYPE_SPACE_USER_CREATE_INFO_FB, nullptr };
		userCreateInfo.userId = SpaceUserId;

		auto result = xrCreateSpaceUserFB(OpenXRHMD->GetSession(), &userCreateInfo, (XrSpaceUserFB*)&OutUser);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[CreateSpaceUser] Create space user failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::GetSpaceUserId(const SpaceUser& User, uint64& OutId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetSpaceUserId] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsUserExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetSpaceUserId] Spatial entity user extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!User)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetSpaceUserId] Invalid space user."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		auto result = xrGetSpaceUserIdFB((XrSpaceUserFB)User, (XrSpaceUserIdFB*)&OutId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[GetSpaceUserId] Get space user id failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::DestroySpaceUser(const SpaceUser& User)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DestroySpaceUser] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsUserExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DestroySpaceUser] Spatial entity user extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!User)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DestroySpaceUser] Invalid space user."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		auto result = xrDestroySpaceUserFB((XrSpaceUserFB)User);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[DestroySpaceUser] Destroy space user failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::EraseAnchor(uint64 AnchorHandle, EOculusXRSpaceStorageLocation StorageLocation, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchor] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsStorageExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchor] Spatial entity storage extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!AnchorHandle)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchor] Supplied anchor handle is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		XrSpaceEraseInfoFB spaceEraseInfo = { XR_TYPE_SPACE_ERASE_INFO_FB, nullptr };
		spaceEraseInfo.space = (XrSpace)AnchorHandle;
		spaceEraseInfo.location = OculusXRAnchors::ToStorageLocation(StorageLocation);

		auto result = xrEraseSpaceFB(OpenXRHMD->GetSession(), &spaceEraseInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchor] Erase anchor failed. Result: %d"), result);
		}

		return result;
	}

	XrResult FAnchorsXR::EraseAnchors(const TArray<FOculusXRUInt64>& AnchorHandles, const TArray<FOculusXRUUID>& UUIDs, uint64& OutRequestId)
	{
		if (!OpenXRHMD || !OpenXRHMD->GetInstance() || !OpenXRHMD->GetSession())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchors] XR state is invalid."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (!IsPersistenceExtensionSupported())
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchors] Spatial entity persistence extension is unsupported."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		if (AnchorHandles.Num() == 0 && UUIDs.Num() == 0)
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchors] You must supply more than zero anchors to save anchors."));
			return XR_ERROR_VALIDATION_FAILURE;
		}

		TArray<XrSpace> spaces;
		TArray<XrUuidEXT> uuids;
		Algo::Transform(AnchorHandles, spaces, [](const FOculusXRUInt64& in) { return (XrSpace)in.GetValue(); });
		Algo::Transform(UUIDs, uuids, [](const FOculusXRUUID& in) { return OculusXRAnchors::ToUuid(in); });

		XrSpacesEraseInfoMETA spacesEraseInfo = { XR_TYPE_SPACES_ERASE_INFO_META, nullptr };
		spacesEraseInfo.spaces = spaces.GetData();
		spacesEraseInfo.spaceCount = spaces.Num();
		spacesEraseInfo.uuids = uuids.GetData();
		spacesEraseInfo.uuidCount = uuids.Num();

		auto result = xrEraseSpacesMETA(OpenXRHMD->GetSession(), &spacesEraseInfo, (XrAsyncRequestIdFB*)&OutRequestId);
		if (!XR_SUCCEEDED(result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("[EraseAnchors] Erase anchors failed. Result: %d"), result);
		}

		return result;
	}

	void FAnchorsXR::InitOpenXRFunctions(XrInstance InInstance)
	{
		// XR_FB_Spatial_Entity
		if (IsAnchorExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorFB", &xrCreateSpatialAnchorFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrSetSpaceComponentStatusFB", &xrSetSpaceComponentStatusFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrGetSpaceComponentStatusFB", &xrGetSpaceComponentStatusFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrEnumerateSpaceSupportedComponentsFB", &xrEnumerateSpaceSupportedComponentsFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrGetSpaceUuidFB", &xrGetSpaceUuidFB);
		}

		// XR_FB_spatial_entity_container
		if (IsContainerExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrGetSpaceContainerFB", &xrGetSpaceContainerFB);
		}

		// XR_FB_spatial_entity_query
		if (IsQueryExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrQuerySpacesFB", &xrQuerySpacesFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrRetrieveSpaceQueryResultsFB", &xrRetrieveSpaceQueryResultsFB);
		}

		//  XR_FB_spatial_entity_sharing
		if (IsSharingExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrShareSpacesFB", &xrShareSpacesFB);
		}

		// XR_FB_spatial_entity_storage
		if (IsStorageExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrSaveSpaceFB", &xrSaveSpaceFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrEraseSpaceFB", &xrEraseSpaceFB);
		}

		// XR_FB_spatial_entity_storage_batch
		if (IsStorageBatchExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrSaveSpaceListFB", &xrSaveSpaceListFB);
		}

		// XR_FB_spatial_entity_user
		if (IsUserExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrCreateSpaceUserFB", &xrCreateSpaceUserFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrDestroySpaceUserFB", &xrDestroySpaceUserFB);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrGetSpaceUserIdFB", &xrGetSpaceUserIdFB);
		}

		// XR_META_spatial_entity_persistence
		if (IsPersistenceExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrSaveSpacesMETA", &xrSaveSpacesMETA);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrEraseSpacesMETA", &xrEraseSpacesMETA);
		}

		// XR_META_spatial_entity_discovery
		if (IsDiscoveryExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrDiscoverSpacesMETA", &xrDiscoverSpacesMETA);
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrRetrieveSpaceDiscoveryResultsMETA", &xrRetrieveSpaceDiscoveryResultsMETA);
		}

		// XR_META_spatial_entity_sharing
		if (IsSharingExtensionSupported())
		{
			OculusXR::XRGetInstanceProcAddr(InInstance, "xrShareSpacesMETA", &xrShareSpacesMETA);
		}
	}

} // namespace XRAnchors

#undef LOCTEXT_NAMESPACE
