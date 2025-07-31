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

#include "HandPoseDetection/IsdkHandPalmGrabRecognizer.h"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "IsdkHandMeshComponent.h"
#include "StructTypesPrivate.h"

using isdk::api::PalmGrabRecognizer;
using isdk::api::PalmGrabRecognizerPtr;

class UIsdkHandPalmGrabRecognizer::FPalmGrabRecognizerImpl
    : public FApiImpl<PalmGrabRecognizer, PalmGrabRecognizerPtr>
{
 public:
  explicit FPalmGrabRecognizerImpl(std::function<PalmGrabRecognizerPtr()> CreateFn)
      : FApiImpl(std::move(CreateFn))
  {
  }
};

UIsdkHandPalmGrabRecognizer::UIsdkHandPalmGrabRecognizer()
{
  PalmGrabRecognizerImpl = MakePimpl<FPalmGrabRecognizerImpl>(
      [this]() -> PalmGrabRecognizerPtr
      {
        const auto* ApiHandPositionFrame = EnsureHandPositionFrame();
        if (!ApiHandPositionFrame)
        {
          return nullptr;
        }

        auto Instance =
            PalmGrabRecognizer::create(ApiHandPositionFrame->getIHandPositionFrameHandle());
        UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this);

        return Instance;
      });
}

void UIsdkHandPalmGrabRecognizer::BeginDestroy()
{
  UObject::BeginDestroy();

  PalmGrabRecognizerImpl.Reset();
}

void UIsdkHandPalmGrabRecognizer::UpdateRecognizerParams() const
{
  if (Recognizer)
  {
    Recognizer->setFingerMinCount(PalmGrabMinFingers);
    Recognizer->setFingerMinCurlValue(isdk_FingerType_Index, PalmGrabMinCurl);
    Recognizer->setFingerMinCurlValue(isdk_FingerType_Middle, PalmGrabMinCurl);
    Recognizer->setFingerMinCurlValue(isdk_FingerType_Ring, PalmGrabMinCurl);
    Recognizer->setFingerMinCurlValue(isdk_FingerType_Pinky, PalmGrabMinCurl);
  }
}

void UIsdkHandPalmGrabRecognizer::UpdateState(float DeltaTime)
{
  if (!IsValid(HandVisual) || HandVisual->TryGetApiIHandPositionFrame() == nullptr)
  {
    // HandVisual isn't initialized yet. Wait for it to initialize before attempting creation.
    return;
  }

  if (!Recognizer)
  {
    Recognizer = PalmGrabRecognizerImpl->GetOrCreateInstance();
    UpdateRecognizerParams();
  }

  if (!ensureMsgf(Recognizer, TEXT("%s: Failed to create PalmGrabRecognizerImpl"), *GetFullName()))
  {
    return;
  }

  const auto JointsDataSource =
      IIsdkIHandJoints::Execute_IsHandJointDataValid(HandVisual->GetJointsDataSource().GetObject());
  if (!ensureMsgf(
          JointsDataSource,
          TEXT("%s: No joints data source to drive grab recognition"),
          *GetFullName()))
  {
    return;
  }

  const bool bWasActive = !!Recognizer->isActive();
  Recognizer->update(DeltaTime);
  const bool bIsActive = !!Recognizer->isActive();

  if (!bWasActive && bIsActive && PalmGrabStarted.IsBound())
  {
    PalmGrabStarted.Broadcast();
  }
  if (bWasActive && !bIsActive && PalmGrabFinished.IsBound())
  {
    PalmGrabFinished.Broadcast();
  }
}

bool UIsdkHandPalmGrabRecognizer::IsActive()
{
  if (PalmGrabRecognizerImpl->IsInstanceValid())
  {
    return !!PalmGrabRecognizerImpl->GetOrCreateInstance()->isActive();
  }
  return false;
}

isdk::api::IHandPositionFrame* UIsdkHandPalmGrabRecognizer::EnsureHandPositionFrame() const
{
  bool validInputs = true;
  validInputs &= UIsdkChecks::EnsureMsgfDependencyIsValid(HandVisual, this, TEXT("HandVisual"));
  if (!validInputs)
  {
    return nullptr;
  }

  const auto ApiHandPositionFrame = HandVisual->TryGetApiIHandPositionFrame();
  validInputs &= UIsdkChecks::EnsureMsgfDependencyApiIsInitialized(
      ApiHandPositionFrame, this, TEXT("HandVisual"));
  if (!validInputs)
  {
    return nullptr;
  }

  return ApiHandPositionFrame;
}
