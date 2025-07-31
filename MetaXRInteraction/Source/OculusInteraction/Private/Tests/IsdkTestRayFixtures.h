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
#include "IsdkMockInteractable.h"

#include "isdk_api/isdk_api.hpp"
#include "Interaction/IsdkInteractionEvents.h"
#include "Interaction/Pointable/IsdkInteractionPointerEvent.h"
#include "Interaction/Pointable/IsdkIPointable.h"
#include "Interaction/IsdkIInteractorState.h"

#include "IsdkTestRayFixtures.generated.h"

UCLASS()
class OCULUSINTERACTION_API UIsdkTestRayFixture : public UObject
{
  GENERATED_BODY()

 public:
  UIsdkTestRayFixture()
  {
    MockInteractable = CreateDefaultSubobject<UIsdkMockInteractable>("MockPointable");
  }

  void SetUp()
  {
    ovrpPosef pointablePlaneOrigin{ovrpQuatf{0, 0, 0, 1}, ovrpVector3f{0, 0, 0}};
    ovrpVector2f pointablePlaneSize{100.0f, 100.0f};

    // Create Pointable Plane
    PanelPlane =
        isdk::api::PointablePlane::createWithPose(&pointablePlaneOrigin, &pointablePlaneSize);
    // Create Ray Interactable
    RayInteractable =
        isdk::api::RayInteractable::create(*PanelPlane, nullptr, nullptr, nullptr, nullptr);
    // Create Ray Interactor
    isdk_RayInteractor_Config interactorConfig;
    isdk_RayInteractor_Config_init(&interactorConfig);
    RayInteractor =
        isdk::api::RayInteractor::create(&interactorConfig, nullptr, nullptr, nullptr, nullptr);
    RayInteractor->setRayOrigin(&RayOriginTarget);

    Updated.AddDynamic(this, &UIsdkTestRayFixture::HandleUpdatedEvent);
    InteractorStateChanged.AddDynamic(this, &UIsdkTestRayFixture::HandleInteractorStateEvent);
    InteractableStateChanged.AddDynamic(this, &UIsdkTestRayFixture::HandleInteractableStateEvent);
    InteractablePointed.AddDynamic(this, &UIsdkTestRayFixture::HandlePointerEvent);
  }

  void TearDown()
  {
    PanelPlane.Reset();
    RayInteractor.Reset();
    RayInteractable.Reset();
    Updated.RemoveDynamic(this, &UIsdkTestRayFixture::HandleUpdatedEvent);
    InteractorStateChanged.RemoveDynamic(this, &UIsdkTestRayFixture::HandleInteractorStateEvent);
    InteractableStateChanged.RemoveDynamic(
        this, &UIsdkTestRayFixture::HandleInteractableStateEvent);
    InteractablePointed.RemoveDynamic(this, &UIsdkTestRayFixture::HandlePointerEvent);
  }

  UFUNCTION()
  void HandleUpdatedEvent()
  {
    ++UpdatedCallCount;
  }

  UPROPERTY()
  UIsdkMockInteractable* MockInteractable;

  isdk::api::PointablePlanePtr PanelPlane = nullptr;
  isdk::api::RayInteractorPtr RayInteractor = nullptr;
  isdk::api::RayInteractablePtr RayInteractable = nullptr;

  const ovrpPosef RayOriginTarget{{0, 0, 0, 1}, {-1, 0, 0}};
  const float MaxRayDistance_ = 1000.0f;

  TArray<EIsdkInteractorState> HandledInteractorStateEvents;
  TArray<EIsdkInteractableState> HandledInteractableStateEvents;
  TArray<FIsdkInteractionPointerEvent> HandledPointerEvents;
  int UpdatedCallCount = 0;
  FIsdkIUpdateEventDelegate Updated;
  FIsdkInteractorStateChanged InteractorStateChanged;
  FIsdkInteractableStateChanged InteractableStateChanged;
  FIsdkInteractionPointerEventDelegate InteractablePointed;

 private:
  UFUNCTION()
  void HandleInteractorStateEvent(const FIsdkInteractorStateEvent& Event)
  {
    HandledInteractorStateEvents.Add(Event.Args.NewState);
  }

  UFUNCTION()
  void HandleInteractableStateEvent(const FIsdkInteractableStateEvent& Event)
  {
    HandledInteractableStateEvents.Add(Event.Args.NewState);
  }

  UFUNCTION()
  void HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
  {
    HandledPointerEvents.Add(PointerEvent);
  }
};
