// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusSceneActor.h"
#include "OculusSceneModule.h"
#include "OculusHMDModule.h"
#include "OculusHMD.h"
#include "OculusAnchorManager.h"
#include "OculusDelegates.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMeshActor.h"
#include "ProceduralMeshComponent.h"
#include "OculusSceneGlobalMeshComponent.h"

#define LOCTEXT_NAMESPACE "OculusSceneActor"

//////////////////////////////////////////////////////////////////////////
// ASceneActor

AOculusSceneActor::AOculusSceneActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ResetStates();

	// Create required components
	RoomLayoutManagerComponent = CreateDefaultSubobject<UOculusRoomLayoutManagerComponent>(TEXT("RoomLayoutManagerComponent"));

	// Following are the semantic labels we want to support default properties for.  User can always add new ones through the properties panel if needed.
	const FString default2DSemanticClassifications[] = {
		TEXT("WALL_FACE"),
		TEXT("CEILING"),
		TEXT("FLOOR"),
		TEXT("COUCH"),
		TEXT("TABLE"),
		TEXT("DOOR_FRAME"),
		TEXT("WINDOW_FRAME"),
		TEXT("WALL_ART"),
		TEXT("INVISIBLE_WALL_FACE"),
		TEXT("OTHER")
	};

	const FString default3DSemanticClassifications[] = {
		TEXT("COUCH"),
		TEXT("TABLE"),
		TEXT("SCREEN"),
		TEXT("BED"),
		TEXT("LAMP"),
		TEXT("PLANT"),
		TEXT("STORAGE"),
		TEXT("OTHER")
	};

	FSpawnedSceneAnchorProperties spawnedAnchorProps;
	spawnedAnchorProps.ActorComponent = nullptr;
	// Populate default UPROPERTY for the "ScenePlaneSpawnedSceneAnchorProperties" member
	spawnedAnchorProps.StaticMesh = nullptr; //TSoftObjectPtr<UStaticMesh>(FSoftObjectPath("/OculusVR/Meshes/ScenePlane.ScenePlane"));
	/*for (int32 i = 0; i < sizeof(defaultSemanticClassification) / sizeof(defaultSemanticClassification[0]); ++i)
	{
		FSpawnedSceneAnchorProperties& props = ScenePlaneSpawnedSceneAnchorProperties.Add(defaultSemanticClassification[i], spawnedAnchorProps);
		
		// Orientation constraints
		if (defaultSemanticClassification[i] == "CEILING" ||
			defaultSemanticClassification[i] == "FLOOR" ||
			defaultSemanticClassification[i] == "COUCH" ||
			defaultSemanticClassification[i] == "DESK" ||
			defaultSemanticClassification[i] == "DOOR_FRAME" ||
			defaultSemanticClassification[i] == "WINDOW_FRAME" ||
			defaultSemanticClassification[i] == "SCREEN" ||
			defaultSemanticClassification[i] == "BED" ||
			defaultSemanticClassification[i] == "LAMP" ||
			defaultSemanticClassification[i] == "PLANT" ||
			defaultSemanticClassification[i] == "STORAGE" ||
			defaultSemanticClassification[i] == "OTHER")
		{
			props.ForceParallelToFloor = true;
		}
	}

	// Populate default UPROPERTY for the "SceneVolumeSpawnedSceneAnchorProperties" member
	// For the time being, only "OTHER" semantic label is used for volume scene anchors.
	spawnedAnchorProps.StaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath("/OculusVR/Meshes/SceneVolume.SceneVolume"));
	FSpawnedSceneAnchorProperties& props = SceneVolumeSpawnedSceneAnchorProperties.Add(TEXT("OTHER"), spawnedAnchorProps);
	props.ForceParallelToFloor = true;*/
	// Setup initial scene plane and volume properties
	for (auto& semanticLabel2D : default2DSemanticClassifications)
	{
		FSpawnedSceneAnchorProperties& props = ScenePlaneSpawnedSceneAnchorProperties.Add(semanticLabel2D, spawnedAnchorProps);
		props.ForceParallelToFloor = (semanticLabel2D != "WALL_FACE");
	}

	for (auto& semanticLabel3D : default3DSemanticClassifications)
	{
		FSpawnedSceneAnchorProperties& props = SceneVolumeSpawnedSceneAnchorProperties.Add(semanticLabel3D, spawnedAnchorProps);
		props.ForceParallelToFloor = true;
	}
}

void AOculusSceneActor::ResetStates()
{		
	bCaptureFlowWasLaunched = false;
	ClearScene();
}

