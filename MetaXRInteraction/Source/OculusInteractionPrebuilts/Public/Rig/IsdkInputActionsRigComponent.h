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
#include "IsdkHandData.h"
#include "Components/ActorComponent.h"
#include "StructTypes.h"
#include "IsdkInputActionsRigComponent.generated.h"

/**
 *
 */
UCLASS(ClassGroup = (InteractionSDK))
class OCULUSINTERACTIONPREBUILTS_API UIsdkInputActionsRigComponent : public UActorComponent
{
  GENERATED_BODY()
 public:
  UIsdkInputActionsRigComponent();

  void SetSubobjectPropertyDefaults(EIsdkHandedness InHandedness);

#if WITH_EDITOR
  virtual void CheckForErrors() override;
#endif

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName SelectAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName SelectStrengthAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName GrabSelectAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName GrabSelectStrengthAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName PinchGrabAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName PalmGrabAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName AButtonDownAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName BButtonDownAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName XButtonDownAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName YButtonDownAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName AButtonTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName BButtonTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName XButtonTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName YButtonTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftMenuButtonDownAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftFrontTriggerTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftFrontTriggerAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName RightFrontTriggerTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName RightFrontTriggerAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftGripTriggerAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName RightGripTriggerAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftThumbstickTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftThumbstickXAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftThumbstickYAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName RightThumbstickTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName RightThumbstickXAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName RightThumbstickYAxisAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName LeftPanelTouchedAction;

  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  FName RightPanelTouchedAction;
};
