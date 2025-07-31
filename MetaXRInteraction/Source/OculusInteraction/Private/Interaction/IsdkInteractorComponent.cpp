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

#include "Interaction/IsdkInteractorComponent.h"
#include "Interaction/IsdkInteractableComponent.h"
#include "Interaction/IsdkIInteractableState.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "Templates/PimplPtr.h"
#include "Subsystem/IsdkWorldSubsystem.h"

using isdk::api::ActiveStateBool;
using isdk::api::ActiveStateBoolPtr;
using isdk::api::InteractorPayload;
using isdk::api::InteractorPayloadPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FInteractorPayloadImpl : public FApiImpl<InteractorPayload, InteractorPayloadPtr>
			{
			public:
				explicit FInteractorPayloadImpl(std::function<InteractorPayloadPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};

			class FInteractorActiveStateImpl : public FApiImpl<ActiveStateBool, ActiveStateBoolPtr>
			{
			public:
				explicit FInteractorActiveStateImpl(std::function<ActiveStateBoolPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkInteractorComponent::UIsdkInteractorComponent()
{
  bAutoActivate = true;
  PrimaryComponentTick.bCanEverTick = true;
  CurrentState = EIsdkInteractorState::Disabled;
  InteractorPayloadImpl =
      MakePimpl<isdk::api::helper::FInteractorPayloadImpl>(
          [this]() -> InteractorPayloadPtr
          {
            auto Instance = InteractorPayload::create(nullptr, this);
            checkf(Instance.IsValid(), TEXT("Failed to create interactor payload"));
            return Instance;
          });
  InteractorActiveStateImpl =
      MakePimpl<isdk::api::helper::FInteractorActiveStateImpl>(
          [this]() -> ActiveStateBoolPtr
          {
            auto Instance = ActiveStateBool::create();
            Instance->setActive(ShouldInteractorBeEnabled());
            checkf(Instance.IsValid(), TEXT("Failed to create interactor active state"));
            return Instance;
          });
}

void UIsdkInteractorComponent::BeginPlay()
{
  Super::BeginPlay();

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  WorldSubsystem.RegisterInteractorPayload(this);

  InteractorStateEventToken = WorldSubsystem.RegisterInteractorStateEventHandler(
      this,
      GetInteractorTypeHint(),
      [this]() -> isdk::api::IInteractor* { return GetApiIInteractor(); },
      InteractorStateChanged);
  UpdateEventToken = WorldSubsystem.RegisterUpdateEventHandler(
      [this]() -> isdk::api::IUpdate* { return GetApiIInteractor(); }, Updated);
}

void UIsdkInteractorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  if (GetCurrentState() != EIsdkInteractorState::Disabled)
  {
    // Make sure that state changed events are emitted when this object is deleted.
    GetApiIInteractor()->disable();
  }

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  WorldSubsystem.UnregisterInteractorStateEventHandler(InteractorStateEventToken);
  InteractorStateEventToken = 0;

  WorldSubsystem.UnregisterUpdateEventHandler(UpdateEventToken);
  UpdateEventToken = 0;

  WorldSubsystem.UnregisterInteractorPayload(this);

  InteractorPayloadImpl->DestroyInstance();
  InteractorActiveStateImpl->DestroyInstance();

  Super::EndPlay(EndPlayReason);
}

void UIsdkInteractorComponent::BeginDestroy()
{
  Super::BeginDestroy();

  checkf(
      !IsApiInstanceValid(),
      TEXT("Internal API instance should have been destroyed during EndPlay."));

  InteractorPayloadImpl.Reset();
  InteractorActiveStateImpl.Reset();
}

void UIsdkInteractorComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  UpdateInteractorEnabled();
}

void UIsdkInteractorComponent::Activate(bool bReset)
{
  Super::Activate(bReset);
  UpdateInteractorEnabled();
}

void UIsdkInteractorComponent::Deactivate()
{
  Super::Deactivate();
  UpdateInteractorEnabled();
}

EIsdkInteractorState UIsdkInteractorComponent::GetCurrentState() const
{
  if (IsApiInstanceValid())
  {
    return static_cast<EIsdkInteractorState>(GetApiIInteractor()->getState());
  }
  return EIsdkInteractorState::Disabled;
}

bool UIsdkInteractorComponent::HasCandidate() const
{
  if (!IsApiInstanceValid())
  {
    return false;
  }

  return !!GetApiIInteractor()->hasCandidate();
}

bool UIsdkInteractorComponent::HasInteractable() const
{
  if (!IsApiInstanceValid())
  {
    return false;
  }

  return !!GetApiIInteractor()->hasInteractable();
}

bool UIsdkInteractorComponent::HasSelectedInteractable() const
{
  if (!IsApiInstanceValid())
  {
    return false;
  }

  return !!GetApiIInteractor()->hasSelectedInteractable();
}

InteractorPayload* UIsdkInteractorComponent::GetApiInteractorPayload()
{
  return InteractorPayloadImpl->GetOrCreateInstance();
}

FIsdkInteractionRelationshipCounts UIsdkInteractorComponent::GetInteractorStateRelationshipCounts()
    const
{
  if (!IsApiInstanceValid())
  {
    return {};
  }
  isdk::api::IInteractor* Instance = GetApiIInteractor();
  const auto NumSelectingInteractables = Instance->hasSelectedInteractable() ? 1 : 0;
  const auto NumInteractables = Instance->hasInteractable() ? 1 : 0;
  return {NumInteractables - NumSelectingInteractables, NumSelectingInteractables};
}

void UIsdkInteractorComponent::GetInteractorStateRelationships(
    EIsdkInteractableState State,
    TArray<TScriptInterface<IIsdkIInteractableState>>& OutInteractables) const
{
  if (!IsApiInstanceValid())
  {
    return;
  }
  isdk::api::IInteractor* Instance = GetApiIInteractor();

  isdk_IInteractable* ApiInteractable{};
  if (State == EIsdkInteractableState::Hover && Instance->hasInteractable() &&
      GetCurrentState() == EIsdkInteractorState::Hover)
  {
    ApiInteractable = Instance->getInteractablePtr();
  }
  if (State == EIsdkInteractableState::Select && Instance->hasSelectedInteractable())
  {
    ApiInteractable = Instance->getSelectedInteractablePtr();
  }

  if (ApiInteractable)
  {
    UIsdkInteractableComponent* Interactable =
        UIsdkWorldSubsystem::Get(GetWorld()).LookupInteractable(ApiInteractable);
    OutInteractables.Add(TScriptInterface<IIsdkIInteractableState>(Interactable));
  }
}

isdk::api::IActiveState* UIsdkInteractorComponent::GetApiIActiveState()
{
  return InteractorActiveStateImpl->GetOrCreateInstance();
}

void UIsdkInteractorComponent::UpdateInteractorEnabled()
{
  SetInteractorEnabled(ShouldInteractorBeEnabled());
}

void UIsdkInteractorComponent::SetInteractorEnabled(bool IsEnabled)
{
  UIsdkWorldSubsystem* WorldSubsystem = UIsdkWorldSubsystem::TryGet(GetWorld());
  if (!IsValid(WorldSubsystem))
  {
    return;
  }

  // The active state is the primary controller of whether or not the interactor is enabled.
  // the enable/disable calls below are useful to immediately update state, but their effect will be
  // lost the next time interactor->drive() is called. The ActiveState ensures that the
  // enable/disable state is retained over calls to drive.
  // It is important that drive() is called even when we want to be disabled, so that the
  // interactor's internal event listener queues (e.g. selectors) are flushed and don't keep growing
  if (InteractorActiveStateImpl->IsInstanceValid())
  {
    InteractorActiveStateImpl->GetOrCreateInstance()->setActive(IsEnabled);
  }

  if (IsApiInstanceValid())
  {
    const auto Instance = GetApiIInteractor();
    const bool bWasEnabled = Instance->getState() != isdk_InteractorState_Disabled;
    if (IsEnabled == bWasEnabled)
    {
      return;
    }

    if (IsEnabled)
    {
      Instance->enable();
    }
    else
    {
      Instance->disable();
    }
  }
}

bool UIsdkInteractorComponent::ShouldInteractorBeEnabled() const
{
  return IsActive();
}
