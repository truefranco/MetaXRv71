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

#include "Rig/IsdkHandRigComponent.h"

#include "EnhancedInputComponent.h"
#include "IsdkHandData.h"
#include "IsdkHandMeshComponent.h"
#include "IsdkRuntimeSettings.h"
#include "Rig/IsdkHandVisualsRigComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "DataSources/IsdkSyntheticHand.h"
#include "HandPoseDetection/IsdkHandFingerPinchGrabRecognizer.h"
#include "HandPoseDetection/IsdkHandPalmGrabRecognizer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Rig/IsdkInputActionsRigComponent.h"
#include "Rig/IsdkPokeInteractionRigComponent.h"
#include "Utilities/IsdkXRUtils.h"

UIsdkHandRigComponent::UIsdkHandRigComponent() : UIsdkHandRigComponent(EIsdkHandedness::Left) {}

UIsdkHandRigComponent::UIsdkHandRigComponent(EIsdkHandedness InHandedness)
{
  Handedness = InHandedness;

  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickGroup = TG_PrePhysics;

  PinchGrabRecognizer = CreateDefaultSubobject<UIsdkHandFingerPinchGrabRecognizer>(
      TEXT("IsdkHandFingerPinchGrabRecognizer"));
  PalmGrabRecognizer =
      CreateDefaultSubobject<UIsdkHandPalmGrabRecognizer>(TEXT("IsdkHandPalmGrabRecognizer"));

  SetRigComponentDefaults();

  InteractionTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Device_Hand);
}

void UIsdkHandRigComponent::BeginPlay()
{
  Super::BeginPlay();
  if (IsValid(HandVisualsComponent) && !IsValid(HandVisualsComponent->AttachedToMotionController))
  {
    if (HandVisualsComponent->TryAttachToParentMotionController(this))
    {
      OnHandVisualsAttached();
    }
  }
}

void UIsdkHandRigComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  const FIsdkTrackingDataSources* DataSources = nullptr;
  UIsdkTrackedDataSourceRigComponent* Visuals = GetVisuals();
  if (IsValid(Visuals))
  {
    DataSources = &Visuals->GetDataSources();
  }

  TScriptInterface<IIsdkIRootPose> HandRootPose;
  if (DataSources != nullptr)
  {
    HandRootPose = DataSources->HandRootPose;
  }

  // Tick the pinch grab recognizer
  if (IsValid(PinchGrabRecognizer))
  {
    // Update the pinch grab recognizer's wrist forward vector
    if (IsValid(HandRootPose.GetObject()))
    {
      const auto WristTransform = IIsdkIRootPose::Execute_GetRootPose(HandRootPose.GetObject());
      PinchGrabRecognizer->CurrentWristForward =
          UKismetMathLibrary::GetForwardVector(WristTransform.GetRotation().Rotator());
    }

    // Update the pinch grab recognizer's hmd forward vector
    if (IsValid(HmdDataSource.GetObject()))
    {
      FTransform HmdPose;
      bool bIsTracked;
      IIsdkIHmdDataSource::Execute_GetHmdPose(HmdDataSource.GetObject(), HmdPose, bIsTracked);
      PinchGrabRecognizer->CurrentHMDForward =
          UKismetMathLibrary::GetForwardVector(HmdPose.GetRotation().Rotator());
    }
  }

  PinchGrabRecognizer->UpdateState(DeltaTime);

  // Tick the palm grab recognizer
  if (IsValid(PalmGrabRecognizer))
  {
    PalmGrabRecognizer->UpdateState(DeltaTime);
  }
}

void UIsdkHandRigComponent::BindInputActions(UEnhancedInputComponent* EnhancedInputComponent)
{
  PinchStrength = &EnhancedInputComponent->BindActionValue(InputActions->SelectStrengthAction);

  // If we don't have access to meta XR (we're using OpenXR), the pinch input action won't work, so
  // we'll fall back to using the pinch grab recognizer.
  if (!(IsdkXRUtils::IsOpenXrTrackingSystem() && PinchGrabRecognizer) &&
      IsValid(InputActions->PinchGrabAction))
  {
    EnhancedInputComponent->BindAction(
        InputActions->PinchGrabAction,
        ETriggerEvent::Triggered,
        this,
        &UIsdkHandRigComponent::HandlePinchGrabStarted);
    EnhancedInputComponent->BindAction(
        InputActions->PinchGrabAction,
        ETriggerEvent::Completed,
        this,
        &UIsdkHandRigComponent::HandlePinchGrabFinished);
    EnhancedInputComponent->BindAction(
        InputActions->PinchGrabAction,
        ETriggerEvent::Canceled,
        this,
        &UIsdkHandRigComponent::HandlePinchGrabFinished);
  }
}

