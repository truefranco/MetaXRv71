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
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "Interaction/Pointable/IsdkIPointable.h"
#include "Interaction/Pointable/IsdkInteractionPointerEvent.h"
#include "IsdkWidget.h"
#include "IsdkPointableWidget.generated.h"

class UWidgetComponent;

UCLASS(
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Pointable Widget"))
class OCULUSINTERACTION_API UIsdkPointableWidget : public UActorComponent
{
  GENERATED_BODY()

 public:
  UIsdkPointableWidget();

  UPROPERTY(Transient, BlueprintReadWrite, Category = "InteractionSDK")
  UWidgetComponent* AttachedWidget;

  // Event Delegates
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkWidgetEventDelegate WidgetEventDelegate;

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void Setup(UWidgetComponent* Widget, TScriptInterface<IIsdkIPointable> IPointable);

  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK, EditAnywhere)
  float MinMoveTravelDistance = 0.1f;

  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK, EditAnywhere)
  bool bStopBroadcastOnDrag = true;

 private:
  UFUNCTION()
  void HandleInteractionPointerEvent(const FIsdkInteractionPointerEvent& PointerEvent);
};
