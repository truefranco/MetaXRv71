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

#include "Interaction/IsdkInteractableComponent.h"
#include "Interaction/IsdkInteractorComponent.h"

#include "isdk_api/isdk_api.hpp"
#include "Subsystem/IsdkWorldSubsystem.h"
#include "Runtime/Launch/Resources/Version.h"

UIsdkInteractableComponent::UIsdkInteractableComponent()
{
  PrimaryComponentTick.bCanEverTick = true;
  bAutoActivate = true;
}

void UIsdkInteractableComponent::BeginPlay()
{
  Super::BeginPlay();

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  InteractableStateEventToken = WorldSubsystem.RegisterInteractableStateEventHandler(
      this,
      [this]() -> isdk::api::IInteractable* { return GetApiIInteractable(); },
      InteractableStateChanged);
}

void UIsdkInteractableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  const auto Interactable = GetApiIInteractable();
  if (Interactable && GetCurrentState() != EIsdkInteractableState::Disabled)
  {
    // Make sure that state changed events are emitted when this object is deleted.
    Interactable->disable();
  }

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  WorldSubsystem.UnregisterInteractableStateEventHandler(InteractableStateEventToken);
  WorldSubsystem.UnregisterInteractable(this);
}

void UIsdkInteractableComponent::BeginDestroy()
{
  Super::BeginDestroy();

  checkf(
      !IsApiInstanceValid(),
      TEXT("Internal API instance should have been destroyed during EndPlay."));
}

EIsdkInteractableState UIsdkInteractableComponent::GetCurrentState() const
{
  if (IsApiInstanceValid())
  {
    return static_cast<EIsdkInteractableState>(GetApiIInteractable()->getState());
  }
  else
  {
    return ShouldApiInstanceBeEnabled() ? EIsdkInteractableState::Normal
                                        : EIsdkInteractableState::Disabled;
  }
}

FIsdkInteractionRelationshipCounts
UIsdkInteractableComponent::GetInteractableStateRelationshipCounts() const
{
  if (!IsApiInstanceValid())
  {
    return {};
  }
  isdk::api::IInteractable* Instance = GetApiIInteractable();
  const auto SelectingInteractorsCount = Instance->getSelectingInteractorsCount();
  return {Instance->getInteractorsCount() - SelectingInteractorsCount, SelectingInteractorsCount};
}

void UIsdkInteractableComponent::GetInteractableStateRelationships(
    EIsdkInteractableState State,
    TArray<TScriptInterface<IIsdkIInteractorState>>& OutInteractors) const
{
  if (!IsApiInstanceValid())
  {
    return;
  }
  isdk::api::IInteractable* Instance = GetApiIInteractable();

  TArray<isdk_IInteractor*> ApiInteractors{};
  EIsdkInteractorState RequiredState = EIsdkInteractorState::Hover;
  if (State == EIsdkInteractableState::Select)
  {
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5)
    ApiInteractors.SetNum(Instance->getSelectingInteractorsCount(), EAllowShrinking::No);
#else
    ApiInteractors.SetNum(Instance->getSelectingInteractorsCount(), false);
#endif

    const size_t NumInteractors =
        Instance->getSelectingInteractors(ApiInteractors.GetData(), ApiInteractors.Num());
    check(NumInteractors == ApiInteractors.Num());
    RequiredState = EIsdkInteractorState::Select;
  }
  else if (State == EIsdkInteractableState::Hover)
  {
    ApiInteractors.SetNum(Instance->getInteractorsCount());
    const size_t NumInteractors =
        Instance->getInteractors(ApiInteractors.GetData(), ApiInteractors.Num());
    check(NumInteractors == ApiInteractors.Num());
    // note - this will get selecting AND hover interactors. The selected ones (if any) are filtered
    // out below.
  }

  for (auto* ApiInteractor : ApiInteractors)
  {
    isdk_IInteractorView* ApiInteractorView{};
    isdk_IPayload* ApiPayload;
    isdk_IInteractor_castToIInteractorView(ApiInteractor, &ApiInteractorView);
    isdk_IInteractorView_getPayload(ApiInteractorView, &ApiPayload);

    const UIsdkInteractorComponent* Interactor =
        UIsdkWorldSubsystem::Get(GetWorld()).LookupInteractorFromPayload(ApiPayload);

    if (Interactor->GetInteractorState() == RequiredState)
    {
      OutInteractors.Add(const_cast<UIsdkInteractorComponent*>(Interactor));
    }
  }
}

void UIsdkInteractableComponent::OnVisibilityChanged()
{
  Super::OnVisibilityChanged();
  UpdateInteractableEnabled();
}

void UIsdkInteractableComponent::Activate(bool bReset)
{
  Super::Activate(bReset);
  UpdateInteractableEnabled();
}

void UIsdkInteractableComponent::Deactivate()
{
  Super::Deactivate();
  UpdateInteractableEnabled();
}

void UIsdkInteractableComponent::UpdateInteractableEnabled()
{
  if (!IsApiInstanceValid())
  {
    return;
  }
  auto ShouldBeEnabled = ShouldApiInstanceBeEnabled();

  isdk::api::IInteractable* Instance = GetApiIInteractable();
  const bool bWasEnabled = Instance->getState() != isdk_InteractableState_Disabled;
  if (ShouldBeEnabled == bWasEnabled)
  {
    return;
  }

  if (ShouldBeEnabled)
  {
    Instance->enable();
  }
  else
  {
    Instance->disable();
  }
}

void UIsdkInteractableComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  UpdateInteractableEnabled();
}

bool UIsdkInteractableComponent::ShouldApiInstanceBeEnabled() const
{
  return IsActive() && IsVisible();
}

void UIsdkInteractableComponent::HandleApiInstanceCreated(isdk::api::IInteractable* ApiInstance)
{
  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  WorldSubsystem.RegisterInteractable(this, ApiInstance->getIInteractableHandle());
}
