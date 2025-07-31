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

#include "IsdkHandPoseVisualizer.h"
#include "SceneManagement.h"
#include "IsdkHandMeshComponent.h"
#include "Interaction/Grabbable/IsdkHandGrabPose.h"

IMPLEMENT_HIT_PROXY(HBoneVisProxy, HComponentVisProxy);

void FIsdkHandPoseVisualizer::DrawVisualization(
    const UActorComponent* Component,
    const FSceneView* View,
    FPrimitiveDrawInterface* PDI)
{
  const UIsdkHandGrabPose* ThisHandGrabPose = Cast<UIsdkHandGrabPose>(Component);
  if (ThisHandGrabPose)
  {
    if (!ThisHandGrabPose->bHandPoseEditingEnabled)
    {
      return;
    }
    if (!IsHandGrabPoseValid((UIsdkHandGrabPose*)ThisHandGrabPose))
    {
      UE_LOG(LogTemp, Warning, TEXT("FIsdkHandPoseVisualizer - Hand pose data is not valid"));
      return;
    }
    if (!ThisHandGrabPose->IsVisibleInEditor())
    {
      return;
    }

    const auto DepthPriority = ESceneDepthPriorityGroup::SDPG_Foreground;

    if (bPendingChanges && FPlatformTime::Seconds() - LastVisualizerChange > 0.1)
    {
      CopyChangesToHandPose();
    }

    if (IsSelectingBone())
    {
      DrawWireSphere(
          PDI,
          CachedSelectedBoneTransform,
          FLinearColor::White,
          2.0,
          16,
          DepthPriority,
          3.0,
          0.0,
          true);
    }

    for (uint8 ChainIndex = 0; ChainIndex < 5; ChainIndex++)
    {
      FIsdkBoneChain Chain = ThisHandGrabPose->HandPoseData->HandData->BoneChains[ChainIndex];
      DrawBoneChain(PDI, Chain, ChainIndex, true, (UIsdkHandGrabPose*)ThisHandGrabPose);
    }
  }
  else
  {
    UE_LOG(LogTemp, Warning, TEXT("FIsdkHandPoseVisualizer: Invalid HandGrabPose"));
  }
}

void FIsdkHandPoseVisualizer::DrawBoneChain(
    FPrimitiveDrawInterface* PDI,
    FIsdkBoneChain Chain,
    uint8 ChainIndex,
    bool bGenerateProxy,
    UIsdkHandGrabPose* HandGrabPose) const
{
  if (!IsHandGrabPoseValid(HandGrabPose))
  {
    return;
  }
  AActor* SelectedParentActor = HandGrabPose->GetOwner();
  if (!IsValid(SelectedParentActor))
  {
    return;
  }
  const auto DepthPriority = ESceneDepthPriorityGroup::SDPG_Foreground;
  for (uint8 i = 0; i < 5; i++)
  {
    EIsdkHandBones JointId;
    if (!Chain.GetJoint(i, JointId))
      continue;
    FTransform BoneTransform = HandGrabPose->HandPoseData->HandData->GetJointPose((uint8)JointId);
    BoneTransform *= HandGrabPose->GetRelativeTransform();
    if (IsValid(SelectedParentActor))
    {
      BoneTransform *= SelectedParentActor->GetActorTransform();
    }

    EIsdkHandBones NextJointId;
    bool LastBone = !Chain.GetJoint(i + 1, NextJointId);

    if (bGenerateProxy && !LastBone)
    {
      PDI->SetHitProxy(new HBoneVisProxy(
          Cast<UActorComponent>(HandGrabPose), FIsdkHandPoseSelectionState(JointId, ChainIndex)));
    }
    PDI->DrawPoint(
        BoneTransform.GetLocation(),
        Chain.Color * 0.7,
        LastBone ? 10.0 : 20.0,
        ESceneDepthPriorityGroup::SDPG_Foreground);
    if (bGenerateProxy)
    {
      PDI->SetHitProxy(NULL);
    }

    if (LastBone)
      continue;

    FTransform NextBoneTransform =
        HandGrabPose->HandPoseData->HandData->GetJointPose((uint8)NextJointId);
    NextBoneTransform *= HandGrabPose->GetRelativeTransform();
    if (SelectedParentActor)
    {
      NextBoneTransform *= SelectedParentActor->GetActorTransform();
    }
    FVector Start = BoneTransform.GetLocation();
    FVector End = NextBoneTransform.GetLocation();
    PDI->DrawLine(Start, End, Chain.Color, DepthPriority, 4.0, 0.0, true);
  }
}

