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
#include "IsdkIPose.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class IPose;
	}
}

// This class does not need to be modified.
UINTERFACE(BlueprintType, Category = "InteractionSDK", meta = (DisplayName = "ISDK Pose"))
class OCULUSINTERACTION_API UIsdkIPose : public UInterface
{
  GENERATED_BODY()
};

class OCULUSINTERACTION_API IIsdkIPose
{
  GENERATED_BODY()

  // Add interface functions to this class. This is the class that will be inherited to implement
  // this interface.
 public:
  virtual isdk::api::IPose* GetApiIPose() PURE_VIRTUAL(UIsdkPose::GetApiIPose, return nullptr;);
};