void AOculusSceneActor::BeginPlay()
{
	Super::BeginPlay();

	// Create a scene component as root so we can attach spawned actors to it
	USceneComponent* rootSceneComponent = NewObject<USceneComponent>(this, USceneComponent::StaticClass());
	rootSceneComponent->SetMobility(EComponentMobility::Static);
	rootSceneComponent->RegisterComponent();
	SetRootComponent(rootSceneComponent);

	SceneGlobalMeshComponent = FindComponentByClass<UOculusSceneGlobalMeshComponent>();

	// Register delegates
	RoomLayoutManagerComponent->OculusRoomLayoutSceneCaptureCompleteNative.AddUObject(this, &AOculusSceneActor::SceneCaptureComplete_Handler);

	// Make an initial request to query for the room layout if bPopulateSceneOnBeginPlay was set to true
	if (bPopulateSceneOnBeginPlay)
	{
		PopulateScene();
	}
}

void AOculusSceneActor::EndPlay(EEndPlayReason::Type Reason)
{
	// Unregister delegates
	RoomLayoutManagerComponent->OculusRoomLayoutSceneCaptureCompleteNative.RemoveAll(this);

	// Calling ResetStates will reset member variables to their default values (including the request IDs).
	ResetStates();

	Super::EndPlay(Reason);
}

void AOculusSceneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

EOculusResult::Type AOculusSceneActor::QueryAllRooms()
{
	FOculusSpaceQueryInfo queryInfo;
	queryInfo.MaxQuerySpaces = MaxQueries;
	queryInfo.FilterType = EOculusSpaceQueryFilterType::FilterByComponentType;
	queryInfo.ComponentFilter.Add(EOculusSpaceComponentType::RoomLayout);

	EOculusResult::Type anchorQueryResult;
	OculusAnchors::FOculusAnchors::QueryAnchorsAdvanced(queryInfo,
		FOculusAnchorQueryDelegate::CreateUObject(this, &AOculusSceneActor::RoomLayoutQueryComplete), anchorQueryResult);

	return anchorQueryResult;
}

void AOculusSceneActor::RoomLayoutQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults)
{
	UE_LOG(LogOculusScene, Verbose, TEXT("RoomLayoutQueryComplete (Result = %d)"), AnchorResult);

	for (auto& QueryElement : QueryResults)
	{
		UE_LOG(LogOculusScene, Verbose, TEXT("RoomLayoutQueryComplete -- Query Element (space = %llu, uuid = %s"), QueryElement.Space.Value, *BytesToHex(QueryElement.UUID.UUIDBytes, OCULUS_UUID_SIZE));

		FOculusRoomLayout roomLayout;
		const bool bGetRoomLayoutResult = RoomLayoutManagerComponent->GetRoomLayout(QueryElement.Space.Value, roomLayout, MaxQueries);
		if (!bGetRoomLayoutResult)
		{
			UE_LOG(LogOculusScene, Error, TEXT("RoomLayoutQueryComplete -- Failed to get room layout for space (space = %llu, uuid = %s"),
				QueryElement.Space.Value, *BytesToHex(QueryElement.UUID.UUIDBytes, OCULUS_UUID_SIZE));
			continue;
		}

		// If we're only loading the active room we start that floor check query here, otherwise do the room query
		if (bActiveRoomOnly)
		{
			QueryFloorForActiveRoom(QueryElement.Space, roomLayout);
		}
		else
		{
			StartSingleRoomQuery(QueryElement.Space, roomLayout);
		}
	}
}

EOculusResult::Type AOculusSceneActor::QueryRoomUUIDs(const FUInt64 RoomSpaceID, const TArray<FUUID>& RoomUUIDs)
{
	EOculusResult::Type anchorQueryResult;
	OculusAnchors::FOculusAnchors::QueryAnchors(
		RoomUUIDs,
		EOculusSpaceStorageLocation::Local,
		FOculusAnchorQueryDelegate::CreateUObject(this, &AOculusSceneActor::SceneRoomQueryComplete, RoomSpaceID),
		anchorQueryResult);

	return anchorQueryResult;
}

