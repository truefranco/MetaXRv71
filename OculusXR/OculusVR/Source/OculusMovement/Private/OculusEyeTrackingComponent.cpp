/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "OculusEyeTrackingComponent.h"

#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "OculusHMDPrivate.h"
#include "OculusPluginWrapper.h"
#include "OculusMovementFunctionLibrary.h"
#include "OculusMovementHelpers.h"
#include "OculusMovementLog.h"

int UOculusEyeTrackingComponent::TrackingInstanceCount = 0;

UOculusEyeTrackingComponent::UOculusEyeTrackingComponent()
	: TargetMeshComponentName(NAME_None)
	, bUpdatePosition(true)
	, bUpdateRotation(true)
	, ConfidenceThreshold(0.f)
	, bAcceptInvalid(false)
	, WorldToMeters(100.f)
	, TargetPoseableMeshComponent(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	EyeToBone.Add(EOculusEye::Left, "LeftEye");
	EyeToBone.Add(EOculusEye::Right, "RightEye");
}

void UOculusEyeTrackingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!UOculusMovementFunctionLibrary::IsEyeTrackingSupported())
	{
		// Early exit if eye tracking isn't supported
		UE_LOG(LogOculusMovement, Warning, TEXT("Eye tracking is not supported. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	// Try & check initializing the eye data
	if (!InitializeEyes())
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Failed to initialize eye tracking data. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
	}

	if (!UOculusMovementFunctionLibrary::StartEyeTracking())
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Failed to start eye tracking. (%s: %s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}
	++TrackingInstanceCount;
}

void UOculusEyeTrackingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsComponentTickEnabled())
	{
		if (--TrackingInstanceCount == 0)
		{
			if (!UOculusMovementFunctionLibrary::StopEyeTracking())
			{
				UE_LOG(LogOculusMovement, Warning, TEXT("Failed to stop eye tracking. (%s: %s)"), *GetOwner()->GetName(), *GetName());
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UOculusEyeTrackingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(TargetPoseableMeshComponent))
	{
		UE_LOG(LogOculusMovement, VeryVerbose, TEXT("No target mesh specified. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	FOculusEyeGazesState EyeGazesState;

	if (UOculusMovementFunctionLibrary::TryGetEyeGazesState(EyeGazesState, WorldToMeters))
	{
		for (uint8 i = 0u; i < static_cast<uint8>(EOculusEye::COUNT); ++i)
		{
			if (PerEyeData[i].EyeIsMapped)
			{
				const auto& Bone = PerEyeData[i].MappedBoneName;
				const auto& EyeGaze = EyeGazesState.EyeGazes[i];
				if ((bAcceptInvalid || EyeGaze.bIsValid) && (EyeGaze.Confidence >= ConfidenceThreshold))
				{
					int32 BoneIndex = TargetPoseableMeshComponent->GetBoneIndex(Bone);
					FTransform CurrentTransform = TargetPoseableMeshComponent->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace);

					if (bUpdatePosition)
					{
						CurrentTransform.SetLocation(EyeGaze.Position);
					}

					if (bUpdateRotation)
					{
						CurrentTransform.SetRotation(EyeGaze.Orientation.Quaternion() * PerEyeData[i].InitialRotation);
					}

					TargetPoseableMeshComponent->SetBoneTransformByName(Bone, CurrentTransform, EBoneSpaces::ComponentSpace);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogOculusMovement, VeryVerbose, TEXT("Failed to get Eye state from EyeTrackingComponent. (%s:%s)"), *GetOwner()->GetName(), *GetName());
	}
}

void UOculusEyeTrackingComponent::ClearRotationValues()
{
	if (!IsValid(TargetPoseableMeshComponent))
	{
		UE_LOG(LogOculusMovement, VeryVerbose, TEXT("No target mesh specified. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		return;
	}

	for (uint8 i = 0u; i < static_cast<uint8>(EOculusEye::COUNT); ++i)
	{
		if (PerEyeData[i].EyeIsMapped)
		{
			const auto& Bone = PerEyeData[i].MappedBoneName;

			int32 BoneIndex = TargetPoseableMeshComponent->GetBoneIndex(Bone);
			FTransform CurrentTransform = TargetPoseableMeshComponent->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace);

			CurrentTransform.SetRotation(PerEyeData[i].InitialRotation);

			TargetPoseableMeshComponent->SetBoneTransformByName(Bone, CurrentTransform, EBoneSpaces::ComponentSpace);
		}
	}
}

bool UOculusEyeTrackingComponent::InitializeEyes()
{
	bool bIsAnythingMapped = false;

	TargetPoseableMeshComponent = OculusUtility::FindComponentByName<UPoseableMeshComponent>(GetOwner(), TargetMeshComponentName);

	if (!IsValid(TargetPoseableMeshComponent))
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Could not find mesh with name (%s) for component. (%s:%s)"), *TargetMeshComponentName.ToString(), *GetOwner()->GetName(), *GetName());
		return false;
	}

	for (uint8 i = 0u; i < static_cast<uint8>(EOculusEye::COUNT); ++i)
	{
		const EOculusEye Eye = static_cast<EOculusEye>(i);
		const FName* BoneNameForThisEye = EyeToBone.Find(Eye);
		PerEyeData[i].EyeIsMapped = (nullptr != BoneNameForThisEye);

		if (PerEyeData[i].EyeIsMapped)
		{
			int32 BoneIndex = TargetPoseableMeshComponent->GetBoneIndex(*BoneNameForThisEye);
			if (BoneIndex == INDEX_NONE)
			{
				PerEyeData[i].EyeIsMapped = false; // Eye is explicitly mapped to a bone. But the bone name doesn't exist.
				UE_LOG(LogOculusMovement, Warning, TEXT("Could not find bone by name (%s) in mesh %s. (%s:%s)"), *BoneNameForThisEye->ToString(), *TargetPoseableMeshComponent->GetName(), *GetOwner()->GetName(), *GetName());
			}
			else
			{
				PerEyeData[i].MappedBoneName = *BoneNameForThisEye;
				PerEyeData[i].InitialRotation = TargetPoseableMeshComponent->GetBoneTransformByName(*BoneNameForThisEye, EBoneSpaces::ComponentSpace).GetRotation();
				bIsAnythingMapped = true;
			}
		}
		else
		{
			UE_LOG(LogOculusMovement, Display, TEXT("Eye (%s) is not mapped to any bone on mesh (%s)"), *StaticEnum<EOculusEye>()->GetValueAsString(Eye), *TargetPoseableMeshComponent->GetName());
		}
	}

	if (!bIsAnythingMapped)
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Component name -- %s:%s, doesn't have a valid configuration."), *GetOwner()->GetName(), *GetName());
	}

	if (!OculusHMD::GetUnitScaleFactorFromSettings(GetWorld(), WorldToMeters))
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Cannot get world settings. (%s:%s)"), *GetOwner()->GetName(), *GetName());
	}

	return bIsAnythingMapped;
}
