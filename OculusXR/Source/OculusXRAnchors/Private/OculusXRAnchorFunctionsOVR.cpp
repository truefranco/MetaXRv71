// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRAnchorFunctionsOVR.h"
#include "OculusXRHMD.h"
#include "OculusXRAnchorsModule.h"
#include "OculusXRAnchorDelegates.h"
#include "OculusXRAnchorBPFunctionLibrary.h"
#include "OculusXRAnchorTypesPrivate.h"
#include "OculusXRAnchorsUtil.h"

ovrpSpaceQueryInfo2 ToOvrpSpaceQuery(const FOculusXRSpaceQueryInfo& UEQueryInfo)
{
	static const int32 MaxIdsInFilter = 1024;
	static const int32 MaxComponentTypesInFilter = 1;

	ovrpSpaceQueryInfo2 Result = {};

	Result.queryType = ovrpSpaceQueryType_Action;
	Result.actionType = ovrpSpaceQueryActionType_Load;

	Result.maxQuerySpaces = UEQueryInfo.MaxQuerySpaces;
	Result.timeout = static_cast<double>(UEQueryInfo.Timeout);

	switch (UEQueryInfo.Location)
	{
		case EOculusXRSpaceStorageLocation::Invalid:
			Result.location = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusXRSpaceStorageLocation::Local:
			Result.location = ovrpSpaceStorageLocation_Local;
			break;
		case EOculusXRSpaceStorageLocation::Cloud:
			Result.location = ovrpSpaceStorageLocation_Cloud;
			break;
	}

	switch (UEQueryInfo.FilterType)
	{
		case EOculusXRSpaceQueryFilterType::None:
			Result.filterType = ovrpSpaceQueryFilterType_None;
			break;
		case EOculusXRSpaceQueryFilterType::FilterByIds:
			Result.filterType = ovrpSpaceQueryFilterType_Ids;
			break;
		case EOculusXRSpaceQueryFilterType::FilterByComponentType:
			Result.filterType = ovrpSpaceQueryFilterType_Components;
			break;
		case EOculusXRSpaceQueryFilterType::FilterByGroup:
			Result.filterType = ovrpSpaceQueryFilterType_GroupUuid;
			break;
	}

	Result.IdInfo.numIds = FMath::Min(MaxIdsInFilter, UEQueryInfo.IDFilter.Num());
	for (int i = 0; i < Result.IdInfo.numIds; ++i)
	{
		ovrpUuid OvrUuid;
		FMemory::Memcpy(OvrUuid.data, UEQueryInfo.IDFilter[i].UUIDBytes);
		Result.IdInfo.ids[i] = OvrUuid;
	}

	if (UEQueryInfo.ComponentFilter.Num() > 1)
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("Query info has more than one component. Using first component only."));
	}

	Result.componentsInfo.numComponents = FMath::Min(MaxComponentTypesInFilter, UEQueryInfo.ComponentFilter.Num());
	for (int i = 0; i < Result.componentsInfo.numComponents; ++i)
	{
		Result.componentsInfo.components[i] = ConvertToOvrpComponentType(UEQueryInfo.ComponentFilter[i]);
	}

	FMemory::Memcpy(Result.groupUuidInfo.groupUuid.data, UEQueryInfo.GroupUUIDFilter.UUIDBytes);

	return Result;
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::CreateAnchor(const FTransform& InTransform, uint64& OutRequestId, const FTransform& CameraTransform)
{
	OculusXRHMD::FOculusXRHMD* HMD = OculusXRHMD::FOculusXRHMD::GetOculusXRHMD();
	if (!HMD)
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("FOculusAnchorManager::CreateAnchor failed to retrieve HMD."));
		return EOculusXRAnchorResult::Failure;
	}

	ovrpTrackingOrigin TrackingOriginType;
	ovrpPosef Posef;
	double Time = 0;

	const FTransform TrackingToWorld = HMD->GetLastTrackingToWorld();

	// convert to tracking space
	const FQuat TrackingSpaceOrientation = TrackingToWorld.Inverse().TransformRotation(InTransform.Rotator().Quaternion());
	const FVector TrackingSpacePosition = TrackingToWorld.Inverse().TransformPosition(InTransform.GetLocation());

	const OculusXRHMD::FPose TrackingSpacePose(TrackingSpaceOrientation, TrackingSpacePosition);

