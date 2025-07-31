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

#include "Interaction/IsdkPokeInteractable.h"

#include "StructTypesPrivate.h"
#include "isdk_api/isdk_api.hpp"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "IsdkContentAssetPaths.h"
#include "UObject/ConstructorHelpers.h"
#include "Subsystem/IsdkWorldSubsystem.h"
#include "IsdkRuntimeSettings.h"
#include "Debug/IsdkHasDebugSegments.h"
#include "Utilities/IsdkDebugUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

using isdk::api::IInteractable;
using isdk::api::PointerEventQueue;
using isdk::api::PointerEventQueuePtr;
using isdk::api::PokeInteractable;
using isdk::api::PokeInteractablePtr;

namespace isdk
{
extern TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DebugInteractionVisuals;
}

namespace isdk {
	namespace api {
		namespace helper
		{
			class FPokeInteractableImpl : public FApiImpl<PokeInteractable, PokeInteractablePtr>
			{
			public:
				explicit FPokeInteractableImpl(std::function<PokeInteractablePtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

FIsdkPokeInteractable_Config UIsdkPokeInteractableConfigDataAsset::CreateDefaultPanelConfig()
{
  FIsdkPokeInteractable_Config Config{};

  // Explicitly enable/disable features to match the 'shell home' experience for panels.
  Config.DragThresholdsConfig.Enabled = false;
  Config.PositionPinningConfig.Enabled = true;
  Config.MinThresholdsConfig.Enabled = false;
  Config.RecoilAssistConfig.Enabled = true;
  Config.RecoilAssistConfig.UseVelocityExpansion = true;
  Config.RecoilAssistConfig.UseVelocityRetractExpansion = true;
  Config.RecoilAssistConfig.UseDynamicDecay = true;

  return Config;
}

UIsdkPokeInteractable::UIsdkPokeInteractable()
{
  // Component tick is used only for debug purposes.  Disable for non-editor builds.
#if WITH_EDITOR
  PrimaryComponentTick.bCanEverTick = true;
#else
  PrimaryComponentTick.bCanEverTick = false;
#endif

  PokeInteractableImpl = MakePimpl<isdk::api::helper::FPokeInteractableImpl>(
      [this]() -> PokeInteractablePtr
      {
        // Surface Patch - to perform internal collision detection
        isdk::api::ISurfacePatch* ApiSurfacePatch{};
        if (!IIsdkISurfacePatch::EnsureSurfacePatchValid(
                SurfacePatch, this, TEXT("SurfacePatch"), ApiSurfacePatch))
        {
          return nullptr;
        }

        // Config
        isdk_PokeInteractable_Config ApiConfig;
        StructTypesUtils::Copy(GetCurrentConfig(), ApiConfig);

        // PokeInteractable
        auto Instance = PokeInteractable::create(
            &ApiConfig, SurfacePatch->GetApiISurfacePatch()->getISurfacePatchHandle(), nullptr);
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

  struct FStaticObjectFinders
  {
    ConstructorHelpers::FObjectFinderOptional<UIsdkPokeInteractableConfigDataAsset>
        DefaultConfigDataAsset;

    FStaticObjectFinders()
        : DefaultConfigDataAsset(IsdkContentAssetPaths::Configurations::PokeInteractablePanelConfig)
    {
    }
  } StaticObjectFinders{};

  ConfigAsset = StaticObjectFinders.DefaultConfigDataAsset.Get();

  InteractableTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Interactable_Poke);
}

void UIsdkPokeInteractable::BeginPlay()
{
  Super::BeginPlay();

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  this->PointerEventToken = WorldSubsystem.RegisterPointerEventHandler(
      [this]() -> isdk::api::IPointable* { return GetApiPokeInteractable(); },
      InteractorPointerEvent,
      this);

  // This is just for debugging, so let's turn it off in non-editor builds
#if WITH_EDITOR
  if (!GIsAutomationTesting)
  {
    InteractorPointerEvent.AddDynamic(this, &UIsdkPokeInteractable::HandlePointerEvent);
  }
#endif
}

void UIsdkPokeInteractable::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
    return;

  auto HasDebugSegments = Cast<IIsdkHasDebugSegments>(SurfacePatch.GetObject());
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

void UIsdkPokeInteractable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  WorldSubsystem.UnregisterPointerEventHandler(PointerEventToken);

  PokeInteractableImpl->DestroyInstance();
}

void UIsdkPokeInteractable::BeginDestroy()
{
  Super::BeginDestroy();

  PokeInteractableImpl.Reset();
}

bool UIsdkPokeInteractable::IsApiInstanceValid() const
{
  return PokeInteractableImpl->IsInstanceValid();
}

IInteractable* UIsdkPokeInteractable::GetApiIInteractable() const
{
  return GetApiPokeInteractable();
}

PokeInteractable* UIsdkPokeInteractable::GetApiPokeInteractable() const
{
  return PokeInteractableImpl->GetOrCreateInstance();
}

const FIsdkPokeInteractableConfigOffsets& UIsdkPokeInteractable::GetConfigOffsets() const
{
  return ConfigOffsets;
}

FIsdkPokeInteractable_Config UIsdkPokeInteractable::GetCurrentConfig() const
{
  FIsdkPokeInteractable_Config Config;
  if (IsValid(ConfigAsset))
  {
    Config = ConfigAsset->Config;
  }
  else
  {
    // Fall back to default panel config
    Config = UIsdkPokeInteractableConfigDataAsset::CreateDefaultPanelConfig();
  }

  // Apply user defined offsets additively to the config.
  Config.EnterHoverNormal += ConfigOffsets.NormalOffset;
  Config.EnterHoverTangent += ConfigOffsets.TangentOffset;
  Config.ExitHoverNormal += ConfigOffsets.NormalOffset;
  Config.ExitHoverTangent += ConfigOffsets.TangentOffset;
  Config.CancelSelectNormal += ConfigOffsets.NormalOffset;
  Config.CancelSelectTangent += ConfigOffsets.TangentOffset;

  return Config;
}

void UIsdkPokeInteractable::SetConfigOffsets(const FIsdkPokeInteractableConfigOffsets& InOffsets)
{
  ConfigOffsets = InOffsets;
  ApplyConfigToInstance();
}

void UIsdkPokeInteractable::SetConfigAsset(UIsdkPokeInteractableConfigDataAsset* InConfigAsset)
{
  ConfigAsset = InConfigAsset;
  ApplyConfigToInstance();
}

void UIsdkPokeInteractable::SetSurfacePatch(
    const TScriptInterface<IIsdkISurfacePatch>& InSurfacePatch)
{
  SurfacePatch = InSurfacePatch;

  if (PokeInteractableImpl->IsInstanceValid())
  {
    isdk::api::ISurfacePatch* ApiSurfacePatch{};
    if (IIsdkISurfacePatch::EnsureSurfacePatchValid(
            SurfacePatch, this, TEXT("SurfacePatch"), ApiSurfacePatch))
    {
      PokeInteractableImpl->GetOrCreateInstance()->setSurfacePatch(
          ApiSurfacePatch->getISurfacePatchHandle());
    }
  }
}

void UIsdkPokeInteractable::ApplyConfigToInstance() const
{
  if (PokeInteractableImpl->IsInstanceValid())
  {
    isdk_PokeInteractable_Config ApiConfig;
    StructTypesUtils::Copy(GetCurrentConfig(), ApiConfig);
    PokeInteractableImpl->GetOrCreateInstance()->setConfig(&ApiConfig);
  }
}

void UIsdkPokeInteractable::HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
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
