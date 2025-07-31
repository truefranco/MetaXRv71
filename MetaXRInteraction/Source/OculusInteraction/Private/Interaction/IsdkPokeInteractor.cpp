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

#include "Interaction/IsdkPokeInteractor.h"

#include <Subsystem/IsdkWorldSubsystem.h>
#include "Templates/PimplPtr.h"
#include "StructTypesPrivate.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "DataSources/IsdkIRootPose.h"
#include "IsdkRuntimeSettings.h"
#include "Utilities/IsdkDebugUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

using isdk::api::PokeInteractor;
using isdk::api::PokeInteractorPtr;

namespace isdk
{
extern TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DebugInteractionVisuals;
}

namespace isdk {
	namespace api {
		namespace helper
		{
			class FPokeInteractorImpl : public FApiImpl<PokeInteractor, PokeInteractorPtr>
			{
			public:
				explicit FPokeInteractorImpl(std::function<PokeInteractorPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkPokeInteractor::UIsdkPokeInteractor()
{
  PrimaryComponentTick.bCanEverTick = true;
  PokeInteractorImpl = MakePimpl<isdk::api::helper::FPokeInteractorImpl>(
      [this]() -> PokeInteractorPtr
      {
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

        // Get Time Provider
        const UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
        const isdk::api::ScaledTimeProvider* ScaledTimeProvider =
            WorldSubsystem.GetApiScaledTimeProvider();

        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(
                ScaledTimeProvider != nullptr, this, TEXT("ScaledTimeProvider")))
        {
          return nullptr;
        }

        // Create the interactor
        isdk_PokeInteractor_Config ApiConfig;
        StructTypesUtils::Copy(Config, ApiConfig);
        PokeInteractorPtr Instance = PokeInteractor::createWithOptions(
            &ApiConfig,
            ApiActiveState->getIActiveStateHandle(),
            *InteractorPayload,
            nullptr,
            *ScaledTimeProvider);
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
        {
          return nullptr;
        }

        Instance->setPayload(*InteractorPayload);
        return Instance;
      });
  InteractorTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Interactor_Poke);
}

void UIsdkPokeInteractor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  PokeInteractorImpl->DestroyInstance();
}

void UIsdkPokeInteractor::BeginDestroy()
{
  Super::BeginDestroy();
  PokeInteractorImpl.Reset();
}

void UIsdkPokeInteractor::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  if (IsRootPoseValid())
  {
    SetPointTransform(GetComponentTransform());

#if !UE_BUILD_SHIPPING
    if (bDisableDebugVisuals &&
        isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
    {
      DrawDebugVisuals();
    }
#endif
  }
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UIsdkPokeInteractor::IsApiInstanceValid() const
{
  return PokeInteractorImpl->IsInstanceValid();
}

isdk::api::IInteractor* UIsdkPokeInteractor::GetApiIInteractor() const
{
  return GetApiPokeInteractor();
}

PokeInteractor* UIsdkPokeInteractor::GetApiPokeInteractor() const
{
  return PokeInteractorImpl->GetOrCreateInstance();
}

void UIsdkPokeInteractor::SetRootPose(const TScriptInterface<IIsdkIRootPose>& InRootPose)
{
  RootPose = InRootPose;
}

void UIsdkPokeInteractor::SetConfig(const FIsdkPokeInteractor_Config& InConfig)
{
  Config = InConfig;
  if (PokeInteractorImpl->IsInstanceValid())
  {
    auto Instance = PokeInteractorImpl->GetOrCreateInstance();
    isdk_PokeInteractor_Config ApiConfig;
    StructTypesUtils::Copy(Config, ApiConfig);

    const bool bWasEnabled = Instance->getState() != isdk_InteractorState_Disabled;
    if (bWasEnabled)
    {
      // Can only set config on a disabled interactor.
      Instance->disable();
    }
    Instance->setConfig(&ApiConfig);
    if (bWasEnabled)
    {
      Instance->enable();
    }
  }
}

bool UIsdkPokeInteractor::IsRootPoseValid() const
{
  // If no root pose interface was provided, fall back to just using the transform of the component.
  if (!IsValid(RootPose.GetObject()))
  {
    return true;
  }
  return IIsdkIRootPose::Execute_IsRootPoseValid(RootPose.GetObject());
}

bool UIsdkPokeInteractor::ShouldInteractorBeEnabled() const
{
  bool bIsSelecting = false;
  if (IsApiInstanceValid())
  {
    bIsSelecting = GetApiIInteractor()->getState() == isdk_InteractorState_Select;
  }

  return Super::ShouldInteractorBeEnabled() && (IsRootPoseValid() || bIsSelecting);
}

void UIsdkPokeInteractor::SetPointTransform(const FTransform& Transform)
{
  auto Instance = GetApiPokeInteractor();
  if (Instance != nullptr)
  {
    ovrpPosef InteractorPose;
    StructTypesUtils::Copy(Transform, InteractorPose);
    Instance->setPointTransform(&InteractorPose);
  }
}

void UIsdkPokeInteractor::DrawDebugVisuals() const
{
  const auto InteractorState = GetInteractorState();
  FColor DebugColor = UIsdkDebugUtils::GetInteractorStateDebugColor(InteractorState);

  DrawDebugSphere(GetWorld(), GetComponentLocation(), Config.Radius, 12, DebugColor);
  UE_VLOG_LOCATION(
      GetOwner(),
      LogOculusInteraction,
      Verbose,
      GetComponentLocation(),
      Config.Radius,
      DebugColor,
      TEXT_EMPTY);
}
