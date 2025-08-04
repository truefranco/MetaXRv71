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

#include "Rig/IsdkControllerVisualsRigComponent.h"
#include "Components/InputComponent.h"
#include "Animation/AnimInstance.h"

#include "IsdkContentAssetPaths.h"
#include "IsdkControllerMeshComponent.h"
#include "IsdkFunctionLibrary.h"
#include "IsdkHandMeshComponent.h"
#include "Animation/QuestControllerAnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Rig/IsdkInputActionsRigComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Utilities/IsdkXRUtils.h"

UIsdkControllerVisualsRigComponent::UIsdkControllerVisualsRigComponent()
{
  ControllerMeshComponent =
      CreateDefaultSubobject<UIsdkControllerMeshComponent>(TEXT("ControllerMesh"));
  ControllerMeshComponent->SetAnimationMode(EAnimationMode::Type::AnimationBlueprint);
  AnimatedHandMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
  AnimatedHandMeshComponent->SetAnimationMode(EAnimationMode::Type::AnimationBlueprint);

  PoseableHandMeshComponent =
      CreateDefaultSubobject<UIsdkHandMeshComponent>(TEXT("PoseableHandMesh"));
  PoseableHandMeshComponent->SetMappedBoneNamesAsDefault();
  PoseableHandMeshComponent->SetRootPoseIgnored(true);

  // Adjust the collision settings of the ControllerMesh component
  ControllerMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  ControllerMeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

  HandVisibilityMode = EIsdkRigHandVisibility::TrackedOnly;
}

USceneComponent* UIsdkControllerVisualsRigComponent::GetTrackedVisual() const
{
  return ControllerMeshComponent;
}

void UIsdkControllerVisualsRigComponent::GetInteractorSocket(
    USceneComponent*& OutSocketComponent,
    FName& OutSocketName,
    EIsdkHandBones HandBone) const
{
  const auto ControllerHandBehavior = UIsdkFunctionLibrary::GetControllerHandBehavior(GetWorld());
  const bool bUseHandAttachment =
      ControllerHandBehavior == EControllerHandBehavior::HandsOnlyAnimated ||
      ControllerHandBehavior == EControllerHandBehavior::HandsOnlyProcedural;

  OutSocketName = NAME_None;
  if (bUseHandAttachment)
  {
    // Attach to the animated hand if in animated mode, attach to poseable hand if in procedural
    // mode.
    if (ControllerHandBehavior == EControllerHandBehavior::HandsOnlyAnimated)
    {
      OutSocketComponent = AnimatedHandMeshComponent;
    }
    else
    {
      OutSocketComponent = PoseableHandMeshComponent;
    }

    // Get the bone name for the corresponding input HandBone.  Note that both hands use the same
    // OpenXR hand skeleton, so the mapping is the same either way.
    if (HandBone != EIsdkHandBones::Invalid)
    {
      OutSocketName = PoseableHandMeshComponent->MappedBoneNames[static_cast<size_t>(HandBone)];
    }
  }
  else
  {
    OutSocketComponent = ControllerMeshComponent->GetPointerPoseComponent();
  }
}

USkeletalMeshComponent* UIsdkControllerVisualsRigComponent::GetAnimatedHandMeshComponent()
{
  return AnimatedHandMeshComponent;
}

UIsdkHandMeshComponent* UIsdkControllerVisualsRigComponent::GetPoseableHandMeshComponent()
{
  return PoseableHandMeshComponent;
}

UIsdkControllerMeshComponent* UIsdkControllerVisualsRigComponent::GetControllerMeshComponent()
{
  return ControllerMeshComponent;
}

