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
#include "Interaction/IsdkGrabberComponent.h"
#include "UObject/Object.h"
#include "IsdkGrabDetector.generated.h"

enum class EIsdkGrabInputMethod : uint8;
enum class EIsdkPointerEventType : uint8;
class UIsdkGrabDetector;
class UIsdkGrabberComponent;
class UIsdkGrabbableComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FIsdkGrabDetectorHoverDelegate,
    UIsdkGrabDetector*,
    UIsdkGrabbableComponent*);

/**
 * UIsdkGrabDetector is an abstract base class that provides a grab interaction with a means of
 * detecting grabbables (eg, by hand, distance, or ray).  It is intended to be used by
 * UIsdkGrabberComponent to support multiple grab behaviors.
 *
 * @see UIsdkGrabberComponent
 * @see UIsdkHandGrabDetector
 * @see UIsdkRayGrabDetector
 * @see UIsdkDistanceGrabDetector
 */
UCLASS(Abstract)
class OCULUSINTERACTION_API UIsdkGrabDetector : public UObject
{
  GENERATED_BODY()

 public:
  virtual void Initialize(UIsdkGrabberComponent* InGrabberComponent);

  virtual void Deinitialize() {}

  virtual void Tick(float DeltaTime) {}

  virtual UIsdkGrabbableComponent* GetGrabCandidate(EIsdkGrabInputMethod InputMethod)
      PURE_VIRTUAL(UIsdkGrabDetector::GetGrabCandidate, return nullptr;)

          virtual void Select(EIsdkGrabInputMethod InputMethod);
  virtual void Unselect();

  virtual EIsdkGrabDetectorType GetGrabDetectorType() const
      PURE_VIRTUAL(UIsdkGrabDetector::GetGrabberTransform, return EIsdkGrabDetectorType::None;)

          protected :

      UPROPERTY() UIsdkGrabberComponent* GrabberComponent;

  bool bIsSelecting = false;
};
