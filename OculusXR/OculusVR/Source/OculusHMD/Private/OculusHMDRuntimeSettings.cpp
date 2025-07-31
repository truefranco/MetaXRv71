// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusHMDRuntimeSettings.h"

//////////////////////////////////////////////////////////////////////////
// UOculusHMDRuntimeSettings

#include "OculusHMD_Settings.h"

UOculusHMDRuntimeSettings::UOculusHMDRuntimeSettings(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, bAutoEnabled(true)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	// FSettings is the sole source of truth for Oculus default settings
	OculusHMD::FSettings DefaultSettings; 
	SystemSplashBackground = DefaultSettings.SystemSplashBackground;
	bSupportsDash = DefaultSettings.Flags.bSupportsDash;
	bCompositesDepth = DefaultSettings.Flags.bCompositeDepth;
	bHQDistortion = DefaultSettings.Flags.bHQDistortion;
	SuggestedCpuPerfLevel = DefaultSettings.SuggestedCpuPerfLevel;
	SuggestedGpuPerfLevel = DefaultSettings.SuggestedGpuPerfLevel;
	FoveatedRenderingMethod = DefaultSettings.FoveatedRenderingMethod;
	FoveatedRenderingLevel = DefaultSettings.FoveatedRenderingLevel;
	bDynamicFoveatedRendering = DefaultSettings.bDynamicFoveatedRendering;
	bSupportEyeTrackedFoveatedRendering = DefaultSettings.bSupportEyeTrackedFoveatedRendering;
	PixelDensityMin = DefaultSettings.PixelDensityMin;
	PixelDensityMax = DefaultSettings.PixelDensityMax;
	bFocusAware = DefaultSettings.Flags.bFocusAware;
	bDynamicResolution = DefaultSettings.Flags.bPixelDensityAdaptive;
	XrApi = DefaultSettings.XrApi;
	ColorSpace = DefaultSettings.ColorSpace;
	ControllerPoseAlignment = DefaultSettings.ControllerPoseAlignment;
	bRequiresSystemKeyboard = DefaultSettings.Flags.bRequiresSystemKeyboard;
	HandTrackingSupport = DefaultSettings.HandTrackingSupport;
	HandTrackingFrequency = DefaultSettings.HandTrackingFrequency;
	HandTrackingVersion = DefaultSettings.HandTrackingVersion;
	bInsightPassthroughEnabled = DefaultSettings.Flags.bInsightPassthroughEnabled;
	bBodyTrackingEnabled = DefaultSettings.Flags.bBodyTrackingEnabled;
	bEyeTrackingEnabled = DefaultSettings.Flags.bEyeTrackingEnabled;
	bFaceTrackingEnabled = DefaultSettings.Flags.bFaceTrackingEnabled;
	bPhaseSync = DefaultSettings.bPhaseSync;
	bAnchorSupportEnabled = DefaultSettings.Flags.bAnchorSupportEnabled;
	bAnchorSharingEnabled = DefaultSettings.Flags.bAnchorSharingEnabled;
	bSceneSupportEnabled = DefaultSettings.Flags.bSceneSupportEnabled;
	bSupportExperimentalFeatures = DefaultSettings.bSupportExperimentalFeatures;
	bTileTurnOffEnabled = DefaultSettings.Flags.bTileTurnOffEnabled;


	FaceTrackingDataSource.Empty(static_cast<int8>(EFaceTrackingDataSourceConfig::MAX));
	FaceTrackingDataSource.Append(DefaultSettings.FaceTrackingDataSource);

	// Default this to false, FSettings doesn't have a separate composite depth flag for mobile
	bCompositeDepthMobile = false;

#else
	// Some set of reasonable defaults, since blueprints are still available on non-Oculus platforms.
	SystemSplashBackground = ESystemSplashBackgroundType::Black;
	bSupportsDash = false;
	bCompositesDepth = false;
	bHQDistortion = false;
	SuggestedCpuPerfLevel = EProcessorPerformanceLevel::SustainedLow;
	SuggestedGpuPerfLevel = EProcessorPerformanceLevel::SustainedHigh;
	FoveatedRenderingMethod = EFoveatedRenderingMethod::FixedFoveatedRendering;
	FoveatedRenderingLevel = EFoveatedRenderingLevel::Off;
	bDynamicFoveatedRendering = false;
	bSupportEyeTrackedFoveatedRendering = false;
	PixelDensityMin = 0.8f;
	PixelDensityMax = 1.2f;
	bDynamicResolution = false;
	bCompositeDepthMobile = false;
	bFocusAware = true;
	XrApi = EOculusXrApi::OVRPluginOpenXR;
	bLateLatching = false;
	bPhaseSync = false;
	ColorSpace = EColorSpace::P3;
	ControllerPoseAlignment = EOculusControllerPoseAlignment::Default;
	bRequiresSystemKeyboard = false;
	HandTrackingSupport = EHandTrackingSupport::ControllersOnly;
	HandTrackingFrequency = EHandTrackingFrequency::Low;
	HandTrackingVersion = EHandTrackingVersion::Default;
	bInsightPassthroughEnabled = false;
	bAnchorSupportEnabled = false;
	bSupportExperimentalFeatures = false;
	bBodyTrackingEnabled = false;
	bEyeTrackingEnabled = false;
	bFaceTrackingEnabled = false;
	bAnchorSupportEnabled = false;
	bAnchorSharingEnabled = false;
	bSceneSupportEnabled = false;
	bTileTurnOffEnabled = false;
#endif

	LoadFromIni();
	RenameProperties();
}

