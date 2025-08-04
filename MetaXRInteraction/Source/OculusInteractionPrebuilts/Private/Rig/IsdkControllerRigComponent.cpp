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

#include "Rig/IsdkControllerRigComponent.h"
#include "IsdkContentAssetPaths.h"
#include "IsdkControllerMeshComponent.h"
#include "IsdkFunctionLibrary.h"
#include "IsdkHandMeshComponent.h"
#include "IsdkRuntimeSettings.h"
#include "Rig/IsdkControllerVisualsRigComponent.h"
#include "Rig/IsdkInputActionsRigComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"


UIsdkControllerRigComponent::UIsdkControllerRigComponent()
    : UIsdkControllerRigComponent(EIsdkHandedness::Left)
{
}

UIsdkControllerRigComponent::UIsdkControllerRigComponent(EIsdkHandedness InHandedness)
{
  Handedness = InHandedness;

  SetRigComponentDefaults();

  InteractionTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Device_TrackedController);
}

void UIsdkControllerRigComponent::OnControllerVisualsAttached()
{
  ControllerVisualsComponent->CreateDataSourcesAsTrackedController();
}

FVector UIsdkControllerRigComponent::GetPalmColliderOffset() const
{
  const auto ControllerHandBehavior = UIsdkFunctionLibrary::GetControllerHandBehavior(GetWorld());
  const bool bUseHandOffset =
      ControllerHandBehavior == EControllerHandBehavior::HandsOnlyAnimated ||
      ControllerHandBehavior == EControllerHandBehavior::HandsOnlyProcedural;

  if (bUseHandOffset)
  {
    return HandPalmColliderOffset;
  }

  return ControllerPalmColliderOffset;
}

USkinnedMeshComponent* UIsdkControllerRigComponent::GetPinchAttachMesh() const
{
  const auto ControllerHandBehavior = UIsdkFunctionLibrary::GetControllerHandBehavior(GetWorld());
  const bool bUseHandMesh = ControllerHandBehavior == EControllerHandBehavior::HandsOnlyAnimated ||
      ControllerHandBehavior == EControllerHandBehavior::HandsOnlyProcedural;

  if (bUseHandMesh)
  {
    if (ControllerHandBehavior == EControllerHandBehavior::HandsOnlyAnimated)
    {
      return ControllerVisualsComponent->GetAnimatedHandMeshComponent();
    }

    return ControllerVisualsComponent->GetPoseableHandMeshComponent();
  }

  return nullptr;
}

void UIsdkControllerRigComponent::BindInputActions(UInputComponent* InputComponent)
{
	Super::BindInputActions(InputComponent);
    UE_LOG(LogTemp, Warning, TEXT("UIsdkControllerRigComponent::BindInputActions called. InputComponent is %s"), IsValid(InputComponent) ? TEXT("Valid") : TEXT("Invalid"));
	if (!IsValid(ControllerVisualsComponent) || !IsValid(InputActions))
	{
		return;
	}

	ControllerVisualsComponent->BindInputActions(InputComponent);

	// Configure pinch grab
	if (!InputActions->PinchGrabAction.IsNone())
	{
		InputComponent->BindAction(InputActions->PinchGrabAction, EInputEvent::IE_Pressed, this, &UIsdkControllerRigComponent::HandlePinchGrabStartedInput);
		InputComponent->BindAction(InputActions->PinchGrabAction, EInputEvent::IE_Released, this, &UIsdkControllerRigComponent::HandlePinchGrabFinishedInput);
	}

	// Configure palm grab
	if (!InputActions->PalmGrabAction.IsNone())
	{
		InputComponent->BindAction(InputActions->PalmGrabAction, EInputEvent::IE_Pressed, this, &UIsdkControllerRigComponent::HandlePalmGrabStartedInput);
		InputComponent->BindAction(InputActions->PalmGrabAction, EInputEvent::IE_Released, this, &UIsdkControllerRigComponent::HandlePalmGrabFinishedInput);
	}
}

FName UIsdkControllerRigComponent::GetThumbTipSocketName() const
{
  const auto PoseableHandMesh = ControllerVisualsComponent->GetPoseableHandMeshComponent();
  if (!IsValid(PoseableHandMesh))
  {
    return NAME_None;
  }

  return PoseableHandMesh->MappedBoneNames[static_cast<int>(EIsdkHandBones::HandThumbTip)];
}

TSubclassOf<AActor> UIsdkControllerRigComponent::FindBPFromPath(const FString& Path)
{
  ConstructorHelpers::FClassFinder<AActor> ClassFinder(*Path);
  if (ClassFinder.Succeeded())
  {
    return ClassFinder.Class;
  }

  return nullptr;
}

void UIsdkControllerRigComponent::HandleControllerHandBehaviorChanged(
    TScriptInterface<IIsdkITrackingDataSubsystem> IsdkITrackingDataSubsystem,
    EControllerHandBehavior ControllerHandBehavior,
    EControllerHandBehavior ControllerHandBehavior1)
{
  // Interactors may need to have their attach components / sockets updated when controller hand
  // behavior changes (for instance, hand-only interactor colliders should be placed differently
  // from behaviors which include a controller mesh)
  UpdateComponentDataSources();
}

void UIsdkControllerRigComponent::HandlePinchGrabStartedInput()
{
  OnPinchSelectDelegate.Broadcast(this);
}

void UIsdkControllerRigComponent::HandlePinchGrabFinishedInput()
{
  OnPinchUnselectDelegate.Broadcast(this);
}

void UIsdkControllerRigComponent::HandlePalmGrabStartedInput()
{
  OnPalmSelectDelegate.Broadcast(this);
}

void UIsdkControllerRigComponent::HandlePalmGrabFinishedInput()
{
  OnPalmUnselectDelegate.Broadcast(this);
}

void UIsdkControllerRigComponent::BeginPlay()
{
  Super::BeginPlay();

  if (IsValid(ControllerVisualsComponent) &&
      !IsValid(ControllerVisualsComponent->AttachedToMotionController))
  {
    if (ControllerVisualsComponent->TryAttachToParentMotionController(this))
    {
      OnControllerVisualsAttached();
    }
  }

  const auto Delegate = UIsdkFunctionLibrary::GetControllerHandBehaviorDelegate(GetWorld());

  if (!Delegate)
  {
    return;
  }

  Delegate->AddUObject(this, &UIsdkControllerRigComponent::HandleControllerHandBehaviorChanged);
}

UIsdkControllerVisualsRigComponent* UIsdkControllerRigComponent::GetControllerVisuals() const
{
  return ControllerVisualsComponent;
}

UIsdkTrackedDataSourceRigComponent* UIsdkControllerRigComponent::GetVisuals() const
{
  return ControllerVisualsComponent;
}

float UIsdkControllerRigComponent::GetPinchStrength() const
{
	return 0.0f;
}

UIsdkControllerRigComponentLeft::UIsdkControllerRigComponentLeft()
    : UIsdkControllerRigComponent(EIsdkHandedness::Left)
{
  ControllerVisualsComponent =
      CreateDefaultSubobject<UIsdkControllerVisualsRigComponentLeft>(TEXT("ControllerVisualsLeft"));
  ControllerVisualsComponent->SetupAttachment(this);
}

UIsdkControllerRigComponentRight::UIsdkControllerRigComponentRight()
    : UIsdkControllerRigComponent(EIsdkHandedness::Right)
{
  ControllerVisualsComponent = CreateDefaultSubobject<UIsdkControllerVisualsRigComponentRight>(
      TEXT("ControllerVisualsRight"));
  ControllerVisualsComponent->SetupAttachment(this);
}
