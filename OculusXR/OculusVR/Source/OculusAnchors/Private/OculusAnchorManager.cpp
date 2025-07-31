/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchorManager.h"

#include <vector>

#include "OculusHMD.h"
#include "OculusAnchorsModule.h"
#include "OculusAnchorDelegates.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "OculusAnchorTypesPrivate.h"

namespace OculusAnchors
{
	OculusHMD::FOculusHMD* GetHMD(bool& OutSuccessful)
	{
		OculusHMD::FOculusHMD* OutHMD = GEngine->XRSystem.IsValid() ? (OculusHMD::FOculusHMD*)(GEngine->XRSystem->GetHMDDevice()) : nullptr;
		if (!OutHMD)
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to retrieve OculusHMD"));
			OutSuccessful = false;
		}

		OutSuccessful = true;

		return OutHMD;
	}

	ovrpUuid ConvertFUUIDtoOvrpUuid(const FUUID& UUID)
	{
		ovrpUuid Result;
		FMemory::Memcpy(Result.data, UUID.UUIDBytes);

		return Result;
	}

	ovrpSpaceQueryInfo ConvertToOVRPSpaceQueryInfo(const FOculusSpaceQueryInfo& UEQueryInfo)
	{
		static const int32 MaxIdsInFilter = 1024;
		static const int32 MaxComponentTypesInFilter = 16;

		ovrpSpaceQueryInfo Result;
		Result.queryType = ovrpSpaceQueryType_Action;
		Result.actionType = ovrpSpaceQueryActionType_Load;

		Result.maxQuerySpaces = UEQueryInfo.MaxQuerySpaces;
		Result.timeout = static_cast<double>(UEQueryInfo.Timeout); // Prevent compiler warnings, though there is a possible loss of data here.

		switch (UEQueryInfo.Location)
		{
		case EOculusSpaceStorageLocation::Invalid:
			Result.location = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusSpaceStorageLocation::Local:
			Result.location = ovrpSpaceStorageLocation_Local;
			break;
		case EOculusSpaceStorageLocation::Cloud:
			Result.location = ovrpSpaceStorageLocation_Cloud;
			break;
		}

		switch (UEQueryInfo.FilterType)
		{
		case EOculusSpaceQueryFilterType::None:
			Result.filterType = ovrpSpaceQueryFilterType_None;
			break;
		case EOculusSpaceQueryFilterType::FilterByIds:
			Result.filterType = ovrpSpaceQueryFilterType_Ids;
			break;
		case EOculusSpaceQueryFilterType::FilterByComponentType:
			Result.filterType = ovrpSpaceQueryFilterType_Components;
			break;
		}

		Result.IdInfo.numIds = FMath::Min(MaxIdsInFilter, UEQueryInfo.IDFilter.Num());
		for (int i = 0; i < Result.IdInfo.numIds; ++i)
		{
			ovrpUuid OvrUuid = ConvertFUUIDtoOvrpUuid(UEQueryInfo.IDFilter[i]);
			Result.IdInfo.ids[i] = OvrUuid;
		}

		Result.componentsInfo.numComponents = FMath::Min(MaxComponentTypesInFilter, UEQueryInfo.ComponentFilter.Num());
		for (int i = 0; i < Result.componentsInfo.numComponents; ++i)
		{
			Result.componentsInfo.components[i] = ConvertToOvrpComponentType(UEQueryInfo.ComponentFilter[i]);
		}

		return Result;
	}

	template<typename T>
	void GetEventData(ovrpEventDataBuffer& Buffer, T& OutEventData)
	{
		unsigned char* BufData = Buffer.EventData;
		BufData -= sizeof(uint64); //correct offset 

		memcpy(&OutEventData, BufData, sizeof(T));
	}

	void FOculusAnchorManager::OnPollEvent(ovrpEventDataBuffer* EventDataBuffer, bool& EventPollResult)
	{
		ovrpEventDataBuffer& buf = *EventDataBuffer;

		switch (buf.EventType) 
		{
		
			case ovrpEventType_SpatialAnchorCreateComplete:
			{
				ovrpEventDataSpatialAnchorCreateComplete AnchorCreateEvent;
				GetEventData(buf, AnchorCreateEvent);

				const FUInt64 RequestId(AnchorCreateEvent.requestId);
				const FUInt64 Space(AnchorCreateEvent.space);
				const FUUID BPUUID(AnchorCreateEvent.uuid.data);
			
				FOculusAnchorEventDelegates::OculusSpatialAnchorCreateComplete.Broadcast(RequestId, AnchorCreateEvent.result, Space, BPUUID);

				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpatialAnchorCreateComplete Request ID: %llu  --  Space: %llu  --  UUID: %s  --  Result: %d"), 
					RequestId.GetValue(), 
					Space.GetValue(),
					*BPUUID.ToString(),
					AnchorCreateEvent.result
				);

				break;
			}
			case ovrpEventType_SpaceSetComponentStatusComplete:
			{
				ovrpEventDataSpaceSetStatusComplete SetStatusEvent;
				GetEventData(buf, SetStatusEvent);
				
				//translate to BP types
				const FUInt64 RequestId(SetStatusEvent.requestId);
				const FUInt64 Space(SetStatusEvent.space);
				EOculusSpaceComponentType BPSpaceComponentType = ConvertToUe4ComponentType(SetStatusEvent.componentType);
				const FUUID BPUUID(SetStatusEvent.uuid.data);
				const bool bEnabled = (SetStatusEvent.enabled == ovrpBool_True);
				
				FOculusAnchorEventDelegates::OculusSpaceSetComponentStatusComplete.Broadcast(
					RequestId,
					SetStatusEvent.result,
					Space,
					BPUUID,
					BPSpaceComponentType,
					bEnabled
				);

				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceSetComponentStatusComplete Request ID: %llu  --  Type: %d  --  Enabled: %d  --  Space: %llu  --  Result: %d"), 
					SetStatusEvent.requestId,
					SetStatusEvent.componentType,
					SetStatusEvent.enabled,
					SetStatusEvent.space,
					SetStatusEvent.result
				);

				break;
			}
			case ovrpEventType_SpaceQueryResults:
			{
				ovrpEventSpaceQueryResults QueryEvent;
				GetEventData(buf, QueryEvent);

				const FUInt64 RequestId(QueryEvent.requestId);

				FOculusAnchorEventDelegates::OculusSpaceQueryResults.Broadcast(RequestId);

				ovrpUInt32 ovrpOutCapacity = 0;

				//first get capacity
				const bool bGetCapacityResult = FOculusHMDModule::GetPluginWrapper().GetInitialized() && OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().RetrieveSpaceQueryResults(
					&QueryEvent.requestId,
					0,
					&ovrpOutCapacity,
					nullptr));

				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceQueryResults Request ID: %llu  --  Capacity: %d  --  Result: %d"), QueryEvent.requestId, ovrpOutCapacity, bGetCapacityResult);

				std::vector<ovrpSpaceQueryResult> ovrpResults(ovrpOutCapacity);

				// Get Query Data
				const bool bGetQueryDataResult = FOculusHMDModule::GetPluginWrapper().GetInitialized() && OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().RetrieveSpaceQueryResults(
					&QueryEvent.requestId,
					ovrpResults.size(),
					&ovrpOutCapacity,
					ovrpResults.data()));

				for(auto queryResultElement: ovrpResults)
				{
					UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceQueryResult Space: %llu  --  Result: %d"), queryResultElement.space, bGetQueryDataResult);

					//translate types 
					FUInt64 Space(queryResultElement.space);
					FUUID BPUUID(queryResultElement.uuid.data);
					FOculusAnchorEventDelegates::OculusSpaceQueryResult.Broadcast(RequestId, Space, BPUUID);
				}

				break;
			}
			case ovrpEventType_SpaceQueryComplete:
			{
				ovrpEventSpaceQueryComplete QueryCompleteEvent;
				GetEventData(buf, QueryCompleteEvent);

				//translate to BP types
				const FUInt64 RequestId(QueryCompleteEvent.requestId);
            
                FOculusAnchorEventDelegates::OculusSpaceQueryComplete.Broadcast(RequestId, QueryCompleteEvent.result);

				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceQueryComplete Request ID: %llu  --  Result: %d"), QueryCompleteEvent.requestId, QueryCompleteEvent.result);

				break;
			}
			case ovrpEventType_SpaceSaveComplete:
			{
				ovrpEventSpaceStorageSaveResult StorageResult;
				GetEventData(buf, StorageResult);
    
                //translate to BP types
                const FUUID uuid(StorageResult.uuid.data);
                const FUInt64 FSpace(StorageResult.space);
				const FUInt64 FRequest(StorageResult.requestId);
				const bool bResult = StorageResult.result >= 0;

				FOculusAnchorEventDelegates::OculusSpaceSaveComplete.Broadcast(FRequest, FSpace, bResult, StorageResult.result, uuid);
                
				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceSaveComplete  Request ID: %llu  --  Space: %llu  --  Result: %d"), StorageResult.requestId, StorageResult.space, StorageResult.result);

                break;
			}
			case ovrpEventType_SpaceListSaveResult:
			{
				ovrpEventSpaceListSaveResult SpaceListSaveResult;
				GetEventData(buf, SpaceListSaveResult);

				FUInt64 RequestId(SpaceListSaveResult.requestId);

				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceListSaveResult  Request ID: %llu  --  Result: %d"), SpaceListSaveResult.requestId, SpaceListSaveResult.result);
				FOculusAnchorEventDelegates::OculusSpaceListSaveComplete.Broadcast(RequestId, SpaceListSaveResult.result);

				break;
			}
			case ovrpEventType_SpaceEraseComplete:
			{
				ovrpEventSpaceStorageEraseResult SpaceEraseEvent;
				GetEventData(buf, SpaceEraseEvent);

				//translate to BP types
				const FUUID uuid(SpaceEraseEvent.uuid.data);
				const FUInt64 FRequestId(SpaceEraseEvent.requestId);
				const FUInt64 FResult(SpaceEraseEvent.result);
				const EOculusSpaceStorageLocation BPLocation = (SpaceEraseEvent.location == ovrpSpaceStorageLocation_Local) ? EOculusSpaceStorageLocation::Local : EOculusSpaceStorageLocation::Invalid;
				
				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceEraseComplete  Request ID: %llu  --  Result: %d  -- UUID: %s"), SpaceEraseEvent.requestId, SpaceEraseEvent.result, *UOculusAnchorBPFunctionLibrary::AnchorUUIDToString(SpaceEraseEvent.uuid.data));

				FOculusAnchorEventDelegates::OculusSpaceEraseComplete.Broadcast(FRequestId, FResult.Value, uuid, BPLocation);
				break;
			}
			case ovrpEventType_SpaceShareResult:
			{
				unsigned char* BufData = buf.EventData;
				ovrpUInt64 OvrpRequestId = 0;
				memcpy(&OvrpRequestId, BufData, sizeof(OvrpRequestId));

				ovrpEventSpaceShareResult SpaceShareSpaceResult;
				GetEventData(buf, SpaceShareSpaceResult);

				FUInt64 RequestId(SpaceShareSpaceResult.requestId);

				UE_LOG(LogOculusAnchors, Log, TEXT("ovrpEventType_SpaceShareSpaceResult  Request ID: %llu  --  Result: %d"),
					SpaceShareSpaceResult.requestId,
					SpaceShareSpaceResult.result
				);

				FOculusAnchorEventDelegates::OculusSpaceShareComplete.Broadcast(RequestId, SpaceShareSpaceResult.result);

				break;
			}


			default:
			{
				EventPollResult = false;
				break;
			}
		}

		EventPollResult = true;
	}

	/**
	 * @brief Creates a spatial anchor
	 * @param InTransform The transform of the object
	 * @param OutRequestId the handle of the spatial anchor created
	 * @return True if the async call started successfully
	 */
	EOculusResult::Type FOculusAnchorManager::CreateAnchor(const FTransform& InTransform, uint64 &OutRequestId, const FTransform& CameraTransform)
	{
		bool bValidHMD;
		OculusHMD::FOculusHMD* HMD = GetHMD(bValidHMD);
		if (!bValidHMD)
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("FOculusAnchorManager::CreateAnchor failed to retrieve HMD."));
			return EOculusResult::Failure;
		}
		
		ovrpTrackingOrigin TrackingOriginType;
		ovrpPosef Posef;
		double Time = 0;

		const FTransform TrackingToWorld = HMD->GetLastTrackingToWorld();

		// convert to tracking space
		const FQuat TrackingSpaceOrientation = TrackingToWorld.Inverse().TransformRotation(InTransform.Rotator().Quaternion());
		const FVector TrackingSpacePosition = TrackingToWorld.Inverse().TransformPosition(InTransform.GetLocation());
		const OculusHMD::FPose TrackingSpacePose(TrackingSpaceOrientation, TrackingSpacePosition);
		
