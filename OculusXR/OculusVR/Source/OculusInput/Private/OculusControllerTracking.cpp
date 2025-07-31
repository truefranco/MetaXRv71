// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusControllerTracking.h"
#include "OculusHMD.h"
#include "OculusInput.h"
#include "Misc/CoreDelegates.h"
#include "IOculusInputModule.h"

namespace OculusInput
{
	void FOculusControllerTracking::PlayHapticEffect(
		class UHapticFeedbackEffect_Base* HapticEffect,
		EControllerHand Hand,
		EOculusHandHapticsLocation Location,
		bool bAppend,
		float Scale,
		bool bLoop)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		TSharedPtr<FOculusInput> OculusInputModule = StaticCastSharedPtr<FOculusInput>(IOculusInputModule::Get().GetInputDevice());
		OculusInputModule.Get()->PlayHapticEffect(HapticEffect, Hand, Location, bAppend, Scale, bLoop);
#endif
	}

	int FOculusControllerTracking::PlayHapticEffect(EControllerHand Hand, TArray<uint8>& Amplitudes, int SampleRate, bool bPCM, bool bAppend)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		TSharedPtr<FOculusInput> OculusInputModule = StaticCastSharedPtr<FOculusInput>(IOculusInputModule::Get().GetInputDevice());
		return OculusInputModule.Get()->PlayHapticEffect(Hand, Amplitudes.Num(), Amplitudes.GetData(), SampleRate, bPCM, bAppend);
#endif
	}

	void FOculusControllerTracking::StopHapticEffect(EControllerHand Hand, EOculusHandHapticsLocation Location)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		SetHapticsByValue(0.f, 0.f, Hand, Location);
#endif
	}

	void FOculusControllerTracking::SetHapticsByValue(float Frequency, float Amplitude, EControllerHand Hand, EOculusHandHapticsLocation Location)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		TSharedPtr<FOculusInput> OculusInputModule = StaticCastSharedPtr<FOculusInput>(IOculusInputModule::Get().GetInputDevice());
		OculusInputModule.Get()->SetHapticsByValue(Frequency, Amplitude, Hand, Location);
#endif
	}

	float FOculusControllerTracking::GetControllerSampleRateHz(EControllerHand Hand)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		TSharedPtr<FOculusInput> OculusInputModule = StaticCastSharedPtr<FOculusInput>(IOculusInputModule::Get().GetInputDevice());
		return OculusInputModule.Get()->GetControllerSampleRateHz(Hand);
#endif
		return 0;
	}

	int FOculusControllerTracking::GetMaxHapticDuration(EControllerHand Hand)
	{
#if OCULUS_INPUT_SUPPORTED_PLATFORMS
		TSharedPtr<FOculusInput> OculusInputModule = StaticCastSharedPtr<FOculusInput>(IOculusInputModule::Get().GetInputDevice());
		return OculusInputModule.Get()->GetMaxHapticDuration(Hand);
#endif
		return 0;
	}
}
