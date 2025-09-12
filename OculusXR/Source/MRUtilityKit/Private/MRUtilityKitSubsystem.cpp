// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "MRUtilityKitSubsystem.h"
#include "MRUtilityKitAnchor.h"
#include "Kismet/GameplayStatics.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MRUtilityKitPositionGenerator.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/Pawn.h"
#include "OculusXRRoomLayoutManagerComponent.h"
#include "OculusXRSceneEventDelegates.h"
#include "OculusXRSceneFunctionLibrary.h"
#include "Engine/Engine.h"
#include "MRUtilityKitOpenXrExtensionPlugin.h"
#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR
#include "OculusXRFunctionLibrary.h"
#include "Generated/MRUtilityKitShared.h"
#include "XRTrackingSystemBase.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "MRUtilityKitSharedHelper.h"
#include "MRUtilityKitTelemetry.h"
#include "OculusXRRoomLayoutManagerComponent.h"

AMRUKRoom* FindRoomByUuid(UMRUKSubsystem* Subsystem, const FOculusXRUUID& RoomUuid)
{
	for (int i = 0; i < Subsystem->Rooms.Num(); ++i)
	{
		if (RoomUuid == Subsystem->Rooms[i]->AnchorUUID)
		{
			return Subsystem->Rooms[i];
		}
	}
	return nullptr;
}

static void MrukOnPreRoomAnchorAdded(const MRUKShared::MrukRoomAnchor* RoomAnchor, void* UserContext)
{
	auto* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	AMRUKRoom* Room = Subsystem->SpawnRoom();
	Room->AnchorUUID = ToOculusXR(RoomAnchor->uuid);
	Room->SpaceHandle = RoomAnchor->space;
}

static void MrukOnRoomAnchorAdded(const MRUKShared::MrukRoomAnchor* RoomAnchor, void* UserContext)
{
	auto* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	AMRUKRoom* Room = FindRoomByUuid(Subsystem, ToOculusXR(RoomAnchor->uuid));
	// Room has been added before in MrukOnPreRoomAnchorAdded()
	check(Room);
	Room->InitializeRoom();
	Subsystem->OnRoomCreated.Broadcast(Room);
}

static void MrukOnRoomAnchorUpdated(const MRUKShared::MrukRoomAnchor* RoomAnchor, const MRUKShared::MrukUuid* OldRoomAnchorUuid, bool SignificantChange, void* UserContext)
{
	UMRUKSubsystem* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	AMRUKRoom* Room = FindRoomByUuid(Subsystem, ToOculusXR(*OldRoomAnchorUuid));
	check(Room);
	Room->AnchorUUID = ToOculusXR(RoomAnchor->uuid);
	Room->SpaceHandle = RoomAnchor->space;
	if (SignificantChange)
	{
		Room->InitializeRoom();
		Subsystem->OnRoomUpdated.Broadcast(Room);
	}
}

static void MrukOnRoomAnchorRemoved(const MRUKShared::MrukRoomAnchor* RoomAnchor, void* UserContext)
{
	UMRUKSubsystem* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	AMRUKRoom* Room = FindRoomByUuid(Subsystem, ToOculusXR(RoomAnchor->uuid));
	Subsystem->OnRoomRemoved.Broadcast(Room);
	Subsystem->Rooms.Remove(Room);
	Room->Destroy();
}

static void UpdateAnchorProperties(const MRUKShared::MrukSceneAnchor* SceneAnchor, AMRUKRoom* Room, AMRUKAnchor* Anchor)
{
	FBox2D Plane;
	TArray<FVector2D> PlaneBoundary;
	if (SceneAnchor->hasPlane)
	{
		Plane = ToOculusXR(Room->GetWorld(), SceneAnchor->plane);
		PlaneBoundary = ToOculusXR(Room->GetWorld(), SceneAnchor->planeBoundary, SceneAnchor->planeBoundaryCount);
	}
	else
	{
		Plane.bIsValid = false;
	}

	FBox3d Volume;
	if (SceneAnchor->hasVolume)
	{
		Volume = ToOculusXR(Room->GetWorld(), SceneAnchor->volume);
	}
	else
	{
		Volume.IsValid = false;
	}

	TArray<FVector> GlobalMeshPositions;
	TArray<int> GlobalMeshIndices;
	if (SceneAnchor->globalMeshPositionsCount > 0 && SceneAnchor->globalMeshIndicesCount > 0)
	{
		GlobalMeshIndices.SetNum(SceneAnchor->globalMeshIndicesCount);
		memcpy(GlobalMeshIndices.GetData(), SceneAnchor->globalMeshIndices, GlobalMeshIndices.Num() * sizeof(int));

		GlobalMeshPositions.SetNum(SceneAnchor->globalMeshPositionsCount);
		for (uint32 i = 0; i < SceneAnchor->globalMeshPositionsCount; ++i)
		{
			const FVector3f& P = SceneAnchor->globalMeshPositions[i];
			GlobalMeshPositions[i] = FVector(-P.Z, P.X, P.Y);
		}
	}

	Anchor->Setup(ToOculusXR(SceneAnchor->uuid), SceneAnchor->space, ToOculusXR(SceneAnchor->pose),
		TArray{ ToOculusXR(SceneAnchor->semanticLabel) }, Plane,
		PlaneBoundary, Volume, std::move(GlobalMeshPositions), std::move(GlobalMeshIndices));
}

