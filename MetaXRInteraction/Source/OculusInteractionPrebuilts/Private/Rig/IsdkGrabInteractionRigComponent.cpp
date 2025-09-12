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

#include "Rig/IsdkGrabInteractionRigComponent.h"

#include "OculusInteractionPrebuiltsLog.h"
#include "Core/IsdkConditionalComponentIsActive.h"
#include "Core/IsdkConditionalGroupAll.h"
#include "Interaction/IsdkGrabberComponent.h"
#include "Interaction/GrabDetectors/IsdkHandGrabDetector.h"
#include "Rig/IsdkRigComponent.h"
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UIsdkGrabInteractionRigComponent::UIsdkGrabInteractionRigComponent()
{
  PrimaryComponentTick.bCanEverTick = true;
  bWantsInitializeComponent = true;
  bAutoActivate = true;

  Grabber = CreateDefaultSubobject<UIsdkGrabberComponent>(
      TEXT("IsdkGrabInteraction IsdkGrabberComponent"));

  EnabledConditionalGroup =
      CreateDefaultSubobject<UIsdkConditionalGroupAll>(TEXT("GrabEnabledConditionalAll"));
  IsActiveConditional =
      CreateDefaultSubobject<UIsdkConditionalComponentIsActive>(TEXT("GrabIsActiveConditional"));
}

void UIsdkGrabInteractionRigComponent::InitializeComponent()
{
  Super::InitializeComponent();

  IsActiveConditional->SetComponent(this);
  EnabledConditionalGroup->AddConditional(IsActiveConditional);
  EnabledConditionalGroup->ResolvedValueChanged.AddWeakLambda(
      this, [this](bool bIsEnabled) { HandleIsEnabledConditionalChanged(bIsEnabled); });

  RigComponent = Cast<UIsdkRigComponent>(GetOuter());
  ensureMsgf(
      IsValid(RigComponent),
      TEXT("UIsdkGrabInteractionRigComponent must be a child of an UIsdkRigComponent"));
  AddTickPrerequisiteComponent(RigComponent);
}

void UIsdkGrabInteractionRigComponent::BeginPlay()
{
  Super::BeginPlay();

  RigComponent->OnPinchSelectDelegate.AddUObject(
      this, &UIsdkGrabInteractionRigComponent::HandleRigPinchSelect);
  RigComponent->OnPinchUnselectDelegate.AddUObject(
      this, &UIsdkGrabInteractionRigComponent::HandleRigPinchUnselect);
  RigComponent->OnPalmSelectDelegate.AddUObject(
      this, &UIsdkGrabInteractionRigComponent::HandleRigPalmSelect);
  RigComponent->OnPalmUnselectDelegate.AddUObject(
      this, &UIsdkGrabInteractionRigComponent::HandleRigPalmUnselect);
}

void UIsdkGrabInteractionRigComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  const auto PinchStrength = RigComponent->GetPinchStrength();
  Grabber->SetPinchStrength(PinchStrength);

  // Handle changes to grab state
  if (Grabber->IsGrabbing() != bLastIsGrabbing)
  {
    Grabber->GetHandGrabDetector()->UpdatePinchCollidersAttachment(
        PinchAttachMesh, ThumbTipSocketName);
  }

  // Store last grabbing state for next frame
  bLastIsGrabbing = Grabber->IsGrabbing();
}

void UIsdkGrabInteractionRigComponent::HandleIsEnabledConditionalChanged(bool bIsEnabled)
{
  UE_VLOG(
      GetOwner(),
      LogOculusInteractionPrebuilts,
      Log,
      TEXT("GrabInteractionRigComponent for Rig \"%s\" Handled enabled changed: %d"),
      *GetNameSafe(RigComponent),
      bIsEnabled ? 1 : 0);

  if (!bIsEnabled)
  {
    Grabber->Deactivate();

    // Deactivate normally disables tick, but in early lifecycle events the grabber may be
    // considered "disabled", but still ticking, in which case deactivate will not disable tick.
    // We'll do it explicitly here to cover our bases.
    Grabber->SetComponentTickEnabled(false);
  }
  else
  {
    Grabber->Activate();
  }
}

void UIsdkGrabInteractionRigComponent::BindDataSources(
    const FIsdkTrackingDataSources& DataSources,
    const TScriptInterface<IIsdkIHmdDataSource>& HmdDataSourceIn,
    USceneComponent* AttachToComponent,
    FName AttachToComponentSocket)
{
  Grabber->AttachToComponent(
      AttachToComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachToComponentSocket);

  // Watch for the data sources becoming enabled/disabled.
  const auto IsConnectedConditional =
      IIsdkIRootPose::Execute_GetRootPoseConnectedConditional(DataSources.HandRootPose.GetObject());
  if (ensure(IsValid(IsConnectedConditional)))
  {
    EnabledConditionalGroup->AddConditional(IsConnectedConditional);
  }

  // Ultimately we probably want an easy, direct line to hand data for any consumer that wants it,
  // but for now we'll manually push it down into the grabber while we have a reference to it.
  HandPointerPose = DataSources.HandPointerPose;
  Grabber->SetHandPointerPose(HandPointerPose);

  if (IsValid(HmdDataSourceIn.GetObject()))
  {
    HmdDataSource = HmdDataSourceIn;
  }
}

void UIsdkGrabInteractionRigComponent::UpdateMeshDependencies(
    const FVector& PalmColliderOffset,
    USkinnedMeshComponent* InPinchAttachMesh,
    const FVector& PinchColliderOffset,
    const FName& InThumbTipSocketName)
{
  PinchAttachMesh = InPinchAttachMesh;
  ThumbTipSocketName = InThumbTipSocketName;

  Grabber->GetHandGrabDetector()->UpdatePalmOffset(PalmColliderOffset);
  Grabber->GetHandGrabDetector()->UpdatePinchCollidersAttachment(
      InPinchAttachMesh, InThumbTipSocketName);

  Grabber->GetHandGrabDetector()->UpdatePinchOffset(PinchColliderOffset);
}

void UIsdkGrabInteractionRigComponent::HandleRigPinchSelect(UIsdkRigComponent* IsdkRigComponent)
{
  if (!Grabber)
  {
    return;
  }

  Grabber->PinchGrab();
}

void UIsdkGrabInteractionRigComponent::HandleRigPinchUnselect(UIsdkRigComponent* IsdkRigComponent)
{
  if (!Grabber)
  {
    return;
  }

  Grabber->PinchRelease();
}

void UIsdkGrabInteractionRigComponent::HandleRigPalmSelect(UIsdkRigComponent* IsdkRigComponent)
{
  if (!Grabber)
  {
    return;
  }

  Grabber->PalmGrab();
}

void UIsdkGrabInteractionRigComponent::HandleRigPalmUnselect(UIsdkRigComponent* IsdkRigComponent)
{
  if (!Grabber)
  {
    return;
  }

  Grabber->PalmRelease();
}
