/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "MRUtilityKitSubsystem.h"
#include "MRUtilityKitAnchor.h"
#include "MRUtilityKitTelemetry.h"
#include "OculusAnchors.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/Pawn.h"

AMRUKAnchor* UMRUKSubsystem::Raycast(const FVector& Origin, const FVector& Direction, float MaxDist, const FMRUKLabelFilter& LabelFilter, FMRUKHit& OutHit)
{
	AMRUKAnchor* HitComponent = nullptr;
	for (const auto& Room : Rooms)
	{
		FMRUKHit HitResult;
		if (!Room)
		{
			continue;
		}
		if (AMRUKAnchor* Anchor = Room->Raycast(Origin, Direction, MaxDist, LabelFilter, HitResult))
		{
			// Prevent further hits which are further away from being found
			MaxDist = HitResult.HitDistance;
			OutHit = HitResult;
			HitComponent = Anchor;
		}
	}
	return HitComponent;
}

bool UMRUKSubsystem::RaycastAll(const FVector& Origin, const FVector& Direction, float MaxDist, const FMRUKLabelFilter& LabelFilter, TArray<FMRUKHit>& OutHits, TArray<AMRUKAnchor*>& OutAnchors)
{
	bool HitAnything = false;
	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}
		if (Room->RaycastAll(Origin, Direction, MaxDist, LabelFilter, OutHits, OutAnchors))
		{
			HitAnything = true;
		}
	}
	return HitAnything;
}

void UMRUKSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UMRUKSettings* Settings = GetMutableDefault<UMRUKSettings>();
	EnableWorldLock = Settings->EnableWorldLock;
}

void UMRUKSubsystem::Deinitialize()
{
	ClearScene();
}

TSharedRef<FJsonObject> UMRUKSubsystem::JsonSerialize()
{
	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> RoomsArray;

	for (const auto& Room : Rooms)
	{
		if (Room)
		{
			RoomsArray.Add(MakeShareable(new FJsonValueObject(Room->JsonSerialize())));
		}
	}

	JsonObject->SetArrayField(TEXT("Rooms"), RoomsArray);

	return JsonObject;
}

void UMRUKSubsystem::UnregisterRoom(AMRUKRoom* Room)
{
	Rooms.Remove(Room);
}

AMRUKRoom* UMRUKSubsystem::GetCurrentRoom() const
{
	// Return the first valid room
	// TODO: Use some better heuristics to determine the current room
	// e.g. the room where the camera is currently in or closest to
	for (const auto& Room : Rooms)
	{
		if (Room)
		{
			return Room;
		}
	}
	return nullptr;
}

FString UMRUKSubsystem::SaveSceneToJsonString()
{
	FString Json;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&Json, 0);
	FJsonSerializer::Serialize(JsonSerialize(), JsonWriter);
	return Json;
}

void UMRUKSubsystem::LoadSceneFromJsonString(const FString& String)
{
	if (SceneData || SceneLoadStatus == EMRUKInitStatus::Busy)
	{
		UE_LOG(LogMRUK, Error, TEXT("Can't start loading a scene from JSON while the scene is already loading"));
		return;
	}

	SceneData = NewObject<UMRUKSceneData>(this);

	if (SceneLoadStatus == EMRUKInitStatus::Complete)
	{
		// Update the scene
		UE_LOG(LogMRUK, Log, TEXT("Update scene from JSON"));
		SceneData->OnComplete.AddDynamic(this, &UMRUKSubsystem::UpdatedSceneDataLoadedComplete);
	}
	else
	{
		UE_LOG(LogMRUK, Log, TEXT("Load scene from JSON"));
		SceneData->OnComplete.AddDynamic(this, &UMRUKSubsystem::SceneDataLoadedComplete);
	}
	SceneLoadStatus = EMRUKInitStatus::Busy;
	SceneData->LoadFromJson(String);
}

void UMRUKSubsystem::LoadSceneFromDevice(int MaxQueries)
{
	if (SceneData || SceneLoadStatus == EMRUKInitStatus::Busy)
	{
		UE_LOG(LogMRUK, Error, TEXT("Can't start loading a scene from device while the scene is already loading"));
		if (SceneData)
		{
			UE_LOG(LogMRUK, Error, TEXT("Ongoing scene data query"));
		}
		return;
	}

	SceneData = NewObject<UMRUKSceneData>(this);
	if (!Rooms.IsEmpty())
	{
		// Update the scene
		UE_LOG(LogMRUK, Log, TEXT("Update scene from device"));
		SceneData->OnComplete.AddDynamic(this, &UMRUKSubsystem::UpdatedSceneDataLoadedComplete);
	}
	else
	{
		UE_LOG(LogMRUK, Log, TEXT("Load scene from device"));
		SceneData->OnComplete.AddDynamic(this, &UMRUKSubsystem::SceneDataLoadedComplete);
	}
	SceneLoadStatus = EMRUKInitStatus::Busy;
	SceneData->LoadFromDevice(MaxQueries);
}

