// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXROpenXRDynamicResolutionState.h"
#include "LegacyScreenPercentageDriver.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "IHeadMountedDisplay.h"
#include "OculusXROpenXRUtilities.h"
#include "SceneView.h"

namespace OculusXR
{
	FOpenXRDynamicResolutionState::FOpenXRDynamicResolutionState(const OculusXRHMD::FSettingsPtr InSettings)
		: Settings(InSettings)
		, ResolutionFraction(1.0f)
		, ResolutionFractionUpperBound(1.0f)
	{
		check(Settings.IsValid());
	}

	void FOpenXRDynamicResolutionState::ResetHistory() {
	};

	bool FOpenXRDynamicResolutionState::IsSupported() const
	{
		return true;
	}

	void FOpenXRDynamicResolutionState::SetupMainViewFamily(class FSceneViewFamily& ViewFamily)
	{
		check(IsInGameThread());
		check(ViewFamily.EngineShowFlags.ScreenPercentage == true);

		if (IsEnabled())
		{
			IXRTrackingSystem* TrackingSystem = OculusXR::GetOpenXRTrackingSystem();
			if (TrackingSystem != nullptr)
			{
				IHeadMountedDisplay* Hmd = TrackingSystem->GetHMDDevice();
				check(Hmd != nullptr);

				// FSettings::PixelDensity is relative to device recommendedImageRectWidth
				// ScreenPercentage is percent of screen resolution (where screen resolution is recommendedImageRectWidth * xr.SecondaryScreenPercentage.HMDRenderTarget )
				// 		xr.SecondaryScreenPercentage.HMDRenderTarget is also known as vr.PixelDensity
				const float InvMaxPixelDensity = 1.0f / Hmd->GetPixelDenity();
				ResolutionFraction = Settings->PixelDensity * InvMaxPixelDensity;

				const float MinResolutionFraction = FMath::Max(Settings->GetPixelDensityMin() * InvMaxPixelDensity, ISceneViewFamilyScreenPercentage::kMinResolutionFraction);
				ResolutionFraction = FMath::Clamp(ResolutionFraction, MinResolutionFraction, 1.0f);

				ViewFamily.SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(ViewFamily, ResolutionFraction, ResolutionFractionUpperBound));
			}
		}
	}

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
	void FOpenXRDynamicResolutionState::SetTemporalUpscaler(const UE::Renderer::Private::ITemporalUpscaler* InTemporalUpscaler)
	{
		return;
	}
#endif // !UE_VERSION_OLDER_THAN(5, 4, 0)

	DynamicRenderScaling::TMap<float> FOpenXRDynamicResolutionState::GetResolutionFractionsApproximation() const
	{
		DynamicRenderScaling::TMap<float> ResolutionFractions;
		ResolutionFractions.SetAll(1.0f);
		ResolutionFractions[GDynamicPrimaryResolutionFraction] = ResolutionFraction;
		return ResolutionFractions;
	}

	DynamicRenderScaling::TMap<float> FOpenXRDynamicResolutionState::GetResolutionFractionsUpperBound() const
	{
		DynamicRenderScaling::TMap<float> ResolutionFractions;
		ResolutionFractions.SetAll(1.0f);
		ResolutionFractions[GDynamicPrimaryResolutionFraction] = ResolutionFractionUpperBound;
		return ResolutionFractionUpperBound;
	}

	void FOpenXRDynamicResolutionState::SetEnabled(bool bEnable)
	{
		check(IsInGameThread());
		Settings->Flags.bPixelDensityAdaptive = bEnable;
	}

	bool FOpenXRDynamicResolutionState::IsEnabled() const
	{
		check(IsInGameThread());
		return Settings->Flags.bPixelDensityAdaptive;
	}

	void FOpenXRDynamicResolutionState::ProcessEvent(EDynamicResolutionStateEvent Event) {
	};

} // namespace OculusXR

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
