/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "MRUtilityKitData.h"
#include "MRUtilityKitSubsystem.h"
#include "MRUtilityKitSerializationHelpers.h"
#include "MRUtilityKitTelemetry.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AMRUKLocalizer::AMRUKLocalizer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMRUKLocalizer::Tick(float DeltaTime)
{
	for (int i = 0; i < AnchorsData.Num(); ++i)
	{
		auto Query = AnchorsData[i];
		if (UOculusAnchorBPFunctionLibrary::GetAnchorTransformByHandle(Query->SpaceQuery.Space, Query->Transform))
		{
			Query->NeedAnchorLocalization = false;
			UE_LOG(LogMRUK, Log, TEXT("Localized anchor"));
			AnchorsData.RemoveAt(i);
			--i;
		}
	}

	if (AnchorsData.IsEmpty())
	{
		UE_LOG(LogMRUK, Log, TEXT("All anchors localized"));
		OnComplete.Broadcast(true);
	}
}

void UMRUKAnchorData::LoadFromDevice(FOculusSpaceQueryResult SpaceQueryResult, int32 MaxQueries)
{
	SpaceQuery = SpaceQueryResult;

	Transform = FTransform::Identity;
	NeedAnchorLocalization = false;
	if (!UOculusAnchorBPFunctionLibrary::GetAnchorTransformByHandle(SpaceQuery.Space, Transform))
	{
		UE_LOG(LogMRUK, Error, TEXT("Failed to get anchor transform. Querying it async."));
		NeedAnchorLocalization = true;
	}

	EOculusResult::Type Result;
	if (!OculusAnchors::FOculusAnchors::GetSpaceSemanticClassification(SpaceQuery.Space.Value, SemanticClassifications, Result))
	{
		UE_LOG(LogMRUK, Error, TEXT("Failed to get semantic classification space."));
	}

	UWorld* World = GetWorld();
	const float WorldToMeters = World ? World->GetWorldSettings()->WorldToMeters : 100.0;

	FVector ScenePlanePos;
	FVector ScenePlaneSize;
	if (OculusAnchors::FOculusAnchors::GetSpaceScenePlane(SpaceQuery.Space, ScenePlanePos, ScenePlaneSize, Result))
	{
		FVector2D PlanePos = FVector2D(ScenePlanePos.Y, ScenePlanePos.Z) * WorldToMeters;
		FVector2D PlaneSize = FVector2D(ScenePlaneSize.Y, ScenePlaneSize.Z) * WorldToMeters;
		PlaneBounds = FBox2D(PlanePos, PlanePos + PlaneSize);
		TArray<FVector2D> SpaceBoundary2D;
		if (OculusAnchors::FOculusAnchors::GetSpaceBoundary2D(SpaceQuery.Space, SpaceBoundary2D, Result))
		{
			PlaneBoundary2D.Reserve(SpaceBoundary2D.Num());
			for (int i = 0; i < SpaceBoundary2D.Num(); ++i)
			{
				PlaneBoundary2D.Push(FVector2D(SpaceBoundary2D[i].X * WorldToMeters, SpaceBoundary2D[i].Y * WorldToMeters));
			}
		}
	}

	FVector SceneVolumePos;
	FVector SceneVolumeSize;
	if (OculusAnchors::FOculusAnchors::GetSpaceSceneVolume(SpaceQuery.Space, SceneVolumePos, SceneVolumeSize, Result))
	{
		FVector VolumePos = SceneVolumePos * WorldToMeters;
		FVector VolumeSize = SceneVolumeSize * WorldToMeters;
		VolumeBounds = FBox(VolumePos, VolumePos + VolumeSize);
	}
}

void UMRUKAnchorData::LoadFromJson(const FJsonValue& Value)
{
	auto Object = Value.AsObject();
	MRUKDeserialize(*Object->GetField<EJson::None>(TEXT("UUID")), SpaceQuery.UUID);
	MRUKDeserialize(*Object->GetField<EJson::None>(TEXT("SemanticClassifications")), SemanticClassifications);
	MRUKDeserialize(*Object->GetField<EJson::None>(TEXT("Transform")), Transform);
	if (auto JsonValue = Object->TryGetField(TEXT("PlaneBounds")))
	{
		MRUKDeserialize(*JsonValue, PlaneBounds);
	}
	if (auto JsonValue = Object->TryGetField(TEXT("PlaneBoundary2D")))
	{
		MRUKDeserialize(*JsonValue, PlaneBoundary2D);
	}
	if (auto JsonValue = Object->TryGetField(TEXT("VolumeBounds")))
	{
		MRUKDeserialize(*JsonValue, VolumeBounds);
	}
	NeedAnchorLocalization = false;
}