void AOculusSceneActor::SceneRoomQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults, const FUInt64 RoomSpaceID)
{
	if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(AnchorResult))
	{
		return;
	}

	bool bOutPending = false;

	for (auto& AnchorQueryElement : QueryResults)
	{
		if (SceneGlobalMeshComponent)
		{
			TArray<FString> semanticClassifications;
			GetSemanticClassifications(AnchorQueryElement.Space.Value, semanticClassifications);
			UE_LOG(LogOculusScene, Log, TEXT("SpatialAnchor Scene label is %s"), semanticClassifications.Num() > 0 ? *semanticClassifications[0] : TEXT("unknown"));
			if (semanticClassifications.Contains(UOculusSceneGlobalMeshComponent::GlobalMeshSemanticLabel))
			{
				bool bIsTriangleMesh = false;
				EOculusResult::Type result = OculusAnchors::FOculusAnchorManager::GetSpaceComponentStatus(
					AnchorQueryElement.Space.Value, EOculusSpaceComponentType::TriangleMesh, bIsTriangleMesh, bOutPending);

				if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(result) || !bIsTriangleMesh)
				{
					UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to load Triangle Mesh Component for a GLOBAL_MESH"));
					continue;
				}

				UClass* sceneAnchorComponentInstanceClass = SceneGlobalMeshComponent->GetAnchorComponentClass();

				AActor* globalMeshAnchor = SpawnActorWithSceneComponent(AnchorQueryElement.Space.Value, RoomSpaceID, semanticClassifications, sceneAnchorComponentInstanceClass);

				SceneGlobalMeshComponent->CreateMeshComponent(AnchorQueryElement.Space, globalMeshAnchor, RoomLayoutManagerComponent);
				continue;
			}
		}

		bool bIsScenePlane = false;
		bool bIsSceneVolume = false;
		EOculusResult::Type isPlaneResult = OculusAnchors::FOculusAnchorManager::GetSpaceComponentStatus(
			AnchorQueryElement.Space.Value, EOculusSpaceComponentType::ScenePlane, bIsScenePlane, bOutPending);

		EOculusResult::Type isVolumeResult = OculusAnchors::FOculusAnchorManager::GetSpaceComponentStatus(
			AnchorQueryElement.Space.Value, EOculusSpaceComponentType::SceneVolume, bIsSceneVolume, bOutPending);

		bool bIsPlaneResultSuccess = UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(isPlaneResult);
		bool bIsVolumeResultSuccess = UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(isVolumeResult);

		AActor* anchor = nullptr;

		if (bIsPlaneResultSuccess && bIsScenePlane)
		{
			EOculusResult::Type Result;
			FVector scenePlanePos;
			FVector scenePlaneSize;
			bool ResultSuccess = OculusAnchors::FOculusAnchors::GetSpaceScenePlane(AnchorQueryElement.Space.Value, scenePlanePos, scenePlaneSize, Result);
			if (ResultSuccess)
			{
				UE_LOG(LogOculusScene, Log, TEXT("SpatialAnchorQueryResult_Handler ScenePlane pos = [%.2f, %.2f, %.2f], size = [%.2f, %.2f, %.2f]."),
					scenePlanePos.X, scenePlanePos.Y, scenePlanePos.Z,
					scenePlaneSize.X, scenePlaneSize.Y, scenePlaneSize.Z);

				TArray<FString> semanticClassifications;
				GetSemanticClassifications(AnchorQueryElement.Space.Value, semanticClassifications);

				UE_LOG(LogOculusScene, Log, TEXT("SpatialAnchor ScenePlane label is %s"), semanticClassifications.Num() > 0 ? *semanticClassifications[0] : TEXT("unknown"));

				anchor = SpawnOrUpdateSceneAnchor(anchor, AnchorQueryElement.Space, RoomSpaceID, scenePlanePos, scenePlaneSize, semanticClassifications, EOculusSpaceComponentType::ScenePlane);
				if (!anchor)
				{
					UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to spawn scene anchor."));
				}
			}
			else
			{
				UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to get bounds for ScenePlane space."));
			}
		}

		if (bIsVolumeResultSuccess && bIsSceneVolume)
		{
			EOculusResult::Type Result;
			FVector sceneVolumePos;
			FVector sceneVolumeSize;
			bool ResultSuccess = OculusAnchors::FOculusAnchors::GetSpaceSceneVolume(AnchorQueryElement.Space.Value, sceneVolumePos, sceneVolumeSize, Result);
			if (ResultSuccess)
			{
				UE_LOG(LogOculusScene, Log, TEXT("SpatialAnchorQueryResult_Handler SceneVolume pos = [%.2f, %.2f, %.2f], size = [%.2f, %.2f, %.2f]."),
					sceneVolumePos.X, sceneVolumePos.Y, sceneVolumePos.Z,
					sceneVolumeSize.X, sceneVolumeSize.Y, sceneVolumeSize.Z);

				TArray<FString> semanticClassifications;
				GetSemanticClassifications(AnchorQueryElement.Space.Value, semanticClassifications);

				UE_LOG(LogOculusScene, Log, TEXT("SpatialAnchor SceneVolume label is %s"), semanticClassifications.Num() > 0 ? *semanticClassifications[0] : TEXT("unknown"));

				anchor = SpawnOrUpdateSceneAnchor(anchor, AnchorQueryElement.Space, RoomSpaceID, sceneVolumePos, sceneVolumeSize, semanticClassifications, EOculusSpaceComponentType::SceneVolume);
				if (!anchor)
				{
					UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to spawn scene anchor."));
				}
			}
			else
			{
				UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to get bounds for SceneVolume space."));
			}
		}
	}
}

