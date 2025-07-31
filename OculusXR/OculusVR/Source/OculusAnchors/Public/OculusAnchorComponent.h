/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "OculusAnchorTypes.h"
#include "Components/ActorComponent.h"
#include "OculusAnchorComponent.generated.h"

UCLASS(meta = (DisplayName = "Oculus Anchor Component"))
class OCULUSANCHORS_API UOculusAnchorComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOculusAnchorComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Oculus|Anchor", meta = (DefaultToSelf = Target))
	FUInt64 GetHandle() const;

	UFUNCTION(BlueprintCallable, Category = "Oculus|Anchor", meta = (DefaultToSelf = Target))
	void SetHandle(FUInt64 Handle);

	UFUNCTION(BlueprintPure, Category = "Oculus|Anchor", meta = (DefaultToSelf = Target))
	bool HasValidHandle() const;

	UFUNCTION(BlueprintPure, Category = "Oculus|Anchor", meta = (DefaultToSelf = Target))
	FUUID GetUUID() const;

	void SetUUID(FUUID NewUUID);

	UFUNCTION(BlueprintPure, Category = "Oculus|Anchor", meta = (DefaultToSelf = Target))
	bool IsStoredAtLocation(EOculusSpaceStorageLocation Location) const;

	// Not exposed to BP because this is managed in code
	void SetStoredLocation(EOculusSpaceStorageLocation Location, bool Stored);

	UFUNCTION(BlueprintPure, Category = "Oculus|Anchor", meta = (DefaultToSelf = Target))
	bool IsSaved() const;

protected:
	bool bUpdateHeadSpaceTransform;

private:
	FUInt64 AnchorHandle;
	FUUID AnchorUUID;
	int32 StorageLocations;

	UPROPERTY()
	class APlayerCameraManager* PlayerCameraManager;

	void UpdateAnchorTransform() const;
	bool ToWorldSpacePose(FTransform CameraTransform, FTransform& OutTrackingSpaceTransform) const;
};