static void MrukOnSceneAnchorAdded(const MRUKShared::MrukSceneAnchor* SceneAnchor, void* UserContext)
{
	UMRUKSubsystem* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	AMRUKRoom* Room = FindRoomByUuid(Subsystem, ToOculusXR(SceneAnchor->roomUuid));
	check(Room);
	AMRUKAnchor* Anchor = Room->SpawnAnchor();
	UpdateAnchorProperties(SceneAnchor, Room, Anchor);
	Room->AddAnchorToRoom(Anchor);
	Room->OnAnchorCreated.Broadcast(Anchor);
}

static void MrukOnSceneAnchorUpdated(const MRUKShared::MrukSceneAnchor* SceneAnchor, bool SignificantChange, void* UserContext)
{
	UMRUKSubsystem* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	AMRUKRoom* Room = FindRoomByUuid(Subsystem, ToOculusXR(SceneAnchor->roomUuid));
	check(Room);
	AMRUKAnchor* Anchor = Room->FindAnchorByUuid(ToOculusXR(SceneAnchor->uuid));
	check(Anchor);
	UpdateAnchorProperties(SceneAnchor, Room, Anchor);
	if (SignificantChange)
	{
		Room->OnAnchorUpdated.Broadcast(Anchor);
	}
}

static void MrukOnSceneAnchorRemoved(const MRUKShared::MrukSceneAnchor* SceneAnchor, void* UserContext)
{
	UMRUKSubsystem* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	AMRUKRoom* Room = FindRoomByUuid(Subsystem, ToOculusXR(SceneAnchor->roomUuid));
	check(Room);
	AMRUKAnchor* Anchor = Room->FindAnchorByUuid(ToOculusXR(SceneAnchor->uuid));
	Room->OnAnchorRemoved.Broadcast(Anchor);
	Room->RemoveAnchor(Anchor);
}

static void MrukOnDiscoveryFinished(MRUKShared::MrukResult Result, void* UserContext)
{
	auto* Subsystem = static_cast<UMRUKSubsystem*>(UserContext);
	const bool Success = Result == MRUKShared::MRUK_SUCCESS;
	if (Success)
	{
		Subsystem->SceneLoadStatus = EMRUKInitStatus::Complete;
	}
	else
	{
		Subsystem->SceneLoadStatus = EMRUKInitStatus::Failed;
	}

	if (Subsystem->Rooms.Num() > 0 && Subsystem->Rooms[Subsystem->Rooms.Num() - 1]->SpaceHandle != FOculusXRUInt64{})
	{
		OculusXRTelemetry::TScopedMarker<MRUKTelemetry::FLoadSceneFromDeviceMarker> Event(static_cast<int>(GetTypeHash(Subsystem)));
		Event.AddAnnotation("NumRooms", TCHAR_TO_ANSI(*FString::FromInt(Subsystem->Rooms.Num())));
		Event.SetResult(Subsystem->Rooms.Num() > 0 ? OculusXRTelemetry::EAction::Success : OculusXRTelemetry::EAction::Fail);
	}
	else
	{
		OculusXRTelemetry::TScopedMarker<MRUKTelemetry::FLoadSceneFromJsonMarker> Event(static_cast<int>(GetTypeHash(Subsystem)));
		Event.AddAnnotation("NumRooms", TCHAR_TO_ANSI(*FString::FromInt(Subsystem->Rooms.Num())));
		Event.SetResult(Subsystem->Rooms.Num() > 0 ? OculusXRTelemetry::EAction::Success : OculusXRTelemetry::EAction::Fail);
	}

	Subsystem->OnSceneLoaded.Broadcast(Success);
}

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