void UIsdkControllerVisualsRigComponent::OnDataSourcesCreated()
{
  Super::OnDataSourcesCreated();

  ControllerMeshComponent->AttachToComponent(
      AttachedToMotionController, FAttachmentTransformRules::KeepRelativeTransform);
  ControllerMeshComponent->SetHandPointerPoseDataSource(LocalDataSources.HandPointerPose);
  ControllerMeshComponent->SetRootPoseDataSource(LocalDataSources.HandRootPose);

  AnimatedHandMeshComponent->AttachToComponent(
      ControllerMeshComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);

  // Create a hand data source used to drive oculus runtime derived hand posing for the
  // poseable hand mesh while in controller + hands mode
  const auto TrackingDataSubsystem = GetTrackingDataSubsystem();
  FIsdkTrackingDataSources HandDataSources;
  if (IsValid(TrackingDataSubsystem.GetObject()))
  {
    HandDataSources = IIsdkITrackingDataSubsystem::Execute_GetOrCreateHandDataSourceComponent(
        TrackingDataSubsystem.GetObject(), AttachedToMotionController, Handedness);
  }

  // Transform of the hand mesh needs to be different for left vs right hands
  const auto Transform = Handedness == EIsdkHandedness::Left
      ? IsdkXRUtils::OVR::LeftControllerHandTransform
      : IsdkXRUtils::OVR::RightControllerHandTransform;

  PoseableHandMeshComponent->AttachToComponent(
      AttachedToMotionController, FAttachmentTransformRules::SnapToTargetIncludingScale);
  PoseableHandMeshComponent->SetWorldTransform(AttachedToMotionController->GetComponentTransform());
  PoseableHandMeshComponent->SetRelativeTransform(Transform);
  PoseableHandMeshComponent->SetJointsDataSource(HandDataSources.DataSourceComponent);
  PoseableHandMeshComponent->SetRootPoseDataSource(HandDataSources.DataSourceComponent);
}

void UIsdkControllerVisualsRigComponent::OnVisibilityUpdated(
    bool bTrackedVisibility,
    bool bSyntheticVisibility)
{
  Super::OnVisibilityUpdated(bTrackedVisibility, bSyntheticVisibility);

  const auto ControllerHandBehavior = UIsdkFunctionLibrary::GetControllerHandBehavior(GetWorld());

  UpdateAnimatedHandMeshVisibility(bTrackedVisibility, ControllerHandBehavior);
  UpdatePoseableHandMeshVisibility(bTrackedVisibility, ControllerHandBehavior);
  UpdateControllerMeshVisibility(bTrackedVisibility, ControllerHandBehavior);
}

