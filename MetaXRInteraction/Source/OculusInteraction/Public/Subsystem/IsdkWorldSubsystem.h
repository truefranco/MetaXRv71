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

#include <functional>
#include <unordered_map>

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Templates/PimplPtr.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interaction/IsdkIInteractorState.h"
#include "Interaction/IsdkIInteractableState.h"
#include "Interaction/IsdkInteractionEvents.h"
#include "Interaction/Pointable/IsdkInteractionPointerEvent.h"
#include "IsdkWorldSubsystem.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class ScaledTimeProvider;
		class IPointable;
		class IPayload;
		class InteractorPayload;
		class IInteractable;
		class IInteractor;
		class IUpdate;
		class FIsdkScaledTimeProviderImpl;
		namespace helper
		{
			class IEventQueueWrapper;
			class FUpdateEventQueueImpl;
			class FInteractableStateEventQueueImpl;
			class FInteractorStateEventQueueImpl;
			class FUpdateEventQueueImpl;
			class FPointerEventQueueImpl;
		} // namespace helper
	}
} // namespace isdk::api

typedef struct isdk_IPayload_ isdk_IPayload;
typedef struct isdk_IInteractable_ isdk_IInteractable;

class UIsdkInteractorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIsdkWorldFrameEventDelegate);

/**
 *
 */
UCLASS()
class OCULUSINTERACTION_API UIsdkWorldSubsystem : public UTickableWorldSubsystem
{
  GENERATED_BODY()

 public:
  using EventHandlerToken = int64;

  UIsdkWorldSubsystem();
  virtual void BeginDestroy() override;

  // USubsystem implementation Begin
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;
  // USubsystem implementation End

  // FTickableGameObject implementation Begin
  virtual void Tick(float DeltaSeconds) override;
  virtual bool IsTickableInEditor() const override
  {
    return false;
  }
  virtual ETickableTickType GetTickableTickType() const override;
  virtual TStatId GetStatId() const override;
  // FTickableGameObject implementation End

  isdk::api::ScaledTimeProvider* GetApiScaledTimeProvider() const;

  FIsdkWorldFrameEventDelegate& GetFrameStartingEventDelegate()
  {
    return FrameStartingEventDelegate;
  }
  FIsdkWorldFrameEventDelegate& GetFrameFinishedEventDelegate()
  {
    return FrameFinishedEventDelegate;
  }

  /**
   * Static helper function to try and get an Interaction SDK subsystem from a world.
   * Will return false if the subsystem does not exist on the given world. Use this in code
   * paths that will run game or in the editor.
   */
  static UIsdkWorldSubsystem* TryGet(const UWorld* InWorld)
  {
    check(InWorld);
    return InWorld->GetSubsystem<UIsdkWorldSubsystem>();
  }

  /**
   * Static helper function to get an Interaction SDK subsystem from a world.
   * Will fail (via check) if the subsystem does not exist on the given world.
   */
  static UIsdkWorldSubsystem& Get(const UWorld* InWorld)
  {
    checkf(IsValid(InWorld), TEXT("World passed to UIsdkWorldSubsystem::Get was nullptr"));
    UIsdkWorldSubsystem* Instance = InWorld->GetSubsystem<UIsdkWorldSubsystem>();
    checkf(
        IsValid(Instance),
        TEXT("Failed to find a UIsdkWorldSubsystem for the world %s"),
        *InWorld->GetDebugDisplayName());

    return *Instance;
  }

  using ApiPointerEventTargetFn = std::function<isdk::api::IPointable*()>;

  /**
   * Registers a handler that is used to forward pointer events that belong to the native type
   * described by InPointerEventTargetFn. The events are forwarded to the given delegate.
   * The caller is responsible for calling UnregisterPointerEventHandler with the returned token
   * before the world is destroyed.
   * Events will be ignored unless their payload was registered via RegisterInteractorPayload
   */
  EventHandlerToken RegisterPointerEventHandler(
      ApiPointerEventTargetFn&& InApiPointerEventTargetFn,
      const FIsdkInteractionPointerEventDelegate& PointerEventDelegate,
      UIsdkInteractableComponent* SrcInteractable);
  void UnregisterPointerEventHandler(EventHandlerToken Token);

  void RegisterInteractorPayload(UIsdkInteractorComponent* InInteractor);
  void UnregisterInteractorPayload(UIsdkInteractorComponent* InInteractor);

  void RegisterInteractable(
      UIsdkInteractableComponent* InInteractable,
      const isdk_IInteractable* InApiInteractable);
  void UnregisterInteractable(UIsdkInteractableComponent* InInteractable);

  using ApiInteractableEventTargetFn = std::function<isdk::api::IInteractable*()>;
  EventHandlerToken RegisterInteractableStateEventHandler(
      UObject* Interactable,
      ApiInteractableEventTargetFn&& InApiInteractableStateEventTargetFn,
      const FIsdkInteractableStateChanged& InteractableStateEventDelegate);
  void UnregisterInteractableStateEventHandler(EventHandlerToken Token);

  using ApiInteractorEventTargetFn = std::function<isdk::api::IInteractor*()>;
  EventHandlerToken RegisterInteractorStateEventHandler(
      UObject* Interactor,
      const TCHAR* InteractorTypeHint,
      ApiInteractorEventTargetFn&& InApiInteractorStateEventTargetFn,
      const FIsdkInteractorStateChanged& InteractorStateEventDelegate);
  void UnregisterInteractorStateEventHandler(EventHandlerToken Token);

