/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "IEyeTrackerModule.h"
#include "EyeTrackerTypes.h"
#include "IEyeTracker.h"
#include "Modules/ModuleManager.h"

#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "IXRTrackingSystem.h"

#include "OculusHMDModule.h"
#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS

namespace OculusHMD
{
	class FOculusEyeTracker : public IEyeTracker
	{
	public:
		FOculusEyeTracker()
		{
			GetUnitScaleFactorFromSettings(GWorld, WorldToMeters);
			if (GEngine != nullptr)
			{
				OculusHMD = GEngine->XRSystem.Get();
			}
		}

		virtual ~FOculusEyeTracker()
		{
			if (bIsTrackerStarted)
			{
				ensureMsgf(OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StopEyeTracking()), TEXT("Cannot stop eye tracker."));
			}
		}

	private:
		// IEyeTracker
		virtual void SetEyeTrackedPlayer(APlayerController*) override
		{
			unimplemented();
		}

		virtual bool GetEyeTrackerGazeData(FEyeTrackerGazeData& OutGazeData) const override
		{
			return ReactOnEyeTrackerState([this, &OutGazeData](const ovrpEyeGazesState& OVREyeGazesState, const FTransform& TrackingToWorld) {
				OutGazeData.FixationPoint = GetFixationPoint(OVREyeGazesState);
				OutGazeData.ConfidenceValue = MergeConfidence(OVREyeGazesState);

				OutGazeData.GazeDirection = TrackingToWorld.TransformVector(MergeOrientation(OVREyeGazesState).GetForwardVector());
				OutGazeData.GazeOrigin = TrackingToWorld.TransformPosition(MergePosition(OVREyeGazesState) * WorldToMeters);
			});
		}

		virtual bool GetEyeTrackerStereoGazeData(FEyeTrackerStereoGazeData& OutGazeData) const override
		{
			return ReactOnEyeTrackerState([this, &OutGazeData](const ovrpEyeGazesState& OVREyeGazesState, const FTransform& TrackingToWorld) {
				OutGazeData.FixationPoint = GetFixationPoint(OVREyeGazesState);
				OutGazeData.ConfidenceValue = MergeConfidence(OVREyeGazesState);

				const auto& LeftEyePose = OVREyeGazesState.EyeGazes[ovrpEye_Left].Pose;
				const auto& RightEyePose = OVREyeGazesState.EyeGazes[ovrpEye_Right].Pose;
				OutGazeData.LeftEyeDirection = TrackingToWorld.TransformVector(OculusHMD::ToFQuat(LeftEyePose.Orientation).GetForwardVector());
				OutGazeData.RightEyeDirection = TrackingToWorld.TransformVector(OculusHMD::ToFQuat(RightEyePose.Orientation).GetForwardVector());
				OutGazeData.LeftEyeOrigin = TrackingToWorld.TransformPosition(OculusHMD::ToFVector(LeftEyePose.Position) * WorldToMeters);
				OutGazeData.RightEyeOrigin = TrackingToWorld.TransformPosition(OculusHMD::ToFVector(RightEyePose.Position) * WorldToMeters);
			});
		}

		virtual EEyeTrackerStatus GetEyeTrackerStatus() const override
		{
			ovrpBool IsSupported = ovrpBool_False;
			ovrpBool IsEnabled = ovrpBool_False;
			ovrpResult TrackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetEyeTrackingSupported(&IsSupported);
			ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetEyeTrackingEnabled(&IsEnabled);
			if (OVRP_SUCCESS(TrackingSupportedResult) && OVRP_SUCCESS(TrackingEnabledResult))
			{
				if ((IsSupported == ovrpBool_True) && (IsEnabled == ovrpBool_True))
				{
					return EEyeTrackerStatus::Tracking;
				}
				else if (IsSupported == ovrpBool_True)
				{
					return EEyeTrackerStatus::NotTracking;
				}
			}

			return EEyeTrackerStatus::NotConnected;
		}

		virtual bool IsStereoGazeDataAvailable() const override
		{
			return true;
		}

	private:
		// FOculusEyeTracker
		template <typename ReactOnState>
		bool ReactOnEyeTrackerState(ReactOnState&& React) const
		{
			if (!bIsTrackerStarted)
			{
				bIsTrackerStarted = OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartEyeTracking());
			}

