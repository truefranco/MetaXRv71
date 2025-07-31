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
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "IsdkPokeInteractionRigComponent.generated.h"

class UIsdkRigComponent;
class UIsdkConditionalComponentIsActive;
class UIsdkConditionalGroupAll;
class UIsdkPokeInteractor;
class UIsdkPokeLimiterVisual;
class UIsdkSyntheticHand;

/**
 * @class UIsdkPokeInteractionRigComponent
 * @brief Actor component that stores references to poke-related interaction components, data
 * sources and limiters visuals as well as utilities for initializing them with data sources.
 *
 * @see UIsdkRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(ClassGroup = (InteractionSDK))
class OCULUSINTERACTIONPREBUILTS_API UIsdkPokeInteractionRigComponent : public UActorComponent
{
  GENERATED_BODY()
 public:
  /**
   * @brief PokeInteractor component reference, utilized for generating events based on state
   */
  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  UIsdkPokeInteractor* PokeInteractor;

  /**
   * @brief PokeLimiterVisual component reference, optionally utilized for preventing hand visuals
   * from poking through poked surfaces
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkPokeLimiterVisual* PokeLimiterVisual;

  /**
   * @brief Sets default values for this component's properties
   */
  UIsdkPokeInteractionRigComponent();

  /**
   * @brief Overridden from UActorComponent, called when component is initialized. Sets up
   * Conditional Groups and a reference to and tick pre-requisite for the parent RigComponent
   */
  virtual void InitializeComponent() override;

  /**
   * @brief Attaches given USceneComponent to the PokeInteractor, passes the data sources for
   * joints to it
   * @param DataSources Data source for active hand/joint poses
   * @param AttachToComponent Component to attach to
   * @param AttachToComponentSocket Component socket name to attach to on given component
   */
  void BindDataSources(
      const FIsdkTrackingDataSources& DataSources,
      USceneComponent* AttachToComponent,
      FName AttachToComponentSocket);

  /**
   * @brief Binds the hand data source and Synthetic Hand reference to the Poke Limiter Visual
   * component
   * @param DataSources Data source for active hand/joint poses
   * @param SyntheticHand The synthetic hand (related to the PokeInteractor) being used in this poke
   * interaction
   */
  void BindHandDataSources(
      const FIsdkTrackingDataSources& DataSources,
      UIsdkSyntheticHand* SyntheticHand);

  /**
   * @brief Retrieves the Conditional Group for if this component is enabled.
   * @return UIsdkConditionalGroupAll* Conditional Group. All constituent conditionals of the group
   * must be true for it to resolve true.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkConditionalGroupAll* GetEnabledConditional() const
  {
    return EnabledConditionalGroup;
  }
  /**
   * @brief Reference to the parent RigComponent
   */
 protected:
  UPROPERTY()
  UIsdkRigComponent* RigComponent;

 private:
  UFUNCTION()
  void HandleIsEnabledConditionalChanged(bool bIsEnabled);

  UPROPERTY(BlueprintGetter = GetEnabledConditional, Category = InteractionSDK)
  UIsdkConditionalGroupAll* EnabledConditionalGroup;

  UPROPERTY()
  UIsdkConditionalComponentIsActive* IsActiveConditional;
};
