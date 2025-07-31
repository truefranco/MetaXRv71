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
#include "IsdkInteractionPointerEvent.h"

#include "IsdkIPointable.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class IPointable;
	}
}

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class OCULUSINTERACTION_API UIsdkIPointable : public UInterface
{
  GENERATED_BODY()
};

/**
 * Any interactable that can be pointed at, and emit pointer events.
 */
class OCULUSINTERACTION_API IIsdkIPointable
{
  GENERATED_BODY()

  // Add interface functions to this class. This is the class that will be inherited to implement
  // this interface.
 public:
  virtual FIsdkInteractionPointerEventDelegate& GetInteractionPointerEventDelegate()
      PURE_VIRTUAL(IIsdkIPointable::GetInteractorPointerEventDelegate,
                   static FIsdkInteractionPointerEventDelegate Default;
                   return Default;);
};