void AOculusSceneActor::StartSingleRoomQuery(FUInt64 RoomSpaceID, FOculusRoomLayout RoomLayout)
{
	EOculusResult::Type queryResult = QueryRoomUUIDs(RoomSpaceID, RoomLayout.RoomObjectUUIDs);
	if (UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(queryResult))
	{
		RoomLayouts.Add(RoomSpaceID, std::move(RoomLayout));
	}
}

EOculusResult::Type AOculusSceneActor::QueryFloorForActiveRoom(FUInt64 RoomSpaceID, FOculusRoomLayout RoomLayout)
{
	EOculusResult::Type anchorQueryResult;
	OculusAnchors::FOculusAnchors::QueryAnchors(
		TArray<FUUID>({ RoomLayout.FloorUuid }),
		EOculusSpaceStorageLocation::Local,
		FOculusAnchorQueryDelegate::CreateUObject(this, &AOculusSceneActor::ActiveRoomFloorQueryComplete, RoomSpaceID, RoomLayout),
		anchorQueryResult);

	return anchorQueryResult;
}

void AOculusSceneActor::ActiveRoomFloorQueryComplete(EOculusResult::Type AnchorResult, const TArray<FOculusSpaceQueryResult>& QueryResults, FUInt64 RoomSpaceID, FOculusRoomLayout RoomLayout)
{
	if (QueryResults.Num() != 1)
	{
		UE_LOG(LogOculusScene, Error, TEXT("Wrong number of elements returned from query for floor UUID. Result count (%d), UUID (%s), Room Space ID (%llu)"), QueryResults.Num(), *RoomLayout.FloorUuid.ToString(), RoomSpaceID.Value);
		return;
	}

	const FOculusSpaceQueryResult& floorQueryResult = QueryResults[0];

	TArray<FString> semanticClassifications;
	GetSemanticClassifications(floorQueryResult.Space.Value, semanticClassifications);
	if (!semanticClassifications.Contains("FLOOR"))
	{
		UE_LOG(LogOculusScene, Error, TEXT("Queried floor in room doesn't contain a floor semantic label. UUID (%s), Room Space ID (%llu)"), *RoomLayout.FloorUuid.ToString(), RoomSpaceID.Value);
		return;
	}

	EOculusResult::Type getBoundaryResult;
	TArray<FVector2D> boundaryVertices;
	if (!OculusAnchors::FOculusAnchors::GetSpaceBoundary2D(floorQueryResult.Space.Value, boundaryVertices, getBoundaryResult))
	{
		UE_LOG(LogOculusScene, Error, TEXT("Failed to get space boundary vertices for floor. UUID (%s), Room Space ID (%llu)"), *RoomLayout.FloorUuid.ToString(), RoomSpaceID.Value);
		return;
	}

	OculusHMD::FOculusHMD* HMD = OculusHMD::FOculusHMD::GetOculusHMD();
	check(HMD);

	OculusHMD::FPose headPose;
	HMD->GetCurrentPose(OculusHMD::ToExternalDeviceId(ovrpNode_Head), headPose.Orientation, headPose.Position);

	TArray<FVector> convertedBoundaryPoints;

	// Convert the boundary vertices to game engine world space
	for (auto& it : boundaryVertices)
	{
		FTransform floorTransform;
		UOculusAnchorBPFunctionLibrary::GetAnchorTransformByHandle(floorQueryResult.Space, floorTransform);

		FVector pos = floorTransform.TransformPosition(FVector(0, it.X * HMD->GetWorldToMetersScale(), it.Y * HMD->GetWorldToMetersScale()));
		convertedBoundaryPoints.Add(pos);
	}

	// Create the new 2D boundary
	TArray<FVector2D> new2DBoundary;
	for (auto& it : convertedBoundaryPoints)
	{
		new2DBoundary.Add(FVector2D(it.X, it.Y));
	}

	// Check if inside poly
	if (!PointInPolygon2D(FVector2D(headPose.Position.X, headPose.Position.Y), new2DBoundary))
	{
		UE_LOG(LogOculusScene, Verbose, TEXT("Floor failed active room check. UUID (%s), Room Space ID (%llu)"), *RoomLayout.FloorUuid.ToString(), RoomSpaceID.Value);
		return;
	}

	StartSingleRoomQuery(RoomSpaceID, RoomLayout);
}

