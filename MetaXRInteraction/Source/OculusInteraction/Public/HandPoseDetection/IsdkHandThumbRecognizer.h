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
#include "Templates/PimplPtr.h"
#include "IsdkHandDigitRecognizer.h"
#include "UObject/Object.h"
#include "IsdkHandThumbRecognizer.generated.h"

namespace isdk {
	namespace api {
		namespace helper
		{
			class FThumbRecognizerImpl;
		}
	}
}

/**
 * Child of UIsdkHandDigitRecognizer, used specifically for recognizing thumbs (non-finger)
 */
UCLASS(BlueprintType, Category = InteractionSDK)
class OCULUSINTERACTION_API UIsdkHandThumbRecognizer : public UIsdkHandDigitRecognizer
{
  GENERATED_BODY()
 public:
  UIsdkHandThumbRecognizer();
  virtual void BeginDestroy() override;

  /* The type of thumb (non-finger) action/position this is set to recognize */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK, meta = (ExposeOnSpawn))
  EIsdkDetection_ThumbCalcType CalcType{};

  virtual FVector2D GetRawExpectedRange() override;

 protected:
  virtual isdk::api::DigitRecognizer* CreateDigitRecognizer() override;

  TPimplPtr<isdk::api::helper::FThumbRecognizerImpl> ThumbRecognizerImpl;
};
