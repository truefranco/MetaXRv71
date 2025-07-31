// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusInputFunctionLibrary.h"
#include "OculusHandTracking.h"
#include "OculusControllerTracking.h"
#include "Logging/MessageLog.h"
#include "Haptics/HapticFeedbackEffect_Buffer.h"
#include "Haptics/HapticFeedbackEffect_Curve.h"
#include "Haptics/HapticFeedbackEffect_SoundWave.h"

//-------------------------------------------------------------------------------------------------
// UOculusHandTrackingFunctionLibrary
//-------------------------------------------------------------------------------------------------
UOculusInputFunctionLibrary::UOculusInputFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UOculusInputFunctionLibrary::FHandMovementFilterDelegate UOculusInputFunctionLibrary::HandMovementFilter;

EOculusFinger UOculusInputFunctionLibrary::ConvertBoneToFinger(const EBone Bone)
{
	switch (Bone)
	{
	case EBone::Index_1:
	case EBone::Index_2:
	case EBone::Index_3:
	case EBone::Index_Tip:
		return EOculusFinger::Index;
	case EBone::Middle_1:
	case EBone::Middle_2:
	case EBone::Middle_3:
	case EBone::Middle_Tip:
		return EOculusFinger::Middle;
	case EBone::Pinky_0:
	case EBone::Pinky_1:
	case EBone::Pinky_2:
	case EBone::Pinky_3:
	case EBone::Pinky_Tip:
		return EOculusFinger::Pinky;
	case EBone::Ring_1:
	case EBone::Ring_2:
	case EBone::Ring_3:
	case EBone::Ring_Tip:
		return EOculusFinger::Ring;
	case EBone::Thumb_0:
	case EBone::Thumb_1:
	case EBone::Thumb_2:
	case EBone::Thumb_3:
	case EBone::Thumb_Tip:
		return EOculusFinger::Thumb;
	default:
		return EOculusFinger::Invalid;
	}
}

ETrackingConfidence UOculusInputFunctionLibrary::GetFingerTrackingConfidence(const EOculusHandType DeviceHand, const EOculusFinger Finger, const int32 ControllerIndex)
{
	return OculusInput::FOculusHandTracking::GetFingerTrackingConfidence(ControllerIndex, DeviceHand, (OculusInput::EOculusHandAxes)(uint8)Finger);
}

bool UOculusInputFunctionLibrary::GetHandSkeletalMesh(USkeletalMesh* HandSkeletalMesh, EOculusHandType SkeletonType, EOculusHandType MeshType, float WorldToMeters)
{
	return OculusInput::FOculusHandTracking::GetHandSkeletalMesh(HandSkeletalMesh, SkeletonType, MeshType, WorldToMeters);
}

TArray<FOculusCapsuleCollider> UOculusInputFunctionLibrary::InitializeHandPhysics(EOculusHandType SkeletonType, USkinnedMeshComponent* HandComponent, const float WorldToMeters)
{
	return OculusInput::FOculusHandTracking::InitializeHandPhysics(SkeletonType, HandComponent, WorldToMeters);
}

FQuat UOculusInputFunctionLibrary::GetBoneRotation(const EOculusHandType DeviceHand, const EBone BoneId, const int32 ControllerIndex)
{
	return OculusInput::FOculusHandTracking::GetBoneRotation(ControllerIndex, DeviceHand, BoneId);
}

ETrackingConfidence UOculusInputFunctionLibrary::GetTrackingConfidence(const EOculusHandType DeviceHand, const int32 ControllerIndex)
{
	return OculusInput::FOculusHandTracking::GetTrackingConfidence(ControllerIndex, DeviceHand);
}

FTransform UOculusInputFunctionLibrary::GetPointerPose(const EOculusHandType DeviceHand, const int32 ControllerIndex)
{
	return OculusInput::FOculusHandTracking::GetPointerPose(ControllerIndex, DeviceHand);
}

bool UOculusInputFunctionLibrary::IsPointerPoseValid(const EOculusHandType DeviceHand, const int32 ControllerIndex)
{
	return OculusInput::FOculusHandTracking::IsPointerPoseValid(ControllerIndex, DeviceHand);
}

float UOculusInputFunctionLibrary::GetHandScale(const EOculusHandType DeviceHand, const int32 ControllerIndex)
{
	return OculusInput::FOculusHandTracking::GetHandScale(ControllerIndex, DeviceHand);
}