static void OpenXrEventHandler(void* Data, void* Context)
{
	MRUKShared::GetInstance()->AnchorStoreOnOpenXrEvent(Data);
}

static bool IsOpenXRSystem()
{
	const FName SystemName(TEXT("OpenXR"));
	return GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName);
}

static void SharedLibraryLogPrinter(MRUKShared::MrukLogLevel Level, const char* Message, uint32_t Length)
{
	switch (Level)
	{
		case MRUKShared::MRUK_LOG_LEVEL_DEBUG:
			UE_LOG(LogMRUK, Verbose, TEXT("MRUK Shared: %.*hs"), Length, Message);
			break;
		case MRUKShared::MRUK_LOG_LEVEL_INFO:
			UE_LOG(LogMRUK, Log, TEXT("MRUK Shared: %.*hs"), Length, Message);
			break;
		case MRUKShared::MRUK_LOG_LEVEL_WARN:
			UE_LOG(LogMRUK, Warning, TEXT("MRUK Shared: %.*hs"), Length, Message);
			break;
		case MRUKShared::MRUK_LOG_LEVEL_ERROR:
			UE_LOG(LogMRUK, Error, TEXT("MRUK Shared: %.*hs"), Length, Message);
			break;
	}
}

void UMRUKSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	const UOculusXRHMDRuntimeSettings* Settings = GetMutableDefault<UOculusXRHMDRuntimeSettings>();
	EnableWorldLock = Settings->EnableWorldLock;

	FMRUKModule& MRUKModule = FMRUKModule::GetInstance();
	if (IsOpenXRSystem())
	{
		MRUKModule.OpenXrExtension->OpenXrEventHandler = OpenXrEventHandler;
		MRUKModule.OpenXrExtension->Context = this;
	}

	MRUKShared::LoadMRUKSharedLibrary();

	MRUKShared::GetInstance()->SetLogPrinter(SharedLibraryLogPrinter);

	uint64_t OpenXrInstance = 0;
	uint64_t OpenXrSession = 0;
	UOculusXRFunctionLibrary::GetNativeOpenXRHandles(&OpenXrInstance, &OpenXrSession);
	if (OpenXrInstance == 0 || OpenXrSession == 0)
	{
		// We are probably running in unit tests
		EnableOpenXr = false;
	}
	else
	{
		EnableOpenXr = true;
	}

	if (EnableOpenXr)
	{
		void* OpenXrInstanceProcAddr = nullptr;
		UOculusXRFunctionLibrary::GetOpenXRInstanceProcAddrFunc(&OpenXrInstanceProcAddr);
		ensureMsgf(OpenXrInstanceProcAddr != nullptr, TEXT("OpenXrInstanceProcAddr is not set"));

		UOculusXRFunctionLibrary::GetAppSpace(&OpenXrBaseSpace);
		ensureMsgf(OpenXrBaseSpace != 0, TEXT("BaseSpace not set"));
		if (!IsOpenXRSystem())
		{
			UOculusXRFunctionLibrary::RegisterOpenXrEventHandler(OpenXrEventHandler, this);
		}

		MRUKShared::MrukResult result = MRUKShared::GetInstance()->AnchorStoreCreate(OpenXrInstance, OpenXrSession, OpenXrInstanceProcAddr, OpenXrBaseSpace, NULL, 0);
		if (result != MRUKShared::MRUK_SUCCESS)
		{
			UE_LOG(LogMRUK, Error, TEXT("Failed to initialize anchor store. It will not be possible to load anchors: %d"), result);
		}
		else
		{
			UE_LOG(LogMRUK, Log, TEXT("Anchor store initialized successfully"));
		}
	}
	else
	{
		MRUKShared::MrukResult result = MRUKShared::GetInstance()->AnchorStoreCreateWithoutOpenXr();
		if (result != MRUKShared::MRUK_SUCCESS)
		{
			UE_LOG(LogMRUK, Error, TEXT("Failed to initialize anchor store. It will not be possible to load anchors: %d"), result);
		}
		else
		{
			UE_LOG(LogMRUK, Log, TEXT("Anchor store initialized successfully"));
		}
	}

	MRUKShared::MrukEventListener EventListener{
		.onPreRoomAnchorAdded = MrukOnPreRoomAnchorAdded,
		.onRoomAnchorAdded = MrukOnRoomAnchorAdded,
		.onRoomAnchorUpdated = MrukOnRoomAnchorUpdated,
		.onRoomAnchorRemoved = MrukOnRoomAnchorRemoved,
		.onSceneAnchorAdded = MrukOnSceneAnchorAdded,
		.onSceneAnchorUpdated = MrukOnSceneAnchorUpdated,
		.onSceneAnchorRemoved = MrukOnSceneAnchorRemoved,
		.onDiscoveryFinished = MrukOnDiscoveryFinished,
		.userContext = this,
	};
	MRUKShared::GetInstance()->AnchorStoreRegisterEventListener(EventListener);
}

