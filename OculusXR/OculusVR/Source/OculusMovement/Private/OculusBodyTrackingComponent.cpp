/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "OculusBodyTrackingComponent.h"

#include "DrawDebugHelpers.h"
#include "OculusHMD.h"
#include "OculusPluginWrapper.h"
#include "OculusMovementFunctionLibrary.h"
#include "OculusMovementLog.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarOVRBodyDebugDraw(
	TEXT("ovr.BodyDebugDraw"),
	0,
	TEXT("Enables or disables debug drawing for body tracking.\n")
		TEXT("<=0: disabled (no drawing)\n")
			TEXT("  1: enabled  (debug drawing)\n"));
#endif

int UOculusBodyTrackingComponent::TrackingInstanceCount = 0;

UOculusBodyTrackingComponent::UOculusBodyTrackingComponent()
	: BodyTrackingMode(EOculusBodyTrackingMode::PositionAndRotation)
	, ConfidenceThreshold(0.f)
	, WorldToMeters(100.f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Setup defaults
	BoneNames.Add(EOculusBoneID::BodyRoot, "Root");
	BoneNames.Add(EOculusBoneID::BodyHips, "Hips");
	BoneNames.Add(EOculusBoneID::BodySpineLower, "SpineLower");
	BoneNames.Add(EOculusBoneID::BodySpineMiddle, "SpineMiddle");
	BoneNames.Add(EOculusBoneID::BodySpineUpper, "SpineUpper");
	BoneNames.Add(EOculusBoneID::BodyChest, "Chest");
	BoneNames.Add(EOculusBoneID::BodyNeck, "Neck");
	BoneNames.Add(EOculusBoneID::BodyHead, "Head");
	BoneNames.Add(EOculusBoneID::BodyLeftShoulder, "LeftShoulder");
	BoneNames.Add(EOculusBoneID::BodyLeftScapula, "LeftScapula");
	BoneNames.Add(EOculusBoneID::BodyLeftArmUpper, "LeftArmUpper");
	BoneNames.Add(EOculusBoneID::BodyLeftArmLower, "LeftArmLower");
	BoneNames.Add(EOculusBoneID::BodyLeftHandWristTwist, "LeftHandWristTwist");
	BoneNames.Add(EOculusBoneID::BodyRightShoulder, "RightShoulder");
	BoneNames.Add(EOculusBoneID::BodyRightScapula, "RightScapula");
	BoneNames.Add(EOculusBoneID::BodyRightArmUpper, "RightArmUpper");
	BoneNames.Add(EOculusBoneID::BodyRightArmLower, "RightArmLower");
	BoneNames.Add(EOculusBoneID::BodyRightHandWristTwist, "RightHandWristTwist");
	BoneNames.Add(EOculusBoneID::BodyLeftHandPalm, "LeftHandPalm");
	BoneNames.Add(EOculusBoneID::BodyLeftHandWrist, "LeftHandWrist");
	BoneNames.Add(EOculusBoneID::BodyLeftHandThumbMetacarpal, "LeftHandThumbMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandThumbProximal, "LeftHandThumbProximal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandThumbDistal, "LeftHandThumbDistal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandThumbTip, "LeftHandThumbTip");
	BoneNames.Add(EOculusBoneID::BodyLeftHandIndexMetacarpal, "LeftHandIndexMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandIndexProximal, "LeftHandIndexProximal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandIndexIntermediate, "LeftHandIndexIntermediate");
	BoneNames.Add(EOculusBoneID::BodyLeftHandIndexDistal, "LeftHandIndexDistal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandIndexTip, "LeftHandIndexTip");
	BoneNames.Add(EOculusBoneID::BodyLeftHandMiddleMetacarpal, "LeftHandMiddleMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandMiddleProximal, "LeftHandMiddleProximal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandMiddleIntermediate, "LeftHandMiddleIntermediate");
	BoneNames.Add(EOculusBoneID::BodyLeftHandMiddleDistal, "LeftHandMiddleDistal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandMiddleTip, "LeftHandMiddleTip");
	BoneNames.Add(EOculusBoneID::BodyLeftHandRingMetacarpal, "LeftHandRingMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandRingProximal, "LeftHandRingProximal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandRingIntermediate, "LeftHandRingIntermediate");
	BoneNames.Add(EOculusBoneID::BodyLeftHandRingDistal, "LeftHandRingDistal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandRingTip, "LeftHandRingTip");
	BoneNames.Add(EOculusBoneID::BodyLeftHandLittleMetacarpal, "LeftHandLittleMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandLittleProximal, "LeftHandLittleProximal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandLittleIntermediate, "LeftHandLittleIntermediate");
	BoneNames.Add(EOculusBoneID::BodyLeftHandLittleDistal, "LeftHandLittleDistal");
	BoneNames.Add(EOculusBoneID::BodyLeftHandLittleTip, "LeftHandLittleTip");
	BoneNames.Add(EOculusBoneID::BodyRightHandPalm, "RightHandPalm");
	BoneNames.Add(EOculusBoneID::BodyRightHandWrist, "RightHandWrist");
	BoneNames.Add(EOculusBoneID::BodyRightHandThumbMetacarpal, "RightHandThumbMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyRightHandThumbProximal, "RightHandThumbProximal");
	BoneNames.Add(EOculusBoneID::BodyRightHandThumbDistal, "RightHandThumbDistal");
	BoneNames.Add(EOculusBoneID::BodyRightHandThumbTip, "RightHandThumbTip");
	BoneNames.Add(EOculusBoneID::BodyRightHandIndexMetacarpal, "RightHandIndexMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyRightHandIndexProximal, "RightHandIndexProximal");
	BoneNames.Add(EOculusBoneID::BodyRightHandIndexIntermediate, "RightHandIndexIntermediate");
	BoneNames.Add(EOculusBoneID::BodyRightHandIndexDistal, "RightHandIndexDistal");
	BoneNames.Add(EOculusBoneID::BodyRightHandIndexTip, "RightHandIndexTip");
	BoneNames.Add(EOculusBoneID::BodyRightHandMiddleMetacarpal, "RightHandMiddleMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyRightHandMiddleProximal, "RightHandMiddleProximal");
	BoneNames.Add(EOculusBoneID::BodyRightHandMiddleIntermediate, "RightHandMiddleIntermediate");
	BoneNames.Add(EOculusBoneID::BodyRightHandMiddleDistal, "RightHandMiddleDistal");
	BoneNames.Add(EOculusBoneID::BodyRightHandMiddleTip, "RightHandMiddleTip");
	BoneNames.Add(EOculusBoneID::BodyRightHandRingMetacarpal, "RightHandRingMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyRightHandRingProximal, "RightHandRingProximal");
	BoneNames.Add(EOculusBoneID::BodyRightHandRingIntermediate, "RightHandRingIntermediate");
	BoneNames.Add(EOculusBoneID::BodyRightHandRingDistal, "RightHandRingDistal");
	BoneNames.Add(EOculusBoneID::BodyRightHandRingTip, "RightHandRingTip");
	BoneNames.Add(EOculusBoneID::BodyRightHandLittleMetacarpal, "RightHandLittleMetacarpal");
	BoneNames.Add(EOculusBoneID::BodyRightHandLittleProximal, "RightHandLittleProximal");
	BoneNames.Add(EOculusBoneID::BodyRightHandLittleIntermediate, "RightHandLittleIntermediate");
	BoneNames.Add(EOculusBoneID::BodyRightHandLittleDistal, "RightHandLittleDistal");
	BoneNames.Add(EOculusBoneID::BodyRightHandLittleTip, "RightHandLittleTip");
	BoneNames.Add(EOculusBoneID::BodyLeftUpperLeg, "LeftUpperLeg");
	BoneNames.Add(EOculusBoneID::BodyLeftLowerLeg, "LeftLowerLeg");
	BoneNames.Add(EOculusBoneID::BodyLeftFootAnkleTwist, "LeftFootAnkleTwist");
	BoneNames.Add(EOculusBoneID::BodyLeftFootAnkle, "LeftFootAnkle");
	BoneNames.Add(EOculusBoneID::BodyLeftFootSubtalar, "LeftFootSubtalar");
	BoneNames.Add(EOculusBoneID::BodyLeftFootTransverse, "LeftFootTransverse");
	BoneNames.Add(EOculusBoneID::BodyLeftFootBall, "LeftFootBall");
	BoneNames.Add(EOculusBoneID::BodyRightUpperLeg, "RightUpperLeg");
	BoneNames.Add(EOculusBoneID::BodyRightLowerLeg, "RightLowerLeg");
	BoneNames.Add(EOculusBoneID::BodyRightFootAnkleTwist, "RightFootAnkleTwist");
	BoneNames.Add(EOculusBoneID::BodyRightFootAnkle, "RightFootAnkle");
	BoneNames.Add(EOculusBoneID::BodyRightFootSubtalar, "RightFootSubtalar");
	BoneNames.Add(EOculusBoneID::BodyRightFootTransverse, "RightFootTransverse");
	BoneNames.Add(EOculusBoneID::BodyRightFootBall, "RightFootBall");
}

void UOculusBodyTrackingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!UOculusMovementFunctionLibrary::IsBodyTrackingSupported())
	{
		// Early exit if body tracking isn't supported
		UE_LOG(LogOculusMovement, Warning, TEXT("Body tracking is not supported. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	if (!OculusHMD::GetUnitScaleFactorFromSettings(GetWorld(), WorldToMeters))
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Cannot get world settings. (%s:%s)"), *GetOwner()->GetName(), *GetName());
	}

	if (!InitializeBodyBones())
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Failed to initialize body data. (%s: %s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	if (!UOculusMovementFunctionLibrary::StartBodyTracking())
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Failed to start body tracking. (%s: %s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}
	++TrackingInstanceCount;
}

void UOculusBodyTrackingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsComponentTickEnabled())
	{
		if (--TrackingInstanceCount == 0)
		{
			if (!UOculusMovementFunctionLibrary::StopBodyTracking())
			{
				UE_LOG(LogOculusMovement, Warning, TEXT("Failed to stop body tracking. (%s: %s)"), *GetOwner()->GetName(), *GetName());
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UOculusBodyTrackingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UOculusMovementFunctionLibrary::TryGetBodyState(BodyState, WorldToMeters))
	{
		if (BodyState.IsActive && BodyState.Confidence > ConfidenceThreshold)
		{
			for (int i = 0; i < BodyState.Joints.Num(); ++i)
			{
				const FOculusBodyJoint& Joint = BodyState.Joints[i];
				const FVector& Position = Joint.Position;
				const FRotator& Orientation = Joint.Orientation;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (CVarOVRBodyDebugDraw.GetValueOnGameThread() > 0)
				{
					const FTransform& ParentTransform = GetOwner()->GetActorTransform();

					FVector DebugPosition = ParentTransform.TransformPosition(Position);
					FRotator DebugOrientation = ParentTransform.TransformRotation(Orientation.Quaternion()).Rotator();

					DrawDebugLine(GetWorld(), DebugPosition, DebugPosition + DebugOrientation.Quaternion().GetUpVector(), FColor::Blue);
					DrawDebugLine(GetWorld(), DebugPosition, DebugPosition + DebugOrientation.Quaternion().GetForwardVector(), FColor::Red);
					DrawDebugLine(GetWorld(), DebugPosition, DebugPosition + DebugOrientation.Quaternion().GetRightVector(), FColor::Green);
				}
#endif

				int32* BoneIndex = MappedBoneIndices.Find(static_cast<EOculusBoneID>(i));
				if (BoneIndex != nullptr)
				{
					switch (BodyTrackingMode)
					{
						case EOculusBodyTrackingMode::PositionAndRotation:
							SetBoneTransformByName(BoneNames[static_cast<EOculusBoneID>(i)], FTransform(Orientation, Position), EBoneSpaces::ComponentSpace);
							break;
						case EOculusBodyTrackingMode::RotationOnly:
							SetBoneRotationByName(BoneNames[static_cast<EOculusBoneID>(i)], Orientation, EBoneSpaces::ComponentSpace);
							break;
						case EOculusBodyTrackingMode::NoTracking:
							break;
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogOculusMovement, Verbose, TEXT("Failed to get body state (%s:%s)."), *GetOwner()->GetName(), *GetName());
	}
}

void UOculusBodyTrackingComponent::ResetAllBoneTransforms()
{
	for (int i = 0; i < BodyState.Joints.Num(); ++i)
	{
		int32* BoneIndex = MappedBoneIndices.Find(static_cast<EOculusBoneID>(i));
		if (BoneIndex != nullptr)
		{
			ResetBoneTransformByName(BoneNames[static_cast<EOculusBoneID>(i)]);
		}
	}
}

bool UOculusBodyTrackingComponent::InitializeBodyBones()
{
	if (SkeletalMesh == nullptr)
	{
		UE_LOG(LogOculusMovement, Display, TEXT("No SkeletalMesh in this component."));
		return false;
	}

	for (const auto& it : BoneNames)
	{
		int32 BoneIndex = GetBoneIndex(it.Value);

		if (BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogOculusMovement, Display, TEXT("Could not find bone %s in skeletal mesh %s"), *StaticEnum<EOculusBoneID>()->GetValueAsString(it.Key), *SkeletalMesh->GetName());
		}
		else
		{
			MappedBoneIndices.Add(it.Key, BoneIndex);
		}
	}

	return true;
}
