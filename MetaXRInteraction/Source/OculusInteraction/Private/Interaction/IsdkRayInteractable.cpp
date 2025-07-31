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

#include "Interaction/IsdkRayInteractable.h"
#include "Templates/PimplPtr.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "Subsystem/IsdkWorldSubsystem.h"
#include "IsdkRuntimeSettings.h"
#include "Debug/IsdkHasDebugSegments.h"
#include "Utilities/IsdkDebugUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

using isdk::api::IInteractable;
using isdk::api::PointerEventQueue;
using isdk::api::PointerEventQueuePtr;
using isdk::api::RayInteractable;
using isdk::api::RayInteractablePtr;

namespace isdk
{
extern TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DebugInteractionVisuals;
}

namespace isdk {
	namespace api {
		namespace helper
		{
			class FRayInteractableImpl : public FApiImpl<RayInteractable, RayInteractablePtr>
			{
			public:
				explicit FRayInteractableImpl(std::function<RayInteractablePtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkRayInteractable::UIsdkRayInteractable()
{
  // Component tick is used only for debug purposes.  Disable for non-editor builds.
#if WITH_EDITOR
  PrimaryComponentTick.bCanEverTick = true;
#else
  PrimaryComponentTick.bCanEverTick = false;
#endif

  RayInteractableImpl = MakePimpl<isdk::api::helper::FRayInteractableImpl>(
      [this]() -> RayInteractablePtr
      {
        // Surface Patch - to perform internal collision detection
        if (!UIsdkChecks::EnsureMsgfDependencyIsValid(Surface.GetObject(), this, TEXT("Surface")))
        {
          return nullptr;
        }
        const isdk::api::ISurface* ApiSurface = Surface->GetApiISurface();
        if (!UIsdkChecks::EnsureMsgfDependencyApiIsInitialized(ApiSurface, this, TEXT("Surface")))
        {
          return nullptr;
        }

        isdk_ISurface* SelectSurfaceHandle = nullptr;
        if (SelectSurface)
        {
          if (const isdk::api::ISurface* ApiSelectSurface = SelectSurface->GetApiISurface())
          {
            SelectSurfaceHandle = ApiSelectSurface->getISurfaceHandle();
          }
        }

        // RayInteractable
        RayInteractablePtr Instance = RayInteractable::create(
            ApiSurface->getISurfaceHandle(), nullptr, SelectSurfaceHandle, nullptr, nullptr);
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
        {
          return nullptr;
        }

        // Default enabled state
        if (!ShouldApiInstanceBeEnabled())
        {
          Instance->disable();
        }

        HandleApiInstanceCreated(&Instance.Get());

        return Instance;
      });

  InteractableTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Interactable_Ray);
}

void UIsdkRayInteractable::BeginPlay()
{
  Super::BeginPlay();

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  this->PointerEventToken = WorldSubsystem.RegisterPointerEventHandler(
      [this]() -> isdk::api::IPointable* { return GetApiRayInteractable(); },
      InteractorPointerEvent,
      this);

  // This is just for debugging, so let's turn it off in non-editor builds
#if WITH_EDITOR
  if (!GIsAutomationTesting)
  {
    InteractorPointerEvent.AddDynamic(this, &UIsdkRayInteractable::HandlePointerEvent);
  }
#endif
}

void UIsdkRayInteractable::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
    return;

  auto HasDebugSegments = Cast<IIsdkHasDebugSegments>(Surface.GetObject());
  if (!HasDebugSegments)
  {
    return;
  }

  TArray<TPair<FVector, FVector>> DebugSegments;
  HasDebugSegments->GetDebugSegments(DebugSegments);

  const FColor DebugColor = UIsdkDebugUtils::GetInteractableStateDebugColor(GetCurrentState());
  for (const auto& DebugSegment : DebugSegments)
  {
    DrawDebugLine(GetWorld(), DebugSegment.Key, DebugSegment.Value, DebugColor);
    UE_VLOG_SEGMENT(
        GetOwner(),
        LogOculusInteraction,
        Verbose,
        DebugSegment.Key,
        DebugSegment.Value,
        DebugColor,
        TEXT_EMPTY);
  }
}

void UIsdkRayInteractable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  WorldSubsystem.UnregisterPointerEventHandler(PointerEventToken);

  RayInteractableImpl->DestroyInstance();
}

void UIsdkRayInteractable::BeginDestroy()
{
  Super::BeginDestroy();
  RayInteractableImpl.Reset();
}

bool UIsdkRayInteractable::IsApiInstanceValid() const
{
  return RayInteractableImpl->IsInstanceValid();
}

IInteractable* UIsdkRayInteractable::GetApiIInteractable() const
{
  return GetApiRayInteractable();
}

RayInteractable* UIsdkRayInteractable::GetApiRayInteractable() const
{
  return RayInteractableImpl->GetOrCreateInstance();
}

void UIsdkRayInteractable::SetSurface(const TScriptInterface<IIsdkISurface>& InSurface)
{
  Surface = InSurface;

  if (RayInteractableImpl->IsInstanceValid())
  {
    if (ensureMsgf(Surface, TEXT("Surface is a required input.")) &&
        ensureMsgf(Surface->GetApiISurface(), TEXT("Surface failed to initialize")))
    {
      const auto ApiSurface = Surface->GetApiISurface();
      RayInteractableImpl->GetOrCreateInstance()->setSurface(ApiSurface->getISurfaceHandle());
    }
  }
}

void UIsdkRayInteractable::SetSelectSurface(const TScriptInterface<IIsdkISurface>& InSurface)
{
  SelectSurface = InSurface;

  if (RayInteractableImpl->IsInstanceValid())
  {
    RayInteractableImpl->GetOrCreateInstance()->setSelectSurface(
        SelectSurface ? SelectSurface->GetApiISurface()->getISurfaceHandle() : nullptr);
  }
}

void UIsdkRayInteractable::HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
{
  // Only debug draw if the appropriate cvar is on
  if (!isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
  {
    return;
  }

  // Don't draw move pointer events, they're too noisy
  if (PointerEvent.Type == EIsdkPointerEventType::Move)
  {
    return;
  }

  const auto HasDebugSegments = Cast<IIsdkHasDebugSegments>(Surface.GetObject());
  if (!HasDebugSegments)
  {
    return;
  }

  const FColor DebugColor = UIsdkDebugUtils::GetPointerEventDebugColor(PointerEvent.Type);
  const auto DebugLocation = FVector(PointerEvent.Pose.Position);
  const auto DebugRadius = GetDefault<UIsdkRuntimeSettings>()->PointerEventDebugRadius;
  const auto DebugDuration = GetDefault<UIsdkRuntimeSettings>()->PointerEventDebugDuration;
  DrawDebugSphere(GetWorld(), DebugLocation, DebugRadius, 12, DebugColor, false, DebugDuration);
  UE_VLOG_LOCATION(
      GetOwner(), LogOculusInteraction, Log, DebugLocation, DebugRadius, DebugColor, TEXT_EMPTY);
  UE_VLOG(
      GetOwner(),
      LogOculusInteraction,
      Log,
      TEXT("PointerEvent\nType: %s\nInteractor: %s\nInteractable: %s\nLocation: %s"),
      *UEnum::GetValueAsString(PointerEvent.Type),
      *GetFullNameSafe(PointerEvent.Interactor),
      *GetFullNameSafe(PointerEvent.Interactable.GetObject()),
      *PointerEvent.Pose.Position.ToString());
}