bool AOculusSceneActor::PointInPolygon2D(FVector2D PointToTest, const TArray<FVector2D>& PolyVerts) const
{
	if (PolyVerts.Num() < 3)
	{
		return false;
	}

	int collision = 0;
	float x = PointToTest.X;
	float y = PointToTest.Y;

	int vertCount = PolyVerts.Num();
	for (int i = 0; i < vertCount; i++)
	{
		float x1 = PolyVerts[i].X;
		float y1 = PolyVerts[i].Y;

		float x2 = PolyVerts[(i + 1) % vertCount].X;
		float y2 = PolyVerts[(i + 1) % vertCount].Y;

		if (y < y1 != y < y2 && x < x1 + ((y - y1) / (y2 - y1)) * (x2 - x1))
		{
			collision += (y1 < y2) ? 1 : -1;
		}
	}

	return collision != 0;
}

void AOculusSceneActor::GetSemanticClassifications(uint64 Space, TArray<FString>& OutSemanticLabels) const
{
	EOculusResult::Type SemanticLabelAnchorResult;
	bool Result = OculusAnchors::FOculusAnchors::GetSpaceSemanticClassification(Space, OutSemanticLabels, SemanticLabelAnchorResult);
	if (Result)
	{
		UE_LOG(LogOculusScene, Verbose, TEXT("GetSemanticClassifications -- Space (%llu) Classifications:"), Space);
		for (FString& label : OutSemanticLabels)
		{
			UE_LOG(LogOculusScene, Verbose, TEXT("%s"), *label);
		}
	}
	else
	{
		UE_LOG(LogOculusScene, Error, TEXT("SpatialAnchorQueryResult_Handler Failed to get semantic classification space."));
	}
}

bool AOculusSceneActor::IsValidUuid(const FUUID& Uuid)
{
	return Uuid.UUIDBytes != nullptr;
}

void AOculusSceneActor::LaunchCaptureFlow()
{
	UE_LOG(LogOculusScene, Verbose, TEXT("Launch capture flow"));

	if (RoomLayoutManagerComponent)
	{
		UE_LOG(LogOculusScene, Verbose, TEXT("Launch capture flow -- RoomLayoutManagerComponent"));

		const bool bResult = RoomLayoutManagerComponent->LaunchCaptureFlow();
		if (!bResult)
		{
			UE_LOG(LogOculusScene, Error, TEXT("LaunchCaptureFlow() failed!"));
		}
	}
}

void AOculusSceneActor::LaunchCaptureFlowIfNeeded()
{
#if WITH_EDITOR
	UE_LOG(LogOculusScene, Display, TEXT("Scene Capture does not work over Link. Please capture a scene with the HMD in standalone mode, then access the scene model over Link."));
#else
	// Depending on LauchCaptureFlowWhenMissingScene, we might not want to launch Capture Flow
	if (LauchCaptureFlowWhenMissingScene != ELaunchCaptureFlowWhenMissingScene::NEVER)
	{
		if (LauchCaptureFlowWhenMissingScene == ELaunchCaptureFlowWhenMissingScene::ALWAYS || (!bCaptureFlowWasLaunched && LauchCaptureFlowWhenMissingScene == ELaunchCaptureFlowWhenMissingScene::ONCE))
		{
			LaunchCaptureFlow();
		}
	}
#endif
}

AActor* AOculusSceneActor::SpawnActorWithSceneComponent(const FUInt64& Space, const FUInt64& RoomSpaceID, const TArray<FString>& SemanticClassifications, UClass* sceneAnchorComponentInstanceClass)
{
	FActorSpawnParameters actorSpawnParams;
	actorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Anchor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, actorSpawnParams);

	USceneComponent* rootComponent = NewObject<USceneComponent>(Anchor, USceneComponent::StaticClass());
	rootComponent->SetMobility(EComponentMobility::Movable);
	rootComponent->RegisterComponent();
	Anchor->SetRootComponent(rootComponent);
	rootComponent->SetWorldLocation(FVector::ZeroVector);

	Anchor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

#if WITH_EDITOR
	if (SemanticClassifications.Num() > 0)
	{
		Anchor->SetActorLabel(FString::Join(SemanticClassifications, TEXT("-")), false);
	}
