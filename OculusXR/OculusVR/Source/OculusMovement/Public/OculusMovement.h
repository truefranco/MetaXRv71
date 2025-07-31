/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "OculusHMD.h"
#include "OculusMovementTypes.h"

struct OCULUSMOVEMENT_API OculusMovement
{
	static bool GetBodyState(FOculusBodyState& outOculusBodyState, float WorldToMeters = 100.0f);
	static bool IsBodyTrackingEnabled();
	static bool IsBodyTrackingSupported();
	static bool StartBodyTracking();
	static bool StopBodyTracking();
	static bool StartBodyTrackingByJointSet(EOculusHMDBodyJointSet jointSet);
	static bool RequestBodyTrackingFidelity(EOculusHMDBodyTrackingFidelity fidelity);
	static bool ResetBodyTrackingCalibration();
	static bool SuggestBodyTrackingCalibrationOverride(float height);

private:
	static bool IsFullBodyTrackingEnabled();
public:
	static bool GetFaceState(FOculusFaceState& outOculusFaceState);
	static bool IsFaceTrackingEnabled();
	static bool IsFaceTrackingSupported();
	static bool StartFaceTracking();
	static bool StopFaceTracking();

	static bool GetEyeGazesState(FOculusEyeGazesState& outOculusEyeGazesState, float WorldToMeters = 100.0f);
	static bool IsEyeTrackingEnabled();
	static bool IsEyeTrackingSupported();
	static bool StartEyeTracking();
	static bool StopEyeTracking();
};
