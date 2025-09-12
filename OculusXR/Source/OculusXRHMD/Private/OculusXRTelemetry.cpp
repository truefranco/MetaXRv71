// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRTelemetry.h"
#include "OculusXRHMDModule.h"
#include "OculusXRTelemetryPrivacySettings.h"
#include "Async/Async.h"
#include "GeneralProjectSettings.h"

namespace OculusXRTelemetry
{
	namespace
	{
		const char* TelemetrySource = "UE5Integration";
	}

	bool IsActive()
	{
#if OCULUS_HMD_SUPPORTED_PLATFORMS
		if constexpr (FTelemetryBackend::IsNullBackend())
		{
			return false;
		}
		// IsActive() can be called during shutdown, after the FOculusXRHMDModule has been unloaded.
		// This means we can't use the checked FOculusXRHMDModule::Get(), we instead need to use a fallible
		// GetModule(..) and check that the module exists.
		FOculusXRHMDModule* HMDModule = static_cast<FOculusXRHMDModule*>(FModuleManager::Get().GetModule(IOculusXRHMDModule::NAME_OculusXRHMD));
		if (HMDModule && HMDModule->IsOVRPluginAvailable() && FOculusXRHMDModule::GetPluginWrapper().IsInitialized())
		{
			return true;
		}
#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
		return false;
	}

	void IfActiveThen(TUniqueFunction<void()> Function)
	{
		AsyncTask(ENamedThreads::GameThread, [F = MoveTemp(Function)]() {
			if (IsActive())
			{
				F();
			}
		});
	}

	void PropagateTelemetryConsent()
	{
#ifdef WITH_EDITOR
		if (FOculusXRHMDModule::Get().IsOVRPluginAvailable() && FOculusXRHMDModule::GetPluginWrapper().IsInitialized())
		{
			const bool bHasConsent = FOculusXRHMDModule::GetPluginWrapper().GetUnifiedConsent(UNREAL_TOOL_ID) == ovrpBool_True;
			FOculusXRHMDModule::GetPluginWrapper().QplSetConsent(bHasConsent);
			FOculusXRHMDModule::GetPluginWrapper().SetDeveloperTelemetryConsent(bHasConsent);
		}
#endif
	}

	TTuple<FString, FString> GetProjectIdAndName()
	{
		const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
		return MakeTuple(ProjectSettings.ProjectID.ToString(), ProjectSettings.ProjectName);
	}

	bool IsConsentGiven()
	{
#ifdef WITH_EDITOR
		if (const UOculusXRTelemetryPrivacySettings* EditorPrivacySettings = GetDefault<UOculusXRTelemetryPrivacySettings>())
		{
			return EditorPrivacySettings->bIsEnabled;
		}
#endif
		return false;
	}

	void SendEvent(const TCHAR* EventName, float Param)
	{
		const FString StrVal = FString::Printf(TEXT("%f"), Param);
		SendEvent(EventName, *StrVal);
	}

	void SendEvent(const TCHAR* EventName, bool bParam)
	{
		SendEvent(EventName, bParam ? TEXT("true") : TEXT("false"));
	}

	void SendEvent(const TCHAR* EventName, const TCHAR* Param)
	{
		if (IsActive())
		{
			FOculusXRHMDModule::GetPluginWrapper().SendEvent2(TCHAR_TO_ANSI(EventName), TCHAR_TO_ANSI(Param), TelemetrySource);
		}
	}

} // namespace OculusXRTelemetry
