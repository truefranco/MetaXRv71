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

#include "Interaction/Grabbable/IsdkHandGrabPose.h"
#include "IsdkHandPoseData.h"
#include "Engine/SkeletalMesh.h"
#include "Subsystem/IsdkHandPoseSubsystem.h"
#include "IsdkHandMeshComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace isdk
{
TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_HandGrabPoses_DebugPoseVisuals(
    TEXT("Meta.InteractionSDK.HandGrabPoses.DebugPoseVisuals"),
    false,
    TEXT("Prevents Visuals from Being Destroyed during Runtime"));
}
// namespace isdk

UIsdkHandGrabPose::UIsdkHandGrabPose()
{
  PrimaryComponentTick.bCanEverTick = false;
}

void UIsdkHandGrabPose::OnComponentCreated()
{
#if WITH_EDITOR
  if (bShowDebugVisualInEditor)
  {
    InitializeDebugVisual();
  }
#endif
}

bool UIsdkHandGrabPose::InitializeDebugVisual()
{
  // Destroy old one if it exists
  DestroyDebugVisual();

  DebugHandVisual = NewObject<UIsdkHandMeshComponent>(this);
  DebugHandVisual->SetComponentTickEnabled(false);

  if (!IsValid(HandPoseData))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandGrabPose::InitializeDebugVisual() - HandPoseData is invalid!"));
    return false;
  }

  UWorld* WorldObject = GetWorld();
#if WITH_EDITOR
  WorldObject = GEditor->GetEditorWorldContext().World();
#endif
  UIsdkHandPoseSubsystem* HandPoseSubsystem = UIsdkHandPoseSubsystem::Get(WorldObject);
  if (!IsValid(HandPoseSubsystem))
  {
    return false;
  }

  UIsdkHandPoseData* VisualHandPoseData = nullptr;

  if (HandGrabPoseProperties.PoseMode == EIsdkHandGrabPoseMode::MirrorReference)
  {
    UIsdkHandPoseData* NewMirrorHandPoseData = nullptr;
    if (HandPoseSubsystem->GenerateMirroredHandPoseData(HandPoseData, NewMirrorHandPoseData))
    {
      bVisualMirrored = true;
      MirrorHandPoseData = NewMirrorHandPoseData;
      VisualHandPoseData = MirrorHandPoseData;
    }
    else
    {
      return false;
    }
  }
  else
  {
    VisualHandPoseData = HandPoseData;
  }

  Handedness = VisualHandPoseData->Handedness;
  DebugHandVisual->SetJointsDataSource(VisualHandPoseData);
  DebugHandVisual->SetMappedBoneNamesFromJointSourceHandedness();

  DebugHandVisual->SetSkeletalMesh(HandPoseSubsystem->GetHandMesh(Handedness));
  if (!IsValid(DebugHandVisual->SkeletalMesh))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandGrabPose::InitializeDebugVisual() - HandVisual Skeletal Mesh is invalid"));
    return false;
  }

  DebugHandVisual->SetMaterial(0, HandPoseSubsystem->GetHandMeshMaterial());
  if (!IsValid(DebugHandVisual->GetMaterial(0)))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandGrabPose::InitializeDebugVisual() - HandVisual Material is invalid"));
    return false;
  }

  DebugHandVisual->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
  DebugHandVisual->RegisterComponent();

  return true;
}

void UIsdkHandGrabPose::DestroyDebugVisual()
{
  if (IsValid(DebugHandVisual))
  {
    DebugHandVisual->UnregisterComponent();
    DebugHandVisual->DestroyComponent(false);
    DebugHandVisual = nullptr;
  }
}

void UIsdkHandGrabPose::BeginPlay()
{
  Super::BeginPlay();
  const bool bDestroyAfterRegistration = bRemoveDebugVisualOnPlay &&
      !isdk::CVar_Meta_InteractionSDK_HandGrabPoses_DebugPoseVisuals.GetValueOnAnyThread();

  // Mirror References will be registered and queued for destruction by their parent Hand Grab Pose
  if (HandGrabPoseProperties.PoseMode != EIsdkHandGrabPoseMode::MirrorReference)
  {
    UIsdkHandPoseSubsystem* HandPoseSubsystem = UIsdkHandPoseSubsystem::Get(GetWorld());

    if (HandPoseSubsystem)
    {
      HandPoseSubsystem->RegisterHandPoseData(this, bDestroyAfterRegistration);
    }
  }

  // Destroy visual if it exists & is configured to do so
  if (bDestroyAfterRegistration)
  {
    DestroyDebugVisual();
  }
}

void UIsdkHandGrabPose::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);
}
