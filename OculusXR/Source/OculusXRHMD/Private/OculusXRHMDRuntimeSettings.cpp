// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRHMDRuntimeSettings.h"
#include "Algo/ForEach.h"

//////////////////////////////////////////////////////////////////////////
// UOculusXRHMDRuntimeSettings

#include "OculusXRHMD_Settings.h"
#include "OculusXRSimulator.h"

#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"

UOculusXRHMDRuntimeSettings::UOculusXRHMDRuntimeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bAutoEnabled(false)
{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
	// FSettings is the sole source of truth for Oculus default settings
	OculusXRHMD::FSettings DefaultSettings;
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
	bSupportExperimentalFeatures = DefaultSettings.bSupportExperimentalFeatures;
	bAnchorSupportEnabled = DefaultSettings.Flags.bAnchorSupportEnabled;
	bAnchorSharingEnabled = DefaultSettings.Flags.bAnchorSharingEnabled;
	bSceneSupportEnabled = DefaultSettings.Flags.bSceneSupportEnabled;
	bIterativeCookOnTheFly = DefaultSettings.Flags.bIterativeCookOnTheFly;
	bSetActivePIEToPrimary = DefaultSettings.Flags.bSetActivePIEToPrimary;
	bSetCVarPIEToPrimary = DefaultSettings.Flags.bSetCVarPIEToPrimary;
	bUpdateHeadPoseForInactivePlayer = DefaultSettings.Flags.bUpdateHeadPoseForInactivePlayer;
	MPPoseRestoreType = DefaultSettings.MPPoseRestoreType;
	bBoundaryVisibilitySupportEnabled = DefaultSettings.Flags.bBoundaryVisibilitySupportEnabled;
	bDefaultBoundaryVisibilitySuppressed = DefaultSettings.Flags.bDefaultBoundaryVisibilitySuppressed;
	bColocationSessionsEnabled = DefaultSettings.Flags.bColocationSessionsEnabled;
	ProcessorFavor = DefaultSettings.ProcessorFavor;
	bTileTurnOffEnabled = DefaultSettings.Flags.bTileTurnOffEnabled;

	BodyTrackingFidelity = DefaultSettings.BodyTrackingFidelity;
	BodyTrackingJointSet = DefaultSettings.BodyTrackingJointSet;

	FaceTrackingDataSource.Empty(static_cast<int8>(EFaceTrackingDataSourceConfig::MAX));
	FaceTrackingDataSource.Append(DefaultSettings.FaceTrackingDataSource);
	bFaceTrackingVisemesEnabled = DefaultSettings.bFaceTrackingVisemesEnabled;

	// Default this to false, FSettings doesn't have a separate composite depth flag for mobile
	bCompositeDepthMobile = false;

	bThumbstickDpadEmulationEnabled = true;

	bSupportSBC = DefaultSettings.Flags.bSupportSBC;
	SBCPath = DefaultSettings.SBCPath;
#else
	// Some set of reasonable defaults, since blueprints are still available on non-Oculus platforms.
	SystemSplashBackground = ESystemSplashBackgroundType::Black;
	bSupportsDash = false;
	bCompositesDepth = false;
	bHQDistortion = false;
	SuggestedCpuPerfLevel = EOculusXRProcessorPerformanceLevel::SustainedLow;
	SuggestedGpuPerfLevel = EOculusXRProcessorPerformanceLevel::SustainedHigh;
	FoveatedRenderingMethod = EOculusXRFoveatedRenderingMethod::FixedFoveatedRendering;
	FoveatedRenderingLevel = EOculusXRFoveatedRenderingLevel::Off;
	bDynamicFoveatedRendering = false;
	bSupportEyeTrackedFoveatedRendering = false;
	bDynamicResolution = false;
	bCompositeDepthMobile = false;
	bFocusAware = true;
	XrApi = EOculusXRXrApi::OVRPluginOpenXR;
	bLateLatching = false;
	ColorSpace = EOculusXRColorSpace::P3;
	ControllerPoseAlignment = EOculusXRControllerPoseAlignment::Default;
	bRequiresSystemKeyboard = false;
	HandTrackingSupport = EOculusXRHandTrackingSupport::ControllersOnly;
	HandTrackingFrequency = EOculusXRHandTrackingFrequency::Low;
	HandTrackingVersion = EOculusXRHandTrackingVersion::Default;
	bInsightPassthroughEnabled = false;
	bSupportExperimentalFeatures = false;
	bBodyTrackingEnabled = false;
	bEyeTrackingEnabled = false;
	bFaceTrackingEnabled = false;
	bSupportSBC = false;
	bFaceTrackingVisemesEnabled = false;
	bAnchorSupportEnabled = false;
	bAnchorSharingEnabled = false;
	bSceneSupportEnabled = false;
	bIterativeCookOnTheFly = false;
	bSetActivePIEToPrimary = false;
	bSetCVarPIEToPrimary = false;
	bUpdateHeadPoseForInactivePlayer = false;
	MPPoseRestoreType = EOculusXRMPPoseRestoreType::Disabled;
	bBoundaryVisibilitySupportEnabled = false;
	bDefaultBoundaryVisibilitySuppressed = false;
	bColocationSessionsEnabled = false;
	ProcessorFavor = EProcessorFavor::FavorEqually;
	bTileTurnOffEnabled = false;
	bThumbstickDpadEmulationEnabled = true;
#endif

	LoadFromIni();
}

