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

#include "Interaction/IsdkPokeButtonVisual.h"
#include "Interaction/IsdkPokeInteractable.h"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "Math/UnrealMathUtility.h"
#include "IsdkChecks.h"
#include "StructTypesPrivate.h"
#include "Subsystem/IsdkWorldSubsystem.h"

using isdk::api::IInteractable;
using isdk::api::PokeButtonVisual;
using isdk::api::PokeButtonVisualPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FPokeButtonVisualImpl : public FApiImpl<PokeButtonVisual, PokeButtonVisualPtr>
			{
			public:
				explicit FPokeButtonVisualImpl(std::function<PokeButtonVisualPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkPokeButtonVisual::UIsdkPokeButtonVisual()
{
  PrimaryComponentTick.bCanEverTick = true;
  bWantsInitializeComponent = true;
  bWantsOnUpdateTransform = true;
  PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

  PokeButtonVisualImpl = MakePimpl<isdk::api::helper::FPokeButtonVisualImpl>(
      [this]() -> PokeButtonVisualPtr
      {
        // Check Dependencies
        if (!UIsdkChecks::EnsureMsgfDependencyIsValid(
                PokeInteractable, this, TEXT("PokeInteractable")))
        {
          return nullptr;
        }

        const auto ApiPokeInteractable = PokeInteractable->GetApiPokeInteractable();

        if (!UIsdkChecks::ValidateDependency(
                ApiPokeInteractable,
                this,
                TEXT("Poke Interactable"),
                ANSI_TO_TCHAR(__FUNCTION__),
                nullptr))
        {
          return nullptr;
        }

        const FTransform PokeInteractableTransform = PokeInteractable->GetComponentTransform();

        const FVector PlanePoint = PokeInteractableTransform.GetTranslation();
        const FVector PlaneNormal = -PokeInteractableTransform.GetRotation().GetForwardVector();
        const FVector Point = GetComponentLocation();
        const double ExtentDistance = FGenericPlatformMath::Max<double>(
            0.0, FVector::PointPlaneDist(Point, PlanePoint, PlaneNormal));

        const FTransform BaseTransform = FTransform(
            PokeInteractableTransform.GetRotation(), Point - PlaneNormal * ExtentDistance);

        // Create new visual button instance
        ovrpPosef BaseTransformOvrp;
        StructTypesUtils::Copy(BaseTransform, BaseTransformOvrp);
        PokeButtonVisualPtr Instance =
            PokeButtonVisual::create(*ApiPokeInteractable, BaseTransformOvrp, ExtentDistance);
        this->MaxExtentDistance = ExtentDistance;
        RelativeTransform = BaseTransform.GetRelativeTransform(PokeInteractableTransform);

        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
        {
          return nullptr;
        }
        return Instance;
      });
}

void UIsdkPokeButtonVisual::BeginPlay()
{
  USceneComponent::BeginPlay();

  SubscribeInteractableEvents();

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  // Ensure our instance exists at the start of the first input tick in which it is used.
  WorldSubsystem.GetFrameStartingEventDelegate().AddDynamic(
      this, &UIsdkPokeButtonVisual::HandleWorldFrameStartedEvent);
}

void UIsdkPokeButtonVisual::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  UnsubscribeInteractableEvents();
}

void UIsdkPokeButtonVisual::BeginDestroy()
{
  Super::BeginDestroy();

  PokeButtonVisualImpl.Reset();
}

void UIsdkPokeButtonVisual::HandleInteractableStateChanged(const FIsdkInteractableStateEvent& Event)
{
  SetUpdatePositionEnabled(Event.Args.NewState);
}

void UIsdkPokeButtonVisual::HandleWorldFrameStartedEvent()
{
  PokeButtonVisualImpl->GetOrCreateInstance();

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());

  // We only need to perform initialization once, unsubscribe from future events.
  WorldSubsystem.GetFrameStartingEventDelegate().RemoveDynamic(
      this, &UIsdkPokeButtonVisual::HandleWorldFrameStartedEvent);
}

