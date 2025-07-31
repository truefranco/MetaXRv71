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

#include "Rig/IsdkTrackedDataSourceRigComponent.h"
#include "Components/SceneComponent.h"

#include "IsdkFunctionLibrary.h"
#include "OculusInteractionPrebuiltsLog.h"
#include "Subsystem/IsdkWorldSubsystem.h"

// Sets default values for this component's properties
UIsdkTrackedDataSourceRigComponent::UIsdkTrackedDataSourceRigComponent()
{
  PrimaryComponentTick.bCanEverTick = false;
  ForceOffVisibility =
      CreateDefaultSubobject<UIsdkConditionalGroupAll>(TEXT("ForceOffVisibilityConditionalAll"));
}

void UIsdkTrackedDataSourceRigComponent::BeginPlay()
{
  Super::BeginPlay();

  UIsdkWorldSubsystem::Get(GetWorld())
      .GetFrameFinishedEventDelegate()
      .AddDynamic(this, &UIsdkTrackedDataSourceRigComponent::HandleIsdkFrameFinished);
}

void UIsdkTrackedDataSourceRigComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  UIsdkWorldSubsystem::Get(GetWorld())
      .GetFrameFinishedEventDelegate()
      .RemoveDynamic(this, &UIsdkTrackedDataSourceRigComponent::HandleIsdkFrameFinished);
}

bool UIsdkTrackedDataSourceRigComponent::TryAttachToParentMotionController(USceneComponent* Target)
{
  // If a motion controller wasn't attached by someone in BeginPlay via AttachToMotionController,
  // look for one now.
  // This needs to happen in Tick() since the ChildActorComponent will not attach our root
  // component until after BeginPlay() has been called.
  USceneComponent* AttachParentComponent = Target;
  while (IsValid(AttachParentComponent))
  {
    if (AttachParentComponent->IsA(UMotionControllerComponent::StaticClass()))
    {
      AttachedToMotionController = Cast<UMotionControllerComponent>(AttachParentComponent);
      break;
    }
    AttachParentComponent = AttachParentComponent->GetAttachParent();
  }

  if (ensureMsgf(
          AttachedToMotionController,
          TEXT("%s must be attached to the hierarchy beneith a UMotionControllerComponent"),
          *GetName()))
  {
    auto* TrackedVisual = GetTrackedVisual();
    if (IsValid(TrackedVisual))
    {
      TrackedVisual->AttachToComponent(
          AttachedToMotionController, FAttachmentTransformRules::KeepRelativeTransform);
    }

    auto* SyntheticVisual = GetSyntheticVisual();
    if (IsValid(SyntheticVisual))
    {
      // This actor doesn't support cases where the MotionControllerComponent is the root component.
      const AActor* SyntheticAttachParent = AttachedToMotionController->GetOwner();
      if (ensureMsgf(
              SyntheticAttachParent != nullptr,
              TEXT("%s: Parent UMotionControllerComponent must be attached to an actor"),
              *GetName()))
      {
        // Since we are wanting to attach components directly to the attach parent actor's root,
        // above the motion controller - the motion controller itself cannot be the root component.
        checkf(
            SyntheticAttachParent->GetRootComponent() != AttachedToMotionController,
            TEXT("%s: Parent UMotionControllerComponent cannot be the root component of its actor"),
            *GetName());

        SyntheticVisual->AttachToComponent(
            SyntheticAttachParent->GetRootComponent(),
            FAttachmentTransformRules::KeepRelativeTransform);

        return true;
      }
    }
    else
    {
      return true;
    }
  }

  return false;
}

void UIsdkTrackedDataSourceRigComponent::CreateDataSourcesAsTrackedHand()
{
  const auto TrackingDataSubsystem = GetTrackingDataSubsystem();

  LocalDataSources = FIsdkTrackingDataSources();
  if (IsValid(TrackingDataSubsystem.GetObject()))
  {
    LocalDataSources = IIsdkITrackingDataSubsystem::Execute_GetOrCreateHandDataSourceComponent(
        TrackingDataSubsystem.GetObject(), AttachedToMotionController, Handedness);
  }

  OnDataSourcesCreated();
}

void UIsdkTrackedDataSourceRigComponent::CreateDataSourcesAsTrackedController()
{
  const auto TrackingDataSubsystem = GetTrackingDataSubsystem();

  LocalDataSources = FIsdkTrackingDataSources();
  if (IsValid(TrackingDataSubsystem.GetObject()))
  {
    LocalDataSources =
        IIsdkITrackingDataSubsystem::Execute_GetOrCreateControllerDataSourceComponent(
            TrackingDataSubsystem.GetObject(), AttachedToMotionController, Handedness);
  }

  OnDataSourcesCreated();
}

