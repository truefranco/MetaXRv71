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

#include "HandPoseDetection/IsdkHandThumbRecognizer.h"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "isdk_api/isdk_api.hpp"
#include "StructTypesPrivate.h"

using isdk::api::DigitRecognizer;
using isdk::api::ThumbRecognizer;
using isdk::api::ThumbRecognizerPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FThumbRecognizerImpl : public FApiImpl<ThumbRecognizer, ThumbRecognizerPtr>
			{
			public:
				explicit FThumbRecognizerImpl(std::function<ThumbRecognizerPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}

				void SetExpectedRanges(
					const isdk_ThumbRecognizer_ExpectedThumbValueRanges& Ranges,
					EIsdkDetection_ThumbCalcType InCalcType)
				{
					switch (InCalcType)
					{
					case EIsdkDetection_ThumbCalcType::Curl:
						ExpectedValueRange = { Ranges.curl.minValue, Ranges.curl.maxValue };
						break;
					case EIsdkDetection_ThumbCalcType::Flexion:
						ExpectedValueRange = { Ranges.flexion.minValue, Ranges.flexion.maxValue };
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

UIsdkHandThumbRecognizer::UIsdkHandThumbRecognizer()
{
  ThumbRecognizerImpl = MakePimpl<isdk::api::helper::FThumbRecognizerImpl>(
      [this]() -> ThumbRecognizerPtr
      {
        const auto* ApiHandPositionFrame = EnsureHandPositionFrame();
        if (!ApiHandPositionFrame)
        {
          return nullptr;
        }

        isdk_DigitRangeParams ApiRangeParameters;
        StructTypesUtils::Copy(RangeParameters, ApiRangeParameters);
        const auto ApiCalcType = static_cast<isdk_Detection_ThumbCalcType>(CalcType);

        // De-normalize the range parameters to ISDK space. Note that the return value is not
        // coordinate-system safe, as the fields on isdk_DigitRangeParams are marked as
        // ISDK_CNVT_NONE.
        ApiRangeParameters =
            ThumbRecognizer::getThumbRangeParamsFromNormalized(&ApiRangeParameters, ApiCalcType);

        auto Instance = ThumbRecognizer::create(
            ApiHandPositionFrame->getIHandPositionFrameHandle(), &ApiRangeParameters, ApiCalcType);
        UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this);

        ThumbRecognizerImpl->SetExpectedRanges(
            ThumbRecognizer::getExpectedThumbRawValueRanges(), CalcType);

        return Instance;
      });
}

void UIsdkHandThumbRecognizer::BeginDestroy()
{
  Super::BeginDestroy();
  ThumbRecognizerImpl.Reset();
}

FVector2D UIsdkHandThumbRecognizer::GetRawExpectedRange()
{
  return ThumbRecognizerImpl->ExpectedValueRange;
}

DigitRecognizer* UIsdkHandThumbRecognizer::CreateDigitRecognizer()
{
  return ThumbRecognizerImpl->GetOrCreateInstance();
}