void UMRUKSubsystem::Deinitialize()
{
	if (EnableOpenXr)
	{
		UOculusXRFunctionLibrary::UnregisterOpenXrEventHandler(OpenXrEventHandler);
	}

	if (MRUKShared::GetInstance())
	{
		MRUKShared::GetInstance()->AnchorStoreDestroy();
	}

	MRUKShared::FreeMRUKSharedLibrary();

	FMRUKModule& MRUKModule = FMRUKModule::GetInstance();
	MRUKModule.OpenXrExtension->OpenXrEventHandler = nullptr;
	MRUKModule.OpenXrExtension->Context = nullptr;
}

void UMRUKSubsystem::UnregisterRoom(AMRUKRoom* Room)
{
	bool Found = false;
	for (const AMRUKRoom* R : Rooms)
	{
		if (Room->AnchorUUID == R->AnchorUUID)
		{
			Found = true;
			break;
		}
	}
	if (Found)
	{
		MRUKShared::MrukUuid Uuid{};
		memcpy(&Uuid.data, Room->AnchorUUID.UUIDBytes, 2 * sizeof(uint64_t));
		MRUKShared::GetInstance()->AnchorStoreClearRoom(Uuid);
	}
}

AMRUKRoom* UMRUKSubsystem::GetCurrentRoom() const
{
	// This is a rather expensive operation, we should only do it at most once per frame.
	if (CachedCurrentRoomFrame != GFrameCounter)
	{
		if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				const auto& PawnTransform = Pawn->GetActorTransform();

				FVector HeadPosition;
				FRotator Unused;

				// Get the position and rotation of the VR headset
				UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(Unused, HeadPosition);

				HeadPosition = PawnTransform.TransformPosition(HeadPosition);

				for (const auto& Room : Rooms)
				{
					if (IsValid(Room) && Room->IsPositionInRoom(HeadPosition))
					{
						CachedCurrentRoom = Room;
						CachedCurrentRoomFrame = GFrameCounter;
						return Room;
					}
				}
			}
		}
	}

	if (IsValid(CachedCurrentRoom))
	{
		return CachedCurrentRoom;
	}

	for (const auto& Room : Rooms)
	{
		if (IsValid(Room))
		{
			return Room;
		}
	}
	return nullptr;
}

FString UMRUKSubsystem::SaveSceneToJsonString()
{
	const char* Json = MRUKShared::GetInstance()->AnchorStoreSaveSceneToJson(true, NULL, 0);
	const FString Result(Json);
	MRUKShared::GetInstance()->AnchorStoreFreeJson(Json);
	return Result;
}

void UMRUKSubsystem::LoadSceneFromJsonString(const FString& String)
{
	if (SceneLoadStatus == EMRUKInitStatus::Busy)
	{
		UE_LOG(LogMRUK, Error, TEXT("Can't start loading a scene from JSON while the scene is already loading"));
		return;
	}

	SceneLoadStatus = EMRUKInitStatus::Busy;
	MRUKShared::MrukResult result = MRUKShared::GetInstance()->AnchorStoreLoadSceneFromJson(TCHAR_TO_ANSI(*String), true, MRUKShared::MRUK_SCENE_MODEL_V1);
	if (result != MRUKShared::MRUK_SUCCESS)
	{
		SceneLoadStatus = EMRUKInitStatus::Failed;
		MrukOnDiscoveryFinished(result, this);
	}
}

