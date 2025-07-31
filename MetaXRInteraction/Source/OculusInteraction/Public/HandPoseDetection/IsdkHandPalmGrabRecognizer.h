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
#include "StructTypes.h"
#include "UObject/Object.h"
#include "IsdkHandPalmGrabRecognizer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIsdkPalmGrabRecognizerEventDelegate);

class UIsdkHandMeshComponent;

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class PalmGrabRecognizer;
		class IHandPositionFrame;
	}
} // namespace isdk::api

/**
 * Used to determine if a HandVisual is exhibiting a palm grab (a minimum amount of fingers curling
 * near an object)
 */
UCLASS(BlueprintType, Category = InteractionSDK)
class OCULUSINTERACTION_API UIsdkHandPalmGrabRecognizer : public UObject
{
  GENERATED_BODY()
 public:
  UIsdkHandPalmGrabRecognizer();
  virtual void BeginDestroy() override;
  void UpdateRecognizerParams() const;

  /* Primary driver for detection logic, called externally */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void UpdateState(float DeltaTime);

  /* Returns if the recognizer is currently active and a palm grab is detected */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool IsActive();

  /* Set the minimum angle required by the fingers to register a curl */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetPalmGrabMinFingerCurl(float Angle)
  {
    PalmGrabMinCurl = Angle;
    UpdateRecognizerParams();
  }

  /* Set the minimum amount of fingers required to be curling to register a palm grab*/
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetPalmGrabMinFingers(int Count)
  {
    PalmGrabMinFingers = Count;
    UpdateRecognizerParams();
  }

  /* The HandVisualComponent used for recognizing */
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK, meta = (ExposeOnSpawn))
  UIsdkHandMeshComponent* HandVisual{};

  /* Delegate broadcast when a palm grab becomes active */
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkPalmGrabRecognizerEventDelegate PalmGrabStarted;

  /* Delegate broadcast when a palm grab is no longer active*/
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkPalmGrabRecognizerEventDelegate PalmGrabFinished;

 protected:
  /* The minimum angle required by the fingers to register a curl */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  float PalmGrabMinCurl{90};

  /* Minimum amount of fingers required to be curling to register a palm grab*/
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  int PalmGrabMinFingers{3};

  isdk::api::IHandPositionFrame* EnsureHandPositionFrame() const;

  class FPalmGrabRecognizerImpl;
  TPimplPtr<FPalmGrabRecognizerImpl> PalmGrabRecognizerImpl = nullptr;
  isdk::api::PalmGrabRecognizer* Recognizer{nullptr};
};
