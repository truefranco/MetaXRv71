/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "OculusRoomLayoutManagerComponent.h"
#include "OculusHMD.h"
#include "OculusRoomLayoutManager.h"
#include "OculusAnchorDelegates.h"
#include "OculusAnchorManager.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "OculusAnchorsModule.h"

UOculusRoomLayoutManagerComponent::UOculusRoomLayoutManagerComponent(const FObjectInitializer& ObjectInitializer)
{
	bWantsInitializeComponent = true; // so that InitializeComponent() gets called
}

void UOculusRoomLayoutManagerComponent::OnRegister()
{
	Super::OnRegister();

	FOculusAnchorEventDelegates::OculusSceneCaptureComplete.AddUObject(this, &UOculusRoomLayoutManagerComponent::OculusRoomLayoutSceneCaptureComplete_Handler);
}

void UOculusRoomLayoutManagerComponent::OnUnregister()
{
	Super::OnUnregister();

	FOculusAnchorEventDelegates::OculusSceneCaptureComplete.RemoveAll(this);
}

void UOculusRoomLayoutManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UOculusRoomLayoutManagerComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

bool UOculusRoomLayoutManagerComponent::LaunchCaptureFlow()
{
	uint64 OutRequest = 0;
	const bool bSuccess = OculusAnchors::FOculusRoomLayoutManager::RequestSceneCapture(OutRequest);
	if (bSuccess)
	{
		EntityRequestList.Add(OutRequest);
	}
	
	UE_LOG(LogOculusAnchors, Verbose, TEXT("Launch capture flow -- RequestSceneCapture -- %d"), bSuccess);
	
	return bSuccess;
}

bool UOculusRoomLayoutManagerComponent::GetRoomLayout(FUInt64 Space, FOculusRoomLayout& RoomLayoutOut, int32 MaxWallsCapacity)
{
	return UOculusAnchorBPFunctionLibrary::GetRoomLayout(Space, RoomLayoutOut, MaxWallsCapacity);
}

bool UOculusRoomLayoutManagerComponent::LoadTriangleMesh(FUInt64 Space, UProceduralMeshComponent* Mesh, bool CreateCollision) const
{
	ensure(Mesh);
	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	bool Success = OculusAnchors::FOculusRoomLayoutManager::GetSpaceTriangleMesh(Space, Vertices, Triangles);
	if (!Success)
	{
		return false;
	}

	// Mesh->bUseAsyncCooking = true;
	TArray<FVector> EmptyNormals;
	TArray<FVector2D> EmptyUV;
	TArray<FColor> EmptyVertexColors;
	TArray<FProcMeshTangent> EmptyTangents;
	Mesh->CreateMeshSection(0, Vertices, Triangles, EmptyNormals, EmptyUV, EmptyVertexColors, EmptyTangents, CreateCollision);

	return true;
}