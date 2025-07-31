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

#include "Interaction/IsdkInteractionEvents.h"
#include "ApiImpl.h"
#include "StructTypesPrivate.h"
#include "Interaction/Pointable/IsdkInteractionPointerEvent.h"
#include "Interaction/IsdkInteractableComponent.h"
#include "Interaction/IsdkInteractorComponent.h"
#include "OculusInteractionLog.h"
#include "IsdkChecks.h"

namespace isdk {
	namespace api {
		namespace helper
		{
			inline auto CreatePointerEventConverter(
				UIsdkInteractableComponent* SrcInteractable,
				std::function<UIsdkInteractorComponent* (const isdk_IPayload*)>&& PayloadLookup)
			{
				return [WeakInteractable = TWeakObjectPtr<UIsdkInteractableComponent>(SrcInteractable),
					PayloadLookup](const isdk_PointerEvent& ApiEvent)
					{
						auto PointerEvent =
							FIsdkInteractionPointerEvent::CreateFromPointerEvent(ApiEvent, WeakInteractable.Get());
						PointerEvent.Interactor = PayloadLookup(ApiEvent.payload);
						return PointerEvent;
					};
			}

			class IEventQueueWrapper
			{
			public:
				virtual ~IEventQueueWrapper() = default;
				virtual void TryHandleEvents() = 0;
				virtual void CreateInstanceIfNotExists() = 0;
			};

			template <
				typename TForwardingDelegate,
				typename TForwardingDelegateArg0,
				typename TTargetApiType,
				typename TTargetApiHandleType,
				typename TQueueApiType,
				typename TQueueApiTypePtr,
				typename TQueueApiEventType>
			class FEventQueueApiImpl : public FApiImpl<TQueueApiType, TQueueApiTypePtr>,
				public IEventQueueWrapper
			{
				using TBase = FApiImpl<TQueueApiType, TQueueApiTypePtr>;

			public:
				using CreateEventFn = std::function<TForwardingDelegateArg0(const TQueueApiEventType&)>;

				FEventQueueApiImpl(
					std::function<TTargetApiType* ()> GetTargetFn,
					TTargetApiHandleType* (TTargetApiType::* PGetHandleFun)() const,
					CreateEventFn&& CreateEvent,
					const TForwardingDelegate& ForwardingDelegate)
					: TBase(
						[this, GetTargetFn, PGetHandleFun]() -> TQueueApiTypePtr
						{
							const TTargetApiType* ApiTarget = GetTargetFn();
							if (!UIsdkChecks::ValidateDependency(
								ApiTarget,
								nullptr,
								TEXT("Event Queue API Target"),
								ANSI_TO_TCHAR(__FUNCTION__),
								nullptr))
							{
								return nullptr;
							}

							// EventQueue - to access events
							TQueueApiTypePtr ApiQueueInstance =
								TQueueApiType::create((ApiTarget->*PGetHandleFun)());
							if (!UIsdkChecks::ValidateCondition(
								ApiQueueInstance.IsValid(),
								nullptr,
								TEXT("Event Queue failed to create"),
								ANSI_TO_TCHAR(__FUNCTION__),
								nullptr))
							{
								return nullptr;
							}

							return ApiQueueInstance;
						}),
					ForwardingDelegate(ForwardingDelegate),
					CreateEvent(CreateEvent)
				{
				}

				virtual void TryHandleEvents() override
				{
					const auto EventQueue = static_cast<TBase*>(this)->GetOrCreateInstance();
					if (!EventQueue)
					{
						return;
					}

					// Handle the events
					if (ForwardingDelegate.IsBound())
					{
						// Process each pointer event
						while (!EventQueue->isEmpty())
						{
							const TQueueApiEventType ApiEvent{ EventQueue->pop() };
							// Let 'em know!
							TForwardingDelegateArg0 Event = CreateEvent(ApiEvent);
							ForwardingDelegate.Broadcast(Event);
						}
					}
					else
					{
						// If no listeners, just clear out the pointer events
						auto EventCount = EventQueue->getCount();
						if (EventCount > 0)
						{
							UE_LOG(
								LogOculusInteraction,
								VeryVerbose,
								TEXT("Ignoring %d events, there are no subscribers"),
								EventCount);
							EventQueue->clear();
						}
					}
				}

				virtual void CreateInstanceIfNotExists() override
				{
					static_cast<TBase*>(this)->GetOrCreateInstance();
				}

			private:
				const TForwardingDelegate& ForwardingDelegate;
				CreateEventFn CreateEvent;
			};