#if WITH_EDITOR
		// Link only head space position update
		FVector OutHeadPosition;
		FQuat OutHeadOrientation;
		const bool bGetPose = HMD->GetCurrentPose(HMD->HMDDeviceId, OutHeadOrientation, OutHeadPosition);
		if (!bGetPose)
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("FOculusAnchorManager::CreateAnchor failed to get current headset pose."));
			return EOculusResult::Failure;
		}

		OculusHMD::FPose HeadPose(OutHeadOrientation, OutHeadPosition);

		OculusHMD::FPose MainCameraPose(CameraTransform.GetRotation(), CameraTransform.GetLocation());
		OculusHMD::FPose PoseInHeadSpace = MainCameraPose.Inverse() * TrackingSpacePose;

		// To world space pose
		OculusHMD::FPose WorldPose = HeadPose * PoseInHeadSpace;

		const bool bConverted = HMD->ConvertPose(WorldPose, Posef);
#else
		const bool bConverted = HMD->ConvertPose(TrackingSpacePose, Posef);
#endif

		if (!bConverted)
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("FOculusAnchorManager::CreateAnchor failed to convert pose."));
			return EOculusResult::Failure;
		}
		
		FOculusHMDModule::GetPluginWrapper().GetTrackingOriginType2(&TrackingOriginType);
		FOculusHMDModule::GetPluginWrapper().GetTimeInSeconds(&Time);
		
		const ovrpSpatialAnchorCreateInfo SpatialAnchorCreateInfo = {
			TrackingOriginType,
			Posef,
			Time
		};

		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().CreateSpatialAnchor(&SpatialAnchorCreateInfo, &OutRequestId);
		UE_LOG(LogOculusAnchors, Log, TEXT("CreateAnchor Request ID: %llu"), OutRequestId);

		if(!OVRP_SUCCESS(Result))
		{
			UE_LOG(LogOculusAnchors, Error, TEXT("FOculusAnchorManager::CreateAnchor failed. Result: %d"), Result);
		}

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief Destroys the given space;
	 * @param Space The space handle.
	 * @return True if the space was destroyed successfully.
	 */
	EOculusResult::Type FOculusAnchorManager::DestroySpace(uint64 Space)
	{
		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().DestroySpace(static_cast<ovrpSpace*>(&Space));

		UE_LOG(LogOculusAnchors, Log, TEXT("DestroySpace  Space ID: %llu"), Space);

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief Sets the given component status on an anchor
	 * @param Space The space handle
	 * @param SpaceComponentType The component type to change  
	 * @param Enable true or false
	 * @param Timeout Timeout for the command
	 * @param OutRequestId The requestId returned from the system
	 * @return True if the async call started successfully
	 */
	EOculusResult::Type FOculusAnchorManager::SetSpaceComponentStatus(uint64 Space, EOculusSpaceComponentType SpaceComponentType, bool Enable, float Timeout, uint64& OutRequestId)
	{
		ovrpSpaceComponentType ovrpType = ConvertToOvrpComponentType(SpaceComponentType);
		ovrpUInt64 OvrpOutRequestId = 0;

		const ovrpUInt64 OVRPSpace = Space;

		// validate existing status
		ovrpBool isEnabled = false;
		ovrpBool changePending = false;
		const ovrpResult getComponentStatusResult = FOculusHMDModule::GetPluginWrapper().GetSpaceComponentStatus(&OVRPSpace, ovrpType, &isEnabled, &changePending);

		bool isStatusChangingOrSame = (static_cast<bool>(isEnabled) == Enable && !changePending) || (static_cast<bool>(isEnabled) != Enable && changePending);
		if (OVRP_SUCCESS(getComponentStatusResult) && isStatusChangingOrSame)
		{
			return EOculusResult::Success;
		}

		// set status
		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().SetSpaceComponentStatus(
			&OVRPSpace,
			ovrpType,
			Enable,
			Timeout,
			&OvrpOutRequestId);

		memcpy(&OutRequestId, &OvrpOutRequestId, sizeof(uint64));

		UE_LOG(LogOculusAnchors, Log, TEXT("SetSpaceComponentStatus  Request ID: %llu"), OutRequestId);

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief 
	 * @param Space The space handle for the space to determine the component status on
	 * @param SpaceComponentType The space component type
	 * @param OutEnabled true if the component is enabled 
	 * @param OutChangePending true if there are pending changes
	 * @return True if the async call started successfully
	 */
	EOculusResult::Type FOculusAnchorManager::GetSpaceComponentStatus(uint64 Space, EOculusSpaceComponentType SpaceComponentType, bool& OutEnabled, bool& OutChangePending)
	{
		const ovrpUInt64 OVRPSpace = Space;
		ovrpBool OutOvrpEnabled = ovrpBool_False;;
		ovrpBool OutOvrpChangePending = ovrpBool_False;
		
		ovrpSpaceComponentType ovrpType = ConvertToOvrpComponentType(SpaceComponentType);

		const bool Result = FOculusHMDModule::GetPluginWrapper().GetInitialized() && OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().GetSpaceComponentStatus(
			&OVRPSpace,
			ovrpType,
			&OutOvrpEnabled,
			&OutOvrpChangePending
			));

		OutEnabled = (OutOvrpEnabled == ovrpBool_True);
		OutChangePending = (OutOvrpChangePending == ovrpBool_True);
	
		return static_cast<EOculusResult::Type>(Result);
	}
	EOculusResult::Type FOculusAnchorManager::GetSupportedAnchorComponents(uint64 Handle, TArray<EOculusSpaceComponentType>& OutSupportedTypes)
	{
		if (!FOculusHMDModule::GetPluginWrapper().GetInitialized())
		{
			return EOculusResult::Failure;
		}

		ovrpSpace ovrSpace = Handle;
		TArray<ovrpSpaceComponentType> ovrComponentTypes;
		ovrpUInt32 input = 0;
		ovrpUInt32 output = 0;

		ovrpResult enumerateResult = FOculusHMDModule::GetPluginWrapper().EnumerateSpaceSupportedComponents(&ovrSpace, input, &output, nullptr);
		if (!OVRP_SUCCESS(enumerateResult))
		{
			return static_cast<EOculusResult::Type>(enumerateResult);
		}

		input = output;
		ovrComponentTypes.SetNumZeroed(output);

		enumerateResult = FOculusHMDModule::GetPluginWrapper().EnumerateSpaceSupportedComponents(&ovrSpace, input, &output, ovrComponentTypes.GetData());
		if (!OVRP_SUCCESS(enumerateResult))
		{
			return static_cast<EOculusResult::Type>(enumerateResult);
		}

		OutSupportedTypes.SetNumZeroed(ovrComponentTypes.Num());
		for (int i = 0; i < ovrComponentTypes.Num(); ++i)
		{
			OutSupportedTypes[i] = ConvertToUe4ComponentType(ovrComponentTypes[i]);
		}

		return static_cast<EOculusResult::Type>(enumerateResult);
	}
	/**
	 * @brief Saves a space to the given storage location
	 * @param Space The space handle
	 * @param StorageLocation Which location to store the anchor
	 * @param StoragePersistenceMode Storage persistence, not valid for cloud storage.
	 * @param OutRequestId The requestId returned by the system
	 * @return True if the async call started successfully
	 */
	EOculusResult::Type FOculusAnchorManager::SaveAnchor(uint64 Space,
		EOculusSpaceStorageLocation StorageLocation,
		EOculusSpaceStoragePersistenceMode StoragePersistenceMode, uint64& OutRequestId)
	{
		ovrpSpaceStorageLocation OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
		switch(StorageLocation)
		{
		case EOculusSpaceStorageLocation::Invalid:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusSpaceStorageLocation::Local:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
			break;
		case EOculusSpaceStorageLocation::Cloud:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Cloud;
			break;
		default: 
			break;
		}

		ovrpSpaceStoragePersistenceMode OvrpStoragePersistenceMode = ovrpSpaceStoragePersistenceMode_Invalid;
		switch(StoragePersistenceMode)
		{
		case EOculusSpaceStoragePersistenceMode::Invalid:
			OvrpStoragePersistenceMode = ovrpSpaceStoragePersistenceMode_Invalid;
			break;
		case EOculusSpaceStoragePersistenceMode::Indefinite:
			OvrpStoragePersistenceMode = ovrpSpaceStoragePersistenceMode_Indefinite;
			break;
		default: 
			break;
		}
		
		ovrpUInt64 OvrpOutRequestId = 0;
		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().SaveSpace(&Space, OvrpStorageLocation, OvrpStoragePersistenceMode, &OvrpOutRequestId);
		
		UE_LOG(LogOculusAnchors, Log, TEXT("Saving space with: SpaceID: %llu  --  Location: %d  --  Persistence: %d  --  OutID: %llu"), Space, OvrpStorageLocation, OvrpStoragePersistenceMode, OvrpOutRequestId);

		memcpy(&OutRequestId,&OvrpOutRequestId,sizeof(uint64));

		if (!OVRP_SUCCESS(Result))
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("FOculusHMD::SaveAnchor failed with: SpaceID: %llu  --  Location: %d  --  Persistence: %d"), Space, OvrpStorageLocation, OvrpStoragePersistenceMode);
		}

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief Saves a list of spaces to the specified storage location
	 * @param Spaces The array of spaces to save
	 * @param StorageLocation The location 
	 * @param OutRequestId The requestId returned by the OVR call.
	 * @return True if the async call started successfully
	 */
	EOculusResult::Type FOculusAnchorManager::SaveAnchorList(const TArray<uint64>& Spaces, EOculusSpaceStorageLocation StorageLocation, uint64& OutRequestId)
	{
		ovrpSpaceStorageLocation OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
		switch (StorageLocation)
		{
		case EOculusSpaceStorageLocation::Invalid:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusSpaceStorageLocation::Local:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
			break;
		case EOculusSpaceStorageLocation::Cloud:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Cloud;
			break;
		default:
			break;
		}

		ovrpUInt64 OvrpOutRequestId = 0;
		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().SaveSpaceList(Spaces.GetData(), Spaces.Num(), OvrpStorageLocation, &OvrpOutRequestId);

		UE_LOG(LogOculusAnchors, Log, TEXT("Saving space list: Location: %d --  OutID: %llu"), OvrpStorageLocation, OvrpOutRequestId);
		for (auto& it : Spaces)
		{
			UE_LOG(LogOculusAnchors, Log, TEXT("\tSpaceID: %llu"), it);
		}

		memcpy(&OutRequestId, &OvrpOutRequestId, sizeof(uint64));

		if (!OVRP_SUCCESS(Result))
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("SaveSpaceList failed -- Result: %d"), Result);
		}

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief Erases a space from the given storage location
	 * @param Handle The handle of the space to erase
	 * @param StorageLocation Where the space to erase is stored
	 * @param OutRequestId The requestId returned by the system
	 * @return True if the async call started successfully
	 */
	EOculusResult::Type FOculusAnchorManager::EraseAnchor(uint64 AnchorHandle,
		EOculusSpaceStorageLocation StorageLocation,uint64 &OutRequestId)
	{
		ovrpSpaceStorageLocation ovrpStorageLocation = ovrpSpaceStorageLocation_Local;
		switch(StorageLocation)
		{
		case EOculusSpaceStorageLocation::Invalid:
			ovrpStorageLocation = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusSpaceStorageLocation::Local:
			ovrpStorageLocation = ovrpSpaceStorageLocation_Local;
			break;
		default: ;
		}
		
		ovrpUInt64 OvrpOutRequestId = 0;
		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().EraseSpace(&AnchorHandle, ovrpStorageLocation, &OvrpOutRequestId);
		memcpy(&OutRequestId, &OvrpOutRequestId, sizeof(uint64));

		UE_LOG(LogOculusAnchors, Log, TEXT("Erasing anchor -- Handle: %llu  --  Location: %d  --  OutID: %llu"), AnchorHandle, ovrpStorageLocation, OvrpOutRequestId);

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief Queries for spaces given a set of query parameters
	 * @param QueryInfo The query parameters.
	 * @param OutRequestId An async work tracking ID.
	 * @return True if the async call started successfully
	 */
	EOculusResult::Type FOculusAnchorManager::QuerySpaces(const FOculusSpaceQueryInfo& QueryInfo, uint64& OutRequestId)
	{
		ovrpUInt64 OvrpOutRequestId = 0;
		ovrpResult QuerySpacesResult = ovrpFailure;
		ovrpSpaceQueryInfo ovrQueryInfo = ConvertToOVRPSpaceQueryInfo(QueryInfo);
		QuerySpacesResult = FOculusHMDModule::GetPluginWrapper().QuerySpaces(&ovrQueryInfo, &OvrpOutRequestId);
		
		memcpy(&OutRequestId, &OvrpOutRequestId, sizeof(uint64));

		UE_LOG(LogOculusAnchors, Log, TEXT("Query Spaces\n ovrpSpaceQueryInfo:\n\tQueryType: %d\n\tMaxQuerySpaces: %d\n\tTimeout: %f\n\tLocation: %d\n\tActionType: %d\n\tFilterType: %d\n\n\tRequest ID: %llu"), 
			ovrQueryInfo.queryType, ovrQueryInfo.maxQuerySpaces, (float)ovrQueryInfo.timeout, ovrQueryInfo.location, ovrQueryInfo.actionType, ovrQueryInfo.filterType, OutRequestId);

		if (QueryInfo.FilterType == EOculusSpaceQueryFilterType::FilterByIds)
		{
			UE_LOG(LogOculusAnchors, Log, TEXT("Query contains %d UUIDs"), QueryInfo.IDFilter.Num());
			for (auto& it : QueryInfo.IDFilter)
			{
				UE_LOG(LogOculusAnchors, Log, TEXT("UUID: %s"), *it.ToString());
			}
		}
		else if (QueryInfo.FilterType == EOculusSpaceQueryFilterType::FilterByComponentType)
		{
			UE_LOG(LogOculusAnchors, Log, TEXT("Query contains %d Component Types"), QueryInfo.ComponentFilter.Num());
			for (auto& it : QueryInfo.ComponentFilter)
			{
				UE_LOG(LogOculusAnchors, Log, TEXT("ComponentType: %s"), *UEnum::GetValueAsString(it));
			}
		}

		return static_cast<EOculusResult::Type>(QuerySpacesResult);
	}

	/**
	 * @brief Shares the provided spaces with the provided users list.
	 * @param Spaces The set of spaces to share.
	 * @param PrincipalIDs The Oculus user IDs of the users that will have spaces shared with them.
	 * @param OutRequestId An async work tracking ID.
	 * @return True if the async call started successfully.
	 */
	EOculusResult::Type FOculusAnchorManager::ShareSpaces(const TArray<uint64>& Spaces, const TArray<uint64>& UserIds, uint64& OutRequestId)
	{
		TArray<const char*> stringStorage;
		TArray<ovrpUser> OvrpUsers;
		for (const auto& UserId : UserIds)
		{
			ovrpUser OvrUser;
			ovrpResult Result = FOculusHMDModule::GetPluginWrapper().CreateSpaceUser(&UserId, &OvrUser);
			if (!OVRP_SUCCESS(Result))
			{
				UE_LOG(LogOculusAnchors, Log, TEXT("Failed to create space user from ID  -  string: %s, uint64: %llu"), UserId);
				continue;
			}

			OvrpUsers.Add(OvrUser);
		}

		ovrpUInt64 OvrpOutRequestId = 0;
		const ovrpResult ShareSpacesResult = FOculusHMDModule::GetPluginWrapper().ShareSpaces(Spaces.GetData(), Spaces.Num(), OvrpUsers.GetData(), OvrpUsers.Num(), &OvrpOutRequestId);

		UE_LOG(LogOculusAnchors, Log, TEXT("Sharing space list  --  OutID: %llu"), OvrpOutRequestId);
		for (auto& User : OvrpUsers)
		{
			UE_LOG(LogOculusAnchors, Log, TEXT("\tOvrpUser: %llu"), User);
			ovrpResult Result = FOculusHMDModule::GetPluginWrapper().DestroySpaceUser(&User);
			if (!OVRP_SUCCESS(Result))
			{
				UE_LOG(LogOculusAnchors, Log, TEXT("Failed to destroy space user: %llu"), User);
				continue;
			}
		}

		for (auto& it : Spaces)
		{
			UE_LOG(LogOculusAnchors, Log, TEXT("\tSpaceID: %llu"), it);
		}

		memcpy(&OutRequestId, &OvrpOutRequestId, sizeof(uint64));

		return static_cast<EOculusResult::Type>(ShareSpacesResult);
	}
	EOculusResult::Type FOculusAnchorManager::GetSpaceContainerUUIDs(uint64 Space, TArray<FUUID>& OutUUIDs)
	{
		TArray<ovrpUuid> ovrUuidArray;

		// Get the number of elements in the container
		ovrpSpaceContainer ovrSpaceContainer;
		ovrSpaceContainer.uuidCapacityInput = 0;
		ovrSpaceContainer.uuidCountOutput = 0;
		ovrSpaceContainer.uuids = nullptr;
		ovrpResult result = FOculusHMDModule::GetPluginWrapper().GetSpaceContainer(&Space, &ovrSpaceContainer);
		if (OVRP_FAILURE(result))
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to get space container %d"), result);
			return static_cast<EOculusResult::Type>(result);
		}

		// Retrieve the actual array of UUIDs
		ovrUuidArray.SetNum(ovrSpaceContainer.uuidCountOutput);
		ovrSpaceContainer.uuidCapacityInput = ovrSpaceContainer.uuidCountOutput;
		ovrSpaceContainer.uuids = ovrUuidArray.GetData();

		result = FOculusHMDModule::GetPluginWrapper().GetSpaceContainer(&Space, &ovrSpaceContainer);
		if (OVRP_FAILURE(result))
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to get space container %d"), result);
			return static_cast<EOculusResult::Type>(result);
		}

		// Write out the remaining UUIDs
		OutUUIDs.Reserve(ovrUuidArray.Num());
		for (auto& it : ovrUuidArray)
		{
			OutUUIDs.Add(FUUID(it.data));
		}

		return EOculusResult::Success;
	}
	/**
	 * @brief Gets bounds of a specific space
	 * @param Handle The handle of the space
	 * @param OutPos Position of the rectangle (in local frame of reference).  Since it's 2D, X member will be 0.f.
	 * @param OutSize Size of the rectangle (in local frame of reference).  Since it's 2D, X member will be 0.f.
	 * @return True if successfully retrieved space scene plane
	 */
	EOculusResult::Type FOculusAnchorManager::GetSpaceScenePlane(uint64 Space, FVector& OutPos, FVector& OutSize)
	{
		OutPos.X = OutPos.Y = OutPos.Z = 0.f;
		OutSize.X = OutSize.Y = OutSize.Z = 0.f;

		ovrpRectf rect;
		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().GetSpaceBoundingBox2D(&Space, &rect);

		if (OVRP_SUCCESS(Result))
		{
			// Convert to UE4's coordinates system
			OutPos.Y = rect.Pos.x;
			OutPos.Z = rect.Pos.y;
			OutSize.Y = rect.Size.w;
			OutSize.Z = rect.Size.h;
		}

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief Gets bounds of a specific space
	 * @param Handle The handle of the space
	 * @param OutPos Position of the rectangle (in local frame of reference)
	 * @param OutSize Size of the rectangle (in local frame of reference)
	 * @return True if successfully retrieved space scene volume
	 */
	EOculusResult::Type FOculusAnchorManager::GetSpaceSceneVolume(uint64 Space, FVector& OutPos, FVector& OutSize)
	{
		OutPos.X = OutPos.Y = OutPos.Z = 0.f;
		OutSize.X = OutSize.Y = OutSize.Z = 0.f;

		ovrpBoundsf bounds;
		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().GetSpaceBoundingBox3D(&Space, &bounds);

		if (OVRP_SUCCESS(Result))
		{
			// Convert to UE4's coordinates system
			OutPos.X = bounds.Pos.z;
			OutPos.Y = bounds.Pos.x;
			OutPos.Z = bounds.Pos.y;

			OutPos.X -= bounds.Size.d;

			OutSize.X = bounds.Size.d;
			OutSize.Y = bounds.Size.w;
			OutSize.Z = bounds.Size.h;
		}

		return static_cast<EOculusResult::Type>(Result);
	}

	/**
	 * @brief Gets semantic classification associated with a specific space
	 * @param Handle The handle of the space
	 * @param OutSemanticClassifications Array of the semantic classification associated with Space	 
	 * @return returns true if successfully retrieved the semantic classification
	 */
	EOculusResult::Type FOculusAnchorManager::GetSpaceSemanticClassification(uint64 Space, TArray<FString>& OutSemanticClassifications)
	{
		OutSemanticClassifications.Empty();

		const int32 maxByteSize = 1024;
		char labelsChars[maxByteSize];

		ovrpSemanticLabels labels;
		labels.byteCapacityInput = maxByteSize;
		labels.labels = labelsChars;

		const ovrpResult Result = FOculusHMDModule::GetPluginWrapper().GetSpaceSemanticLabels(&Space, &labels);

		if (OVRP_SUCCESS(Result))
		{
			FString labelsStr(labels.byteCountOutput, labels.labels);
			labelsStr.ParseIntoArray(OutSemanticClassifications, TEXT(",")); \
		}
		
		return static_cast<EOculusResult::Type>(Result);
	}
	EOculusResult::Type FOculusAnchorManager::GetSpaceContainer(uint64 Space, TArray<FUUID>& OutContainerUuids)
	{
		OutContainerUuids.Empty();

		ovrpSpaceContainer container;

		ovrpResult Result = FOculusHMDModule::GetPluginWrapper().GetSpaceContainer(&Space, &container);

		if (OVRP_SUCCESS(Result))
		{
			TArray<ovrpUuid> uuids;
			size_t size = container.uuidCountOutput;
			uuids.InsertZeroed(0, size);
			container.uuidCapacityInput = size;
			container.uuids = uuids.GetData();
			Result = FOculusHMDModule::GetPluginWrapper().GetSpaceContainer(&Space, &container);
			if (OVRP_SUCCESS(Result))
			{
				OutContainerUuids.InsertZeroed(0, size);
				for (size_t i = 0; i < size; i++)
				{
					OutContainerUuids[i] = FUUID(uuids[i].data);
				}
			}
		}

		return static_cast<EOculusResult::Type>(Result);
	}

	EOculusResult::Type FOculusAnchorManager::GetSpaceBoundary2D(uint64 Space, TArray<FVector2D>& OutVertices)
	{
		TArray<ovrpVector2f> vertices;

		// Get the number of elements in the container
		ovrpBoundary2D boundary;
		boundary.vertexCapacityInput = 0;
		boundary.vertexCountOutput = 0;
		boundary.vertices = nullptr;

		ovrpResult result = FOculusHMDModule::GetPluginWrapper().GetSpaceBoundary2D(&Space, &boundary);
		if (OVRP_FAILURE(result))
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to get space boundary 2d %d"), result);
			return static_cast<EOculusResult::Type>(result);
		}

		// Retrieve the actual array of vertices
		vertices.SetNum(boundary.vertexCountOutput);
		boundary.vertexCapacityInput = boundary.vertexCountOutput;
		boundary.vertices = vertices.GetData();

		result = FOculusHMDModule::GetPluginWrapper().GetSpaceBoundary2D(&Space, &boundary);
		if (OVRP_FAILURE(result))
		{
			UE_LOG(LogOculusAnchors, Warning, TEXT("Failed to get space boundary 2d %d"), result);
			return static_cast<EOculusResult::Type>(result);
		}

		// Write out the vertices
		OutVertices.Reserve(vertices.Num());
		for (const auto& it : vertices)
		{
			OutVertices.Add(FVector2D(it.x, it.y));
		}

		return EOculusResult::Success;
	}
}