void UMRUKSubsystem::SceneDataLoadedComplete(bool Success)
{
	UE_LOG(LogMRUK, Log, TEXT("Loaded scene data. Sucess==%d"), Success);
	if (!SceneData)
	{
		UE_LOG(LogMRUK, Warning, TEXT("Can't process scene data if it's not loaded"));
		FinishedLoading(false);
		return;
	}
	if (SceneData->RoomsData.IsEmpty())
	{
		UE_LOG(LogMRUK, Warning, TEXT("No room data found"));
		FinishedLoading(false);
		return;
	}

	if (Success)
	{
		UE_LOG(LogMRUK, Log, TEXT("Spawn rooms from scene data"));
		for (const auto& RoomData : SceneData->RoomsData)
		{
			AMRUKRoom* Room = SpawnRoom();
			Room->LoadFromData(RoomData);
		}
	}
	FinishedLoading(Success);

	for (auto Room : Rooms)
	{
		OnRoomCreated.Broadcast(Room);
	}
}

void UMRUKSubsystem::UpdatedSceneDataLoadedComplete(bool Success)
{
	UE_LOG(LogMRUK, Log, TEXT("Loaded updated scene data from device. Sucess==%d"), Success);

	TArray<AMRUKRoom*> RoomsCreated;
	TArray<AMRUKRoom*> RoomsUpdated;

	if (Success)
	{
		UE_LOG(LogMRUK, Log, TEXT("Update found %d rooms"), SceneData->RoomsData.Num());

		TArray<AMRUKRoom*> RoomsToRemove = Rooms;
		Rooms.Empty();

		for (int i = 0; i < SceneData->RoomsData.Num(); ++i)
		{
			UMRUKRoomData* RoomData = SceneData->RoomsData[i];
			AMRUKRoom** RoomFound = RoomsToRemove.FindByPredicate([RoomData](AMRUKRoom* Room) {
				return Room->Corresponds(RoomData);
			});
			AMRUKRoom* Room = nullptr;
			if (RoomFound)
			{
				Room = *RoomFound;
				UE_LOG(LogMRUK, Log, TEXT("Update room from query"));
				Rooms.Push(Room);
				RoomsToRemove.Remove(Room);
				RoomsUpdated.Push(Room);
			}
			else
			{
				UE_LOG(LogMRUK, Log, TEXT("Spawn room from query"));
				Room = SpawnRoom();
				RoomsCreated.Push(Room);
			}
			Room->LoadFromData(RoomData);
		}

		UE_LOG(LogMRUK, Log, TEXT("Destroy %d old rooms"), RoomsToRemove.Num());
		for (auto Room : RoomsToRemove)
		{
			OnRoomRemoved.Broadcast(Room);
			Room->Destroy();
		}
	}
	FinishedLoading(Success);

	for (auto Room : RoomsUpdated)
	{
		OnRoomUpdated.Broadcast(Room);
	}
	for (auto Room : RoomsCreated)
	{
		OnRoomCreated.Broadcast(Room);
	}
}

void UMRUKSubsystem::ClearScene()
{
	if (SceneLoadStatus == EMRUKInitStatus::Busy)
	{
		UE_LOG(LogMRUK, Error, TEXT("Cannot clear scene while scene is loading"));
		return;
	}
	SceneLoadStatus = EMRUKInitStatus::None;
	for (auto Room : Rooms)
	{
		if (Room)
		{
			Room->Destroy();
		}
	}
	Rooms.Empty();
}

AMRUKAnchor* UMRUKSubsystem::TryGetClosestSurfacePosition(const FVector& WorldPosition, FVector& OutSurfacePosition, const FMRUKLabelFilter& LabelFilter, float MaxDistance)
{
	AMRUKAnchor* ClosestAnchor = nullptr;
	double ClosestSurfaceDistance = DBL_MAX;

	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}
		double SurfaceDistance = DBL_MAX;
		const auto Anchor = Room->TryGetClosestSurfacePosition(WorldPosition, OutSurfacePosition, (float&)SurfaceDistance, LabelFilter, MaxDistance);
		if (Anchor && SurfaceDistance < ClosestSurfaceDistance)
		{
			ClosestAnchor = Anchor;
			ClosestSurfaceDistance = MaxDistance;
		}
	}

	return ClosestAnchor;
}

AMRUKAnchor* UMRUKSubsystem::IsPositionInSceneVolume(const FVector& WorldPosition, bool TestVerticalBounds, float Tolerance)
{
	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}
		const auto Anchor = Room->IsPositionInSceneVolume(WorldPosition, TestVerticalBounds, Tolerance);
		if (Anchor)
		{
			return Anchor;
		}
	}
	return nullptr;
}