void UMRUKRoomData::LoadFromDevice(FOculusSpaceQueryResult SpaceQueryResult, int32 MaxQueries)
{
	SpaceQuery = SpaceQueryResult;

	auto Subsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMRUKSubsystem>();

	if (!Subsystem->GetRoomLayoutManager()->GetRoomLayout(SpaceQuery.Space.Value, RoomLayout))
	{
		UE_LOG(LogMRUK, Error, TEXT("Could not query room layout"));
		FinishQuery(false);
		return;
	}

	EOculusResult::Type AnchorQueryResult;
	if (!OculusAnchors::FOculusAnchors::QueryAnchors(
			RoomLayout.RoomObjectUUIDs,
			EOculusSpaceStorageLocation::Local,
			FOculusAnchorQueryDelegate::CreateUObject(this, &UMRUKRoomData::RoomQueryComplete, SpaceQuery.Space),
			AnchorQueryResult))
	{
		UE_LOG(LogMRUK, Error, TEXT("Could not query anchors"));
		FinishQuery(false);
		return;
	}
}

void UMRUKRoomData::LoadFromJson(const FJsonValue& Value)
{
	auto Object = Value.AsObject();
	MRUKDeserialize(*Object->GetField<EJson::None>(TEXT("UUID")), SpaceQuery.UUID);
	MRUKDeserialize(*Object->GetField<EJson::None>(TEXT("RoomLayout")), RoomLayout);
	auto AnchorsJson = Object->GetArrayField(TEXT("Anchors"));
	for (const auto& AnchorJson : AnchorsJson)
	{
		auto AnchorQuery = NewObject<UMRUKAnchorData>(this);
		AnchorsData.Push(AnchorQuery);
		RoomLayout.RoomObjectUUIDs.Add(AnchorQuery->SpaceQuery.UUID);
		AnchorQuery->LoadFromJson(*AnchorJson);
	}
	FinishQuery(true);
}

void UMRUKRoomData::FinishQuery(bool Success)
{
	OnComplete.Broadcast(Success);
}

void UMRUKRoomData::RoomQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults, const FUInt64 RoomSpaceID)
{
	if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(AnchorResult))
	{
		UE_LOG(LogMRUK, Error, TEXT("Querying room data failed"));
		FinishQuery(false);
		return;
	}

	TArray<UMRUKAnchorData*> AnchorQueriesLocalization;

	UE_LOG(LogMRUK, Log, TEXT("Received %d anchors from device"), QueryResults.Num());

	for (auto& AnchorQueryElement : QueryResults)
	{
		auto AnchorQuery = NewObject<UMRUKAnchorData>(this);
		AnchorQuery->LoadFromDevice(AnchorQueryElement);
		if (AnchorQuery->NeedAnchorLocalization)
		{
			AnchorQueriesLocalization.Push(AnchorQuery);
		}
		AnchorsData.Push(AnchorQuery);
	}

	if (!AnchorQueriesLocalization.IsEmpty())
	{
		UE_LOG(LogMRUK, Log, TEXT("Could not localize all anchors. Going to localize them async"));
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		LocalizationActor = GetWorld()->SpawnActor<AMRUKLocalizer>(ActorSpawnParams);
		LocalizationActor->AnchorsData = AnchorQueriesLocalization;
		LocalizationActor->OnComplete.AddDynamic(this, &UMRUKRoomData::AnchorsInitialized);
	}
	else
	{
		AnchorsInitialized(true);
	}
}

void UMRUKRoomData::AnchorsInitialized(bool Success)
{
	UE_LOG(LogMRUK, Log, TEXT("Anchors data initialized Success==%d"), Success);
	if (LocalizationActor)
	{
		LocalizationActor->Destroy();
		LocalizationActor = nullptr;
	}
	FinishQuery(Success);
}

