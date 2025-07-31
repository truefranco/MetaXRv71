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
#include "IsdkInteractableComponent.h"
#include "Pointable/IsdkIPointable.h"
#include "Surfaces/IsdkISurface.h"

#include "IsdkRayInteractable.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class RayInteractable;

		namespace helper
		{
			class FPointerEventQueueImpl;
			class FRayInteractableImpl;
		} // namespace helper
	}
} // namespace isdk::api

/**
 * IsdkRayInteractable drives the ability for an actor to be pointed at with a directed hand raycast
 * or cursor. It is expected to be interacted with by a IsdkRayInteractor on the player pawn.
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Ray Interactable"))
class OCULUSINTERACTION_API UIsdkRayInteractable : public UIsdkInteractableComponent,
                                                   public IIsdkIPointable
{
  GENERATED_BODY()

 public:
  UIsdkRayInteractable();
  virtual void BeginPlay() override;
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void BeginDestroy() override;

  virtual bool IsApiInstanceValid() const override;
  virtual isdk::api::IInteractable* GetApiIInteractable() const override;
  isdk::api::RayInteractable* GetApiRayInteractable() const;

  /* Returns the surface implementing IIsdkISurface that this Interactable is associated with */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  TScriptInterface<IIsdkISurface> GetSurface() const
  {
    return Surface;
  }

  /* Returns the selecting surface implementing IIsdkISurface that this Interactable is associated
   * with */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  TScriptInterface<IIsdkISurface> GetSelectSurface() const
  {
    return SelectSurface;
  }

  /* Sets the surface implementing IIsdkISurface that this Interactable should be associated with */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetSurface(const TScriptInterface<IIsdkISurface>& InSurface);

  /* Sets the selecting surface implementing IIsdkISurface that this Interactable should be
   * associated with */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetSelectSurface(const TScriptInterface<IIsdkISurface>& InSurface);

  // Event Getters
  virtual FIsdkInteractionPointerEventDelegate& GetInteractionPointerEventDelegate() override
  {
    return InteractorPointerEvent;
  }

 private:
  // Event for PointerEvents
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkInteractionPointerEventDelegate InteractorPointerEvent;

  UPROPERTY(BlueprintGetter = GetSurface, BlueprintSetter = SetSurface, Category = InteractionSDK)
  TScriptInterface<IIsdkISurface> Surface{};

  UPROPERTY(
      BlueprintGetter = GetSelectSurface,
      BlueprintSetter = SetSelectSurface,
      Category = InteractionSDK)
  TScriptInterface<IIsdkISurface> SelectSurface{};

  // Internal Impl
  TPimplPtr<isdk::api::helper::FRayInteractableImpl> RayInteractableImpl;
  int64 PointerEventToken{};

  UFUNCTION()
  void HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent);
};
