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

#include "HandPoseDetection/IsdkHandDigitRecognizer.h"

#include "IsdkChecks.h"
#include "StructTypesPrivate.h"

using isdk::api::DigitRecognizer;

void UIsdkHandDigitRecognizer::UpdateState(float DeltaTime)
{
  if (!ApiDigitRecognizer)
  {
    if (!IsValid(HandMesh) || !HandMesh->IsApiInstanceValid())
    {
      // HandMesh isn't initialized yet. Wait for it to initialize before attempting creation.
      return;
    }

    ApiDigitRecognizer = CreateDigitRecognizer();
    if (!ApiDigitRecognizer)
    {
      // Creation failed. This can happen if dependencies (such as HandMesh) were invalid.
      return;
    }
  }

  return ApiDigitRecognizer->update(DeltaTime);
}

bool UIsdkHandDigitRecognizer::IsActive()
{
  if (ApiDigitRecognizer)
  {
    return ApiDigitRecognizer->isActive() > 0;
  }
  return false;
}

float UIsdkHandDigitRecognizer::GetNormalizedRawValue()
{
  if (ApiDigitRecognizer)
  {
    return ApiDigitRecognizer->getRawValueNormalized();
  }
  return 0.0f;
}

float UIsdkHandDigitRecognizer::GetRawValue()
{
  const float NormalizedRawValue = GetNormalizedRawValue();
  const FVector2D RawExpectedRange = GetRawExpectedRange();
  return FMath::Lerp(RawExpectedRange.X, RawExpectedRange.Y, NormalizedRawValue);
}

isdk::api::IHandPositionFrame* UIsdkHandDigitRecognizer::EnsureHandPositionFrame() const
{
  bool validInputs = true;
  validInputs &= UIsdkChecks::EnsureMsgfDependencyIsValid(HandMesh, this, TEXT("HandMesh"));
  if (!validInputs)
  {
    return nullptr;
  }

  const auto ApiHandPositionFrame = HandMesh->TryGetApiIHandPositionFrame();
  validInputs &= UIsdkChecks::EnsureMsgfDependencyApiIsInitialized(
      ApiHandPositionFrame, this, TEXT("HandVisual"));
  if (!validInputs)
  {
    return nullptr;
  }

  return ApiHandPositionFrame;
}
