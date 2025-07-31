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
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "IsdkIInteractableState.h"

#include "StructTypes.h"
#include "IsdkIInteractableState.h"
#include "Core/IsdkIGameplayTagContainer.h"

#include "IsdkInteractableComponent.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class IInteractable;
		class PointerEventQueue;
	}
} // namespace isdk::api

/**
 * @class UIsdkInteractableComponent
 * @brief Abstract base class for interactables tracked by the API
 *
 * Scene component used as base class for interactable components tracked by the API. Provides
 * functions and methods for getting current state, delegates, querying relationships and
 * GameplayTag containers. Relationships are established between UIsdkInteractableComponents and
 * UIsdkInteractorComponents by WorldSubsystem and the API instance.
 *
 * @see IIsdkIGameplayTagContainer
 * @see IIsdkIInteractableState
 * @see UIsdkInteractorComponent
 * @addtogroup InteractionSDK
 */
UCLASS(Abstract)
class OCULUSINTERACTION_API UIsdkInteractableComponent : public USceneComponent,
                                                         public IIsdkIGameplayTagContainer,
                                                         public IIsdkIInteractableState
{
  GENERATED_BODY()
 public:
  UIsdkInteractableComponent();
  /**
   * @brief Called on BeginPlay, overridden from USceneComponent. Registers with World subsystem
   */
  virtual void BeginPlay() override;

  /**
   * @brief Called on EndPlay, overridden from USceneComponent. Destroys API instance and
   * unregisters this with World Subsystem.
   */
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  /**
   * @brief Called on BeginDestroy, overridden from USceneComponent. Checks if API instance was
   * destroyed during EndPlay.
   */
  virtual void BeginDestroy() override;

  /**
   * @brief Returns the current state of this interactable as an enum, implemented from interface,
   * calls GetCurrentState()
   * @see IIsdkIInteractableState
   * @return EIsdkInteractableState Current state as an enum (Normal, Hover, Select or Disabled)
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractionSDK|Scene Interactable")
  virtual EIsdkInteractableState GetInteractableState() const override
  {
    return GetCurrentState();
  }

  /**
   * @brief Returns the delegate used by this interactable for broadcasting state changes.
   * Implemented from interface
   * @see IIsdkIInteractableState
   * @return FIsdkInteractableStateChanged* Delegate used for broadcasting state changes
   */
  virtual FIsdkInteractableStateChanged* GetInteractableStateChangedDelegate() override
  {
    return &InteractableStateChanged;
  }

  /**
   * @brief Returns the current state of this interactable as an enum
   * @return EIsdkInteractableState Current state as an enum (Normal, Hover, Select or Disabled)
   */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  EIsdkInteractableState GetCurrentState() const;

  /**
   * @brief Returns a struct representing the number of interactors hovering and selecting this
   * interactable. Implemented from interface
   * @see IIsdkIInteractableState
   * @return FIsdkInteractionRelationshipCounts Struct containing the number of interactors hovering
   * and/or selecting this interactable
   */
  virtual FIsdkInteractionRelationshipCounts GetInteractableStateRelationshipCounts()
      const override;

  /**
   * @brief Queries for all interactors that match the given state in relationship to this
   * interactable component. Implemented from interface.
   * @see IIsdkIInteractableState
   * @param State for which to check relationships (Normal, Hover, Select or Disabled)
   * @param OutInteractors Array of objects that implement IIsdkIInteractorState, passed by
   * reference, representing Interactors that meet the criteria of this query
   */
  virtual void GetInteractableStateRelationships(
      EIsdkInteractableState State,
      TArray<TScriptInterface<IIsdkIInteractorState>>& OutInteractors) const override;

  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  /**
   * @brief Sets the component as active, overridden from UActorComponent. Calls
   * UpdateInteractableEnabled()
   */
  virtual void Activate(bool bReset) override;

  /**
   * @brief Sets the component as inactive, overridden from UActorComponent. Calls
   * UpdateInteractableEnabled()
   */
  virtual void Deactivate() override;

  /* GameplayTags used for labeling and driving interaction behavior. Returned by
   IIsdkIGameplayTagContainer::GetGameplayTagContainer(). */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  FGameplayTagContainer InteractableTags;

  /**
   * @brief Retrieves the current GameplayTag container for this component. Implemented from
   * interface
   * @see IIsdkIGameplayTagContainer
   * @param ContainerOut GameplayTag Container returned as reference.
   * @return bool Whether or not the returned GameplayTag container is valid and/or contains tags.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  virtual bool GetGameplayTagContainer_Implementation(FGameplayTagContainer& ContainerOut) override
  {
    ContainerOut = InteractableTags;
    return !ContainerOut.IsEmpty();
  };

 protected:
  /**
   * @brief Called when the visibility of this component has changed, calls
   * UpdateInteractableEnabled. Overridden from USceneComponent.
   */
  virtual void OnVisibilityChanged() override;

  /**
   * @brief Takes output from ShouldApiInstanceBeEnabled(), and then enables/distables the instance
   * accordingly
   */
  void UpdateInteractableEnabled();

  /**
   * @brief Determines if the instance of this interactable should be enabled within the API,
   * determined by if it is active and visible.
   * @return bool Whether or not the instance of this interactable should be enabled in the API
   */
  virtual bool ShouldApiInstanceBeEnabled() const;

  virtual bool IsApiInstanceValid() const
  {
    return false;
  }

  virtual isdk::api::IInteractable* GetApiIInteractable() const
      PURE_VIRTUAL(UIsdkInteractableComponent::GetApiIInteractable, return nullptr;);

  virtual void HandleApiInstanceCreated(isdk::api::IInteractable* ApiInstance);

  /* Current Interactable State */
  UPROPERTY(BlueprintReadOnly, BlueprintGetter = GetCurrentState, Category = InteractionSDK)
  EIsdkInteractableState CurrentState = EIsdkInteractableState::Disabled;
  int64 InteractableStateEventToken{};

  /* Delegate broadcast when the interactable state changes */
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkInteractableStateChanged InteractableStateChanged;
};