void UMRUKSceneData::LoadFromDevice(int32 MaxQueries)
{
	NumRoomsLeftToInitialize = 0;

	FOculusSpaceQueryInfo QueryInfo;
	QueryInfo.MaxQuerySpaces = MaxQueries;
	QueryInfo.FilterType = EOculusSpaceQueryFilterType::FilterByComponentType;
	QueryInfo.ComponentFilter.Add(EOculusSpaceComponentType::RoomLayout);

	EOculusResult::Type AnchorQueryResult;
	if (!OculusAnchors::FOculusAnchors::QueryAnchorsAdvanced(QueryInfo, FOculusAnchorQueryDelegate::CreateUObject(this, &UMRUKSceneData::SceneDataLoadedComplete), AnchorQueryResult))
	{
		UE_LOG(LogMRUK, Error, TEXT("Failed to query scene"));
		FinishQuery(false);
	}
}

void UMRUKSceneData::LoadFromJson(const FString& Json)
{
	TSharedPtr<FJsonValue> Value;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(JsonReader, Value))
	{
		UE_LOG(LogMRUK, Warning, TEXT("Could not deserialize JSON scene data: %s"), *JsonReader->GetErrorMessage());
		FinishQuery(false);
		return;
	}

	auto Object = Value->AsObject();
	auto RoomsJson = Object->GetArrayField(TEXT("Rooms"));

/*#if WITH_EDITOR
	if (OculusXRTelemetry::IsActive())
	{
		MRUKTelemetry::FLoadSceneFromJsonMarker()
			.Start()
			.AddAnnotation("NumRooms", TCHAR_TO_ANSI(*FString::FromInt(RoomsJson.Num())))
			.End(RoomsJson.Num() > 0 ? OculusXRTelemetry::EAction::Success : OculusXRTelemetry::EAction::Fail);
	}
#endif*/

	if (RoomsJson.IsEmpty())
	{
		UE_LOG(LogMRUK, Warning, TEXT("Could not find Rooms in JSON"));
		FinishQuery(false);
		return;
	}
	NumRoomsLeftToInitialize = RoomsJson.Num();
	UE_LOG(LogMRUK, Log, TEXT("Found %d rooms in JSON"), NumRoomsLeftToInitialize);
	for (const auto& RoomJson : RoomsJson)
	{
		auto RoomQuery = NewObject<UMRUKRoomData>(this);
		RoomsData.Push(RoomQuery);
		RoomQuery->OnComplete.AddDynamic(this, &UMRUKSceneData::RoomQueryComplete);
		RoomQuery->LoadFromJson(*RoomJson);
	}
}

void UMRUKSceneData::FinishQuery(bool Success)
{
	if (!Success)
	{
		AnyRoomFailed = true;
	}
	--NumRoomsLeftToInitialize;
	if (NumRoomsLeftToInitialize <= 0)
	{
		OnComplete.Broadcast(!AnyRoomFailed);
	}
}

void UMRUKSceneData::SceneDataLoadedComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults)
{
	NumRoomsLeftToInitialize = QueryResults.Num();
	UE_LOG(LogMRUK, Log, TEXT("Found on %d rooms on the device"), NumRoomsLeftToInitialize);

/*#if WITH_EDITOR
	if (OculusXRTelemetry::IsActive())
	{
		MRUKTelemetry::FLoadSceneFromDeviceMarker()
			.Start()
			.AddAnnotation("NumRooms", TCHAR_TO_ANSI(*FString::FromInt(QueryResults.Num())))
			.End(QueryResults.Num() > 0 ? OculusXRTelemetry::EAction::Success : OculusXRTelemetry::EAction::Fail);
	}
#endif*/

	if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(AnchorResult) || NumRoomsLeftToInitialize == 0)
	{
		UE_LOG(LogMRUK, Error, TEXT("Scene query failed"));
		FinishQuery(false);
		return;
	}

	for (auto& QueryElement : QueryResults)
	{
		auto RoomQuery = NewObject<UMRUKRoomData>(this);
		RoomsData.Push(RoomQuery);
		RoomQuery->OnComplete.AddDynamic(this, &UMRUKSceneData::RoomQueryComplete);
		RoomQuery->LoadFromDevice(QueryElement);
	}
}

void UMRUKSceneData::RoomQueryComplete(bool Success)
{
	if (!Success)
	{
		AnyRoomFailed = true;
	}
	--NumRoomsLeftToInitialize;
	if (NumRoomsLeftToInitialize == 0)
	{
		FinishQuery(!AnyRoomFailed);
	}
}
