/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"

#include "OculusMovementTypes.h"

#include "OculusBodyTrackingComponent.generated.h"

UENUM(BlueprintType)
enum class EOculusBodyTrackingMode : uint8
{
	PositionAndRotation,
	RotationOnly,
	NoTracking
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Oculus Body Tracking Component"), ClassGroup = OculusHMD)
class OCULUSMOVEMENT_API UOculusBodyTrackingComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()
public:
	UOculusBodyTrackingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Restore all bones to their initial transforms
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|OculusFaceTracking")
	void ResetAllBoneTransforms();

	/**
	* How are the results of body tracking applied to the mesh.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Movement")
	EOculusBodyTrackingMode BodyTrackingMode;

	/**
	 * The bone name associated with each bone ID.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oculus|Movement")
	TMap<EOculusBoneID, FName> BoneNames;

	/**
	 * Do not apply body state to bones if confidence is lower than this value. Confidence is in range [0,1].
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Movement", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float ConfidenceThreshold;

private:
	bool InitializeBodyBones();

	// One meter in unreal world units.
	float WorldToMeters;

	// The index of each mapped bone after the discovery and association of bone names.
	TMap<EOculusBoneID, int32> MappedBoneIndices;

	// Saved body state.
	FOculusBodyState BodyState;

	// Stop the tracker just once.
	static int TrackingInstanceCount;
};
