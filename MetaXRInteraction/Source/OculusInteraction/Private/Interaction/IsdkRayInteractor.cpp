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

#include "Interaction/IsdkRayInteractor.h"
#include "Templates/PimplPtr.h"
#include "StructTypesPrivate.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "Select/SelectorImpl.h"
#include "IsdkRuntimeSettings.h"
#include "Utilities/IsdkDebugUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

using isdk::api::RayInteractor;
using isdk::api::RayInteractorPtr;

namespace isdk
{
extern TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DebugInteractionVisuals;
extern TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DisableDistanceDebugging;
} // namespace isdk

namespace isdk {
	namespace api {
		namespace helper
		{
			class FRayInteractorImpl : public FApiImpl<RayInteractor, RayInteractorPtr>
			{
			public:
				explicit FRayInteractorImpl(std::function<RayInteractorPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkRayInteractor::UIsdkRayInteractor()
{
  PrimaryComponentTick.bCanEverTick = true;

  SelectorImpl = MakePimpl<isdk::api::helper::FSelectorImpl>();
  RayInteractorImpl = MakePimpl<isdk::api::helper::FRayInteractorImpl>(
      [this]() -> RayInteractorPtr
      {
        // Create the selector
        auto ApiSelector = SelectorImpl->GetOrCreateInstance();
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(
                ApiSelector != nullptr, this, TEXT("Selector")))
        {
          return nullptr;
        }

        // Get Payload
        auto InteractorPayload = GetApiInteractorPayload();
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(
                InteractorPayload != nullptr, this, TEXT("Payload")))
        {
          return nullptr;
        }

        // Get Active State
        auto ApiActiveState = GetApiIActiveState();
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(
                ApiActiveState != nullptr, this, TEXT("IActiveState")))
        {
          return nullptr;
        }

        // Create the interactor
        isdk_RayInteractor_Config ApiConfig;
        StructTypesUtils::Copy(Config, ApiConfig);
        auto Instance = RayInteractor::create(
            &ApiConfig,
            nullptr,
            *ApiSelector,
            ApiActiveState->getIActiveStateHandle(),
            *InteractorPayload);

        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
        {
          return nullptr;
        }

        return Instance;
      });
  InteractorTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Interactor_Ray);
}

void UIsdkRayInteractor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  RayInteractorImpl->DestroyInstance();
  SelectorImpl->DestroyInstance();
}

void UIsdkRayInteractor::BeginDestroy()
{
  Super::BeginDestroy();
  RayInteractorImpl.Reset();
  SelectorImpl.Reset();
}

bool UIsdkRayInteractor::IsApiInstanceValid() const
{
  return RayInteractorImpl->IsInstanceValid();
}

isdk::api::IInteractor* UIsdkRayInteractor::GetApiIInteractor() const
{
  return GetApiRayInteractor();
}

isdk::api::RayInteractor* UIsdkRayInteractor::GetApiRayInteractor() const
{
  return RayInteractorImpl->GetOrCreateInstance();
}

void UIsdkRayInteractor::SetHandPointerPose(
    const TScriptInterface<IIsdkIHandPointerPose>& InHandPointerPose)
{
  HandPointerPose = InHandPointerPose;
}

void UIsdkRayInteractor::SetConfig(const FIsdkRayInteractor_Config& InConfig)
{
  Config = InConfig;
  if (RayInteractorImpl->IsInstanceValid())
  {
    isdk_RayInteractor_Config ApiConfig;
    StructTypesUtils::Copy(Config, ApiConfig);
    RayInteractorImpl->GetOrCreateInstance()->setConfig(&ApiConfig);
  }
}

bool UIsdkRayInteractor::ShouldInteractorBeEnabled() const
{
  bool bIsSelecting = false;
  if (IsApiInstanceValid())
  {
    bIsSelecting = GetApiIInteractor()->getState() == isdk_InteractorState_Select;
  }

  bool bIsPointerPoseValid = true;
  if (IsValid(HandPointerPose.GetObject()))
  {
    bIsPointerPoseValid =
        IIsdkIHandPointerPose::Execute_IsPointerPoseValid(HandPointerPose.GetObject());
  }

  return Super::ShouldInteractorBeEnabled() && (bIsPointerPoseValid || bIsSelecting);
}