TArray<AActor*> UMRUKSubsystem::SpawnInterior(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, UMaterialInterface* ProceduralMaterial)
{
	return SpawnInteriorFromStream(SpawnGroups, FRandomStream(NAME_None), ProceduralMaterial);
}

TArray<AActor*> UMRUKSubsystem::SpawnInteriorFromStream(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, const FRandomStream& RandomStream, UMaterialInterface* ProceduralMaterial)
{
	TArray<AActor*> AllInteriorActors;

	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}
		auto InteriorActors = Room->SpawnInteriorFromStream(SpawnGroups, RandomStream, ProceduralMaterial);
		AllInteriorActors.Append(InteriorActors);
	}

	return AllInteriorActors;
}

bool UMRUKSubsystem::LaunchSceneCapture()
{
	bool Success = GetRoomLayoutManager()->LaunchCaptureFlow();
	if (Success)
	{
		UE_LOG(LogMRUK, Log, TEXT("Capture flow launched with success"));
	}
	else
	{
		UE_LOG(LogMRUK, Error, TEXT("Launching capture flow failed!"));
	}
	return Success;
}

void UMRUKSubsystem::SceneCaptureComplete(FUInt64 RequestId, bool bSuccess)
{
	UE_LOG(LogMRUK, Log, TEXT("Scene capture complete Success==%d"), bSuccess);
	OnCaptureComplete.Broadcast(bSuccess);
}

UOculusRoomLayoutManagerComponent* UMRUKSubsystem::GetRoomLayoutManager()
{
	if (!RoomLayoutManager)
	{
		FActorSpawnParameters Params{};
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = nullptr;
		RoomLayoutManagerActor = GetWorld()->SpawnActor<AActor>(Params);
		RoomLayoutManagerActor->SetRootComponent(NewObject<USceneComponent>(RoomLayoutManagerActor, TEXT("SceneComponent")));

		RoomLayoutManagerActor->AddComponentByClass(UOculusRoomLayoutManagerComponent::StaticClass(), false, FTransform::Identity, false);
		RoomLayoutManager = RoomLayoutManagerActor->GetComponentByClass<UOculusRoomLayoutManagerComponent>();
		RoomLayoutManager->OculusRoomLayoutSceneCaptureComplete.AddDynamic(this, &UMRUKSubsystem::SceneCaptureComplete);
	}
	return RoomLayoutManager;
}

AMRUKRoom* UMRUKSubsystem::SpawnRoom()
{
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMRUKRoom* Room = GetWorld()->SpawnActor<AMRUKRoom>(ActorSpawnParams);

#if WITH_EDITOR
	Room->SetActorLabel(TEXT("ROOM"));
#endif

	Rooms.Push(Room);

	return Room;
}

void UMRUKSubsystem::FinishedLoading(bool Success)
{
	UE_LOG(LogMRUK, Log, TEXT("Finished loading: Success==%d"), Success);
	if (SceneData)
	{
		SceneData->MarkPackageDirty();
		SceneData = nullptr;
	}

	if (Success)
	{
		SceneLoadStatus = EMRUKInitStatus::Complete;
	}
	else
	{
		SceneLoadStatus = EMRUKInitStatus::Failed;
	}
	OnSceneLoaded.Broadcast(Success);
}

FBox UMRUKSubsystem::GetActorClassBounds(TSubclassOf<AActor> Actor)
{
	auto Entry = ActorClassBoundsCache.Find(Actor);
	if (Entry)
	{
		return *Entry;
	}
	const auto TempActor = GetWorld()->SpawnActor(Actor);
	auto Bounds = TempActor->CalculateComponentsBoundingBoxInLocalSpace(true);
	TempActor->Destroy();
	ActorClassBoundsCache.Add(Actor, Bounds);
	return Bounds;
}

void UMRUKSubsystem::Tick(float DeltaTime)
{
	if (EnableWorldLock)
	{
		auto Room = GetCurrentRoom();
		if (Room)
		{
			APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
			if (PlayerController)
			{
				APawn* Pawn = PlayerController->GetPawn();
				if (Pawn)
				{
					const auto& PawnTransform = Pawn->GetActorTransform();

					FVector HeadPosition;
					FRotator Unused;

					// Get the position and rotation of the VR headset
					UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(Unused, HeadPosition);

					HeadPosition = PawnTransform.TransformPosition(HeadPosition);

					Room->UpdateWorldLock(Pawn, HeadPosition);
				}
			}
		}
	}
}

bool UMRUKSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_BeginDestroyed) && IsValid(this) && (EnableWorldLock);
}