#endif

	UOculusSceneAnchorComponent* sceneAnchorComponent = NewObject<UOculusSceneAnchorComponent>(Anchor, sceneAnchorComponentInstanceClass);
	sceneAnchorComponent->RegisterComponent();

	sceneAnchorComponent->SetHandle(Space);
	sceneAnchorComponent->SemanticClassifications = SemanticClassifications;
	sceneAnchorComponent->RoomSpaceID = RoomSpaceID;

	EOculusResult::Type Result;
	OculusAnchors::FOculusAnchors::SetAnchorComponentStatus(sceneAnchorComponent, EOculusSpaceComponentType::Locatable, true, 0.0f, FOculusAnchorSetComponentStatusDelegate(), Result);

	return Anchor;
}

AActor* AOculusSceneActor::SpawnOrUpdateSceneAnchor(AActor* Anchor, const FUInt64& Space, const FUInt64& RoomSpaceID, const FVector& BoundedPos, const FVector& BoundedSize, const TArray<FString>& SemanticClassifications, const EOculusSpaceComponentType AnchorComponentType)
{
	if (Space.Value == 0)
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusXRSceneActor::SpawnOrUpdateSceneAnchor Invalid Space handle."));
		return Anchor;
	}

	if (!(AnchorComponentType == EOculusSpaceComponentType::ScenePlane || AnchorComponentType == EOculusSpaceComponentType::SceneVolume))
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusXRSceneActor::SpawnOrUpdateSceneAnchor Anchor doesn't have ScenePlane or SceneVolume component active."));
		return Anchor;
	}

	if (0 == SemanticClassifications.Num())
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusXRSceneActor::SpawnOrUpdateSceneAnchor No semantic classification found."));
		return Anchor;
	}

	FSpawnedSceneAnchorProperties* foundProperties = (AnchorComponentType == EOculusSpaceComponentType::ScenePlane) ? ScenePlaneSpawnedSceneAnchorProperties.Find(SemanticClassifications[0]) : SceneVolumeSpawnedSceneAnchorProperties.Find(SemanticClassifications[0]);

	if (!foundProperties)
	{
		UE_LOG(LogOculusScene, Warning, TEXT("AOculusXRSceneActor::SpawnOrUpdateSceneAnchor Scene object has an unknown semantic label.  Will not be spawned."));
		return Anchor;
	}

	TSoftClassPtr<UOculusSceneAnchorComponent>* sceneAnchorComponentClassPtrRef = &foundProperties->ActorComponent;
	TSoftObjectPtr<UStaticMesh>* staticMeshObjPtrRef = &foundProperties->StaticMesh;

	UClass* sceneAnchorComponentInstanceClass = sceneAnchorComponentClassPtrRef->LoadSynchronous();
	if (!sceneAnchorComponentInstanceClass)
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusXRSceneActor::SpawnOrUpdateSceneAnchor Scene anchor component class is invalid!  Cannot spawn actor to populate the scene."));
		return Anchor;
	}

	if (!Anchor)
	{
		Anchor = SpawnActorWithSceneComponent(Space, RoomSpaceID, SemanticClassifications, sceneAnchorComponentInstanceClass);
	}

	if (staticMeshObjPtrRef && staticMeshObjPtrRef->IsPending())
	{
		staticMeshObjPtrRef->LoadSynchronous();
	}
	UStaticMesh* refStaticMesh = staticMeshObjPtrRef ? staticMeshObjPtrRef->Get() : nullptr;
	if (refStaticMesh == nullptr)
	{
		UE_LOG(LogOculusScene, Warning, TEXT("AOculusXRSceneActor::SpawnOrUpdateSceneAnchor Spawn scene anchor mesh is invalid for %s!"), *SemanticClassifications[0]);
		return Anchor;
	}

	UStaticMeshComponent* staticMeshComponent = NewObject<UStaticMeshComponent>(Anchor, UStaticMeshComponent::StaticClass());
	staticMeshComponent->RegisterComponent();
	staticMeshComponent->SetStaticMesh(refStaticMesh);
	staticMeshComponent->AttachToComponent(Anchor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	const float worldToMeters = GetWorld()->GetWorldSettings()->WorldToMeters;
	FVector offset(0.0f, BoundedSize.Y / 2.0f, BoundedSize.Z / 2.0f);
	staticMeshComponent->SetRelativeLocation(foundProperties->AddOffset + ((BoundedPos + offset) * worldToMeters), false, nullptr, ETeleportType::ResetPhysics);

	// Setup scale based on bounded size and the actual size of the mesh
	UStaticMesh* staticMesh = staticMeshComponent->GetStaticMesh();
	FBoxSphereBounds staticMeshBounds;
	staticMeshBounds.BoxExtent = FVector{ 1.f, 1.f, 1.f };
	if (staticMesh)
	{
		staticMeshBounds = staticMesh->GetBounds();
	}

	staticMeshComponent->SetRelativeScale3D(FVector(
		(BoundedSize.X < SMALL_NUMBER) ? 1 : (BoundedSize.X / (staticMeshBounds.BoxExtent.X * 2.f)) * worldToMeters,
		(BoundedSize.Y < SMALL_NUMBER) ? 1 : (BoundedSize.Y / (staticMeshBounds.BoxExtent.Y * 2.f)) * worldToMeters,
		(BoundedSize.Z < SMALL_NUMBER) ? 1 : (BoundedSize.Z / (staticMeshBounds.BoxExtent.Z * 2.f)) * worldToMeters));

	return Anchor;
}

