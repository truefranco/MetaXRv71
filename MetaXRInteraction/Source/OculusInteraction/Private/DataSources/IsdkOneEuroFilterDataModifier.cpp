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

#include "DataSources/IsdkOneEuroFilterDataModifier.h"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "isdk_api/isdk_api.hpp"
#include "IsdkChecks.h"

using isdk::api::IHandDataModifier;
using isdk::api::IHandDataSource;
using isdk::api::OneEuroHandFilter;
using isdk::api::OneEuroHandFilterPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FOneEuroHandFilterImpl : public FApiImpl<OneEuroHandFilter, OneEuroHandFilterPtr>
			{
			public:
				explicit FOneEuroHandFilterImpl(std::function<OneEuroHandFilterPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkOneEuroFilterDataModifier::UIsdkOneEuroFilterDataModifier()
{
  OneEuroHandFilterImpl =
      MakePimpl<isdk::api::helper::FOneEuroHandFilterImpl>(
          [this]() -> OneEuroHandFilterPtr
          {
            // If FromDataSource is set, validate its handle. Otherwise, create our instance with no
            // 'From', with the expectation that it will be set in the future.
            isdk_IHandDataSource* FromDataSourceHandle{};
            TryGetApiFromDataSource(FromDataSourceHandle);

            auto Instance =
                OneEuroHandFilter::create(FromDataSourceHandle, bRecursiveUpdate ? 1 : 0);
            if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
            {
              return nullptr;
            }

            return Instance;
          });
}

void UIsdkOneEuroFilterDataModifier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  OneEuroHandFilterImpl->DestroyInstance();
  Super::EndPlay(EndPlayReason);
}

void UIsdkOneEuroFilterDataModifier::BeginDestroy()
{
  Super::BeginDestroy();
  OneEuroHandFilterImpl.Reset();
}

bool UIsdkOneEuroFilterDataModifier::IsApiInstanceValid() const
{
  return OneEuroHandFilterImpl->IsInstanceValid();
}

IHandDataSource* UIsdkOneEuroFilterDataModifier::GetApiIHandDataSource()
{
  return GetApiOneEuroHandFilter();
}

IHandDataModifier* UIsdkOneEuroFilterDataModifier::GetApiIHandDataModifier()
{
  return GetApiOneEuroHandFilter();
}

OneEuroHandFilter* UIsdkOneEuroFilterDataModifier::GetApiOneEuroHandFilter()
{
  return OneEuroHandFilterImpl->GetOrCreateInstance();
}
