// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_Settings.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FSettings
//-------------------------------------------------------------------------------------------------

FSettings::FSettings() :
	BaseOffset(0, 0, 0)
	, BaseOrientation(FQuat::Identity)
	, PixelDensity(1.0f)
	, PixelDensityMin(0.8f)
	, PixelDensityMax(1.2f)
	, SystemHeadset(ovrpSystemHeadset_None)
	, SuggestedCpuPerfLevel(EProcessorPerformanceLevel::SustainedLow)
	, SuggestedGpuPerfLevel(EProcessorPerformanceLevel::SustainedHigh)
	, FoveatedRenderingMethod(EFoveatedRenderingMethod::FixedFoveatedRendering)
	, FoveatedRenderingLevel(EFoveatedRenderingLevel::Off)
	, bDynamicFoveatedRendering(true)
	, bSupportEyeTrackedFoveatedRendering(false)
	, SystemSplashBackground(ESystemSplashBackgroundType::Black)
	, XrApi(EOculusXrApi::OVRPluginOpenXR)
	, ColorSpace(EColorSpace::P3)
	, ControllerPoseAlignment(EOculusControllerPoseAlignment::Default)
	, HandTrackingSupport(EHandTrackingSupport::ControllersOnly)
	, HandTrackingFrequency(EHandTrackingFrequency::LOW)
	, HandTrackingVersion(EHandTrackingVersion::Default)
	, ColorScale(ovrpVector4f{1,1,1,1})
	, ColorOffset(ovrpVector4f{0,0,0,0})
	, bApplyColorScaleAndOffsetToAllLayers(false)
	, CurrentFeatureLevel(GMaxRHIFeatureLevel)
	, bLateLatching(false)
	, bPhaseSync(false)
	, bSupportExperimentalFeatures(false)
	, BodyTrackingFidelity(EOculusHMDBodyTrackingFidelity::Low)
	, BodyTrackingJointSet(EOculusHMDBodyJointSet::UpperBody)
{
	Flags.Raw = 0;
	Flags.bHMDEnabled = true;
	Flags.bUpdateOnRT = true;
	Flags.bHQBuffer = false;
#if PLATFORM_ANDROID
	Flags.bCompositeDepth = false;
	Flags.bsRGBEyeBuffer = true;
	//oculus mobile is always-on stereo, no need for enableStereo codepaths
	Flags.bStereoEnabled = true;
	CurrentShaderPlatform = EShaderPlatform::SP_VULKAN_ES3_1_ANDROID;
#else
	Flags.bCompositeDepth = true;
	Flags.bsRGBEyeBuffer = false;
	Flags.bStereoEnabled = false;
	CurrentFeatureLevel = GEngine ? GEngine->GetDefaultWorldFeatureLevel() : GMaxRHIFeatureLevel;
	CurrentShaderPlatform = GShaderPlatformForFeatureLevel[CurrentFeatureLevel];
#endif

	Flags.bSupportsDash = true;
	Flags.bFocusAware = true;
	Flags.bRequiresSystemKeyboard = false;
	Flags.bInsightPassthroughEnabled = false;
	Flags.bAnchorSupportEnabled = false;
	Flags.bAnchorSharingEnabled = false;
	Flags.bSceneSupportEnabled = false;
	Flags.bBodyTrackingEnabled = false;
	Flags.bEyeTrackingEnabled = false;
	Flags.bFaceTrackingEnabled = false;
	EyeRenderViewport[0] = EyeRenderViewport[1] = FIntRect(0, 0, 0, 0);

	RenderTargetSize = FIntPoint(0, 0);

	Flags.bTileTurnOffEnabled = false;
}

TSharedPtr<FSettings, ESPMode::ThreadSafe> FSettings::Clone() const
{
	TSharedPtr<FSettings, ESPMode::ThreadSafe> NewSettings = MakeShareable(new FSettings(*this));
	return NewSettings;
}

void FSettings::SetPixelDensity(float NewPixelDensity)
{
	if (Flags.bPixelDensityAdaptive)
	{
		PixelDensity = FMath::Clamp(NewPixelDensity, PixelDensityMin, PixelDensityMax);
	}
	else
	{
		PixelDensity = FMath::Clamp(NewPixelDensity, ClampPixelDensityMin, ClampPixelDensityMax);
	}
}

void FSettings::SetPixelDensityMin(float NewPixelDensityMin)
{
	PixelDensityMin = FMath::Clamp(NewPixelDensityMin, ClampPixelDensityMin, ClampPixelDensityMax);
	PixelDensityMax = FMath::Max(PixelDensityMin, PixelDensityMax);
	SetPixelDensity(PixelDensity);
}

void FSettings::SetPixelDensityMax(float NewPixelDensityMax)
{
	PixelDensityMax = FMath::Clamp(NewPixelDensityMax, ClampPixelDensityMin, ClampPixelDensityMax);
	PixelDensityMin = FMath::Min(PixelDensityMin, PixelDensityMax);
	SetPixelDensity(PixelDensity);
}


void FSettings::SetPixelDensitySmooth(float NewPixelDensity)
{
	// Pixel Density changes need to be smooth both for artifacts with FFR/TTO (FFR/tile-turnoff is one frame late so shouldn't change too fast)
	// but also so that if the developer uses the CVar and not the runtime (which is already smooth) there is no jump artifacts.
	constexpr float MaxPerFrameIncrease = 0.010;
	constexpr float MaxPerFrameDecrease = 0.045;

	float NewClampedPixelDensity = FMath::Clamp(NewPixelDensity, PixelDensity - MaxPerFrameDecrease, PixelDensity + MaxPerFrameIncrease);
	if (Flags.bPixelDensityAdaptive)
	{
		PixelDensity = FMath::Clamp(NewClampedPixelDensity, PixelDensityMin, PixelDensityMax);
	}
	else
	{
		PixelDensity = FMath::Clamp(NewClampedPixelDensity, ClampPixelDensityMin, ClampPixelDensityMax);
	}
}

} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
