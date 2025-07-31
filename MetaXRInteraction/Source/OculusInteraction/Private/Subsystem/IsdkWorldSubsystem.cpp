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

#include "Subsystem/IsdkWorldSubsystem.h"
#include "Templates/PimplPtr.h"
#include "Interaction/IsdkInteractorComponent.h"
#include "IsdkChecks.h"
#include "IsdkEventQueueImpl.h"
#include "StructTypesPrivate.h"

namespace isdk {
	namespace api
	{
		class FIsdkScaledTimeProviderImpl : public FApiImpl<ScaledTimeProvider, ScaledTimeProviderPtr>
		{
		public:
			explicit FIsdkScaledTimeProviderImpl(std::function<ScaledTimeProviderPtr()> CreateFn)
				: FApiImpl(MoveTemp(CreateFn))
			{
			}
		};
	}
} // namespace isdk::api

struct FInteractorPayloadLookupFunctor
{
	TWeakObjectPtr<UIsdkWorldSubsystem> WkThis;

	FInteractorPayloadLookupFunctor(UIsdkWorldSubsystem* InSubsystem)
		: WkThis(InSubsystem)
	{
	}

	UIsdkInteractorComponent* operator()(const isdk_IPayload* InPayload) const
	{
		return UIsdkWorldSubsystem::LookupInteractorFromPayload(WkThis, InPayload);
	}
};

UIsdkWorldSubsystem::UIsdkWorldSubsystem()
{
  IsdkScaledTimeProviderImpl = MakePimpl<isdk::api::FIsdkScaledTimeProviderImpl>(
      [] { return isdk::api::ScaledTimeProvider::create(); });
}

void UIsdkWorldSubsystem::BeginDestroy()
{
  Super::BeginDestroy();
  IsdkScaledTimeProviderImpl.Reset();
}

void UIsdkWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
  Super::Initialize(Collection);
}

void UIsdkWorldSubsystem::Deinitialize()
{
  Super::Deinitialize();

  PointerEventSubscriptions.Reset();
  InteractableStateEventSubscriptions.Reset();
  InteractorStateEventSubscriptions.Reset();
  UpdateEventSubscriptions.Reset();
}

void UIsdkWorldSubsystem::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  // [BeginFrame]: Replicate ISDK world state
  if (FrameStartingEventDelegate.IsBound())
  {
    FrameStartingEventDelegate.Broadcast();
  }

  auto Instance = IsdkScaledTimeProviderImpl->GetOrCreateInstance();
  if (Instance)
  {
    Instance->setScaledTimeSeconds(GetWorldRef().GetTimeSeconds());
  }

  // [BeginFrame]: Resolve any native object lazy creation. This is important for event subscribers
  // so that they exist when the events are fired from the UpdateEventSubscriptions step.
  PointerEventSubscriptions.BeginFrame();
  InteractableStateEventSubscriptions.BeginFrame();
  InteractorStateEventSubscriptions.BeginFrame();
  UpdateEventSubscriptions.BeginFrame();

  // [BeginFrame]: Prepare the Payload lookup cache, if it is stale.
  UpdateInteractorPayloadLookup();

  // [BeginFrame]: Interactors will 'drive' inside the IUpdate event
  UpdateEventSubscriptions.TryHandleEvents();

  // [PopEvent]: Read output events
  PointerEventSubscriptions.TryHandleEvents();
  InteractableStateEventSubscriptions.TryHandleEvents();
  InteractorStateEventSubscriptions.TryHandleEvents();

  // [EndFrame]
  if (FrameFinishedEventDelegate.IsBound())
  {
    FrameFinishedEventDelegate.Broadcast();
  }
}

ETickableTickType UIsdkWorldSubsystem::GetTickableTickType() const
{
  return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
}

TStatId UIsdkWorldSubsystem::GetStatId() const
{
  RETURN_QUICK_DECLARE_CYCLE_STAT(IsdkSubsystem, STATGROUP_Tickables);
}

