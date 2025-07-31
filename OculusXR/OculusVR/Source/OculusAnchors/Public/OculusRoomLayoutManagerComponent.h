/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "OculusSpatialAnchorComponent.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "OculusRoomLayoutManagerComponent.generated.h"

UCLASS(meta = (DisplayName = "Oculus Room Layout Manager Component", BlueprintSpawnableComponent))
class OCULUSANCHORS_API UOculusRoomLayoutManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOculusRoomLayoutManagerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOculusRoomLayoutSceneCaptureCompleteDelegate,
		FUInt64, requestId,
		bool, result);

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOculusRoomLayoutSceneCompleteNativeDelegate, FUInt64 /*requestId*/, bool /*success*/);
	FOculusRoomLayoutSceneCompleteNativeDelegate OculusRoomLayoutSceneCaptureCompleteNative;

	UPROPERTY(BlueprintAssignable, Category= "Oculus|Room Layout Manager")
	FOculusRoomLayoutSceneCaptureCompleteDelegate OculusRoomLayoutSceneCaptureComplete;

	// Requests to launch Capture Flow
	UFUNCTION(BlueprintCallable, Category = "Oculus|Room Layout Manager")
	bool LaunchCaptureFlow();

	// Gets room layout for a specific space
	UFUNCTION(BlueprintCallable, Category = "Oculus|Room Layout Manager")
	bool GetRoomLayout(FUInt64 Space, UPARAM(ref) FOculusRoomLayout& RoomLayoutOut, int32 MaxWallsCapacity = 64);

	// Loads mesh data (vertices, indeces) associated with the space into UProceduralMeshComponent
	UFUNCTION(BlueprintCallable, Category = "OculusVR|Room Layout Manager")
	bool LoadTriangleMesh(FUInt64 Space, class UProceduralMeshComponent* Mesh, bool CreateCollision) const;

protected:
	UPROPERTY(Transient)
	TSet<uint64> EntityRequestList;

	UPROPERTY(Transient)
	TMap<FUInt64, FOculusRoomLayout> RoomLayouts;

private:
	UFUNCTION()
	void OculusRoomLayoutSceneCaptureComplete_Handler(FUInt64 RequestId, bool bSuccess)
	{
		if (EntityRequestList.Find(RequestId.Value) != nullptr)
		{
			OculusRoomLayoutSceneCaptureComplete.Broadcast(RequestId, bSuccess);
			OculusRoomLayoutSceneCaptureCompleteNative.Broadcast(RequestId, bSuccess);
			EntityRequestList.Remove(RequestId.Value);
		}
	}
};
