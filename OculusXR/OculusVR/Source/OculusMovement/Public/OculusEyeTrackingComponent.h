/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PoseableMeshComponent.h"

#include "OculusMovementTypes.h"

#include "OculusEyeTrackingComponent.generated.h"

struct FOculusEyeTrackingData
{
public:
	FOculusEyeTrackingData()
		: EyeIsMapped(false)
		, MappedBoneName(NAME_None)
	{
	}

	bool EyeIsMapped;
	FName MappedBoneName;
	FQuat InitialRotation;
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Oculus Eye Tracking Component"), ClassGroup = OculusHMD)
class OCULUSMOVEMENT_API UOculusEyeTrackingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOculusEyeTrackingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Reset the rotation values of the eyes to their initial rotation
	*/
	UFUNCTION(BlueprintCallable, Category = "Oculus|Movement")
	void ClearRotationValues();

	/**
	 * The name of the poseable mesh component that this component targets for eyes glazes movement.
	 * This must be the name of a component on this actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oculus|Movement")
	FName TargetMeshComponentName;

	/**
	 * The map of eye to mesh bone that this component supports.
	 * Names are validated on (@see BeginPlay) so only valid bone names will be targeted.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oculus|Movement")
	TMap<EOculusEye, FName> EyeToBone;

	/**
	 * Update the target mesh position when eye state changes
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Movement")
	bool bUpdatePosition;

	/**
	 * Update the target mesh rotation when eye state changes
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Movement")
	bool bUpdateRotation;

	/**
	 * Do not accept an eye gaze state if confidence is lower than this value. Confidence is in range [0,1].
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Movement")
	float ConfidenceThreshold;

	/**
	 * Bypass eye gaze state validity.
	 * 
	 * @Note: It doesn't check the confidence (@see ConfidenceThreshold). The eye gaze state can be marked as invalid. This flag bypass that state flag.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Movement")
	bool bAcceptInvalid;

private:
	bool InitializeEyes();

	// One meter in unreal world units.
	float WorldToMeters;

	// Per eye, eye tracking data
	TStaticArray<FOculusEyeTrackingData, static_cast<uint32>(EOculusEye::COUNT)> PerEyeData;

	// The mesh component targeted for eyes
	UPROPERTY()
	UPoseableMeshComponent* TargetPoseableMeshComponent;

	// Stop the tracker just once.
	static int TrackingInstanceCount;
};
