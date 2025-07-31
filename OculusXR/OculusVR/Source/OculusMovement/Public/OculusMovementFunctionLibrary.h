/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "OculusMovementTypes.h"

#include "OculusMovementFunctionLibrary.generated.h"

UCLASS()
class OCULUSMOVEMENT_API UOculusMovementFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Oculus|Body")
	static bool TryGetBodyState(FOculusBodyState& outBodyState, float WorldToMeters = 100.0f);

	UFUNCTION(BlueprintPure, Category = "Oculus|Body")
	static bool IsBodyTrackingEnabled();

	UFUNCTION(BlueprintPure, Category = "Oculus|Body")
	static bool IsBodyTrackingSupported();

	UFUNCTION(BlueprintCallable, Category = "OculusXR|Body")
	static bool RequestBodyTrackingFidelity(EOculusHMDBodyTrackingFidelity fidelity);

	UFUNCTION(BlueprintCallable, Category = "OculusXR|Body")
	static bool ResetBodyTrackingCalibration();

	UFUNCTION(BlueprintCallable, Category = "OculusXR|Body")
	static bool SuggestBodyTrackingCalibrationOverride(float height);

	UFUNCTION(BlueprintCallable, Category = "OculusXR|Body")
	static bool StartBodyTrackingByJointSet(EOculusHMDBodyJointSet jointSet);

	UFUNCTION(BlueprintCallable, Category = "Oculus|Body")
	static bool StartBodyTracking();

	UFUNCTION(BlueprintCallable, Category = "Oculus|Body")
	static bool StopBodyTracking();

	UFUNCTION(BlueprintPure, Category = "Oculus|Face")
	static bool TryGetFaceState(FOculusFaceState& outFaceState);

	UFUNCTION(BlueprintPure, Category = "Oculus|Face")
	static bool IsFaceTrackingEnabled();

	UFUNCTION(BlueprintPure, Category = "Oculus|Face")
	static bool IsFaceTrackingSupported();

	UFUNCTION(BlueprintPure, Category = "Oculus|Face")
	static bool StartFaceTracking();

	UFUNCTION(BlueprintPure, Category = "Oculus|Face")
	static bool StopFaceTracking();

	UFUNCTION(BlueprintPure, Category = "Oculus|Eyes")
	static bool TryGetEyeGazesState(FOculusEyeGazesState& outEyeGazesState, float WorldToMeters = 100.0f);

	UFUNCTION(BlueprintPure, Category = "Oculus|Eyes")
	static bool IsEyeTrackingEnabled();

	UFUNCTION(BlueprintPure, Category = "Oculus|Eyes")
	static bool IsEyeTrackingSupported();

	UFUNCTION(BlueprintPure, Category = "Oculus|Eyes")
	static bool StartEyeTracking();

	UFUNCTION(BlueprintPure, Category = "Oculus|Eyes")
	static bool StopEyeTracking();
};
