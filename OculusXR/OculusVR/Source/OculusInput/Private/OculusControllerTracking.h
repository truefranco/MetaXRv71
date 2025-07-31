// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusInputFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "OculusControllerTracking"

DEFINE_LOG_CATEGORY_STATIC(LogOcControllerTracking, Log, All);

//-------------------------------------------------------------------------------------------------
// FOculusControllerTracking
//-------------------------------------------------------------------------------------------------
namespace OculusInput
{
class FOculusControllerTracking
{
public:
	static void PlayHapticEffect(
		class UHapticFeedbackEffect_Base* HapticEffect,
		EControllerHand Hand,
		EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand,
		bool bAppend = false,
		float Scale = 1.f,
		bool bLoop = false);

	static int PlayHapticEffect(EControllerHand Hand, TArray<uint8>& Amplitudes, int SampleRate, bool bPCM = false, bool bAppend = false);

	static void StopHapticEffect(EControllerHand Hand, EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand);

	static void SetHapticsByValue(float Frequency, float Amplitude, EControllerHand Hand, EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand);

	static float GetControllerSampleRateHz(EControllerHand Hand);

	static int GetMaxHapticDuration(EControllerHand Hand);
};

} // namespace OculusInput

#undef LOCTEXT_NAMESPACE