UIsdkWorldSubsystem::EventHandlerToken UIsdkWorldSubsystem::RegisterPointerEventHandler(
	ApiPointerEventTargetFn&& InApiPointerEventTargetFn,
	const FIsdkInteractionPointerEventDelegate& PointerEventDelegate,
	UIsdkInteractableComponent* SrcInteractable)
{
	FInteractorPayloadLookupFunctor LookupFunctor(this);

	auto EventQueue = MakePimpl<isdk::api::helper::FPointerEventQueueImpl>(
		MoveTemp(InApiPointerEventTargetFn),
		&isdk::api::IPointable::getIPointableHandle,
		isdk::api::helper::CreatePointerEventConverter(SrcInteractable, MoveTemp(LookupFunctor)),
		PointerEventDelegate);

	return PointerEventSubscriptions.RegisterEventHandler(MoveTemp(EventQueue));
}

void UIsdkWorldSubsystem::UnregisterPointerEventHandler(EventHandlerToken Token)
{
  PointerEventSubscriptions.UnregisterEventHandler(Token);
}

void UIsdkWorldSubsystem::RegisterInteractorPayload(UIsdkInteractorComponent* InInteractor)
{
  if (!ensureMsgf(
          IsValid(InInteractor),
          TEXT("UIsdkWorldSubsystem::RegisterInteractorPayload - InInteractor is null"),
          *GetFullNameSafe(InInteractor)))
  {
    return;
  }

  if (!ensureMsgf(
          !RegisteredInteractorPayloads.Contains(InInteractor),
          TEXT(
              "UIsdkWorldSubsystem::RegisterInteractorPayload - Already registered interactor '%s'"),
          *GetFullNameSafe(InInteractor)))
  {
    return;
  }

  RegisteredInteractorPayloads.Add(InInteractor);
  // Clear the lookup to force rebuild next tick
  RegisteredInteractorPayloadsLookup.clear();
}

void UIsdkWorldSubsystem::UnregisterInteractorPayload(UIsdkInteractorComponent* InInteractor)
{
  if (!ensureMsgf(
          IsValid(InInteractor),
          TEXT("UIsdkWorldSubsystem::UnregisterInteractorPayload - InInteractor is null"),
          *GetFullNameSafe(InInteractor)))
  {
    return;
  }

  if (!ensureMsgf(
          RegisteredInteractorPayloads.Contains(InInteractor),
          TEXT(
              "UIsdkWorldSubsystem::UnregisterInteractorPayload - interactor was not registered '%s'"),
          *GetFullNameSafe(InInteractor)))
  {
    return;
  }

  RegisteredInteractorPayloads.Remove(InInteractor);
  // Clear the lookup to force rebuild next tick
  RegisteredInteractorPayloadsLookup.clear();
}

void UIsdkWorldSubsystem::RegisterInteractable(
    UIsdkInteractableComponent* InInteractable,
    const isdk_IInteractable* InApiInteractable)
{
  if (!ensureMsgf(
          IsValid(InInteractable),
          TEXT("UIsdkWorldSubsystem::RegisterInteractable - InInteractable is null"),
          *GetFullNameSafe(InInteractable)))
  {
    return;
  }

  if (!ensureMsgf(
          !RegisteredInteractables.Contains(InInteractable),
          TEXT("UIsdkWorldSubsystem::RegisterInteractable - Already registered interactable '%s'"),
          *GetFullNameSafe(InInteractable)))
  {
    return;
  }

  RegisteredInteractables.Add(InInteractable);
  RegisteredInteractablesLookup.FindOrAdd(InApiInteractable) = InInteractable;
  RegisteredInteractablesLookupByValue.FindOrAdd(InInteractable) = InApiInteractable;
}

void UIsdkWorldSubsystem::UnregisterInteractable(UIsdkInteractableComponent* InInteractable)
{
  if (!UIsdkChecks::ValidateUObjectDependency(
          InInteractable, this, TEXT("Interactable"), ANSI_TO_TCHAR(__FUNCTION__), nullptr))
  {
    return;
  }

  if (!UIsdkChecks::ValidateCondition(
          RegisteredInteractables.Contains(InInteractable),
          InInteractable,
          TEXT("Interactable not registered"),
          ANSI_TO_TCHAR(__FUNCTION__),
          nullptr))
  {
    return;
  }

  const auto ApiInteractable =
      RegisteredInteractablesLookupByValue.FindAndRemoveChecked(InInteractable);
  RegisteredInteractablesLookup.Remove(ApiInteractable);
  RegisteredInteractables.Remove(InInteractable);
}

