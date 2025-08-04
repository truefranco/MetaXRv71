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
#include "GameplayTagContainer.h"
#include "Rig/IsdkRigComponent.h"
#include "StructTypes.h"
#include "IsdkRigModifier.generated.h"

struct FIsdkInteractorStateEvent;
class UInputComponent;
/**
 * @class UIsdkRigModifier
 * @brief Abstract class that is intended to modify Rig Components based on certain conditions
 *
 * Can utilize passed in state events & GameplayTag matching to determine if RigModifier should
 * trigger, based on configuration
 * @see UIsdkRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(Blueprintable, Abstract, Category = InteractionSDK)
class OCULUSINTERACTIONPREBUILTS_API UIsdkRigModifier : public UObject
{
  GENERATED_BODY()

 public:
  UIsdkRigModifier(){};

  /**
   * @brief Called by the RigComponent during initialization (Implementable in Blueprint or C++)
   * @param CallingRigComponent The Rig Component initializing this modifier
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = InteractionSDK)
  void InitializeRigModifier(UIsdkRigComponent* CallingRigComponent);

  virtual void InitializeRigModifier_Implementation(UIsdkRigComponent* CallingRigComponent);

  /**
   * @brief Called by the RigComponent during EndPlay (Implementable in Blueprint or C++)
   * @param CallingRigComponent The Rig Component shutting this down
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = InteractionSDK)
  void ShutdownRigModifier(UIsdkRigComponent* CallingRigComponent);

  virtual void ShutdownRigModifier_Implementation(UIsdkRigComponent* CallingRigComponent) {}

  /**
   * @brief Binds input actions with a given EnhancedInputAction System
   *
   *  Called by the RigComponent during initialization, if the EnhancedInputAction system is active.
   * Implementable in C++ only.
   * Must return the handles of any new InputActionBinding that was created, so that it can be
   * correctly unbound.
   */
  virtual TArray<uint32> BindInputActions(UInputComponent* InputComponent)
  {
    return {};
  }

  /** Any state entry in this array will cause this modifier to trigger */
  UPROPERTY(EditAnywhere, Category = InteractionSDK)
  TArray<EIsdkInteractorState> InteractorStateTriggers;

  /** Any state change entry in this array will cause this modifier to trigger */
  UPROPERTY(EditAnywhere, Category = InteractionSDK)
  TArray<FIsdkInteractorStateChangeArgs> InteractorStateChangeTriggers;

  /** Optional gameplay tags that must be on the interactor for this modifier to trigger */
  UPROPERTY(EditAnywhere, Category = InteractionSDK)
  FGameplayTagContainer RequiredInteractorGameplayTags;

  /** Optional gameplay tags that must be on the interactable for this modifier to trigger. Only
   * grabbable components currently supported. */
  UPROPERTY(EditAnywhere, Category = InteractionSDK)
  FGameplayTagContainer RequiredInteractableGameplayTags;

 protected:
  /** The Rig Component this RigModifier was spawned by. Set by the Rig Component during
   * initialization*/
  UPROPERTY()
  UIsdkRigComponent* ParentRigComponent = nullptr;

  /**
   * @brief Given the current triggers set for this Rig Modifier, returns whether or not this should
   * trigger based on an event.
   *
   * With the given StateEvent and Interactable, compares against the contents and state of
   * InteractorStateTriggers, InteractorStateChangeTriggers, RequiredInteractorGameplayTags and
   * RequiredInteractableGameplayTags, determines if the event will result in the RigModifier
   * triggering
   * @param StateEvent The StateEvent created by an interactor used in determining a triggered event
   * @param InteractableReference The interactable triggering the interaction
   * @return bool Whether or not, given the state of member variables, the event, and interactable,
   * if this Rig Modifier will trigger
   */
  bool ShouldModifierTrigger(FIsdkInteractorStateEvent StateEvent, UObject* InteractableReference);
};
