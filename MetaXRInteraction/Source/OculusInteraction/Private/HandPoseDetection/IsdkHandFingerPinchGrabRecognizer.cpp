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

#include "HandPoseDetection/IsdkHandFingerPinchGrabRecognizer.h"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "IsdkHandMeshComponent.h"
#include "StructTypesPrivate.h"
#include "IsdkFunctionLibrary.h"

using isdk::api::FingerPinchGrabRecognizer;
using isdk::api::FingerPinchGrabRecognizerPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FFingerPinchGrabRecognizerImpl
				: public FApiImpl<FingerPinchGrabRecognizer, FingerPinchGrabRecognizerPtr>
			{
			public:
				explicit FFingerPinchGrabRecognizerImpl(std::function<FingerPinchGrabRecognizerPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkHandFingerPinchGrabRecognizer::UIsdkHandFingerPinchGrabRecognizer()
{
  FingerPinchGrabRecognizerImpl =
      MakePimpl<isdk::api::helper::FFingerPinchGrabRecognizerImpl>(
          [this]() -> FingerPinchGrabRecognizerPtr
          {
            const auto* ApiHandPositionFrame = EnsureHandPositionFrame();
            if (!ApiHandPositionFrame)
            {
              return nullptr;
            }

            const auto ApiFingerType = static_cast<isdk_FingerType>(FingerType);

            auto Instance = FingerPinchGrabRecognizer::create(
                ApiHandPositionFrame->getIHandPositionFrameHandle(), ApiFingerType);
            UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this);

            return Instance;
          });
}

void UIsdkHandFingerPinchGrabRecognizer::BeginDestroy()
{
  Super::BeginDestroy();

  FingerPinchGrabRecognizerImpl.Reset();
}

void UIsdkHandFingerPinchGrabRecognizer::UpdatePinchConfidence()
{
  if (CurrentWristForward.IsZero() || CurrentHMDForward.IsZero())
  {
    bLastPinchConfidence = false;
    return;
  }

  FingerPinchGrabRecognizer* Recognizer = FingerPinchGrabRecognizerImpl->GetOrCreateInstance();
  if (ensureMsgf(
          Recognizer != nullptr,
          TEXT("%s: Failed to create FingerPinchGrabRecognizerImpl"),
          *GetFullName()))
  {
    // Set Confidence with Data
    ovrpVector3f DataWristFwd;
    ovrpVector3f DataHMDFwd;

    StructTypesUtils::Copy((FVector3d)CurrentWristForward, DataWristFwd);
    StructTypesUtils::Copy((FVector3d)CurrentHMDForward, DataHMDFwd);
    bLastPinchConfidence =
        !!Recognizer->setInputTrackingConfidenceWithData(DataWristFwd, DataHMDFwd);
  }
}

void UIsdkHandFingerPinchGrabRecognizer::UpdateState(float DeltaTime)
{
  if (!IsValid(HandVisual) || HandVisual->TryGetApiIHandPositionFrame() == nullptr)
  {
    // HandVisual isn't initialized yet. Wait for it to initialize before attempting creation.
    return;
  }

  FingerPinchGrabRecognizer* Recognizer = FingerPinchGrabRecognizerImpl->GetOrCreateInstance();
  if (ensureMsgf(
          Recognizer != nullptr,
          TEXT("%s: Failed to create FingerPinchGrabRecognizerImpl"),
          *GetFullName()))
  {
    UpdatePinchConfidence();

    if (IIsdkIHandJoints::Execute_IsHandJointDataValid(
            HandVisual->GetJointsDataSource().GetObject()))
    {
      const bool bWasActive = !!Recognizer->isActive();
      Recognizer->update(DeltaTime);
      const bool bIsActive = !!Recognizer->isActive();

      if (!bWasActive && bIsActive && PinchGrabStarted.IsBound())
      {
        PinchGrabStarted.Broadcast();
      }
      if (bWasActive && !bIsActive && PinchGrabFinished.IsBound())
      {
        PinchGrabFinished.Broadcast();
      }
    }
  }
}

bool UIsdkHandFingerPinchGrabRecognizer::IsActive()
{
  if (FingerPinchGrabRecognizerImpl->IsInstanceValid())
  {
    return !!FingerPinchGrabRecognizerImpl->GetOrCreateInstance()->isActive();
  }
  return false;
}

float UIsdkHandFingerPinchGrabRecognizer::GetPinchStrength()
{
  if (FingerPinchGrabRecognizerImpl->IsInstanceValid())
  {
    return FingerPinchGrabRecognizerImpl->GetOrCreateInstance()->getPinchStrength();
  }
  return 0.0f;
}

isdk::api::IHandPositionFrame* UIsdkHandFingerPinchGrabRecognizer::EnsureHandPositionFrame() const
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