void UMRUKSubsystem::LoadSceneFromDevice()
{
	if (SceneLoadStatus == EMRUKInitStatus::Busy)
	{
		UE_LOG(LogMRUK, Error, TEXT("Can't start loading from device while the scene is already loading"));
		return;
	}

	SceneLoadStatus = EMRUKInitStatus::Busy;
	MRUKShared::MrukResult result = MRUKShared::GetInstance()->AnchorStoreStartDiscovery(true, MRUKShared::MRUK_SCENE_MODEL_V1);
	if (result != MRUKShared::MRUK_SUCCESS)
	{
		SceneLoadStatus = EMRUKInitStatus::Failed;
		UE_LOG(LogMRUK, Error, TEXT("Failed to start anchor discovery: %d"), result);
		MrukOnDiscoveryFinished(result, this);
	}
}

void UMRUKSubsystem::ClearScene()
{
	SceneLoadStatus = EMRUKInitStatus::None;
	MRUKShared::GetInstance()->AnchorStoreClearRooms();
}

AMRUKAnchor* UMRUKSubsystem::TryGetClosestSurfacePosition(const FVector& WorldPosition, FVector& OutSurfacePosition, const FMRUKLabelFilter& LabelFilter, double MaxDistance)
{
	AMRUKAnchor* ClosestAnchor = nullptr;

	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}
		double SurfaceDistance{};
		FVector SurfacePos{};
		if (const auto Anchor = Room->TryGetClosestSurfacePosition(WorldPosition, SurfacePos, SurfaceDistance, LabelFilter, MaxDistance))
		{
			ClosestAnchor = Anchor;
			OutSurfacePosition = SurfacePos;
			MaxDistance = SurfaceDistance;
		}
	}

	return ClosestAnchor;
}

AMRUKAnchor* UMRUKSubsystem::TryGetClosestSeatPose(const FVector& RayOrigin, const FVector& RayDirection, FTransform& OutSeatTransform)
{
	AMRUKAnchor* ClosestAnchor = nullptr;
	double ClosestSeatDistanceSq = DBL_MAX;

	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}

		FTransform SeatTransform{};
		if (AMRUKAnchor* Anchor = Room->TryGetClosestSeatPose(RayOrigin, RayDirection, SeatTransform))
		{
			const double SeatDistanceSq = (RayOrigin - Anchor->GetActorTransform().GetTranslation()).SquaredLength();
			if (SeatDistanceSq < ClosestSeatDistanceSq)
			{
				ClosestAnchor = Anchor;
				ClosestSeatDistanceSq = SeatDistanceSq;
				OutSeatTransform = SeatTransform;
			}
		}
	}

	return ClosestAnchor;
}

AMRUKAnchor* UMRUKSubsystem::GetBestPoseFromRaycast(const FVector& RayOrigin, const FVector& RayDirection, double MaxDist, const FMRUKLabelFilter& LabelFilter, FTransform& OutPose, EMRUKPositioningMethod PositioningMethod)
{
	AMRUKAnchor* ClosestAnchor = nullptr;
	double ClosestPoseDistanceSq = DBL_MAX;

	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}

		FTransform Pose{};
		AMRUKAnchor* Anchor = Room->GetBestPoseFromRaycast(RayOrigin, RayDirection, MaxDist, LabelFilter, Pose, PositioningMethod);
		if (Anchor)
		{
			const double PoseDistanceSq = (RayOrigin - OutPose.GetTranslation()).SquaredLength();
			if (PoseDistanceSq < ClosestPoseDistanceSq)
			{
				ClosestAnchor = Anchor;
				ClosestPoseDistanceSq = PoseDistanceSq;
				OutPose = Pose;
			}
		}
	}

	return ClosestAnchor;
}

AMRUKAnchor* UMRUKSubsystem::GetKeyWall(double Tolerance)
{
	if (AMRUKRoom* CurrentRoom = GetCurrentRoom())
	{
		return CurrentRoom->GetKeyWall(Tolerance);
	}
	return nullptr;
}

AMRUKAnchor* UMRUKSubsystem::GetLargestSurface(const FString& Label)
{
	if (AMRUKRoom* CurrentRoom = GetCurrentRoom())
	{
		return CurrentRoom->GetLargestSurface(Label);
	}
	return nullptr;
}

AMRUKAnchor* UMRUKSubsystem::IsPositionInSceneVolume(const FVector& WorldPosition, bool TestVerticalBounds, double Tolerance)
{
	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}
		if (const auto Anchor = Room->IsPositionInSceneVolume(WorldPosition, TestVerticalBounds, Tolerance))
		{
			return Anchor;
		}
	}
	return nullptr;
}

