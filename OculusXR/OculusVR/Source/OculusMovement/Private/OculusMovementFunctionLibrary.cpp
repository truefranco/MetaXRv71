/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusMovementFunctionLibrary.h"
#include "OculusHMDPrivate.h"
#include "OculusMovement.h"
#include "OculusHMD.h"

bool UOculusMovementFunctionLibrary::TryGetBodyState(FOculusBodyState& outBodyState, float WorldToMeters)
{
	return OculusMovement::GetBodyState(outBodyState, WorldToMeters);
}

bool UOculusMovementFunctionLibrary::IsBodyTrackingEnabled()
{
	return OculusMovement::IsBodyTrackingEnabled();
}

bool UOculusMovementFunctionLibrary::IsBodyTrackingSupported()
{
	return OculusMovement::IsBodyTrackingSupported();
}

bool UOculusMovementFunctionLibrary::RequestBodyTrackingFidelity(EOculusHMDBodyTrackingFidelity fidelity)
{
	return OculusMovement::RequestBodyTrackingFidelity(fidelity);
}

bool UOculusMovementFunctionLibrary::ResetBodyTrackingCalibration()
{
	return OculusMovement::ResetBodyTrackingCalibration();
}

bool UOculusMovementFunctionLibrary::SuggestBodyTrackingCalibrationOverride(float height)
{
	return OculusMovement::SuggestBodyTrackingCalibrationOverride(height);
}

bool UOculusMovementFunctionLibrary::StartBodyTrackingByJointSet(EOculusHMDBodyJointSet jointSet)
{
	return OculusMovement::StartBodyTrackingByJointSet(jointSet);
}

bool UOculusMovementFunctionLibrary::StartBodyTracking()
{
	return OculusMovement::StartBodyTracking();
}

bool UOculusMovementFunctionLibrary::StopBodyTracking()
{
	return OculusMovement::StopBodyTracking();
}

bool UOculusMovementFunctionLibrary::TryGetFaceState(FOculusFaceState& outFaceState)
{
	return OculusMovement::GetFaceState(outFaceState);
}

bool UOculusMovementFunctionLibrary::IsFaceTrackingEnabled()
{
	return OculusMovement::IsFaceTrackingEnabled();
}

bool UOculusMovementFunctionLibrary::IsFaceTrackingSupported()
{
	return OculusMovement::IsFaceTrackingSupported();
}

bool UOculusMovementFunctionLibrary::StartFaceTracking()
{
	return OculusMovement::StartFaceTracking();
}

bool UOculusMovementFunctionLibrary::StopFaceTracking()
{
	return OculusMovement::StopFaceTracking();
}

bool UOculusMovementFunctionLibrary::TryGetEyeGazesState(FOculusEyeGazesState& outEyeGazesState, float WorldToMeters)
{
	return OculusMovement::GetEyeGazesState(outEyeGazesState, WorldToMeters);
}

bool UOculusMovementFunctionLibrary::IsEyeTrackingEnabled()
{
	return OculusMovement::IsEyeTrackingEnabled();
}

bool UOculusMovementFunctionLibrary::IsEyeTrackingSupported()
{
	return OculusMovement::IsEyeTrackingSupported();
}

bool UOculusMovementFunctionLibrary::StartEyeTracking()
{
	return OculusMovement::StartEyeTracking();
}

bool UOculusMovementFunctionLibrary::StopEyeTracking()
{
	return OculusMovement::StopEyeTracking();
}