/*bool AOculusSceneActor::SpawnSceneAnchor(const FUInt64& Space, const FVector& BoundedSize, const TArray<FString>& SemanticClassifications, const EOculusSpaceComponentType AnchorComponentType)
{
	if (Space.Value == 0)
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor Invalid Space handle."));
		return false;
	}

	if (!(AnchorComponentType == EOculusSpaceComponentType::ScenePlane || AnchorComponentType == EOculusSpaceComponentType::SceneVolume))
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor Anchor doesn't have ScenePlane or SceneVolume component active."));
		return false;
	}

	if (0 == SemanticClassifications.Num())
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor No semantic classification found."));
		return false;
	}

	TSoftClassPtr<UOculusSceneAnchorComponent>* sceneAnchorComponentClassPtrRef = nullptr;
	TSoftObjectPtr<UStaticMesh>* staticMeshObjPtrRef = nullptr;

	FSpawnedSceneAnchorProperties* foundProperties = (AnchorComponentType == EOculusSpaceComponentType::ScenePlane) ? ScenePlaneSpawnedSceneAnchorProperties.Find(SemanticClassifications[0]) : SceneVolumeSpawnedSceneAnchorProperties.Find(SemanticClassifications[0]);

	if (!foundProperties)
	{
		UE_LOG(LogOculusScene, Warning, TEXT("AOculusSceneActor::SpawnSceneAnchor Scene object has an unknown semantic label.  Will not be spawned."));
		return false;
	}

	sceneAnchorComponentClassPtrRef = &foundProperties->ActorComponent;
	staticMeshObjPtrRef = &foundProperties->StaticMesh;

	UClass* sceneAnchorComponentInstanceClass = sceneAnchorComponentClassPtrRef->LoadSynchronous();
	if (!sceneAnchorComponentInstanceClass)
	{
		UE_LOG(LogOculusScene, Error, TEXT("AOculusSceneActor::SpawnSceneAnchor Scene anchor component class is invalid!  Cannot spawn actor to populate the scene."));
		return false;
	}
	
	FActorSpawnParameters actorSpawnParams;
	actorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* newActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(), FRotator(), actorSpawnParams);
	newActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	newActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	
	if (staticMeshObjPtrRef->IsPending())
	{
		staticMeshObjPtrRef->LoadSynchronous();
	}
	newActor->GetStaticMeshComponent()->SetStaticMesh(staticMeshObjPtrRef->Get());
	
	UOculusSceneAnchorComponent* sceneAnchorComponent = NewObject<UOculusSceneAnchorComponent>(newActor, sceneAnchorComponentInstanceClass);
	sceneAnchorComponent->CreationMethod = EComponentCreationMethod::Instance;
	newActor->AddOwnedComponent(sceneAnchorComponent);

	OculusAnchors::FOculusAnchors::SetAnchorComponentStatus(sceneAnchorComponent, EOculusSpaceComponentType::Locatable, true, 0.0f, FOculusAnchorSetComponentStatusDelegate());

	sceneAnchorComponent->RegisterComponent();
	sceneAnchorComponent->InitializeComponent();
	sceneAnchorComponent->Activate();
	sceneAnchorComponent->SetHandle(Space);
	sceneAnchorComponent->SemanticClassifications = SemanticClassifications;
	sceneAnchorComponent->ForceParallelToFloor = foundProperties->ForceParallelToFloor;
	sceneAnchorComponent->AddOffset = foundProperties->AddOffset;

	// Setup scale based on bounded size and the actual size of the mesh
	UStaticMesh* staticMesh = newActor->GetStaticMeshComponent()->GetStaticMesh();
	FBoxSphereBounds staticMeshBounds;
	staticMeshBounds.BoxExtent = FVector{1.f, 1.f, 1.f};
	if (staticMesh)
	{
		staticMeshBounds = staticMesh->GetBounds();
	}
	const float worldToMeters = GetWorld()->GetWorldSettings()->WorldToMeters;

	newActor->SetActorScale3D(FVector(
		(BoundedSize.X / (staticMeshBounds.BoxExtent.X * 2.f)) * worldToMeters,
		(BoundedSize.Y / (staticMeshBounds.BoxExtent.Y * 2.f)) * worldToMeters,
		(BoundedSize.Z / (staticMeshBounds.BoxExtent.Z * 2.f)) * worldToMeters)
	);

	return true;
}*/