#if WITH_EDITOR
	// Link only head space position update
	FVector OutHeadPosition;
	FQuat OutHeadOrientation;
	const bool bGetPose = HMD->GetCurrentPose(HMD->HMDDeviceId, OutHeadOrientation, OutHeadPosition);
	if (!bGetPose)
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("FOculusAnchorManager::CreateAnchor failed to get current headset pose."));
		return EOculusXRAnchorResult::Failure;
	}

	OculusXRHMD::FPose HeadPose(OutHeadOrientation, OutHeadPosition);

	OculusXRHMD::FPose MainCameraPose(CameraTransform.GetRotation(), CameraTransform.GetLocation());
	OculusXRHMD::FPose PoseInHeadSpace = MainCameraPose.Inverse() * TrackingSpacePose;

	// To world space pose
	OculusXRHMD::FPose WorldPose = HeadPose * PoseInHeadSpace;

	const bool bConverted = HMD->ConvertPose(WorldPose, Posef);
#else
	const bool bConverted = HMD->ConvertPose(TrackingSpacePose, Posef);
#endif

	if (!bConverted)
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("FOculusAnchorManager::CreateAnchor failed to convert pose."));
		return EOculusXRAnchorResult::Failure;
	}

	FOculusXRHMDModule::GetPluginWrapper().GetTrackingOriginType2(&TrackingOriginType);
	FOculusXRHMDModule::GetPluginWrapper().GetTimeInSeconds(&Time);

	const ovrpSpatialAnchorCreateInfo SpatialAnchorCreateInfo = {
		TrackingOriginType,
		Posef,
		Time
	};

	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().CreateSpatialAnchor(&SpatialAnchorCreateInfo, &OutRequestId);
	UE_LOG(LogOculusXRAnchors, Verbose, TEXT("CreateAnchor Request ID: %llu"), OutRequestId);

	if (OVRP_FAILURE(Result))
	{
		UE_LOG(LogOculusXRAnchors, Error, TEXT("FOculusAnchorManager::CreateAnchor failed. Result: %d"), Result);
	}

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::DestroyAnchor(uint64 AnchorHandle)
{
	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().DestroySpace(static_cast<ovrpSpace*>(&AnchorHandle));

	UE_LOG(LogOculusXRAnchors, Verbose, TEXT("DestroyAnchor -- ID: %llu"), AnchorHandle);

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::TryGetAnchorTransform(uint64 AnchorHandle, FTransform& OutTransform, FOculusXRAnchorLocationFlags& OutLocationFlags, EOculusXRAnchorSpace Space)
{
	OculusXRHMD::FOculusXRHMD* OutHMD = OculusXRHMD::FOculusXRHMD::GetOculusXRHMD();
	if (!OutHMD)
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("Unable to retrieve OculusXRHMD, cannot calculate anchor transform."));
		return EOculusXRAnchorResult::Failure_InvalidOperation;
	}

	ovrpTrackingOrigin ovrpOrigin = ovrpTrackingOrigin_EyeLevel;
	const bool bTrackingOriginSuccess = OVRP_SUCCESS(FOculusXRHMDModule::GetPluginWrapper().GetTrackingOriginType2(&ovrpOrigin));
	if (!bTrackingOriginSuccess)
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("Unable to get tracking origin, cannot calculate anchor transform."));
		return EOculusXRAnchorResult::Failure_InvalidOperation;
	}

	OutTransform = FTransform::Identity;
	OutLocationFlags = FOculusXRAnchorLocationFlags(0);

	const ovrpUInt64 ovrpSpace = AnchorHandle;
	ovrpSpaceLocationf ovrpSpaceLocation{};

	const ovrpResult result = FOculusXRHMDModule::GetPluginWrapper().LocateSpace2(&ovrpSpaceLocation, &ovrpSpace, ovrpOrigin);
	if (OVRP_SUCCESS(result))
	{
		OutLocationFlags = FOculusXRAnchorLocationFlags(ovrpSpaceLocation.locationFlags);
		if (OutLocationFlags.IsValid())
		{
			OculusXRHMD::FPose Pose;
			OutHMD->ConvertPose(ovrpSpaceLocation.pose, Pose);
			switch (Space)
			{
				case EOculusXRAnchorSpace::World:
				{
					const FTransform trackingToWorld = OutHMD->GetLastTrackingToWorld();
					OutTransform.SetLocation(trackingToWorld.TransformPosition(Pose.Position));
					OutTransform.SetRotation(FRotator(trackingToWorld.TransformRotation(FQuat(Pose.Orientation))).Quaternion());
				}
				break;
				case EOculusXRAnchorSpace::Tracking:
				{
					OutTransform.SetLocation(Pose.Position);
					OutTransform.SetRotation(FRotator(FQuat(Pose.Orientation)).Quaternion());
				}
				break;
			};
		}
		else
		{
			return EOculusXRAnchorResult::Failure_OperationFailed;
		}
	}

	return OculusXRAnchors::GetResultFromOVRResult(result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::SetAnchorComponentStatus(uint64 AnchorHandle, EOculusXRSpaceComponentType ComponentType, bool Enable, float Timeout, uint64& OutRequestId)
{
	ovrpSpaceComponentType ovrpType = ConvertToOvrpComponentType(ComponentType);
	const ovrpUInt64 OVRPSpace = AnchorHandle;

	// validate existing status
	ovrpBool isEnabled = false;
	ovrpBool changePending = false;
	const ovrpResult getComponentStatusResult = FOculusXRHMDModule::GetPluginWrapper().GetSpaceComponentStatus(&OVRPSpace, ovrpType, &isEnabled, &changePending);

	bool isStatusChangingOrSame = (static_cast<bool>(isEnabled) == Enable && !changePending) || (static_cast<bool>(isEnabled) != Enable && changePending);
	if (OVRP_SUCCESS(getComponentStatusResult) && isStatusChangingOrSame)
	{
		return EOculusXRAnchorResult::Success;
	}

	// set status
	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().SetSpaceComponentStatus(
		&OVRPSpace,
		ovrpType,
		Enable,
		Timeout,
		&OutRequestId);

	UE_LOG(LogOculusXRAnchors, Verbose, TEXT("SetSpaceComponentStatus  Request ID: %llu"), OutRequestId);

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::GetAnchorComponentStatus(uint64 AnchorHandle, EOculusXRSpaceComponentType ComponentType, bool& OutEnabled, bool& OutChangePending)
{
	const ovrpUInt64 OVRPSpace = AnchorHandle;
	ovrpBool OutOvrpEnabled = ovrpBool_False;
	ovrpBool OutOvrpChangePending = ovrpBool_False;

	ovrpSpaceComponentType ovrpType = ConvertToOvrpComponentType(ComponentType);

	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().GetSpaceComponentStatus(
		&OVRPSpace,
		ovrpType,
		&OutOvrpEnabled,
		&OutOvrpChangePending);

	OutEnabled = (OutOvrpEnabled == ovrpBool_True);
	OutChangePending = (OutOvrpChangePending == ovrpBool_True);

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::GetSupportedAnchorComponents(uint64 AnchorHandle, TArray<EOculusXRSpaceComponentType>& OutSupportedTypes)
{
	ovrpSpace ovrSpace = AnchorHandle;
	TArray<ovrpSpaceComponentType> ovrComponentTypes;
	ovrpUInt32 input = 0;
	ovrpUInt32 output = 0;

	ovrpResult enumerateResult = FOculusXRHMDModule::GetPluginWrapper().EnumerateSpaceSupportedComponents(&ovrSpace, input, &output, nullptr);
	if (!OVRP_SUCCESS(enumerateResult))
	{
		return OculusXRAnchors::GetResultFromOVRResult(enumerateResult);
	}

	input = output;
	ovrComponentTypes.SetNumZeroed(output);

	enumerateResult = FOculusXRHMDModule::GetPluginWrapper().EnumerateSpaceSupportedComponents(&ovrSpace, input, &output, ovrComponentTypes.GetData());
	if (!OVRP_SUCCESS(enumerateResult))
	{
		return OculusXRAnchors::GetResultFromOVRResult(enumerateResult);
	}

	OutSupportedTypes.SetNumZeroed(ovrComponentTypes.Num());
	for (int i = 0; i < ovrComponentTypes.Num(); ++i)
	{
		OutSupportedTypes[i] = ConvertToUEComponentType(ovrComponentTypes[i]);
	}

	return OculusXRAnchors::GetResultFromOVRResult(enumerateResult);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::GetAnchorContainerUUIDs(uint64 AnchorHandle, TArray<FOculusXRUUID>& OutUUIDs)
{
	TArray<ovrpUuid> ovrUuidArray;

	// Get the number of elements in the container
	ovrpSpaceContainer ovrSpaceContainer = {};
	ovrSpaceContainer.uuidCapacityInput = 0;
	ovrSpaceContainer.uuidCountOutput = 0;
	ovrSpaceContainer.uuids = nullptr;
	ovrpResult result = FOculusXRHMDModule::GetPluginWrapper().GetSpaceContainer(&AnchorHandle, &ovrSpaceContainer);
	if (OVRP_FAILURE(result))
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("Failed to get space container %d"), result);
		return OculusXRAnchors::GetResultFromOVRResult(result);
	}

	// Retrieve the actual array of UUIDs
	ovrUuidArray.SetNum(ovrSpaceContainer.uuidCountOutput);
	ovrSpaceContainer.uuidCapacityInput = ovrSpaceContainer.uuidCountOutput;
	ovrSpaceContainer.uuids = ovrUuidArray.GetData();

	result = FOculusXRHMDModule::GetPluginWrapper().GetSpaceContainer(&AnchorHandle, &ovrSpaceContainer);
	if (OVRP_FAILURE(result))
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("Failed to get space container %d"), result);
		return OculusXRAnchors::GetResultFromOVRResult(result);
	}

	// Write out the remaining UUIDs
	OutUUIDs.Reserve(ovrUuidArray.Num());
	for (auto& it : ovrUuidArray)
	{
		OutUUIDs.Add(FOculusXRUUID(it.data));
	}

	return EOculusXRAnchorResult::Success;
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::SaveAnchor(uint64 AnchorHandle,
	EOculusXRSpaceStorageLocation StorageLocation,
	EOculusXRSpaceStoragePersistenceMode StoragePersistenceMode, uint64& OutRequestId)
{
	ovrpSpaceStorageLocation OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
	switch (StorageLocation)
	{
		case EOculusXRSpaceStorageLocation::Invalid:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusXRSpaceStorageLocation::Local:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
			break;
		case EOculusXRSpaceStorageLocation::Cloud:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Cloud;
			break;
		default:
			break;
	}

	ovrpSpaceStoragePersistenceMode OvrpStoragePersistenceMode = ovrpSpaceStoragePersistenceMode_Invalid;
	switch (StoragePersistenceMode)
	{
		case EOculusXRSpaceStoragePersistenceMode::Invalid:
			OvrpStoragePersistenceMode = ovrpSpaceStoragePersistenceMode_Invalid;
			break;
		case EOculusXRSpaceStoragePersistenceMode::Indefinite:
			OvrpStoragePersistenceMode = ovrpSpaceStoragePersistenceMode_Indefinite;
			break;
		default:
			break;
	}

	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().SaveSpace(&AnchorHandle, OvrpStorageLocation, OvrpStoragePersistenceMode, &OutRequestId);

	UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Saving anchor with, ID: %llu  --  Location: %d  --  Persistence: %d  --  OutID: %llu"), AnchorHandle, OvrpStorageLocation, OvrpStoragePersistenceMode, OutRequestId);

	if (OVRP_FAILURE(Result))
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("FOculusXRHMD::SaveAnchor failed with, ID: %llu  --  Location: %d  --  Persistence: %d"), AnchorHandle, OvrpStorageLocation, OvrpStoragePersistenceMode);
	}

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::SaveAnchorList(const TArray<uint64>& AnchorHandles, EOculusXRSpaceStorageLocation StorageLocation, uint64& OutRequestId)
{
	ovrpSpaceStorageLocation OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
	switch (StorageLocation)
	{
		case EOculusXRSpaceStorageLocation::Invalid:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusXRSpaceStorageLocation::Local:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Local;
			break;
		case EOculusXRSpaceStorageLocation::Cloud:
			OvrpStorageLocation = ovrpSpaceStorageLocation_Cloud;
			break;
		default:
			break;
	}

	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().SaveSpaceList(AnchorHandles.GetData(), AnchorHandles.Num(), OvrpStorageLocation, &OutRequestId);

	UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Saving space list: Location: %d --  OutID: %llu"), OvrpStorageLocation, OutRequestId);
	for (auto& it : AnchorHandles)
	{
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("\tSpaceID: %llu"), it);
	}

	if (OVRP_FAILURE(Result))
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("SaveSpaceList failed -- Result: %d"), Result);
	}

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::SaveAnchors(const TArray<uint64>& AnchorHandles, uint64& OutRequestId)
{
	if (AnchorHandles.Num() == 0)
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("FOculusXRAnchorFunctionsOVR::SaveAnchors has empty handle array"));
	}

	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().SaveSpaces(AnchorHandles.Num(), AnchorHandles.GetData(), &OutRequestId);

	if (!OVRP_SUCCESS(Result))
	{
		UE_LOG(LogOculusXRAnchors, Error, TEXT("FOculusXRAnchorFunctionsOVR::SaveAnchors failed, result: %d"), Result);
	}

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::DiscoverAnchors(const FOculusXRSpaceDiscoveryInfo& DiscoveryInfo, uint64& OutRequestId)
{
	uint32 FiltersCount = (uint32)DiscoveryInfo.Filters.Num();
	ovrpSpaceDiscoveryInfo OvrDiscoveryInfo = {};
	OvrDiscoveryInfo.FilterCount = FiltersCount;

	UE_LOG(LogOculusXRAnchors, Display, TEXT("Staring discovery with %d filter(s)"), FiltersCount);

	TArray<const ovrpSpaceDiscoveryFilterHeader*> filters;
	filters.SetNumZeroed(FiltersCount);

	for (uint32 i = 0; i < FiltersCount; ++i)
	{
		ensure(DiscoveryInfo.Filters[i] != nullptr);
		filters[i] = DiscoveryInfo.Filters[i]->GenerateOVRPFilter();
	}

	OvrDiscoveryInfo.Filters = filters.GetData();
	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().DiscoverSpaces(&OvrDiscoveryInfo, &OutRequestId);

	if (!OVRP_SUCCESS(Result))
	{
		UE_LOG(LogOculusXRAnchors, Error, TEXT("FOculusXRAnchorFunctionsOVR::DiscoverAnchors failed -- Result: %d"), Result);
	}

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::QueryAnchors(const FOculusXRSpaceQueryInfo& QueryInfo, uint64& OutRequestId)
{
	ovrpResult QuerySpacesResult = ovrpFailure;

	ovrpSpaceQueryInfo2 ovrQueryInfo = ToOvrpSpaceQuery(QueryInfo);
	QuerySpacesResult = FOculusXRHMDModule::GetPluginWrapper().QuerySpaces2(&ovrQueryInfo, &OutRequestId);

	UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Query Spaces\n ovrpSpaceQueryInfo:\n\tQueryType: %d\n\tMaxQuerySpaces: %d\n\tTimeout: %f\n\tLocation: %d\n\tActionType: %d\n\tFilterType: %d\n\n\tRequest ID: %llu"),
		ovrQueryInfo.queryType, ovrQueryInfo.maxQuerySpaces, (float)ovrQueryInfo.timeout, ovrQueryInfo.location, ovrQueryInfo.actionType, ovrQueryInfo.filterType, OutRequestId);

	if (QueryInfo.FilterType == EOculusXRSpaceQueryFilterType::FilterByIds)
	{
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Query contains %d UUIDs"), QueryInfo.IDFilter.Num());
		for (auto& it : QueryInfo.IDFilter)
		{
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("UUID: %s"), *it.ToString());
		}
	}
	else if (QueryInfo.FilterType == EOculusXRSpaceQueryFilterType::FilterByComponentType)
	{
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Query contains %d Component Types"), QueryInfo.ComponentFilter.Num());
		for (auto& it : QueryInfo.ComponentFilter)
		{
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("ComponentType: %s"), *UEnum::GetValueAsString(it));
		}
	}
	else if (QueryInfo.FilterType == EOculusXRSpaceQueryFilterType::FilterByGroup)
	{
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Query contains group filter  -  UUID: %s"), *QueryInfo.GroupUUIDFilter.ToString());
	}

	return OculusXRAnchors::GetResultFromOVRResult(QuerySpacesResult);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::ShareAnchors(const TArray<uint64>& AchorHandles, const TArray<uint64>& UserIds, uint64& OutRequestId)
{
	TArray<const char*> stringStorage;
	TArray<ovrpUser> OvrpUsers;
	for (const auto& UserId : UserIds)
	{
		ovrpUser OvrUser;
		ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().CreateSpaceUser(&UserId, &OvrUser);
		if (OVRP_FAILURE(Result))
		{
			UE_LOG(LogOculusXRAnchors, Warning, TEXT("Failed to create space user from ID  -  %llu"), UserId);
			continue;
		}

		OvrpUsers.Add(OvrUser);
	}

	const ovrpResult ShareSpacesResult = FOculusXRHMDModule::GetPluginWrapper().ShareSpaces(AchorHandles.GetData(), AchorHandles.Num(), OvrpUsers.GetData(), OvrpUsers.Num(), &OutRequestId);

	UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Sharing space list  --  OutID: %llu"), OutRequestId);
	for (auto& User : OvrpUsers)
	{
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("\tOvrpUser: %llu"), User);
		ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().DestroySpaceUser(&User);
		if (OVRP_FAILURE(Result))
		{
			UE_LOG(LogOculusXRAnchors, Verbose, TEXT("Failed to destroy space user: %llu"), User);
			continue;
		}
	}

	for (auto& it : AchorHandles)
	{
		UE_LOG(LogOculusXRAnchors, Verbose, TEXT("\tSpaceID: %llu"), it);
	}

	return OculusXRAnchors::GetResultFromOVRResult(ShareSpacesResult);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::ShareAnchors(const TArray<uint64>& AnchorHandles, const TArray<FOculusXRUUID>& Groups, uint64& OutRequestId)
{
	TArray<ovrpUuid> groupUuids;
	groupUuids.Reserve(Groups.Num());
	for (auto& it : Groups)
	{
		if (it == FOculusXRUUID::Zero)
		{
			UE_LOG(LogOculusXRAnchors, Error, TEXT("Anchor sharing failed. One or more group targets provided had a UUID value of zero."));
			return EOculusXRAnchorResult::Failure_InvalidParameter;
		}

		ovrpUuid uuid;
		FMemory::Memcpy(uuid.data, it.UUIDBytes);
		groupUuids.Add(uuid);
	}

	TSharedPtr<ovrpShareSpacesGroupRecipientInfo> groupRecipientInfo = MakeShared<ovrpShareSpacesGroupRecipientInfo>();
	groupRecipientInfo->GroupCount = groupUuids.Num();
	groupRecipientInfo->GroupUuids = groupUuids.GetData();

	ovrpShareSpacesInfo shareInfo;
	shareInfo.SpaceCount = AnchorHandles.Num();
	shareInfo.Spaces = (ovrpSpace*)AnchorHandles.GetData();
	shareInfo.RecipientType = ovrpShareSpacesRecipientType_Group;
	shareInfo.RecipientInfo = groupRecipientInfo.Get();

	ovrpResult result = FOculusXRHMDModule::GetPluginWrapper().ShareSpaces2(&shareInfo, &OutRequestId);

	return OculusXRAnchors::GetResultFromOVRResult(result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::EraseAnchor(uint64 AnchorHandle,
	EOculusXRSpaceStorageLocation StorageLocation, uint64& OutRequestId)
{
	ovrpSpaceStorageLocation ovrpStorageLocation = ovrpSpaceStorageLocation_Local;
	switch (StorageLocation)
	{
		case EOculusXRSpaceStorageLocation::Invalid:
			ovrpStorageLocation = ovrpSpaceStorageLocation_Invalid;
			break;
		case EOculusXRSpaceStorageLocation::Local:
			ovrpStorageLocation = ovrpSpaceStorageLocation_Local;
			break;
		case EOculusXRSpaceStorageLocation::Cloud:
			ovrpStorageLocation = ovrpSpaceStorageLocation_Cloud;
			break;
		default:;
	}

	ovrpUInt64 OvrpOutRequestId = 0;
	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().EraseSpace(&AnchorHandle, ovrpStorageLocation, &OvrpOutRequestId);
	memcpy(&OutRequestId, &OvrpOutRequestId, sizeof(uint64));

	UE_LOG(LogOculusXRAnchors, Log, TEXT("Erasing anchor -- Handle: %llu  --  Location: %d  --  OutID: %llu"), AnchorHandle, ovrpStorageLocation, OvrpOutRequestId);

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}

EOculusXRAnchorResult::Type FOculusXRAnchorFunctionsOVR::EraseAnchors(const TArray<FOculusXRUInt64>& AnchorHandles, const TArray<FOculusXRUUID>& UUIDs, uint64& OutRequestId)
{
	if (AnchorHandles.IsEmpty() && UUIDs.IsEmpty())
	{
		UE_LOG(LogOculusXRAnchors, Warning, TEXT("FOculusXRAnchorFunctionsOVR::EraseAnchors - You cannot have an empty handle and uuid array. At least one array must have elements."));
		return EOculusXRAnchorResult::Failure_InvalidParameter;
	}

	TArray<ovrpSpace> ovrpHandles;
	for (auto& handle : AnchorHandles)
	{
		ovrpHandles.Add(handle.GetValue());
	}

	TArray<ovrpUuid> ovrpUUIDs;
	for (auto& id : UUIDs)
	{
		ovrpUuid OvrUuid;
		FMemory::Memcpy(OvrUuid.data, id.UUIDBytes);
		ovrpUUIDs.Add(OvrUuid);
	}

	const ovrpResult Result = FOculusXRHMDModule::GetPluginWrapper().EraseSpaces(ovrpHandles.Num(), ovrpHandles.GetData(), ovrpUUIDs.Num(), ovrpUUIDs.GetData(), &OutRequestId);

	if (!OVRP_SUCCESS(Result))
	{
		UE_LOG(LogOculusXRAnchors, Error, TEXT("FOculusXRAnchorFunctionsOVR::EraseAnchors failed -- Result: %d"), Result);
	}

	return OculusXRAnchors::GetResultFromOVRResult(Result);
}
