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

#include "IsdkIInteractableState.h"
#include "Components/SceneComponent.h"

#include "IsdkPokeButtonVisual.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class PokeButtonVisual;

		namespace helper
		{
			class FPokeButtonVisualImpl;
		}
	}
} // namespace isdk::api

class UIsdkPokeInteractable;

/**
 * @class UIsdkPokeButtonVisual
 * @brief SceneComponent representing the visual element of a button that responds to Poke
 * Interactions, must be paired with a PokeInteractable.
 *
 * @see UIsdkPokeInteractable
 * @addtogroup InteractionSDK
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Poke Button Visual"))
class OCULUSINTERACTION_API UIsdkPokeButtonVisual : public USceneComponent
{
  GENERATED_BODY()

 public:
  UIsdkPokeButtonVisual();

  /**
   * @brief Called on BeginPlay overridden from USceneComponent. Subscribes this to all interactable
   * events, and creates a callback to UIsdkWorldSubsystem::GetFrameStartingEventDelegate(), which
   * calls HandleWorldFrameStartedEvent()
   */
  virtual void BeginPlay() override;

  /**
   * @brief Called on EndPlay, overridden from USceneComponent. Unsubscribes this from all
   * interactable events previously bound to this poke visual.
   */
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  /**
   * @brief Called on BeginDestroy, overridden from USceneComponent. Resets the reference to this in
   * the implementation API to prevent references to garbage memory.
   */
  virtual void BeginDestroy() override;

  bool IsApiInstanceValid() const;
  isdk::api::PokeButtonVisual* GetApiPokeButtonVisual();

  /**
   * @brief Returns the UIsdkPokeInteractable associated with this Visual
   * @return UIsdkPokeInteractable* The Poke Interactable that this visual element is intended to
   * represent
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkPokeInteractable* GetPokeInteractable() const
  {
    return PokeInteractable;
  }

  /**
   * @brief Sets the UIsdkPokeInteractable associated with this Visual
   * @param InPokeInteractable The Poke Interactable that this visual element is intended to
   * represent
   */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetPokeInteractable(UIsdkPokeInteractable* InPokeInteractable);

  /**
   * @brief Called when the transform of this component is updated. Overridden from USceneComponent.
   * @param UpdateTransformFlags The physics flags to apply when calculating this transform change
   * @param Teleport The type of movement associated with this transform change
   */
  virtual void OnUpdateTransform(
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport = ETeleportType::None) override;

 private:
  float MaxExtentDistance;
  FTransform RelativeTransform;

  void SetWorldLocationFromInstance();
  void SubscribeInteractableEvents();
  void UnsubscribeInteractableEvents();
  void SetUpdatePositionEnabled(EIsdkInteractableState InteractableState);

  UFUNCTION()
  void HandleInteractableStateChanged(const FIsdkInteractableStateEvent& Event);

  UFUNCTION()
  void HandleWorldFrameFinishedEvent();

  UFUNCTION()
  void HandleWorldFrameStartedEvent();

  /* The UIsdkPokeInteractable associated with this Visual */
  UPROPERTY(
      BlueprintGetter = GetPokeInteractable,
      BlueprintSetter = SetPokeInteractable,
      Category = InteractionSDK)
  UIsdkPokeInteractable* PokeInteractable{};

  // Internal Impl
  TPimplPtr<isdk::api::helper::FPokeButtonVisualImpl> PokeButtonVisualImpl;
};
