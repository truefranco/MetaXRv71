// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRCoreExtensionPlugin.h"

#include "DefaultSpectatorScreenController.h"
#include "OculusXRFunctionLibrary.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "OculusXRXRFunctions.h"
#include "OculusXROpenXRUtilities.h"
#include "OpenXRCore.h"
#include "OpenXRHMDSettings.h"

#if PLATFORM_ANDROID
// #include <openxr_oculus.h>
#include <dlfcn.h>
#endif // PLATFORM_ANDROID

DEFINE_LOG_CATEGORY(LogOculusOpenXRPlugin);

namespace OculusXR
{

	bool FCoreExtensionPlugin::IsStandaloneStereoOnlyDevice()
	{
#if PLATFORM_ANDROID
		const bool bIsStandaloneStereoDevice = FAndroidMisc::GetDeviceMake() == FString("Oculus");
#else
		const bool bIsStandaloneStereoDevice = false;
#endif
		return bIsStandaloneStereoDevice;
	}

	bool FCoreExtensionPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		return true;
	}

	bool FCoreExtensionPlugin::GetSpectatorScreenController(FHeadMountedDisplayBase* InHMDBase, TUniquePtr<FDefaultSpectatorScreenController>& OutSpectatorScreenController)
	{
#if PLATFORM_ANDROID
		OutSpectatorScreenController = nullptr;
		return true;
#else  // PLATFORM_ANDROID
		OutSpectatorScreenController = MakeUnique<FDefaultSpectatorScreenController>(InHMDBase);
		return false;
#endif // PLATFORM_ANDROID
	}

	const void* FCoreExtensionPlugin::OnCreateInstance(class IOpenXRHMDModule* InModule, const void* InNext)
	{
		// We need to use GConfig to get the bCompositesDepth setting here because UOculusXRHMDRuntimeSettings isn't available at this point
		check(GConfig);
#if PLATFORM_ANDROID
		bool bCompositeDepth = false;
		GConfig->GetBool(TEXT("/Script/OculusXRHMD.OculusXRHMDRuntimeSettings"), TEXT("bCompositeDepthMobile"), bCompositeDepth, GEngineIni);
#else  // PLATFORM_ANDROID
		bool bCompositeDepth = true;
		GConfig->GetBool(TEXT("/Script/OculusXRHMD.OculusXRHMDRuntimeSettings"), TEXT("bCompositesDepth"), bCompositeDepth, GEngineIni);
#endif // PLATFORM_ANDROID
		if (IConsoleVariable* DepthLayerCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.OpenXRAllowDepthLayer")))
		{
			DepthLayerCVar->Set(static_cast<int>(bCompositeDepth));
		}

		return InNext;
	}

	const void* FCoreExtensionPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		check(IsInGameThread());

		InitOpenXRFunctions(InInstance);

		if (UOpenXRHMDSettings* OpenXRHMDSettings = GetMutableDefault<UOpenXRHMDSettings>())
		{
			OpenXRHMDSettings->bIsFBFoveationEnabled = true;
		}

		if (IConsoleVariable* VariableRateShadingCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VRS.Enable")))
		{
			VariableRateShadingCVar->Set(1);
		}

		if (const UOculusXRHMDRuntimeSettings* OculusXRHMDSettings = GetDefault<UOculusXRHMDRuntimeSettings>())
		{
			if (IConsoleVariable* FoveationLevelCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.OpenXRFBFoveationLevel")))
			{
				FoveationLevelCVar->Set(static_cast<int>(OculusXRHMDSettings->FoveatedRenderingLevel));
			}
			if (IConsoleVariable* FoveationDynamicCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.OpenXRFBFoveationDynamic")))
			{
				FoveationDynamicCVar->Set(OculusXRHMDSettings->bDynamicFoveatedRendering);
			}
		}

#if PLATFORM_ANDROID
#if UE_VERSION_OLDER_THAN(5, 5, 0)
		if (GRHISupportsRHIThread && GIsThreadedRendering && GUseRHIThread_InternalUseOnly)
		{
			SetRHIThreadEnabled(false, false);
		}
#else
		GPendingRHIThreadMode = ERHIThreadMode::None;
#endif // UE_VERSION_OLDER_THAN
#endif // PLATFORM_ANDROID
		return InNext;
	}

} // namespace OculusXR