TArray<AActor*> UMRUKSubsystem::SpawnInterior(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, const TArray<FString>& CutHoleLabels, UMaterialInterface* ProceduralMaterial, bool ShouldFallbackToProcedural)
{
	return SpawnInteriorFromStream(SpawnGroups, FRandomStream(NAME_None), CutHoleLabels, ProceduralMaterial, ShouldFallbackToProcedural);
}

TArray<AActor*> UMRUKSubsystem::SpawnInteriorFromStream(const TMap<FString, FMRUKSpawnGroup>& SpawnGroups, const FRandomStream& RandomStream, const TArray<FString>& CutHoleLabels, UMaterialInterface* ProceduralMaterial, bool ShouldFallbackToProcedural)
{
	TArray<AActor*> AllInteriorActors;

	for (const auto& Room : Rooms)
	{
		if (!Room)
		{
			continue;
		}
		auto InteriorActors = Room->SpawnInteriorFromStream(SpawnGroups, RandomStream, CutHoleLabels, ProceduralMaterial, ShouldFallbackToProcedural);
		AllInteriorActors.Append(InteriorActors);
	}

	return AllInteriorActors;
}

bool UMRUKSubsystem::LaunchSceneCapture()
{
	const bool Success = GetRoomLayoutManager()->LaunchCaptureFlow();
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

FBox UMRUKSubsystem::GetActorClassBounds(TSubclassOf<AActor> Actor)
{
	if (const auto Entry = ActorClassBoundsCache.Find(Actor))
	{
		return *Entry;
	}
	const auto TempActor = GetWorld()->SpawnActor(Actor);
	const auto Bounds = TempActor->CalculateComponentsBoundingBoxInLocalSpace(true);
	TempActor->Destroy();
	ActorClassBoundsCache.Add(Actor, Bounds);
	return Bounds;
}

void UMRUKSubsystem::SceneCaptureComplete(FOculusXRUInt64 RequestId, bool bSuccess)
{
	UE_LOG(LogMRUK, Log, TEXT("Scene capture complete Success==%d"), bSuccess);
	OnCaptureComplete.Broadcast(bSuccess);
}

UOculusXRRoomLayoutManagerComponent* UMRUKSubsystem::GetRoomLayoutManager()
{
	if (!RoomLayoutManager)
	{
		FActorSpawnParameters Params{};
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = nullptr;
		RoomLayoutManagerActor = GetWorld()->SpawnActor<AActor>(Params);
		RoomLayoutManagerActor->SetRootComponent(NewObject<USceneComponent>(RoomLayoutManagerActor, TEXT("SceneComponent")));

		RoomLayoutManagerActor->AddComponentByClass(UOculusXRRoomLayoutManagerComponent::StaticClass(), false, FTransform::Identity, false);
		RoomLayoutManager = RoomLayoutManagerActor->GetComponentByClass<UOculusXRRoomLayoutManagerComponent>();
		RoomLayoutManager->OculusXRRoomLayoutSceneCaptureComplete.AddDynamic(this, &UMRUKSubsystem::SceneCaptureComplete);
	}
	return RoomLayoutManager;
}

bool UMRUKSubsystem::DiscoveryIsRunning() const
{
	return MRUKShared::GetInstance()->AnchorStoreIsDiscoveryRunning();
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

void UMRUKSubsystem::Tick(float DeltaTime)
{
	if (EnableWorldLock)
	{
		if (const auto Room = GetCurrentRoom())
		{
			if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (APawn* Pawn = PlayerController->GetPawn())
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

	uint64_t NextPredictedDisplayTime = 0;
	if (EnableOpenXr)
	{
		uint64_t NewBaseSpace = 0;
		UOculusXRFunctionLibrary::GetAppSpace(&NewBaseSpace);
		if (NewBaseSpace != OpenXrBaseSpace)
		{
			OpenXrBaseSpace = NewBaseSpace;
			MRUKShared::GetInstance()->AnchorStoreSetBaseSpace(OpenXrBaseSpace);
		}

		UOculusXRFunctionLibrary::GetNextPredictedDisplayTime(&NextPredictedDisplayTime);
	}
	MRUKShared::GetInstance()->AnchorStoreTick(NextPredictedDisplayTime);
}

bool UMRUKSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_BeginDestroyed) && IsValidChecked(this);
}