#if WITH_EDITOR
void UOculusHMDRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		// Automatically switch to Fixed Foveated Rendering when removing Eye Tracked Foveated rendering support
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UOculusHMDRuntimeSettings, bSupportEyeTrackedFoveatedRendering) && !bSupportEyeTrackedFoveatedRendering)
		{
			FoveatedRenderingMethod = EFoveatedRenderingMethod::FixedFoveatedRendering;
			UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusHMDRuntimeSettings, FoveatedRenderingMethod)), GetDefaultConfigFilename());
		}
		// Automatically enable support for eye tracked foveated rendering when selecting the Eye Tracked Foveated Rendering method
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UOculusHMDRuntimeSettings, FoveatedRenderingMethod) && FoveatedRenderingMethod == EFoveatedRenderingMethod::EyeTrackedFoveatedRendering)
		{
			bSupportEyeTrackedFoveatedRendering = true;
			UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusHMDRuntimeSettings, bSupportEyeTrackedFoveatedRendering)), GetDefaultConfigFilename());
		}
	}
}
#endif // WITH_EDITOR

void UOculusHMDRuntimeSettings::LoadFromIni()
{
	const TCHAR* OculusSettings = TEXT("Oculus.Settings");
	bool v;
	float f;
	FVector vec;

	if (GConfig->GetFloat(OculusSettings, TEXT("PixelDensityMax"), f, GEngineIni))
	{
		check(!FMath::IsNaN(f));
		PixelDensityMax = f;
	}
	if (GConfig->GetFloat(OculusSettings, TEXT("PixelDensityMin"), f, GEngineIni))
	{
		check(!FMath::IsNaN(f));
		PixelDensityMin = f;
	}
	if (GConfig->GetBool(OculusSettings, TEXT("bHQDistortion"), v, GEngineIni))
	{
		bHQDistortion = v;
	}
	if (GConfig->GetBool(OculusSettings, TEXT("bCompositeDepth"), v, GEngineIni))
	{
		bCompositesDepth = v;
	}
}

/** This essentially acts like redirects for plugin settings saved in the engine config.
	Anything added here should check for the current setting in the config so that if the dev changes the setting manually, we don't overwrite it with the old setting.
	Note: Do not use UpdateSinglePropertyInConfigFile() here, since that uses a temp config to save the single property,
	it'll get overwritten when GConfig->RemoveKey() marks the main config as dirty and it gets saved again **/
void UOculusHMDRuntimeSettings::RenameProperties()
{
	const TCHAR* OculusSettings = TEXT("/Script/OculusHMD.OculusHMDRuntimeSettings");
	bool v = false;
	FString str;

	// FFRLevel was renamed to FoveatedRenderingLevel
	if (!GConfig->GetString(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusHMDRuntimeSettings, FoveatedRenderingLevel), str, GetDefaultConfigFilename()) &&
		GConfig->GetString(OculusSettings, TEXT("FFRLevel"), str, GetDefaultConfigFilename()))
	{
		if (str.Equals(TEXT("FFR_Off")))
		{
			FoveatedRenderingLevel = EFoveatedRenderingLevel::Off;
		}
		else if (str.Equals(TEXT("FFR_Low")))
		{
			FoveatedRenderingLevel = EFoveatedRenderingLevel::Low;
		}
		else if (str.Equals(TEXT("FFR_Medium")))
		{
			FoveatedRenderingLevel = EFoveatedRenderingLevel::Medium;
		}
		else if (str.Equals(TEXT("FFR_High")))
		{
			FoveatedRenderingLevel = EFoveatedRenderingLevel::High;
		}
		else if (str.Equals(TEXT("FFR_HighTop")))
		{
			FoveatedRenderingLevel = EFoveatedRenderingLevel::HighTop;
		}
		// Use UEnum::GetDisplayValueAsText().ToString() here because UEnum::GetValueAsString() includes the type name as well
		GConfig->SetString(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusHMDRuntimeSettings, FoveatedRenderingLevel), *UEnum::GetDisplayValueAsText(FoveatedRenderingLevel).ToString(), GetDefaultConfigFilename());
		GConfig->RemoveKey(OculusSettings, TEXT("FFRLevel"), GetDefaultConfigFilename());
	}

	// FFRDynamic was renamed to bDynamicFoveatedRendering
	if (!GConfig->GetString(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusHMDRuntimeSettings, bDynamicFoveatedRendering), str, GetDefaultConfigFilename()) &&
		GConfig->GetBool(OculusSettings, TEXT("FFRDynamic"), v, GetDefaultConfigFilename()))
	{
		bDynamicFoveatedRendering = v;
		GConfig->SetBool(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusHMDRuntimeSettings, bDynamicFoveatedRendering), bDynamicFoveatedRendering, GetDefaultConfigFilename());
		GConfig->RemoveKey(OculusSettings, TEXT("FFRDynamic"), GetDefaultConfigFilename());
	}

	const TCHAR* AndroidSettings = TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings");
	const TCHAR* PackageForMobileKey = TEXT("+PackageForOculusMobile");

	TArray<FString> PackageList;
	if(GConfig->GetArray(AndroidSettings, PackageForMobileKey, PackageList, GetDefaultConfigFilename()))
	{
		const TCHAR* Quest = TEXT("Quest");
		const TCHAR* Quest2 = TEXT("Quest2");
		if (PackageList.Contains(Quest))
		{
			PackageList.Remove(Quest);
			if (!PackageList.Contains(Quest2))
			{
				PackageList.Add(Quest2);
			}
			GConfig->SetArray(AndroidSettings, PackageForMobileKey, PackageList, GetDefaultConfigFilename());
		}
	}
}