void UIsdkPokeButtonVisual::HandleWorldFrameFinishedEvent()
{
  SetWorldLocationFromInstance();
}

bool UIsdkPokeButtonVisual::IsApiInstanceValid() const
{
  return PokeButtonVisualImpl->IsInstanceValid();
}

PokeButtonVisual* UIsdkPokeButtonVisual::GetApiPokeButtonVisual()
{
  return PokeButtonVisualImpl->GetOrCreateInstance();
}

void UIsdkPokeButtonVisual::SetPokeInteractable(UIsdkPokeInteractable* InPokeInteractable)
{
  if (InPokeInteractable == PokeInteractable)
  {
    return;
  }

  if (IsValid(PokeInteractable))
  {
    UnsubscribeInteractableEvents();
    SetUpdatePositionEnabled(PokeInteractable->GetCurrentState());
  }
  else
  {
    SetUpdatePositionEnabled(EIsdkInteractableState::Disabled);
  }

  PokeInteractable = InPokeInteractable;
  SubscribeInteractableEvents();
}

void UIsdkPokeButtonVisual::SetWorldLocationFromInstance()
{
  if (IsApiInstanceValid())
  {
    const auto Instance = GetApiPokeButtonVisual();
    if (Instance)
    {
      const ovrpPosef CurrentPose = Instance->getCurrentPose();

      FVector3d WorldLocation;
      StructTypesUtils::Copy(CurrentPose.Position, WorldLocation);
      SetWorldLocation((FVector)WorldLocation, false, nullptr, ETeleportType::None);
    }
  }
}

void UIsdkPokeButtonVisual::SubscribeInteractableEvents()
{
  if (PokeInteractable)
  {
    PokeInteractable->GetInteractableStateChangedDelegate()->AddUniqueDynamic(
        this, &UIsdkPokeButtonVisual::HandleInteractableStateChanged);
  }
}

void UIsdkPokeButtonVisual::UnsubscribeInteractableEvents()
{
  if (PokeInteractable)
  {
    PokeInteractable->GetInteractableStateChangedDelegate()->RemoveDynamic(
        this, &UIsdkPokeButtonVisual::HandleInteractableStateChanged);
  }
}

void UIsdkPokeButtonVisual::SetUpdatePositionEnabled(EIsdkInteractableState InteractableState)
{
  const bool bIsEnabled = InteractableState == EIsdkInteractableState::Hover ||
      InteractableState == EIsdkInteractableState::Select;

  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  if (bIsEnabled)
  {
    WorldSubsystem.GetFrameFinishedEventDelegate().AddUniqueDynamic(
        this, &UIsdkPokeButtonVisual::HandleWorldFrameFinishedEvent);
  }
  else
  {
    WorldSubsystem.GetFrameFinishedEventDelegate().RemoveDynamic(
        this, &UIsdkPokeButtonVisual::HandleWorldFrameFinishedEvent);

    // Update visual position to the max extent since it will no longer be updated from world
    // events
    SetWorldLocationFromInstance();
  }
}

void UIsdkPokeButtonVisual::OnUpdateTransform(
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport)
{
  USceneComponent::OnUpdateTransform(UpdateTransformFlags, Teleport);

  if (!IsApiInstanceValid())
  {
    return;
  }
  auto Instance = GetApiPokeButtonVisual();

  if (Instance != nullptr && IsValid(PokeInteractable))
  {
    PokeInteractable->UpdateComponentToWorld();
    const FTransform InteractableTransform = PokeInteractable->GetComponentTransform();
    const FTransform BaseTransform = RelativeTransform * InteractableTransform;
    ovrpPosef BaseTransformOvrp;
    StructTypesUtils::Copy(BaseTransform, BaseTransformOvrp);
    Instance->updateBasePose(&BaseTransformOvrp, MaxExtentDistance);

    SetWorldLocationFromInstance();
  }
}
