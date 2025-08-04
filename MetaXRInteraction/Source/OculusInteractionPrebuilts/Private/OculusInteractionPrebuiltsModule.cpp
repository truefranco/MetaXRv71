/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * Licensed under the Oculus SDK License Agreement (the "License");
 * you may not use the Oculus SDK except in compliance with the License,
 * which is provided at the time of installation or download, or which
 * otherwise accompanies this software in either electronic or hard copy form.
 *
 * You may obtain a copy of the License at
 *
 * https://developer.oculus.com/licenses/oculussdk/
 *
 * Unless required by applicable law or agreed to in writing, the Oculus SDK
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "OculusInteractionPrebuiltsModule.h"
#include "OculusInteractionPrebuiltsLog.h"

DEFINE_LOG_CATEGORY(LogOculusInteractionPrebuilts);

IMPLEMENT_MODULE(FOculusInteractionPrebuiltsModule, OculusInteractionPrebuilts)

void FOculusInteractionPrebuiltsModule::StartupModule()
{
  IModuleInterface::StartupModule();

  if (!TryLoadIsdkDataSourcesMetaXRModule())
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Log,
        TEXT(
            "OculusXR Modules not found at startup - skipping IsdkDataSourcesMetaXR. subscribing to OnModulesChanged."));

    ModuleChangeMetaXRDelegateHandle = FModuleManager::Get().OnModulesChanged().AddLambda(
        [OculusInputModuleName = FName(OculusXRInputModuleName),
         OculusHMDModuleName = FName(OculusXRHMDModuleName)](FName Name, EModuleChangeReason)
        {
          if (Name == OculusInputModuleName || Name == OculusHMDModuleName)
          {
            UE_LOG(LogOculusInteractionPrebuilts, Log, TEXT("Module Loaded: %s"), *Name.ToString());

            TryLoadIsdkDataSourcesMetaXRModule();
          }
        });
  }

  if (!TryLoadIsdkDataSourcesOpenXRModule())
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Log,
        TEXT(
            "OculusVR Modules not found at startup - skipping IsdkDataSourcesOpenXR. subscribing to OnModulesChanged."));

    ModuleChangeOpenXRDelegateHandle = FModuleManager::Get().OnModulesChanged().AddLambda(
        [OculusXRInputModuleName = FName(OpenXRHandTrackingModuleName)](
            FName Name, EModuleChangeReason)
        {
          if (Name == OpenXRHandTrackingModuleName)
          {
            UE_LOG(LogOculusInteractionPrebuilts, Log, TEXT("Module Loaded: %s"), *Name.ToString());

            TryLoadIsdkDataSourcesOpenXRModule();
          }
        });
  }
}

void FOculusInteractionPrebuiltsModule::ShutdownModule()
{
  IModuleInterface::ShutdownModule();
  if (ModuleChangeMetaXRDelegateHandle.IsValid())
  {
    FModuleManager::Get().OnModulesChanged().Remove(ModuleChangeMetaXRDelegateHandle);
    ModuleChangeMetaXRDelegateHandle = {};
  }
}

bool FOculusInteractionPrebuiltsModule::IsOculusXRLoaded()
{
  return FModuleManager::Get().IsModuleLoaded(OculusXRInputModuleName) &&
      FModuleManager::Get().IsModuleLoaded(OculusXRHMDModuleName);
}

bool FOculusInteractionPrebuiltsModule::IsOpenXRLoaded()
{
  return FModuleManager::Get().IsModuleLoaded(OpenXRHandTrackingModuleName);
}

bool FOculusInteractionPrebuiltsModule::TryLoadIsdkDataSourcesMetaXRModule()
{
  if (IsOculusXRLoaded())
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Log,
        TEXT("OculusXR Modules found - loading IsdkDataSourcesMetaXR"));
    FModuleManager::Get().LoadModuleChecked("IsdkDataSourcesMetaXR");
    return true;
  }
  return false;
}

bool FOculusInteractionPrebuiltsModule::TryLoadIsdkDataSourcesOpenXRModule()
{
  if (IsOpenXRLoaded())
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Log,
        TEXT("OpenXR Modules found - loading IsdkDataSourcesOpenXR"));
    FModuleManager::Get().LoadModule("IsdkDataSourcesOpenXR");
    return true;
  }
  return false;
}
