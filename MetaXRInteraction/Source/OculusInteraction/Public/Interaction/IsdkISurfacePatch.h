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
#include "UObject/Interface.h"
#include "IsdkISurfacePatch.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class ISurfacePatch;
	}
}

// This class does not need to be modified.
UINTERFACE()
class OCULUSINTERACTION_API UIsdkISurfacePatch : public UInterface
{
  GENERATED_BODY()
};

class OCULUSINTERACTION_API IIsdkISurfacePatch
{
  GENERATED_BODY()

 public:
  // Add interface functions to this class. This is the class that will be inherited to implement
  // this interface.
  virtual isdk::api::ISurfacePatch* GetApiISurfacePatch()
      PURE_VIRTUAL(ISurfacePatch::GetApiISurfacePatch, return nullptr;);

  static bool EnsureSurfacePatchValid(
      const TScriptInterface<IIsdkISurfacePatch>& SurfacePatch,
      UObject* OnObject,
      const TCHAR* OnObjectFieldName,
      isdk::api::ISurfacePatch*& ApiSurfacePatch);
};