#if WITH_EDITOR
bool UOculusXRHMDRuntimeSettings::CanEditChange(const FProperty* InProperty) const
{
	bool bIsEditable = Super::CanEditChange(InProperty);

	if (bIsEditable && InProperty)
	{
		const FName PropertyName = InProperty->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, XrApi) && !FModuleManager::Get().IsModuleLoaded("OpenXRHMD"))
		{
			bIsEditable = false;
		}

// Disable settings for marketplace release that are only compatible with the Oculus engine fork
#ifndef WITH_OCULUS_BRANCH
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, FoveatedRenderingMethod) || PropertyName == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, bSupportEyeTrackedFoveatedRendering) || PropertyName == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, bDynamicResolution))

		{
			bIsEditable = false;
		}
#endif // WITH_OCULUS_BRANCH
	}

	return bIsEditable;
}

void UOculusXRHMDRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		// Automatically switch to Fixed Foveated Rendering when removing Eye Tracked Foveated rendering support
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, bSupportEyeTrackedFoveatedRendering) && !bSupportEyeTrackedFoveatedRendering)
		{
			FoveatedRenderingMethod = EOculusXRFoveatedRenderingMethod::FixedFoveatedRendering;
			UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, FoveatedRenderingMethod)), GetDefaultConfigFilename());
		}
		// Automatically enable support for eye tracked foveated rendering when selecting the Eye Tracked Foveated Rendering method
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, FoveatedRenderingMethod) && FoveatedRenderingMethod == EOculusXRFoveatedRenderingMethod::EyeTrackedFoveatedRendering)
		{
			bSupportEyeTrackedFoveatedRendering = true;
			UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, bSupportEyeTrackedFoveatedRendering)), GetDefaultConfigFilename());
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, bCompositesDepth))
		{
			if (IConsoleVariable* DepthLayerCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.OpenXRAllowDepthLayer")))
			{
				DepthLayerCVar->Set(static_cast<int>(bCompositesDepth));
			}
		}

		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, SupportedDevices))
		{
			if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
			{
				// Get a list of all available devices
				TArray<EOculusXRSupportedDevices> deviceList;
#define OCULUS_DEVICE_LOOP(device) deviceList.Add(device);
				FOREACH_ENUM_EOCULUSXRSUPPORTEDDEVICES(OCULUS_DEVICE_LOOP);
#undef OCULUS_DEVICE_LOOP
				// Add last device that isn't already in the list
				for (int i = deviceList.Num() - 1; i >= 0; --i)
				{
					if (!SupportedDevices.Contains(deviceList[i]))
					{
						SupportedDevices.Last() = deviceList[i];
						break;
					}
					// Just add another copy of the first device if nothing was available
					SupportedDevices.Last() = deviceList[deviceList.Num() - 1];
				}
			}
		}

		// Force CTXPT when Passthough is enabled
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, bInsightPassthroughEnabled) && bInsightPassthroughEnabled)
		{
			SystemSplashBackground = ESystemSplashBackgroundType::Contextual;
			UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, SystemSplashBackground)), GetDefaultConfigFilename());
		}
	}
}