  using ApiUpdateEventTargetFn = std::function<isdk::api::IUpdate*()>;
  EventHandlerToken RegisterUpdateEventHandler(
      ApiUpdateEventTargetFn&& InApiUpdateEventTargetFn,
      const FIsdkIUpdateEventDelegate& UpdateEventDelegate);
  void UnregisterUpdateEventHandler(EventHandlerToken Token);

  UIsdkInteractorComponent* LookupInteractorFromPayload(const isdk_IPayload* InPayload)
  {
    return LookupInteractorFromPayload(this, InPayload);
  }
  UIsdkInteractableComponent* LookupInteractable(isdk_IInteractable* InInteractable)
  {
    const auto FoundInteractable = RegisteredInteractablesLookup.Find(InInteractable);
    if (ensure(FoundInteractable != nullptr))
    {
      return FoundInteractable->Get();
    }
    return nullptr;
  }

 private:
  TPimplPtr<isdk::api::FIsdkScaledTimeProviderImpl> IsdkScaledTimeProviderImpl;

  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkWorldFrameEventDelegate FrameStartingEventDelegate;

  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkWorldFrameEventDelegate FrameFinishedEventDelegate;

  template <class TQueueImpl, typename TToken = EventHandlerToken>
  class EventQueueWrapper
  {
   public:
    using QueueUniquePtr = TPimplPtr<TQueueImpl>;

    EventQueueWrapper(const WIDECHAR* InName) : Name(InName) {}

    void BeginFrame()
    {
      for (const auto ElementId : PendingCreate)
      {
        const auto& Element = EventSubscriptions.FindChecked(ElementId);
        if (Element.IsValid())
        {
          Element->CreateInstanceIfNotExists();
        }
      }
      PendingCreate.Empty();

      // If anything unsubscribed during a callback, now is when we process those removals.
      ProcessPendingDeletes();
    }

    TToken RegisterEventHandler(QueueUniquePtr&& EventQueueImpl)
    {
      const EventHandlerToken EventId = NextEventId++;
      EventSubscriptions.Add(EventId, MoveTemp(EventQueueImpl));
      PendingCreate.Add(EventId);
      return EventId;
    }

    void UnregisterEventHandler(const TToken Token)
    {
      if (Token != 0)
      {
        const QueueUniquePtr& Entry = EventSubscriptions.FindChecked(Token);
        if (ensureMsgf(
                Entry.IsValid(),
                TEXT(
                    "UIsdkWorldSubsystem::%s: Attempted to unregister an EventHandler that is not registered."),
                Name))
        {
          PendingCreate.Remove(Token);
          PendingDelete.Add(Token);
        }
      }
    }

    void ProcessPendingDeletes()
    {
      for (const auto Element : PendingDelete)
      {
        EventSubscriptions.Remove(Element);
      }
      PendingDelete.Empty();
    }

    void TryHandleEvents()
    {
      for (const auto& Element : EventSubscriptions)
      {
        Element.Value->TryHandleEvents();
      }
    }

    void Reset()
    {
      PendingCreate.Empty();
      ProcessPendingDeletes();

      checkf(
          EventSubscriptions.Num() == 0,
          TEXT(
              "UIsdkWorldSubsystem::%s: %d remaining, At least one ISDK object failed to call UnregisterPointerEventHandler"),
          Name,
          EventSubscriptions.Num());

      EventSubscriptions.Empty();
    }

   private:
    TMap<TToken, QueueUniquePtr> EventSubscriptions{};
    TArray<TToken> PendingCreate{};
    TArray<TToken> PendingDelete{};
    TToken NextEventId{1};
    const WIDECHAR* Name{};
  };

  EventQueueWrapper<isdk::api::helper::FPointerEventQueueImpl> PointerEventSubscriptions{
      TEXT("PointerEventSubscriptions")};
  EventQueueWrapper<isdk::api::helper::FInteractableStateEventQueueImpl>
      InteractableStateEventSubscriptions{TEXT("InteractableStateEventSubscriptions")};
  EventQueueWrapper<isdk::api::helper::FInteractorStateEventQueueImpl>
      InteractorStateEventSubscriptions{TEXT("InteractorStateEventSubscriptions")};
  EventQueueWrapper<isdk::api::helper::FUpdateEventQueueImpl> UpdateEventSubscriptions{
      TEXT("UpdateEventSubscriptions")};

  std::unordered_map<const isdk_IPayload*, TWeakObjectPtr<UIsdkInteractorComponent>>
      RegisteredInteractorPayloadsLookup{};
  TMap<const isdk_IInteractable*, TWeakObjectPtr<UIsdkInteractableComponent>>
      RegisteredInteractablesLookup{};
  TMap<UIsdkInteractableComponent*, const isdk_IInteractable*>
      RegisteredInteractablesLookupByValue{};

  UPROPERTY()
  TArray<UIsdkInteractorComponent*> RegisteredInteractorPayloads{};

  UPROPERTY()
  TArray<UIsdkInteractableComponent*> RegisteredInteractables{};

  public:
  static UIsdkInteractorComponent* LookupInteractorFromPayload(
      TWeakObjectPtr<UIsdkWorldSubsystem> InThis,
      const isdk_IPayload* InPayload);

 protected:
  void UpdateInteractorPayloadLookup();
};