bool AOculusSceneActor::IsScenePopulated()
{
	if (!RootComponent)
	{
		return false;
	}
	return RootComponent->GetNumChildrenComponents() > 0;
}

bool AOculusSceneActor::IsRoomLayoutValid()
{
	return bRoomLayoutIsValid;
}

void AOculusSceneActor::PopulateScene()
{
	if (!RootComponent)
	{
		return;
	}

	const EOculusResult::Type result = QueryAllRooms();
	if (!UOculusAnchorBPFunctionLibrary::IsAnchorResultSuccess(result))
	{
		UE_LOG(LogOculusScene, Error, TEXT("PopulateScene Failed to query available rooms"));
	}
}

void AOculusSceneActor::ClearScene()
{
	if (!RootComponent)
		return;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* SceneComponent : childrenComponents)
	{
		Cast<AActor>(SceneComponent->GetOuter())->Destroy();
	}

	bRoomLayoutIsValid = false;
	bFoundCapturedScene = false;
}

void AOculusSceneActor::SetVisibilityToAllSceneAnchors(const bool bIsVisible)
{
	if (!RootComponent)
		return;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* sceneComponent : childrenComponents)
	{
		sceneComponent->SetVisibility(bIsVisible);
	}
}

void AOculusSceneActor::SetVisibilityToSceneAnchorsBySemanticLabel(const FString SemanticLabel, const bool bIsVisible)
{
	if (!RootComponent)
		return;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* sceneComponent : childrenComponents)
	{
		UObject* outerObject = sceneComponent->GetOuter();
		if (!outerObject)
		{
			continue;
		}

		AActor* outerActor = Cast<AActor>(outerObject);
		if (!outerActor)
		{
			continue;
		}

		UActorComponent* sceneAnchorComponent = outerActor->GetComponentByClass(UOculusSceneAnchorComponent::StaticClass());
		if (!sceneAnchorComponent)
		{
			continue;
		}
		
		if (Cast<UOculusSceneAnchorComponent>(sceneAnchorComponent)->SemanticClassifications.Contains(SemanticLabel))
		{
			sceneComponent->SetVisibility(bIsVisible);
		}
	}
}

TArray<AActor*> AOculusSceneActor::GetActorsBySemanticLabel(const FString SemanticLabel)
{
	FString label = SemanticLabel;
	if (SemanticLabel == TEXT("DESK"))
	{
		label = TEXT("TABLE");
		UE_LOG(LogOculusScene, Warning, TEXT("VR Scene Actor semantic label 'DESK' is deprecated, use 'TABLE' instead."));
	}

	TArray<AActor*> actors;

	if (!RootComponent)
		return actors;

	TArray<USceneComponent*> childrenComponents = RootComponent->GetAttachChildren();
	for (USceneComponent* sceneComponent : childrenComponents)
	{
		UObject* outerObject = sceneComponent->GetOuter();
		if (!outerObject)
		{
			continue;
		}

		AActor* outerActor = Cast<AActor>(outerObject);
		if (!outerActor)
		{
			continue;
		}

		UActorComponent* sceneAnchorComponent = outerActor->GetComponentByClass(UOculusSceneAnchorComponent::StaticClass());
		if (!sceneAnchorComponent)
		{
			continue;
		}

		if (Cast<UOculusSceneAnchorComponent>(sceneAnchorComponent)->SemanticClassifications.Contains(SemanticLabel))
		{
			actors.Add(outerActor);
		}
	}

	return actors;
}

void AOculusSceneActor::SceneCaptureComplete_Handler(FUInt64 RequestId, bool bResult)
{	
	if (!bResult)
	{
		UE_LOG(LogOculusScene, Error, TEXT("Scene Capture Complete failed!"));
		return;
	}

	// Mark that we already launched Capture Flow and try to query spatial anchors again
	bCaptureFlowWasLaunched = true;

	ClearScene();
	PopulateScene();
}

#undef LOCTEXT_NAMESPACE