void UIsdkControllerVisualsRigComponent::BindInputActions(UInputComponent* PlayerInputComponent)
{
  UE_LOG(LogTemp, Warning, TEXT("UIsdkControllerVisualsRigComponent::BindInputActions called. PlayerInputComponent is %s"), IsValid(PlayerInputComponent) ? TEXT("Valid") : TEXT("Invalid"));
	if (!PlayerInputComponent)
	{
		return;
	}

	if (Handedness == EIsdkHandedness::Left)
	{
		PlayerInputComponent->BindAxis(TEXT("GrabAxisLeft"), this, &UIsdkControllerVisualsRigComponent::OnGripAxis);
		PlayerInputComponent->BindAxis(TEXT("TriggerAxisLeft"), this, &UIsdkControllerVisualsRigComponent::OnTriggerAxis);
		PlayerInputComponent->BindAxis(TEXT("MovementAxisLeft_X"), this, &UIsdkControllerVisualsRigComponent::OnThumbstickX);
		PlayerInputComponent->BindAxis(TEXT("MovementAxisLeft_Y"), this, &UIsdkControllerVisualsRigComponent::OnThumbstickY);

		PlayerInputComponent->BindAxis(TEXT("Left_HandPinchSelectStrength"), this, &UIsdkControllerVisualsRigComponent::OnHandPinchSelectStrength);

		PlayerInputComponent->BindAction(TEXT("Oculus_X_Button"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonXPressed);
		PlayerInputComponent->BindAction(TEXT("Oculus_X_Button"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonXReleased);
		PlayerInputComponent->BindAction(TEXT("Oculus_Y_Button"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonYPressed);
		PlayerInputComponent->BindAction(TEXT("Oculus_Y_Button"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonYReleased);

		PlayerInputComponent->BindAction(TEXT("MenuToggleLeft"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnMenuTogglePressed);
		PlayerInputComponent->BindAction(TEXT("MenuToggleLeft"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnMenuToggleReleased);

		PlayerInputComponent->BindAction(TEXT("Oculus_X_Button_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonXTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_X_Button_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonXUnTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Y_Button_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonYTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Y_Button_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonYUnTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Left_Thumbstick_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnThumbstickTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Left_Thumbstick_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnThumbstickUnTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Left_Trigger_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnTriggerTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Left_Trigger_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnTriggerUnTouched);

		PlayerInputComponent->BindAction(TEXT("Left_Panel_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnPanelTouched);
		PlayerInputComponent->BindAction(TEXT("Left_Panel_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnPanelUnTouched);

		PlayerInputComponent->BindAction(TEXT("Left_HandPinchSelect"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnHandPinchSelectPressed);
		PlayerInputComponent->BindAction(TEXT("Left_HandPinchSelect"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnHandPinchSelectReleased);
		PlayerInputComponent->BindAction(TEXT("Left_HandPinchGrab"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnHandPinchGrabPressed);
		PlayerInputComponent->BindAction(TEXT("Left_HandPinchGrab"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnHandPinchGrabReleased);
		PlayerInputComponent->BindAction(TEXT("Left_HandPalmGrab"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnHandPalmGrabPressed);
		PlayerInputComponent->BindAction(TEXT("Left_HandPalmGrab"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnHandPalmGrabReleased);
	}
	else // Right Hand
	{
		PlayerInputComponent->BindAxis(TEXT("GrabAxisRight"), this, &UIsdkControllerVisualsRigComponent::OnGripAxis);
		PlayerInputComponent->BindAxis(TEXT("TriggerAxisRight"), this, &UIsdkControllerVisualsRigComponent::OnTriggerAxis);
		PlayerInputComponent->BindAxis(TEXT("MovementAxisRight_X"), this, &UIsdkControllerVisualsRigComponent::OnThumbstickX);
		PlayerInputComponent->BindAxis(TEXT("MovementAxisRight_Y"), this, &UIsdkControllerVisualsRigComponent::OnThumbstickY);

		PlayerInputComponent->BindAxis(TEXT("Right_HandPinchSelectStrength"), this, &UIsdkControllerVisualsRigComponent::OnHandPinchSelectStrength);

		PlayerInputComponent->BindAction(TEXT("Oculus_A_Button"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonAPressed);
		PlayerInputComponent->BindAction(TEXT("Oculus_A_Button"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonAReleased);
		PlayerInputComponent->BindAction(TEXT("Oculus_B_Button"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonBPressed);
		PlayerInputComponent->BindAction(TEXT("Oculus_B_Button"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonBReleased);

		PlayerInputComponent->BindAction(TEXT("Oculus_A_Button_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonATouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_A_Button_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonAUnTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_B_Button_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnButtonBTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_B_Button_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnButtonBUnTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Right_Thumbstick_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnThumbstickTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Right_Thumbstick_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnThumbstickUnTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Right_Trigger_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnTriggerTouched);
		PlayerInputComponent->BindAction(TEXT("Oculus_Right_Trigger_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnTriggerUnTouched);

		PlayerInputComponent->BindAction(TEXT("Right_Panel_Touched"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnPanelTouched);
		PlayerInputComponent->BindAction(TEXT("Right_Panel_Touched"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnPanelUnTouched);

		PlayerInputComponent->BindAction(TEXT("Right_HandPinchSelect"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnHandPinchSelectPressed);
		PlayerInputComponent->BindAction(TEXT("Right_HandPinchSelect"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnHandPinchSelectReleased);
		PlayerInputComponent->BindAction(TEXT("Right_HandPinchGrab"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnHandPinchGrabPressed);
		PlayerInputComponent->BindAction(TEXT("Right_HandPinchGrab"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnHandPinchGrabReleased);
		PlayerInputComponent->BindAction(TEXT("Right_HandPalmGrab"), IE_Pressed, this, &UIsdkControllerVisualsRigComponent::OnHandPalmGrabPressed);
		PlayerInputComponent->BindAction(TEXT("Right_HandPalmGrab"), IE_Released, this, &UIsdkControllerVisualsRigComponent::OnHandPalmGrabReleased);
	}
}

// Implementations for the new handler functions
void UIsdkControllerVisualsRigComponent::OnGripAxis(float Value)
{
	GripValue = Value;
}

void UIsdkControllerVisualsRigComponent::OnTriggerAxis(float Value)
{
	TriggerValue = Value;
}

void UIsdkControllerVisualsRigComponent::OnThumbstickX(float Value)
{
	ThumbstickXValue = Value;
}

void UIsdkControllerVisualsRigComponent::OnThumbstickY(float Value)
{
	ThumbstickYValue = Value;
}

void UIsdkControllerVisualsRigComponent::OnHandPinchSelectStrength(float value)
{
	HandPinchSelectStrength = value;
}

void UIsdkControllerVisualsRigComponent::OnButtonAPressed() { bButtonADown = true; }
void UIsdkControllerVisualsRigComponent::OnButtonAReleased() { bButtonADown = false; }
void UIsdkControllerVisualsRigComponent::OnButtonBPressed() { bButtonBDown = true; }
void UIsdkControllerVisualsRigComponent::OnButtonBReleased() { bButtonBDown = false; }
void UIsdkControllerVisualsRigComponent::OnButtonXPressed() { bButtonXDown = true; }
void UIsdkControllerVisualsRigComponent::OnButtonXReleased() { bButtonXDown = false; }
void UIsdkControllerVisualsRigComponent::OnButtonYPressed() { bButtonYDown = true; }
void UIsdkControllerVisualsRigComponent::OnButtonYReleased() { bButtonYDown = false; }
void UIsdkControllerVisualsRigComponent::OnButtonATouched() { bButtonATouched = true; }
void UIsdkControllerVisualsRigComponent::OnButtonAUnTouched() { bButtonATouched = false; }
void UIsdkControllerVisualsRigComponent::OnButtonBTouched() { bButtonBTouched = true; }
void UIsdkControllerVisualsRigComponent::OnButtonBUnTouched() { bButtonBTouched = false; }
void UIsdkControllerVisualsRigComponent::OnButtonXTouched() { bButtonXTouched = true; }
void UIsdkControllerVisualsRigComponent::OnButtonXUnTouched() { bButtonXTouched = false; }
void UIsdkControllerVisualsRigComponent::OnButtonYTouched() { bButtonYTouched = true; }
void UIsdkControllerVisualsRigComponent::OnButtonYUnTouched() { bButtonYTouched = false; }
void UIsdkControllerVisualsRigComponent::OnTriggerTouched() { bTriggerTouched = true; }
void UIsdkControllerVisualsRigComponent::OnTriggerUnTouched() { bTriggerTouched = false; }
void UIsdkControllerVisualsRigComponent::OnThumbstickTouched() { bThumbstickTouched = true; }
void UIsdkControllerVisualsRigComponent::OnThumbstickUnTouched() { bThumbstickTouched = false; }
void UIsdkControllerVisualsRigComponent::OnPanelTouched() { bPanelTouched = true; }
void UIsdkControllerVisualsRigComponent::OnPanelUnTouched() { bPanelTouched = false; }

void UIsdkControllerVisualsRigComponent::OnHandPinchSelectPressed() { bHandPinchSelect = true; }
void UIsdkControllerVisualsRigComponent::OnHandPinchSelectReleased() { bHandPinchSelect = false; }
void UIsdkControllerVisualsRigComponent::OnHandPinchGrabPressed() { bHandPinchGrab = true; }
void UIsdkControllerVisualsRigComponent::OnHandPinchGrabReleased() { bHandPinchGrab = false; }
void UIsdkControllerVisualsRigComponent::OnHandPalmGrabPressed() { bHandPalmGrab = true; }
void UIsdkControllerVisualsRigComponent::OnHandPalmGrabReleased() { bHandPalmGrab = false; }

void UIsdkControllerVisualsRigComponent::OnMenuTogglePressed() { bMenuToggle = true; }
void UIsdkControllerVisualsRigComponent::OnMenuToggleReleased() { bMenuToggle = false; }

void UIsdkControllerVisualsRigComponent::UpdateAnimatedHandMeshVisibility(
    bool bTrackedVisibility,
    EControllerHandBehavior ControllerHandBehavior)
{
  if (!bTrackedVisibility)
  {
    AnimatedHandMeshComponent->SetVisibility(false);
    return;
  }

  bool bIsVisible;
  switch (ControllerHandBehavior)
  {
    case EControllerHandBehavior::BothAnimated:
    case EControllerHandBehavior::HandsOnlyAnimated:
      bIsVisible = true;
      break;
    default:
      bIsVisible = false;
  }

  AnimatedHandMeshComponent->SetVisibility(bIsVisible);
}

void UIsdkControllerVisualsRigComponent::UpdatePoseableHandMeshVisibility(
    bool bTrackedVisibility,
    EControllerHandBehavior ControllerHandBehavior)
{
  if (!bTrackedVisibility)
  {
    PoseableHandMeshComponent->SetVisibility(false);
    return;
  }

  bool bIsVisible;
  switch (ControllerHandBehavior)
  {
    case EControllerHandBehavior::BothProcedural:
    case EControllerHandBehavior::HandsOnlyProcedural:
      bIsVisible = true;
      break;
    default:
      bIsVisible = false;
  }

  PoseableHandMeshComponent->SetVisibility(bIsVisible);
}

void UIsdkControllerVisualsRigComponent::UpdateControllerMeshVisibility(
    bool bTrackedVisibility,
    EControllerHandBehavior ControllerHandBehavior)
{
  if (!bTrackedVisibility)
  {
    ControllerMeshComponent->SetVisibility(false);
    return;
  }

  bool bIsVisible;
  switch (ControllerHandBehavior)
  {
    case EControllerHandBehavior::HandsOnlyProcedural:
    case EControllerHandBehavior::HandsOnlyAnimated:
      bIsVisible = false;
      break;
    default:
      bIsVisible = true;
  }

  ControllerMeshComponent->SetVisibility(bIsVisible);
}

UIsdkControllerVisualsRigComponentLeft::UIsdkControllerVisualsRigComponentLeft()
{
  Handedness = EIsdkHandedness::Left;

  static ConstructorHelpers::FObjectFinder<USkeletalMesh> LeftControllerMeshFinder =
      IsdkContentAssetPaths::Models::Controller::OculusControllerSkeletalMeshL;
  static ConstructorHelpers::FClassFinder<UAnimInstance> LeftControllerAnimBlueprintFinder =
      IsdkContentAssetPaths::Models::Controller::OculusControllerAnimBlueprintL;
  static ConstructorHelpers::FObjectFinder<USkeletalMesh> LeftHandMeshFinder =
      IsdkContentAssetPaths::Models::Hand::OpenXRLeftHand;
  static ConstructorHelpers::FClassFinder<UAnimInstance> LeftControllerHandAnimBlueprintFinder =
      IsdkContentAssetPaths::ControllerDrivenHands::ControllerDrivenHandAnimBlueprintL;

  // Update controller mesh defaults
  ControllerMeshComponent->SetSkeletalMesh(LeftControllerMeshFinder.Object);
  ControllerMeshComponent->SetAnimInstanceClass(LeftControllerAnimBlueprintFinder.Class);

  // Update animated hand mesh defaults
  AnimatedHandMeshComponent->SetSkeletalMesh(LeftHandMeshFinder.Object);
  AnimatedHandMeshComponent->SetAnimInstanceClass(LeftControllerHandAnimBlueprintFinder.Class);

  // Update procedural hand mesh defaults
  PoseableHandMeshComponent->SetSkeletalMesh(LeftHandMeshFinder.Object);

}

UIsdkControllerVisualsRigComponentRight::UIsdkControllerVisualsRigComponentRight()
{
  Handedness = EIsdkHandedness::Right;

  static ConstructorHelpers::FObjectFinder<USkeletalMesh> RightControllerMeshFinder =
      IsdkContentAssetPaths::Models::Controller::OculusControllerSkeletalMeshR;
  static ConstructorHelpers::FClassFinder<UAnimInstance> RightControllerAnimBlueprintFinder =
      IsdkContentAssetPaths::Models::Controller::OculusControllerAnimBlueprintR;
  static ConstructorHelpers::FObjectFinder<USkeletalMesh> RightHandMeshFinder =
      IsdkContentAssetPaths::Models::Hand::OpenXRRightHand;
  static ConstructorHelpers::FClassFinder<UAnimInstance> RightControllerHandAnimBlueprintFinder =
      IsdkContentAssetPaths::ControllerDrivenHands::ControllerDrivenHandAnimBlueprintR;

  // Update controller mesh defaults
  ControllerMeshComponent->SetSkeletalMesh(RightControllerMeshFinder.Object);
  ControllerMeshComponent->SetAnimInstanceClass(RightControllerAnimBlueprintFinder.Class);

  // Update animated hand mesh defaults
  AnimatedHandMeshComponent->SetSkeletalMesh(RightHandMeshFinder.Object);
  AnimatedHandMeshComponent->SetAnimInstanceClass(RightControllerHandAnimBlueprintFinder.Class);

  // Update procedural hand mesh defaults
  PoseableHandMeshComponent->SetSkeletalMesh(RightHandMeshFinder.Object);
}
