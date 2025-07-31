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

#include "StructTypes.h"
#include "IsdkHandMeshComponent.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "IsdkHandDigitRecognizer.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class IHandPositionFrame;
		class DigitRecognizer;
	}
} // namespace isdk::api
typedef struct isdk_DigitRecognizer_ isdk_DigitRecognizer;

/**
 * Container for API-driven Digit Recognizer
 */
UCLASS(Blueprintable, Abstract, ClassGroup = (InteractionSDK))
class OCULUSINTERACTION_API UIsdkHandDigitRecognizer : public UObject
{
  GENERATED_BODY()

 public:
  /* The Hand Visual Component referenced during digit recognition */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK, meta = (ExposeOnSpawn))
  UIsdkHandMeshComponent* HandMesh{};

  /* Struct of upper and lower tolerances and limits during digit recognition */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK, meta = (ExposeOnSpawn))
  FIsdkDigitRangeParams RangeParameters{};

  /* Primary driver for updating the recognizer (including instantiating the API implementation if
   * needed), needs to be called externally */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void UpdateState(float DeltaTime);

  isdk::api::DigitRecognizer* GetApiDigitRecognizer() const
  {
    return ApiDigitRecognizer;
  }

  /* Whether or not the API recognizer is active */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool IsActive();

  /* Returns the raw value normalized by the recognizer's expected range */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  float GetNormalizedRawValue();

  /* Returns the raw value of the recognizer */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  float GetRawValue();

  /* Returns the expected range of the recognizer, represented by min/max */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  virtual FVector2D GetRawExpectedRange()
      PURE_VIRTUAL(UIsdkDigitRecognizer::CreateDigitRecognizer, return FVector2D::ZeroVector;);

 protected:
  isdk::api::IHandPositionFrame* EnsureHandPositionFrame() const;
  virtual isdk::api::DigitRecognizer* CreateDigitRecognizer()
      PURE_VIRTUAL(UIsdkDigitRecognizer::CreateDigitRecognizer, return nullptr;);

  isdk::api::DigitRecognizer* ApiDigitRecognizer{};
};