			class FPointerEventQueueImpl : public FEventQueueApiImpl<
				FIsdkInteractionPointerEventDelegate,
				FIsdkInteractionPointerEvent,
				IPointable,
				isdk_IPointable,
				PointerEventQueue,
				PointerEventQueuePtr,
				isdk_PointerEvent>
			{
			public:
				FPointerEventQueueImpl(
					std::function<IPointable* ()> GetTargetFn,
					isdk_IPointable* (IPointable::* PGetHandleFun)() const,
					CreateEventFn&& CreateEvent,
					const FIsdkInteractionPointerEventDelegate& ForwardingDelegate)
					: FEventQueueApiImpl(GetTargetFn, PGetHandleFun, CopyTemp(CreateEvent), ForwardingDelegate)
				{
				}
			};

			inline auto CreateInteractorStateEventConverter(UObject* Interactor)
			{
				return
					[WeakInteractor = TWeakObjectPtr(Interactor)](const isdk_InteractorStateChangeArgs& ApiEvent)
					{
						FIsdkInteractorStateEvent Event{ WeakInteractor.Get(), {} };
						StructTypesUtils::Copy(ApiEvent, Event.Args);
						return Event;
					};
			}

			class FInteractorStateEventQueueImpl : public FEventQueueApiImpl<
				FIsdkInteractorStateChanged,
				FIsdkInteractorStateEvent,
				IInteractor,
				isdk_IInteractor,
				InteractorStateEventQueue,
				InteractorStateEventQueuePtr,
				isdk_InteractorStateChangeArgs>
			{
			public:
				FInteractorStateEventQueueImpl(
					std::function<IInteractor* ()> GetTargetFn,
					isdk_IInteractor* (IInteractor::* PGetHandleFun)() const,
					CreateEventFn&& CreateEvent,
					const TCHAR* InInteractorTypeHint,
					const FIsdkInteractorStateChanged& ForwardingDelegate)
					: FEventQueueApiImpl(GetTargetFn, PGetHandleFun, CopyTemp(CreateEvent), ForwardingDelegate),
					InteractorTypeHint(InInteractorTypeHint)
				{
				}

				virtual void OnInstanceCreated(InteractorStateEventQueue* NewInstance) override
				{
					NewInstance->setEventTypeHint(StringCast<ANSICHAR>(*InteractorTypeHint).Get());
				}

			private:
				FString InteractorTypeHint;
			};

			inline auto CreateInteractableStateEventConverter(UObject* Interactable)
			{
				return [WeakInteractable =
					TWeakObjectPtr(Interactable)](const isdk_InteractableStateChangeArgs& ApiEvent)
					{
						FIsdkInteractableStateEvent Event{ WeakInteractable.Get(), {} };
						StructTypesUtils::Copy(ApiEvent, Event.Args);
						return Event;
					};
			}

			class FInteractableStateEventQueueImpl : public FEventQueueApiImpl<
				FIsdkInteractableStateChanged,
				FIsdkInteractableStateEvent,
				IInteractable,
				isdk_IInteractable,
				InteractableStateEventQueue,
				InteractableStateEventQueuePtr,
				isdk_InteractableStateChangeArgs>
			{
			public:
				FInteractableStateEventQueueImpl(
					std::function<IInteractable* ()> GetTargetFn,
					isdk_IInteractable* (IInteractable::* PGetHandleFun)() const,
					CreateEventFn&& CreateEvent,
					const FIsdkInteractableStateChanged& ForwardingDelegate)
					: FEventQueueApiImpl(GetTargetFn, PGetHandleFun, CopyTemp(CreateEvent), ForwardingDelegate)
				{
				}
			};

			class FUpdateEventQueueImpl
			{
			public:
				explicit FUpdateEventQueueImpl(
					std::function<IUpdate* ()>&& InGetTargetFn,
					const FIsdkIUpdateEventDelegate& InForwardingDelegate)
					: GetTargetFn(InGetTargetFn), ForwardingDelegate(InForwardingDelegate)
				{
				}

				void CreateInstanceIfNotExists()
				{
					GetOrCreateInstance();
				}

				IUpdate* GetOrCreateInstance()
				{
					if (!Instance)
					{
						if (bCreateInstanceFailed)
						{
							return nullptr;
						}

						Instance = GetTargetFn();

						if (!Instance)
						{
							bCreateInstanceFailed = true;
						}
					}
					return Instance;
				}

				void TryHandleEvents()
				{
					GetOrCreateInstance();
					if (Instance == nullptr)
					{
						return;
					}

					Instance->update();

					// Handle the update event listeners
					if (ForwardingDelegate.IsBound())
					{
						// Let 'em know!
						ForwardingDelegate.Broadcast();
					}
				}

				std::function<IUpdate* ()> GetTargetFn;
				IUpdate* Instance{};
				bool bCreateInstanceFailed{};
				const FIsdkIUpdateEventDelegate& ForwardingDelegate;
			};
		}
	}
} // namespace isdk::api::helper
