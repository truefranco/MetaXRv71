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

#include "Rig/IsdkHandPoseRigModifier.h"
#include "Rig/IsdkRigComponent.h"
#include "Core/IsdkConditionalGroup.h"
#include "Rig/IsdkInteractionGroupRigComponent.h"
#include "Interaction/IsdkGrabberComponent.h"
#include "Interaction/IsdkGrabbableComponent.h"
#include "Interaction/IsdkPokeInteractor.h"
#include "Interaction/IsdkRayInteractor.h"
#include "IsdkHandMeshComponent.h"
#include "OculusInteractionPrebuiltsLog.h"
#include "Subsystem/IsdkHandPoseSubsystem.h"
#include "Rig/IsdkTrackedDataSourceRigComponent.h"

void UIsdkHandPoseRigModifier::InitializeRigModifier_Implementation(
    UIsdkRigComponent* CallingRigComponent)
{
  Super::InitializeRigModifier_Implementation(CallingRigComponent);
  if (!IsValid(CallingRigComponent))
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Error,
        TEXT("UIsdkHideVisualRigModifier::InitializeRigModifier - CallingRigComponent is null!"));
    return;
  }

  HandPoseSubsystem = UIsdkHandPoseSubsystem::Get(GetWorld());
  if (HandPoseSubsystem == nullptr)
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Error,
        TEXT("UIsdkHideVisualRigModifier::InitializeRigModifier - HandPoseSubsystem is null!"));
    return;
  }

  // Get the Visual Component of the Rig Component
  UIsdkTrackedDataSourceRigComponent* VisualComponent = CallingRigComponent->GetVisuals();
  if (!IsValid(VisualComponent))
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Error,
        TEXT(
            "UIsdkHideVisualRigModifier::InitializeRigModifier - UIsdkTrackedDataSourceRigComponent is null!"));
    return;
  }
  Handedness = VisualComponent->Handedness;

  // Get the Hand Mesh Component of the Rig Component
  HandMeshComponent = Cast<UIsdkHandMeshComponent>(VisualComponent->GetSyntheticVisual());
  if (!IsValid(HandMeshComponent))
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Error,
        TEXT(
            "UIsdkHideVisualRigModifier::InitializeRigModifier - HandMeshComponent cast to UIsdkHandMeshComponent is null!"));
    return;
  }

  // Establish our bool Conditional and add it to the VisualComponent ConditionalGroup (All)
  // This is the conditional we will be potentially updating as Interactor state changes and our
  // filtering is applied

  UIsdkInteractionGroupRigComponent* InteractionGroupRig =
      CallingRigComponent->GetInteractionGroup();

  bool bBoundToDelegate = false;

  TMap<UObject*, FIsdkInteractorGroupMember> MemberMap = InteractionGroupRig->GetMemberInfoMap();
  for (const auto& Elem : MemberMap)
  {
    if (IsValid(Elem.Key) && Elem.Key->Implements<UIsdkIInteractorState>())
    {
      auto Interactor = TScriptInterface<IIsdkIInteractorState>(Elem.Key);
      FIsdkInteractorStateChanged& StateChangeDelegate =
          *Interactor->GetInteractorStateChangedDelegate();
      StateChangeDelegate.AddUniqueDynamic(
          this, &UIsdkHandPoseRigModifier::HandleInteractorStateEvent);
      bBoundToDelegate = true;
    }
  }

  if (!bBoundToDelegate)
  {
    UE_LOG(
        LogOculusInteractionPrebuilts,
        Warning,
        TEXT(
            "UIsdkHandPoseRigModifier::InitializeRigModifier - No delegates were bound! This modifier will not trigger."));
  }
}

void UIsdkHandPoseRigModifier::ShutdownRigModifier_Implementation(
    UIsdkRigComponent* CallingRigComponent)
{
  UIsdkInteractionGroupRigComponent* InteractionGroupRig =
      CallingRigComponent->GetInteractionGroup();

  TMap<UObject*, FIsdkInteractorGroupMember> MemberMap = InteractionGroupRig->GetMemberInfoMap();
  for (auto& Elem : MemberMap)
  {
    if (IsValid(Elem.Key) && Elem.Key->Implements<UIsdkIInteractorState>())
    {
      auto Interactor = TScriptInterface<IIsdkIInteractorState>(Elem.Key);
      FIsdkInteractorStateChanged& StateChangeDelegate =
          *Interactor->GetInteractorStateChangedDelegate();
      StateChangeDelegate.RemoveDynamic(
          this, &UIsdkHandPoseRigModifier::HandleInteractorStateEvent);
    }
  }
}

void UIsdkHandPoseRigModifier::HandleInteractorStateEvent(
    const FIsdkInteractorStateEvent& StateEvent)
{
  bool bFoundInteractor = false;

  // Ignore any events that involve disabled/disabling interactors or hovers, those are noise for us
  if (StateEvent.Args.NewState == EIsdkInteractorState::Disabled)
  {
    return;
  }

  // TODO: Generalize this for all interactions so we don't have to hard code them
  auto* Interactor = StateEvent.Interactor.GetObject();

  // Poke
  const UIsdkPokeInteractor* EventPokeComponent = Cast<UIsdkPokeInteractor>(Interactor);
  if (IsValid(EventPokeComponent))
  {
    bFoundInteractor = true;
  }

  // Ray
  if (!bFoundInteractor)
  {
    const UIsdkRayInteractor* EventRayComponent = Cast<UIsdkRayInteractor>(Interactor);
    if (IsValid(EventRayComponent))
    {
      bFoundInteractor = true;
    }
  }

  // Grabbable
  if (!bFoundInteractor)
  {
    UIsdkGrabberComponent* EventGrabberComponent = Cast<UIsdkGrabberComponent>(Interactor);
    if (IsValid(EventGrabberComponent))
    {
      // Has the state changed?
      if (StateEvent.Args.NewState == EIsdkInteractorState::Select && !bHandPoseEngaged)
      {
        UIsdkGrabbableComponent* GrabbableComponent = EventGrabberComponent->GetGrabbedComponent();
        if (ShouldModifierTrigger(StateEvent, GrabbableComponent))
        {
          UIsdkHandPoseData* HandPoseData = nullptr;
          USceneComponent* PoseComponent = nullptr;
          AActor* InteractableActor = GrabbableComponent->GetOwner();
          FTransform RootPoseOffset;
          FTransform SnapOffset;
          FIsdkHandGrabPoseProperties HandPosePropertiesOut;
          if (HandPoseSubsystem->CheckForHandPose(
                  Cast<USceneComponent>(GrabbableComponent),
                  HandMeshComponent,
                  StateEvent,
                  HandPoseData,
                  HandPosePropertiesOut,
                  RootPoseOffset))
          {
            if (HandPosePropertiesOut.PoseMode == EIsdkHandGrabPoseMode::SnapObjectToPose)
            {
              // Pass along the offset representing the socket we want the interactor to snap to
              EventGrabberComponent->SetInteractorSnap(
                  RootPoseOffset, HandPosePropertiesOut.SnapMoveDuration);
            }

            HandMeshComponent->SetHandPoseOverride(
                HandPoseData, RootPoseOffset, HandPosePropertiesOut, InteractableActor);
            bHandPoseEngaged = true;
          }
        }
      }
      else if (
          (StateEvent.Args.NewState == EIsdkInteractorState::Normal ||
           StateEvent.Args.NewState == EIsdkInteractorState::Hover) &&
          bHandPoseEngaged)
      {
        HandMeshComponent->ResetHandPoseOverride();
        bHandPoseEngaged = false;
      }
    }
  }
}
