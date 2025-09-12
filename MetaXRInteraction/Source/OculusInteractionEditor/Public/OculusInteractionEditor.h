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

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "IsdkHandPoseDataActions.h"
#include "IsdkHandPoseDetectionProfileActions.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOculusInteractionEditor, Log, All);

/**
 * The public interface to this module.  In most cases, this interface is only public to sibling
 * modules within this plugin.
 */
class IOculusInteractionEditor : public IModuleInterface
{
 public:
  /**
   * Singleton-like access to this module's interface.  This is just for convenience!
   * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded
   * already.
   *
   * @return Returns singleton instance, loading the module on demand if needed
   */
  static inline IOculusInteractionEditor& Get()
  {
    return FModuleManager::LoadModuleChecked<IOculusInteractionEditor>("OculusInteractionEditor");
  }

  /**
   * Checks to see if this module is loaded and ready.  It is only valid to call Get() if
   * IsAvailable() returns true.
   *
   * @return True if the module is loaded and ready to use
   */
  static inline bool IsAvailable()
  {
    return FModuleManager::Get().IsModuleLoaded("OculusInteractionEditor");
  }

  TSharedPtr<FIsdkHandPoseDataAssetTypeActions> IsdkHandPoseDataAssetTypeActions;
  TSharedPtr<FIsdkHandPoseDetectionProfileAssetTypeActions>
      IsdkHandPoseDetectionProfileAssetTypeActions;
};