EOculusHandType UOculusInputFunctionLibrary::GetDominantHand(const int32 ControllerIndex)
{
	EOculusHandType DominantHand = EOculusHandType::None;
	if (OculusInput::FOculusHandTracking::IsHandDominant(ControllerIndex, EOculusHandType::HandLeft))
	{
		DominantHand = EOculusHandType::HandLeft;
	}
	else if (OculusInput::FOculusHandTracking::IsHandDominant(ControllerIndex, EOculusHandType::HandRight))
	{
		DominantHand = EOculusHandType::HandRight;
	}
	return DominantHand;
}

bool UOculusInputFunctionLibrary::IsHandTrackingEnabled()
{
	return OculusInput::FOculusHandTracking::IsHandTrackingEnabled();
}

bool UOculusInputFunctionLibrary::IsHandPositionValid(const EOculusHandType DeviceHand, const int32 ControllerIndex)
{
	return OculusInput::FOculusHandTracking::IsHandPositionValid(ControllerIndex, DeviceHand);
}

void UOculusInputFunctionLibrary::SetMultimodalHandsControllersSupported(const bool Supported)
{
	OculusInput::FOculusHandTracking::SetMultimodalHandsControllersSupported(Supported);
}

void UOculusInputFunctionLibrary::SetSimultaneousHandsAndControllersEnabled(const bool Enabled)
{
	OculusInput::FOculusHandTracking::SetSimultaneousHandsAndControllersEnabled(Enabled);
}

bool UOculusInputFunctionLibrary::GetControllerIsInHand(EOculusHandType DeviceHand)
{
	return OculusInput::FOculusHandTracking::GetControllerIsInHand(DeviceHand);
}

void UOculusInputFunctionLibrary::SetControllerDrivenHandPoses(EOculusXRControllerDrivenHandPoseTypes Type)
{
	OculusInput::FOculusHandTracking::SetControllerDrivenHandPoses(Type);
}

EOculusXRControllerDrivenHandPoseTypes UOculusInputFunctionLibrary::GetControllerDrivenHandPoses()
{
	return OculusInput::FOculusHandTracking::ControllerDrivenHandType;
}

FString UOculusInputFunctionLibrary::GetBoneName(EBone BoneId)
{
	uint32 ovrBoneId = OculusInput::FOculusHandTracking::ToOvrBone(BoneId);
	return OculusInput::FOculusHandTracking::GetBoneName(ovrBoneId);
}

void UOculusInputFunctionLibrary::PlayCurveHapticEffect(class UHapticFeedbackEffect_Curve* HapticEffect, EControllerHand Hand, EOculusHandHapticsLocation Location, float Scale, bool bLoop)
{
	OculusInput::FOculusControllerTracking::PlayHapticEffect(HapticEffect, Hand, Location, false, Scale, bLoop);
}

void UOculusInputFunctionLibrary::PlayBufferHapticEffect(class UHapticFeedbackEffect_Buffer* HapticEffect, EControllerHand Hand, EOculusHandHapticsLocation Location, float Scale, bool bLoop)
{
	OculusInput::FOculusControllerTracking::PlayHapticEffect(HapticEffect, Hand, Location, false, Scale, bLoop);
}

void UOculusInputFunctionLibrary::PlayAmplitudeEnvelopeHapticEffect(class UHapticFeedbackEffect_Buffer* HapticEffect, EControllerHand Hand)
{
	OculusInput::FOculusControllerTracking::PlayHapticEffect(Hand, HapticEffect->Amplitudes, HapticEffect->SampleRate, false, false);
}

void UOculusInputFunctionLibrary::PlaySoundWaveHapticEffect(class UHapticFeedbackEffect_SoundWave* HapticEffect, EControllerHand Hand, bool bAppend, float Scale, bool bLoop)
{
	OculusInput::FOculusControllerTracking::PlayHapticEffect(HapticEffect, Hand, EOculusHandHapticsLocation::Hand, bAppend, Scale, bLoop);
}

void UOculusInputFunctionLibrary::StopHapticEffect(EControllerHand Hand, EOculusHandHapticsLocation Location)
{
	OculusInput::FOculusControllerTracking::StopHapticEffect(Hand, Location);
}

void UOculusInputFunctionLibrary::SetHapticsByValue(const float Frequency, const float Amplitude, EControllerHand Hand, EOculusHandHapticsLocation Location)
{
	OculusInput::FOculusControllerTracking::SetHapticsByValue(Frequency, Amplitude, Hand, Location);
}

float UOculusInputFunctionLibrary::GetControllerSampleRateHz(EControllerHand Hand)
{
	return OculusInput::FOculusControllerTracking::GetControllerSampleRateHz(Hand);
}

int UOculusInputFunctionLibrary::GetMaxHapticDuration(EControllerHand Hand)
{
	return OculusInput::FOculusControllerTracking::GetMaxHapticDuration(Hand);
}