bool FIsdkHandPoseVisualizer::VisProxyHandleClick(
    FEditorViewportClient* InViewportClient,
    HComponentVisProxy* VisProxy,
    const FViewportClick& Click)
{
  if (VisProxy && VisProxy->Component.IsValid())
  {
    if (VisProxy->IsA(HBoneVisProxy::StaticGetType()))
    {
      const UIsdkHandGrabPose* ThisHandGrabPose =
          Cast<const UIsdkHandGrabPose>(VisProxy->Component);
      UIsdkHandGrabPose* SelectedHandGrabPose = (UIsdkHandGrabPose*)ThisHandGrabPose;
      GrabPosePropertyPath = FComponentPropertyPath(SelectedHandGrabPose);
      AActor* SelectedParentActor = SelectedHandGrabPose->GetOwner();
      if (IsValid(SelectedHandGrabPose) && SelectedHandGrabPose->IsDebugVisualActive())
      {
        SelectedHandGrabPose->GetDebugVisual()->SetPauseSkeletonUpdates(true);
        HBoneVisProxy* Proxy = (HBoneVisProxy*)VisProxy;
        CurrentSelection = Proxy->Selection;

        if (SelectedHandGrabPose->GetDebugVisual()->MappedBoneNames.IsValidIndex(
                (uint8)CurrentSelection.JointIndex))
        {
          SelectedBoneName = SelectedHandGrabPose->GetDebugVisual()
                                 ->MappedBoneNames[(uint8)CurrentSelection.JointIndex];
        }

        CachedSelectedBoneTransform = SelectedHandGrabPose->HandPoseData->HandData->GetJointPose(
                                          (uint8)CurrentSelection.JointIndex) *
            SelectedHandGrabPose->GetRelativeTransform() * SelectedParentActor->GetActorTransform();

        AccumulatedWidgetRotation.SetRotation(
            SelectedHandGrabPose->GetDebugVisual()
                ->GetBoneRotationByName(SelectedBoneName, EBoneSpaces::WorldSpace)
                .Quaternion());

        return true;
      }
    }
  }
  return false;
}

bool FIsdkHandPoseVisualizer::HandleInputDelta(
    FEditorViewportClient* ViewportClient,
    FViewport* Viewport,
    FVector& DeltaTranslate,
    FRotator& DeltaRotate,
    FVector& DeltaScale)
{
  if (IsSelectingBone())
  {
    if (ViewportClient->GetWidgetMode() == FWidget::WM_Rotate)
    {
      UIsdkHandGrabPose* SelectedHandGrabPose = GetHandGrabPoseComponent();
      AccumulatedWidgetRotation.SetRotation(
          DeltaRotate.Quaternion() * AccumulatedWidgetRotation.GetRotation());
      SelectedHandGrabPose->GetDebugVisual()->SetBoneRotationByName(
          SelectedBoneName,
          AccumulatedWidgetRotation.GetRotation().Rotator(),
          EBoneSpaces::WorldSpace);

      LastVisualizerChange = FPlatformTime::Seconds();
      bPendingChanges = true;
    }
    return true;
  }
  return false;
}

bool FIsdkHandPoseVisualizer::GetCustomInputCoordinateSystem(
    const FEditorViewportClient* ViewportClient,
    FMatrix& OutMatrix) const
{
  if (IsSelectingBone())
  {
    OutMatrix = CachedSelectedBoneTransform.ToMatrixNoScale();
    return true;
  }
  return false;
}

bool FIsdkHandPoseVisualizer::GetWidgetLocation(
    const FEditorViewportClient* ViewportClient,
    FVector& OutLocation) const
{
  if (IsSelectingBone())
  {
    OutLocation = CachedSelectedBoneTransform.GetLocation();

    if (ViewportClient->GetWidgetMode() != FWidget::WM_Rotate)
    {
      OutLocation += FVector(1000.0, 1000.0, 1000.0);
    }
    return true;
  }
  return false;
}

bool FIsdkHandPoseVisualizer::HasFocusOnSelectionBoundingBox(FBox& OutBoundingBox)
{
  if (IsSelectingBone())
  {
    OutBoundingBox.Init();
    OutBoundingBox += CachedSelectedBoneTransform.GetLocation();
    return true;
  }
  return false;
}

void FIsdkHandPoseVisualizer::CopyChangesToHandPose()
{
  UIsdkHandGrabPose* SelectedHandGrabPose = GetHandGrabPoseComponent();
  if (IsHandGrabPoseValid(SelectedHandGrabPose))
  {
    SelectedHandGrabPose->HandPoseData->HandData->Modify();
    SelectedHandGrabPose->GetDebugVisual()->PushTransformsFromSkeletonToHandData();
    bPendingChanges = false;
  }
}

void FIsdkHandPoseVisualizer::OnRegister()
{
  CurrentSelection = FIsdkHandPoseSelectionState();
}
void FIsdkHandPoseVisualizer::EndEditing()
{
  CurrentSelection = FIsdkHandPoseSelectionState();
  GrabPosePropertyPath.Reset();
}

UIsdkHandGrabPose* FIsdkHandPoseVisualizer::GetHandGrabPoseComponent() const
{
  if (GrabPosePropertyPath.IsValid())
  {
    return Cast<UIsdkHandGrabPose>(GrabPosePropertyPath.GetComponent());
  }
  return nullptr;
}