const FIsdkTrackingDataSources& UIsdkTrackedDataSourceRigComponent::GetDataSources() const
{
  return LocalDataSources;
}

UIsdkConditionalGroupAll* UIsdkTrackedDataSourceRigComponent::GetForceOffVisibility() const
{
  return ForceOffVisibility;
}

TScriptInterface<IIsdkITrackingDataSubsystem>
UIsdkTrackedDataSourceRigComponent::GetTrackingDataSubsystem()
{
  // Get data sources from the subsystem.
  if (IsValid(CustomTrackingData.GetObject()))
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Verbose,
        TEXT("%s: Using TrackingDataSubsystem provided as override"),
        *GetFullName());
    return CustomTrackingData;
  }

  UE_LOG(
      LogOculusInteractionPrebuilts,
      Verbose,
      TEXT("%s: Using TrackingDataSubsystem from world"),
      *GetFullName());

  const auto TrackingData = UIsdkFunctionLibrary::FindTrackingDataSubsystem(GetWorld());
  if (!ensure(IsValid(TrackingData.GetObject())))
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Error,
        TEXT("No Subsystem exists that implements IIsdkITrackingDataSubsystem"));
  }

  return TrackingData;
}

void UIsdkTrackedDataSourceRigComponent::HandleIsdkFrameFinished()
{
  UpdateComponentVisibility();
}

void UIsdkTrackedDataSourceRigComponent::UpdateComponentVisibility()
{
  UObject* HandRootPoseObj = LocalDataSources.HandRootPose.GetObject();
  if (HandVisibilityMode == EIsdkRigHandVisibility::Manual || !IsValid(HandRootPoseObj))
  {
    return;
  }

  bool bTrackedHandVisible = false;
  bool bSyntheticHandVisible = false;

  const bool bIsRootPoseConnected =
      IIsdkIRootPose::Execute_GetRootPoseConnectedConditional(HandRootPoseObj)->GetResolvedValue();
  if (bIsRootPoseConnected)
  {
    if (HandVisibilityMode == EIsdkRigHandVisibility::SyntheticOnly)
    {
      // Show only synthetic visual
      bTrackedHandVisible = false;
      bSyntheticHandVisible = true;
    }
    else if (HandVisibilityMode == EIsdkRigHandVisibility::SyntheticWhenRootChanged)
    {
      // Toggle visibility between the tracked hand and synthetic hand to improve latency.
      // The tracked hand is rooted to the MotionControllerComponent, which has a late-update
      // at the start of the render loop.
      // The synthetic hand is rooted to the parent actor, so does NOT have a late-update.
      // As such, unless we are synthetically modifying the position of display the low-latency
      // tracked hand mesh.
      const FTransform SyntheticTransform = GetCurrentSyntheticTransform();
      const FTransform TrackedTransform = IIsdkIRootPose::Execute_GetRootPose(HandRootPoseObj);

      // Choose to display the tracked hand if the result of the hand modifier stack was the same as
      // its input.
      bTrackedHandVisible =
          // Within 1mm tolerance
          FTransform::AreTranslationsEqual(SyntheticTransform, TrackedTransform, 0.1) &&
          // ~ 1 degree tolerance
          FTransform::AreRotationsEqual(SyntheticTransform, TrackedTransform, 0.003);
      bSyntheticHandVisible = !bTrackedHandVisible;
    }
    else
    {
      // Show only tracked visual.
      bTrackedHandVisible = true;
      bSyntheticHandVisible = false;
    }
  }

  // If present, resolve Hide Visual Conditional as final truth of visibility
  if (IsValid(GetForceOffVisibility()) && !GetForceOffVisibility()->IsEmpty())
  {
    const bool bHideVisualResolved = GetForceOffVisibility()->GetResolvedValue();
    bTrackedHandVisible &= !bHideVisualResolved;
    bSyntheticHandVisible &= !bHideVisualResolved;
  }

  auto* TrackedVisual = GetTrackedVisual();
  if (IsValid(TrackedVisual))
  {
    TrackedVisual->SetVisibility(bTrackedHandVisible);
  }

  auto* SyntheticVisual = GetSyntheticVisual();
  if (IsValid(SyntheticVisual))
  {
    SyntheticVisual->SetVisibility(bSyntheticHandVisible);
  }

  OnVisibilityUpdated(bTrackedHandVisible, bSyntheticHandVisible);
}