			if (bIsTrackerStarted)
			{
				ovrpEyeGazesState OVREyeGazesState;
				ovrpResult OVREyeGazesStateResult = FOculusHMDModule::GetPluginWrapper().GetEyeGazesState(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, &OVREyeGazesState);
				checkf(OVREyeGazesStateResult != ovrpFailure_NotYetImplemented, TEXT("Eye tracking is not implemented on this platform."));

				if (OVRP_SUCCESS(OVREyeGazesStateResult) && IsStateValidForBothEyes(OVREyeGazesState))
				{
					FTransform TrackingToWorld = OculusHMD ? OculusHMD->GetTrackingToWorldTransform() : FTransform::Identity;
					React(OVREyeGazesState, TrackingToWorld);

					return true;
				}
			}

			return false;
		}

		static float IsStateValidForBothEyes(const ovrpEyeGazesState& OVREyeGazesState)
		{
			return OVREyeGazesState.EyeGazes[ovrpEye_Left].IsValid && OVREyeGazesState.EyeGazes[ovrpEye_Right].IsValid;
		}

		static float MergeConfidence(const ovrpEyeGazesState& OVREyeGazesState)
		{
			const auto& LeftEyeConfidence = OVREyeGazesState.EyeGazes[ovrpEye_Left].Confidence;
			const auto& RightEyeConfidence = OVREyeGazesState.EyeGazes[ovrpEye_Right].Confidence;
			return FGenericPlatformMath::Min(LeftEyeConfidence, RightEyeConfidence);
		}

		/// Warn: The result of MergedOrientation is not normalized.
		static FQuat MergeOrientation(const ovrpEyeGazesState& OVREyeGazesState)
		{
			const auto& LeftEyeOrientation = OculusHMD::ToFQuat(OVREyeGazesState.EyeGazes[ovrpEye_Left].Pose.Orientation);
			const auto& RightEyeOrientation = OculusHMD::ToFQuat(OVREyeGazesState.EyeGazes[ovrpEye_Right].Pose.Orientation);
			return FQuat::FastLerp(LeftEyeOrientation, RightEyeOrientation, 0.5f);
		}

		static FVector MergePosition(const ovrpEyeGazesState& OVREyeGazesState)
		{
			const auto& LeftEyePosition = OculusHMD::ToFVector(OVREyeGazesState.EyeGazes[ovrpEye_Left].Pose.Position);
			const auto& RightEyePosition = OculusHMD::ToFVector(OVREyeGazesState.EyeGazes[ovrpEye_Right].Pose.Position);
			return (LeftEyePosition + RightEyePosition) / 2.f;
		}

		static FVector GetFixationPoint(const ovrpEyeGazesState& OVREyeGazesState)
		{
			return FVector::ZeroVector; // Not supported
		}

		float WorldToMeters = 100.f;
		IXRTrackingSystem* OculusHMD = nullptr;
		mutable bool bIsTrackerStarted = false;
	};
} // namespace OculusHMD
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS

class FOculusEyeTrackerModule : public IEyeTrackerModule
{
public:
	static inline FOculusEyeTrackerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FOculusEyeTrackerModule>("OculusEyeTracker");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("OculusEyeTracker");
	}

	virtual FString GetModuleKeyName() const override
	{
		return TEXT("OculusEyeTracker");
	}

	virtual bool IsEyeTrackerConnected() const override
	{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
		if (FOculusHMDModule::Get().IsOVRPluginAvailable() && FOculusHMDModule::GetPluginWrapper().IsInitialized())
		{
			ovrpBool isSupported = ovrpBool_False;
			ovrpResult trackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetEyeTrackingSupported(&isSupported);
			if (OVRP_SUCCESS(trackingSupportedResult))
			{
				return (isSupported == ovrpBool_True);
			}
		}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
		return false;
	}

	virtual TSharedPtr<class IEyeTracker, ESPMode::ThreadSafe> CreateEyeTracker() override
	{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
		return TSharedPtr<class IEyeTracker, ESPMode::ThreadSafe>(new OculusHMD::FOculusEyeTracker);
#else
		return TSharedPtr<class IEyeTracker, ESPMode::ThreadSafe>();
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
	}
};

IMPLEMENT_MODULE(FOculusEyeTrackerModule, OculusEyeTracker)
