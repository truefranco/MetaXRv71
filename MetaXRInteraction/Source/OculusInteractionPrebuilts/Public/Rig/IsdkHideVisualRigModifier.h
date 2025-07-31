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
#include "UObject/Object.h"
#include "Rig/IsdkRigModifier.h"
#include "Core/IsdkConditionalBool.h"
#include "Interaction/IsdkInteractionEvents.h"
#include "StructTypes.h"
#include "IsdkHideVisualRigModifier.generated.h"

/*
  A Prebuilt Rig Modifier that will hide the visual (hands or controller) of a RigComponent if the
  trigger conditions are met. Does not currently support checking gameplay tags on poke & ray
  interactables.
*/
UCLASS(Blueprintable, Category = InteractionSDK)
class OCULUSINTERACTIONPREBUILTS_API UIsdkHideVisualRigModifier : public UIsdkRigModifier
{
  GENERATED_BODY()

 public:
  UIsdkHideVisualRigModifier(){};

  void InitializeRigModifier_Implementation(UIsdkRigComponent* CallingRigComponent) override;
  void ShutdownRigModifier_Implementation(UIsdkRigComponent* CallingRigComponent) override;

 private:
  /* The conditional we add to the ConditionalGroup on the Visual Component.*/
  UPROPERTY()
  UIsdkConditionalBool* VisualHideConditional = nullptr;

  /* The handler for the state event delegate that the Interactors will be firing, used for
   * comparing requirements for triggering. */
  UFUNCTION()
  void HandleInteractorStateEvent(const FIsdkInteractorStateEvent& StateEvent);

  void SetHideVisual(bool bNewHideVisual, USceneComponent* ComponentIn);

  UPROPERTY()
  USceneComponent* LastTriggeringInteractor;

  bool bHideVisualTriggered = false;
};