void UIsdkRayInteractor::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  // Update pointer pose of the ray
  const auto Instance = RayInteractorImpl->GetOrCreateInstance();
  if (Instance != nullptr)
  {
    ovrpPosef InteractorPose;
    FTransform PointerPose;
    bool bIsPointerPoseValid;
    if (IsValid(HandPointerPose.GetObject()))
    {
      IIsdkIHandPointerPose::Execute_GetPointerPose(
          HandPointerPose.GetObject(), PointerPose, bIsPointerPoseValid);
      if (bIsPointerPoseValid)
      {
        StructTypesUtils::Copy(PointerPose, InteractorPose);
      }
    }
    else
    {
      // Fall back to component transform if no pointer pose interface is provided
      bIsPointerPoseValid = true;
      StructTypesUtils::Copy(GetComponentTransform(), InteractorPose);
    }

    if (bIsPointerPoseValid)
    {
      Instance->setRayOrigin(&InteractorPose);
    }
    StructTypesUtils::Copy(Instance->getCollisionInfo(), CollisionInfo);

#if !UE_BUILD_SHIPPING
    if (!bDisableDebugVisuals &&
        isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
    {
      DrawDebugVisuals(PointerPose);
    }
#endif
  }

  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UIsdkRayInteractor::Select()
{
  // Don't select if already selecting
  if (GetInteractorState() == EIsdkInteractorState::Select)
  {
    return;
  }

  // Don't select unless hovering
  if (GetInteractorState() != EIsdkInteractorState::Hover)
  {
    return;
  }

  auto ApiSelector = SelectorImpl->GetOrCreateInstance();
  if (ApiSelector != nullptr)
  {
    ApiSelector->select();
  }
}

void UIsdkRayInteractor::Unselect()
{
  // Don't unselect if not currently selecting
  if (GetInteractorState() != EIsdkInteractorState::Select)
  {
    return;
  }

  auto ApiSelector = SelectorImpl->GetOrCreateInstance();
  if (ApiSelector != nullptr)
  {
    ApiSelector->unselect();
  }
}

void UIsdkRayInteractor::SetSelectStrength(float InSelectStrength)
{
  SelectStrength = InSelectStrength;
}

void UIsdkRayInteractor::DrawDebugVisuals(const FTransform& PointerPose) const
{
  if (isdk::CVar_Meta_InteractionSDK_DisableDistanceDebugging.GetValueOnAnyThread())
  {
    return;
  }
  const auto InteractorState = GetInteractorState();
  FColor DebugColor = UIsdkDebugUtils::GetInteractorStateDebugColor(InteractorState);

  // If there's a collision, render the distance to hit.  Otherwise, render the full length of the
  // ray.
  auto RayDistance = CollisionInfo.Value.Distance;
  const auto Instance = RayInteractorImpl->GetOrCreateInstance();
  if (Instance && RayDistance <= 0.f)
  {
    RayDistance = Instance->getMaxRayLength();
  }

  // If ray length is exactly 0, we consider this to be "infinite", so just draw a long line
  if (RayDistance == 0.f)
  {
    RayDistance = 10000.f;
  }

  const auto Thickness = InteractorState == EIsdkInteractorState::Select ? 0.3f : 0.2f;
  const auto RayStart = PointerPose.GetLocation();
  const auto RayEnd = RayStart + RayDistance * PointerPose.GetRotation().GetForwardVector();
  DrawDebugLine(GetWorld(), RayStart, RayEnd, DebugColor, false, -1, 0, 0.3f);
  UE_VLOG_SEGMENT_THICK(
      GetOwner(),
      LogOculusInteraction,
      Verbose,
      RayStart,
      RayEnd,
      DebugColor,
      Thickness,
      TEXT_EMPTY);
}