TArray<FString> UOculusXRHMDRuntimeSettings::GetMetaXRSimulatorInstalledVersions() const
{
	return FMetaXRSimulator::Get().GetInstalledVersions();
}

#endif // WITH_EDITOR

void UOculusXRHMDRuntimeSettings::PostInitProperties()
{
	Super::PostInitProperties();
	RenameProperties();
	MigratePixelDensityRange();

	const TCHAR* OculusSettings = TEXT("/Script/OculusXRHMD.OculusXRHMDRuntimeSettings");
	if (!FModuleManager::Get().IsModuleLoaded("OpenXRHMD"))
	{
		XrApi = EOculusXRXrApi::OVRPluginOpenXR;
		UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, XrApi)), GetDefaultConfigFilename());
	}

	// Force CTXPT for MR apps
	if (bInsightPassthroughEnabled && SystemSplashBackground != ESystemSplashBackgroundType::Contextual)
	{
		SystemSplashBackground = ESystemSplashBackgroundType::Contextual;
		UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, SystemSplashBackground)), GetDefaultConfigFilename());
	}

	UpdateSinglePropertyInConfigFile(GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UOculusXRHMDRuntimeSettings, SBCPath)), GetDefaultConfigFilename());
}

void UOculusXRHMDRuntimeSettings::LoadFromIni()
{
	const TCHAR* OculusSettings = TEXT("Oculus.Settings");
	bool v;
	float f;

	if (GConfig->GetFloat(OculusSettings, TEXT("PixelDensityMax"), f, GEngineIni))
	{
		UE_LOG(LogTemp, Error, TEXT("DONOT manually add setting to Oculus.Settings. PixelDensityMax is ignored."));
		PixelDensityMax = 0.f;
	}
	if (GConfig->GetFloat(OculusSettings, TEXT("PixelDensityMin"), f, GEngineIni))
	{
		UE_LOG(LogTemp, Error, TEXT("DONOT manually add setting to Oculus.Settings. PixelDensityMin is ignored."));
		PixelDensityMin = 0.f;
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
void UOculusXRHMDRuntimeSettings::RenameProperties()
{
	const TCHAR* OculusSettings = TEXT("/Script/OculusXRHMD.OculusXRHMDRuntimeSettings");
	bool v = false;
	FString str;

	// FFRLevel was renamed to FoveatedRenderingLevel
	if (!GConfig->GetString(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusXRHMDRuntimeSettings, FoveatedRenderingLevel), str, GetDefaultConfigFilename()) && GConfig->GetString(OculusSettings, TEXT("FFRLevel"), str, GetDefaultConfigFilename()))
	{
		if (str.Equals(TEXT("FFR_Off")))
		{
			FoveatedRenderingLevel = EOculusXRFoveatedRenderingLevel::Off;
		}
		else if (str.Equals(TEXT("FFR_Low")))
		{
			FoveatedRenderingLevel = EOculusXRFoveatedRenderingLevel::Low;
		}
		else if (str.Equals(TEXT("FFR_Medium")))
		{
			FoveatedRenderingLevel = EOculusXRFoveatedRenderingLevel::Medium;
		}
		else if (str.Equals(TEXT("FFR_High")))
		{
			FoveatedRenderingLevel = EOculusXRFoveatedRenderingLevel::High;
		}
		else if (str.Equals(TEXT("FFR_HighTop")))
		{
			FoveatedRenderingLevel = EOculusXRFoveatedRenderingLevel::HighTop;
		}
		// Use GetNameStringByValue() here because GetValueAsString() includes the type name as well
		GConfig->SetString(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusXRHMDRuntimeSettings, FoveatedRenderingLevel), *StaticEnum<EOculusXRFoveatedRenderingLevel>()->GetNameStringByValue((int64)FoveatedRenderingLevel), GetDefaultConfigFilename());
		GConfig->RemoveKey(OculusSettings, TEXT("FFRLevel"), GetDefaultConfigFilename());
	}

	// FFRDynamic was renamed to bDynamicFoveatedRendering
	if (!GConfig->GetString(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusXRHMDRuntimeSettings, bDynamicFoveatedRendering), str, GetDefaultConfigFilename()) && GConfig->GetBool(OculusSettings, TEXT("FFRDynamic"), v, GetDefaultConfigFilename()))
	{
		bDynamicFoveatedRendering = v;
		GConfig->SetBool(OculusSettings, GET_MEMBER_NAME_STRING_CHECKED(UOculusXRHMDRuntimeSettings, bDynamicFoveatedRendering), bDynamicFoveatedRendering, GetDefaultConfigFilename());
		GConfig->RemoveKey(OculusSettings, TEXT("FFRDynamic"), GetDefaultConfigFilename());
	}

	const FString Quest = TEXT("Quest");

