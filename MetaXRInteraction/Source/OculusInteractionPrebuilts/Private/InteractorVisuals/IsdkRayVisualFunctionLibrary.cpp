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

#include "InteractorVisuals/IsdkRayVisualFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "IsdkContentAssetPaths.h"
#include "Interaction/IsdkRayInteractor.h"
#include "InteractorVisuals/IsdkRayVisualComponent.h"

float UIsdkRayVisualFunctionLibrary::GetCursorScaleFromDistanceToHmd(
    const FVector CursorPosition,
    const float ReferenceDistance,
    const TScriptInterface<IIsdkIHmdDataSource>& HmdDataSource)
{
  FTransform HmdPose;
  bool IsTracked;
  IIsdkIHmdDataSource::Execute_GetHmdPose(HmdDataSource.GetObject(), HmdPose, IsTracked);
  auto HmdToCursor = CursorPosition - HmdPose.GetLocation();
  return HmdToCursor.Length() / ReferenceDistance;
}

FTransform UIsdkRayVisualFunctionLibrary::GetCursorTransformFromCollision(
    FIsdkOptionalSurfaceHit CollisionInfo)
{
  auto CollisionData = CollisionInfo.Value;
  auto SurfaceNormal = FVector(CollisionData.Normal);
  auto CursorRotation = UKismetMathLibrary::MakeRotFromZ(SurfaceNormal);
  auto CursorPosition = FVector(CollisionData.Point) + SurfaceNormal;
  return FTransform(CursorRotation, CursorPosition);
}

FName UIsdkRayVisualFunctionLibrary::GetPinchStrengthIA(EIsdkHandedness Handedness)
{
	return FName();
}

void UIsdkRayVisualFunctionLibrary::UpdateRayVisualWithRayInteractor(
    UIsdkRayVisualComponent* RayVisual,
    UIsdkRayInteractor* RayInteractor,
    float RaySelectStrength)
{
  if (!ensureMsgf(IsValid(RayVisual), TEXT("Ray Visual is invalid")))
  {
    return;
  }
  if (!ensureMsgf(IsValid(RayInteractor), TEXT("Ray Interactor is invalid")))
  {
    return;
  }
  auto state = RayInteractor->GetCurrentState();
  if (state == EIsdkInteractorState::Normal || state == EIsdkInteractorState::Disabled)
  {
    RayVisual->SetVisibility(false);
    return;
  }
  if (state == EIsdkInteractorState::Select && !RayInteractor->HasSelectedInteractable())
  {
    RayInteractor->Unselect();
    return;
  }
  RayVisual->SetVisibility(true);
  FTransform PointerPose{};
  bool IsPointerPoseValid;
  auto HandPointerPose = RayInteractor->GetHandPointerPose();
  if (!ensureMsgf(IsValid(HandPointerPose.GetObject()), TEXT("Hand Pointer Pose is invalid")))
  {
    return;
  }
  IIsdkIHandPointerPose::Execute_GetPointerPose(
      HandPointerPose.GetObject(), PointerPose, IsPointerPoseValid);

  auto CollisionInfo = RayInteractor->GetCollisionInfo();
  auto CursorTransform =
      UIsdkRayVisualFunctionLibrary::GetCursorTransformFromCollision(CollisionInfo);

  RayVisual->UpdateVisual(PointerPose, CursorTransform, CollisionInfo.HasValue, RaySelectStrength);
}