UIsdkWorldSubsystem::EventHandlerToken UIsdkWorldSubsystem::RegisterInteractableStateEventHandler(
    UObject* Interactor,
    ApiInteractableEventTargetFn&& InApiInteractableStateEventTargetFn,
    const FIsdkInteractableStateChanged& InteractableStateEventDelegate)
{
  if (!ensureMsgf(
          IsValid(Interactor),
          TEXT("UIsdkWorldSubsystem::RegisterInteractableStateEventHandler - Interactor is null")))
  {
    return 0;
  }

  auto EventQueue = MakePimpl<isdk::api::helper::FInteractableStateEventQueueImpl>(
      MoveTemp(InApiInteractableStateEventTargetFn),
      &isdk::api::IInteractable::getIInteractableHandle,
      isdk::api::helper::CreateInteractableStateEventConverter(Interactor),
      InteractableStateEventDelegate);
  return InteractableStateEventSubscriptions.RegisterEventHandler(MoveTemp(EventQueue));
}

void UIsdkWorldSubsystem::UnregisterInteractableStateEventHandler(EventHandlerToken Token)
{
  InteractableStateEventSubscriptions.UnregisterEventHandler(Token);
}

UIsdkWorldSubsystem::EventHandlerToken UIsdkWorldSubsystem::RegisterInteractorStateEventHandler(
    UObject* Interactor,
    const TCHAR* InteractorTypeHint,
    ApiInteractorEventTargetFn&& InApiInteractorStateEventTargetFn,
    const FIsdkInteractorStateChanged& InteractorStateEventDelegate)
{
  if (!ensureMsgf(
          IsValid(Interactor),
          TEXT("UIsdkWorldSubsystem::RegisterInteractorStateEventHandler - Interactor is null")))
  {
    return 0;
  }

  auto EventQueue = MakePimpl<isdk::api::helper::FInteractorStateEventQueueImpl>(
      MoveTemp(InApiInteractorStateEventTargetFn),
      &isdk::api::IInteractor::getIInteractorHandle,
      isdk::api::helper::CreateInteractorStateEventConverter(Interactor),
      InteractorTypeHint,
      InteractorStateEventDelegate);
  return InteractorStateEventSubscriptions.RegisterEventHandler(MoveTemp(EventQueue));
}

void UIsdkWorldSubsystem::UnregisterInteractorStateEventHandler(EventHandlerToken Token)
{
  InteractorStateEventSubscriptions.UnregisterEventHandler(Token);
}

UIsdkWorldSubsystem::EventHandlerToken UIsdkWorldSubsystem::RegisterUpdateEventHandler(
    ApiUpdateEventTargetFn&& InApiUpdateEventTargetFn,
    const FIsdkIUpdateEventDelegate& UpdateEventDelegate)
{
  auto EventQueue = MakePimpl<isdk::api::helper::FUpdateEventQueueImpl>(
      MoveTemp(InApiUpdateEventTargetFn), UpdateEventDelegate);
  return UpdateEventSubscriptions.RegisterEventHandler(MoveTemp(EventQueue));
}

void UIsdkWorldSubsystem::UnregisterUpdateEventHandler(EventHandlerToken Token)
{
  UpdateEventSubscriptions.UnregisterEventHandler(Token);
}

void UIsdkWorldSubsystem::UpdateInteractorPayloadLookup()
{
  if (RegisteredInteractorPayloadsLookup.empty())
  {
    for (const auto RegisteredInteractor : RegisteredInteractorPayloads)
    {
      if (const auto Payload = RegisteredInteractor->GetApiInteractorPayload())
      {
        RegisteredInteractorPayloadsLookup[Payload->getIPayloadHandle()] = RegisteredInteractor;
      }
    }
  }
}

UIsdkInteractorComponent* UIsdkWorldSubsystem::LookupInteractorFromPayload(
    TWeakObjectPtr<UIsdkWorldSubsystem> InThis,
    const isdk_IPayload* InPayload)
{
  const auto* This = InThis.Get();
  if (!IsValid(This))
  {
    return nullptr;
  }

  const auto Pos = This->RegisteredInteractorPayloadsLookup.find(InPayload);
  if (Pos != This->RegisteredInteractorPayloadsLookup.end())
  {
    return Pos->second.Get();
  }
  return nullptr;
}

isdk::api::ScaledTimeProvider* UIsdkWorldSubsystem::GetApiScaledTimeProvider() const
{
  return IsdkScaledTimeProviderImpl->GetOrCreateInstance();
}