#ifndef WITH_OCULUS_BRANCH
	const TCHAR* AndroidSettings = TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings");
	TArray<FString> PackageList;
	const TCHAR* PackageForMobileKey = TEXT("+PackageForOculusMobile");
	if (GConfig->GetArray(AndroidSettings, PackageForMobileKey, PackageList, GetDefaultConfigFilename()))
	{
		const FString Quest2 = TEXT("Quest2");
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
#endif // WITH_OCULUS_BRANCH

	TArray<FString> DeviceList;
	const FString SupportedDevicesKey = FString("+").Append(GET_MEMBER_NAME_STRING_CHECKED(UOculusXRHMDRuntimeSettings, SupportedDevices));
	if (GConfig->GetArray(OculusSettings, *SupportedDevicesKey, DeviceList, GetDefaultConfigFilename()))
	{
		const EOculusXRSupportedDevices LastSupportedDevice = EOculusXRSupportedDevices::Quest2;
		const FString LastSupportedDeviceString = StaticEnum<EOculusXRSupportedDevices>()->GetNameStringByValue((int64)LastSupportedDevice);
		if (DeviceList.Contains(Quest))
		{
			DeviceList.Remove(Quest);
			if (!DeviceList.Contains(LastSupportedDeviceString))
			{
				DeviceList.Add(LastSupportedDeviceString);
			}
			GConfig->SetArray(OculusSettings, *SupportedDevicesKey, DeviceList, GetDefaultConfigFilename());

			// Reflect the config changes in the Project Settings UI
			SupportedDevices.Remove((EOculusXRSupportedDevices)0); // Enums that don't exist just have a value of 0
			if (!SupportedDevices.Contains(LastSupportedDevice))
			{
				SupportedDevices.Add(LastSupportedDevice);
			}
		}
	}
}

void UOculusXRHMDRuntimeSettings::MigratePixelDensityRange()
{
#if WITH_EDITOR
	if (!FMath::IsNearlyZero(PixelDensityMin))
	{
		Algo::ForEach(UDeviceProfileManager::Get().Profiles, [&](UDeviceProfile* Profile) {
			float ProfilePixelDensityMin = 0.f;
			if (Profile->GetConsolidatedCVarValue(VAR_PixelDensityMin, ProfilePixelDensityMin))
			{
				Profile->ModifyCVarValue(VAR_PixelDensityMin, FString::SanitizeFloat(PixelDensityMin), true);
				UDeviceProfileManager::Get().SaveProfiles(true);
			}
		});

		PixelDensityMin = 0.f;
		TryUpdateDefaultConfigFile();
		UE_LOG(LogTemp, Log, TEXT("PixelDensityMin %f is migrated to per device range."), PixelDensityMin);
	}
	if (!FMath::IsNearlyZero(PixelDensityMax))
	{
		Algo::ForEach(UDeviceProfileManager::Get().Profiles, [&](UDeviceProfile* Profile) {
			float ProfilePixelDensityMax = 0.f;
			if (Profile->GetConsolidatedCVarValue(VAR_PixelDensityMax, ProfilePixelDensityMax))
			{
				Profile->ModifyCVarValue(VAR_PixelDensityMax, FString::SanitizeFloat(PixelDensityMax), true);
				UDeviceProfileManager::Get().SaveProfiles(true);
			}
		});

		PixelDensityMax = 0.f;
		TryUpdateDefaultConfigFile();
		UE_LOG(LogTemp, Log, TEXT("PixelDensityMax %f is migrated to per device range."), PixelDensityMax);
	}
#endif
}