FVector UIsdkHandRigComponent::GetPalmColliderOffset() const
{
  return HandPalmColliderOffset;
}

USkinnedMeshComponent* UIsdkHandRigComponent::GetPinchAttachMesh() const
{
  return Cast<USkinnedMeshComponent>(HandVisualsComponent->GetSyntheticVisual());
}

void UIsdkHandRigComponent::OnHandVisualsAttached()
{
  HandVisualsComponent->CreateDataSourcesAsTrackedHand();
}

void UIsdkHandRigComponent::UpdateComponentDataSources()
{
  Super::UpdateComponentDataSources();

  const auto DataSources = HandVisualsComponent->GetDataSources();
  PokeInteraction->BindHandDataSources(DataSources, HandVisualsComponent->SyntheticHand);

  if (IsValid(HandVisualsComponent->TrackedHandVisual))
  {
    AddTickPrerequisiteComponent(HandVisualsComponent->TrackedHandVisual);
  }

  if (IsdkXRUtils::IsOpenXrTrackingSystem() && PinchGrabRecognizer)
  {
    PinchGrabRecognizer->HandVisual = HandVisualsComponent->TrackedHandVisual;
    PinchGrabRecognizer->FingerType = EIsdkFingerType::Index;
    PinchGrabRecognizer->PinchGrabStarted.AddDynamic(
        this, &UIsdkHandRigComponent::HandlePinchGrabStarted);
    PinchGrabRecognizer->PinchGrabFinished.AddDynamic(
        this, &UIsdkHandRigComponent::HandlePinchGrabFinished);
  }

  PalmGrabRecognizer->HandVisual = HandVisualsComponent->TrackedHandVisual;
  PalmGrabRecognizer->PalmGrabStarted.AddDynamic(
      this, &UIsdkHandRigComponent::HandlePalmGrabStarted);
  PalmGrabRecognizer->PalmGrabFinished.AddDynamic(
      this, &UIsdkHandRigComponent::HandlePalmGrabFinished);

  // Setting Inbound HandData Reference to our DataSource from HandVisual
  if (IsValid(HandVisualsComponent->SyntheticHand))
  {
    DataSources.DataSourceComponent->SetInboundHandData(
        IIsdkIHandJoints::Execute_GetHandData(HandVisualsComponent->SyntheticHand));
  }
}

float UIsdkHandRigComponent::GetPinchStrength() const
{
  if (IsdkXRUtils::IsOpenXrTrackingSystem() && PinchGrabRecognizer)
  {
    return PinchGrabRecognizer->GetPinchStrength();
  }

  if (PinchStrength)
  {
    return PinchStrength->GetValue().Get<float>();
  }

  return 0.f;
}

UIsdkHandVisualsRigComponent* UIsdkHandRigComponent::GetHandVisuals() const
{
  return HandVisualsComponent;
}

UIsdkTrackedDataSourceRigComponent* UIsdkHandRigComponent::GetVisuals() const
{
  return HandVisualsComponent;
}

FName UIsdkHandRigComponent::GetThumbTipSocketName() const
{
  const auto TrackedHandVisual = HandVisualsComponent->TrackedHandVisual;
  if (!TrackedHandVisual)
  {
    return NAME_None;
  }

  return TrackedHandVisual->MappedBoneNames[static_cast<int>(EIsdkHandBones::HandThumbTip)];
}

void UIsdkHandRigComponent::HandlePinchGrabStarted()
{
  OnPinchSelectDelegate.Broadcast(this);
}

void UIsdkHandRigComponent::HandlePinchGrabFinished()
{
  OnPinchUnselectDelegate.Broadcast(this);
}

void UIsdkHandRigComponent::HandlePalmGrabStarted()
{
  OnPalmSelectDelegate.Broadcast(this);
}

void UIsdkHandRigComponent::HandlePalmGrabFinished()
{
  OnPalmUnselectDelegate.Broadcast(this);
}

UIsdkHandRigComponentLeft::UIsdkHandRigComponentLeft()
    : UIsdkHandRigComponent(EIsdkHandedness::Left)
{
  HandVisualsComponent =
      CreateDefaultSubobject<UIsdkHandVisualsRigComponentLeft>(TEXT("HandVisualsLeft"));
  HandVisualsComponent->SetupAttachment(this);
}

UIsdkHandRigComponentRight::UIsdkHandRigComponentRight()
    : UIsdkHandRigComponent(EIsdkHandedness::Right)
{
  HandVisualsComponent =
      CreateDefaultSubobject<UIsdkHandVisualsRigComponentRight>(TEXT("HandVisualsRight"));
  HandVisualsComponent->SetupAttachment(this);
}
