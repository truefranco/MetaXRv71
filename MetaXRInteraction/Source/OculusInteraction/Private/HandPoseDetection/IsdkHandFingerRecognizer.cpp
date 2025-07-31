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

#include "HandPoseDetection/IsdkHandFingerRecognizer.h"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "isdk_api/isdk_api.hpp"
#include "StructTypesPrivate.h"

using isdk::api::DigitRecognizer;
using isdk::api::FingerRecognizer;
using isdk::api::FingerRecognizerPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FFingerRecognizerImpl : public FApiImpl<FingerRecognizer, FingerRecognizerPtr>
			{
			public:
				explicit FFingerRecognizerImpl(std::function<FingerRecognizerPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}

				void SetExpectedRanges(
					const isdk_FingerRecognizer_ExpectedFingerValueRanges& Ranges,
					EIsdkDetection_FingerCalcType InCalcType)
				{
					switch (InCalcType)
					{
					case EIsdkDetection_FingerCalcType::Curl:
						ExpectedValueRange = { Ranges.curl.minValue, Ranges.curl.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::Flexion:
						ExpectedValueRange = { Ranges.flexion.minValue, Ranges.flexion.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::Abduction:
						ExpectedValueRange = { Ranges.abduction.minValue, Ranges.abduction.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::Opposition:
						ExpectedValueRange = { Ranges.opposition.minValue, Ranges.opposition.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::Grab:
						ExpectedValueRange = { Ranges.grab.minValue, Ranges.grab.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::OppositionTangentPlane:
						ExpectedValueRange = {
							Ranges.oppositionTangentPlane.minValue, Ranges.oppositionTangentPlane.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::OppositionNormal:
						ExpectedValueRange = { Ranges.oppositionNormal.minValue, Ranges.oppositionNormal.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::OppositionTopTwo:
						ExpectedValueRange = { Ranges.oppositionTopTwo.minValue, Ranges.oppositionTopTwo.maxValue };
						break;
					case EIsdkDetection_FingerCalcType::OppositionTopThree:
						ExpectedValueRange = {
							Ranges.oppositionTopThree.minValue, Ranges.oppositionTopThree.maxValue };
						break;
					default:
						check(false);
					}
				}

				FVector2D ExpectedValueRange = FVector2D::ZeroVector;
			};
		}
	}
} // namespace isdk::api::helper

UIsdkHandFingerRecognizer::UIsdkHandFingerRecognizer()
{
  FingerRecognizerImpl = MakePimpl<isdk::api::helper::FFingerRecognizerImpl>(
      [this]() -> FingerRecognizerPtr
      {
        const auto* ApiHandPositionFrame = EnsureHandPositionFrame();
        if (!ApiHandPositionFrame)
        {
          return nullptr;
        }

        isdk_DigitRangeParams ApiRangeParameters;
        StructTypesUtils::Copy(RangeParameters, ApiRangeParameters);
        const auto ApiFingerType = static_cast<isdk_FingerType>(FingerType);
        const auto ApiCalcType = static_cast<isdk_Detection_FingerCalcType>(CalcType);

        // De-normalize the range parameters to ISDK space. Note that the return value is not
        // coordinate-system safe, as the fields on isdk_DigitRangeParams are marked as
        // ISDK_CNVT_NONE.
        ApiRangeParameters = FingerRecognizer::getFingerRangeParamsFromNormalized(
            &ApiRangeParameters, ApiFingerType, ApiCalcType);

        auto Instance = FingerRecognizer::create(
            ApiHandPositionFrame->getIHandPositionFrameHandle(),
            &ApiRangeParameters,
            ApiFingerType,
            ApiCalcType);
        UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this);

        FingerRecognizerImpl->SetExpectedRanges(
            FingerRecognizer::getExpectedFingerRawValueRanges(ApiFingerType), CalcType);

        return Instance;
      });
}

void UIsdkHandFingerRecognizer::BeginDestroy()
{
  Super::BeginDestroy();
  FingerRecognizerImpl.Reset();
}

FVector2D UIsdkHandFingerRecognizer::GetRawExpectedRange()
{
  return FingerRecognizerImpl->ExpectedValueRange;
}

DigitRecognizer* UIsdkHandFingerRecognizer::CreateDigitRecognizer()
{
  return FingerRecognizerImpl->GetOrCreateInstance();
}
