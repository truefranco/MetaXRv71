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

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IsdkInteractionEvents.h"
#include "Templates/PimplPtr.h"
#include "StructTypes.h"
#include "Interaction/IsdkIInteractorState.h"
#include "Core/IsdkIGameplayTagContainer.h"

#include "IsdkInteractorComponent.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class IInteractor;
		class InteractorPayload;
		class IActiveState;

		namespace helper
		{
			class FInteractorPayloadImpl;
			class FInteractorActiveStateImpl;
		} // namespace helper
	}
} // namespace isdk::api

/**
 * @class UIsdkInteractorComponent
 * @brief Abstract base class for interactors tracked by the API
 *
 * Scene component used as base class for interactor components tracked by the API. Provides
 * functions and methods for getting current state, delegates, querying relationships and
 * GameplayTag containers. Relationships are established between UIsdkInteractableComponents and
 * UIsdkInteractorComponents by WorldSubsystem and the API instance.
 *
 * @see IIsdkIGameplayTagContainer
 * @see IIsdkIInteractorState
 * @see UIsdkInteractableComponent
 * @addtogroup InteractionSDK
 */
UCLASS(Abstract)
class OCULUSINTERACTION_API UIsdkInteractorComponent : public USceneComponent,
                                                       public IIsdkIGameplayTagContainer,
                                                       public IIsdkIInteractorState
{
  GENERATED_BODY()

 public:
  /**
   * @brief Constructor, establishes API implementations and creates instance of this component
   */
  UIsdkInteractorComponent();

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
   * @brief Checks and updates the enabled state of this component every frame
   * @param DeltaTime Time since the last frame.
   * @param TickType Type of update this tick represents.
   * @param ThisTickFunction Tick function details provided by Unreal Engine.
   */
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  /**
   * @brief Sets the component as active, overridden from UActorComponent. Calls
   * UpdateInteractorEnabled()
   */
  virtual void Activate(bool bReset = false) override;

  /**
   * @brief Sets the component as inactive, overridden from UActorComponent. Calls
   * UpdateInteractorEnabled()
   */
  virtual void Deactivate() override;

  virtual isdk::api::InteractorPayload* GetApiInteractorPayload();

  /**
   * @brief Returns the current state of this interactor as an enum, implemented from interface,
   * calls GetCurrentState()
   * @see IIsdkIInteractorState
   * @return EIsdkInteractorState Current state as an enum (Normal, Hover, Select or Disabled)
   */
  virtual EIsdkInteractorState GetInteractorState() const override
  {
    return GetCurrentState();
  }

  /**
   * @brief Returns a struct containing the number of interactables this interactor is hovering and
   * selecting. Implemented from interface
   * @see IIsdkIInteractorState
   * @return FIsdkInteractionRelationshipCounts Struct containing the number of interactables this
   * interactor is hovering and/or selecting
   */
  virtual FIsdkInteractionRelationshipCounts GetInteractorStateRelationshipCounts() const override;

  /**
   * @brief Queries for all interactables that match the given state in relationship to this
   * interactor component. Implemented from interface.
   * @see IIsdkIInteractorState
   * @param State for which to check relationships (Normal, Hover, Select or Disabled)
   * @param OutInteractables Array of objects that implement IIsdkIInteractableState, passed by
   * reference, representing Interactables that meet the criteria of this query
   */
  virtual void GetInteractorStateRelationships(
      EIsdkInteractableState State,
      TArray<TScriptInterface<IIsdkIInteractableState>>& OutInteractables) const override;

  /**
   * @brief Returns the delegate used by this interactor for broadcasting state changes.
   * Implemented from interface
   * @see IIsdkIInteractorState
   * @return FIsdkInteractorStateChanged* Delegate used for broadcasting state changes
   */
  virtual FIsdkInteractorStateChanged* GetInteractorStateChangedDelegate() override
  {
    return &InteractorStateChanged;
  }

  /**
   * @brief Returns whether or not this interactor currently has an interactable selected
   * Implemented from interface
   * @see IIsdkIInteractorState
   * @return bool If this interactor is currently selecting an interactable
   */
  virtual bool HasSelectedInteractable() const override;

  /**
   * @brief Returns the current state of this interactor as an enum
   * @return EIsdkInteractorState Current state as an enum (Normal, Hover, Select or Disabled)
   */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  EIsdkInteractorState GetCurrentState() const;

  /**
   * @brief Retrieves the delegate (by reference) that is broadcast when this interactor is updated
   * @return FIsdkIUpdateEventDelegate& Reference to the Delegate broadcast when this interactor is
   * updated
   */
  FIsdkIUpdateEventDelegate& GetUpdatedEventDelegate()
  {
    return Updated;
  }

  /**
   * @brief Returns whether or not this interactor has a candidate interactable (one that could be
   * selected), called from API
   * @return bool If this interactor has at least one candidate ready
   */
  bool HasCandidate() const;

  /**
   * @brief Returns whether or not this interactor has an interactable ready. Called from API
   * @return bool If this interactor has an interactable
   */
  bool HasInteractable() const;

  // GameplayTags used for labeling and driving interaction behavior. Returned by
  // IIsdkIGameplayTagContainer::GetGameplayTagContainer().
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  FGameplayTagContainer InteractorTags;

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
    ContainerOut = InteractorTags;
    return !ContainerOut.IsEmpty();
  };

 protected:
  virtual isdk::api::IActiveState* GetApiIActiveState();

  /**
   * @brief Takes output from ShouldInteractorBeEnabled(), and then enables/distables the instance
   * accordingly
   */
  void UpdateInteractorEnabled();

  /**
   * @brief Sets whether or not this interactor should be enabled
   * @param IsEnabled The new enabled/disabled state of this interactor
   */
  void SetInteractorEnabled(bool IsEnabled);

  /**
   * @brief Returns whether or not this interactor should be enabled, using its Active status as a
   * metric
   * @return bool If this interactor should be enabled
   */
  virtual bool ShouldInteractorBeEnabled() const;

  virtual const TCHAR* GetInteractorTypeHint() const
      PURE_VIRTUAL(UIsdkInteractorComponent::GetInteractorTypeHint, return nullptr;);

  virtual bool IsApiInstanceValid() const
  {
    return false;
  }

  virtual isdk::api::IInteractor* GetApiIInteractor() const
      PURE_VIRTUAL(UIsdkInteractorComponent::GetApiIInteractor, return nullptr;);

  UPROPERTY(
      BlueprintReadOnly,
      BlueprintGetter = GetCurrentState,
      Category = InteractionSDK,
      meta = (ToolTip = "The current state of the interactor."))
  EIsdkInteractorState CurrentState;

 private:
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkInteractorStateChanged InteractorStateChanged;

  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkIUpdateEventDelegate Updated;

  TPimplPtr<isdk::api::helper::FInteractorPayloadImpl> InteractorPayloadImpl;
  TPimplPtr<isdk::api::helper::FInteractorActiveStateImpl> InteractorActiveStateImpl;

  int64 InteractorStateEventToken;
  int64 UpdateEventToken;
};
