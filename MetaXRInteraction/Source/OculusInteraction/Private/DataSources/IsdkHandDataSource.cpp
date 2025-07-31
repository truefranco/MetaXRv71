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

#include "DataSources/IsdkHandDataSource.h"

#include "Engine/World.h"

#include "StructTypesPrivate.h"
#include "isdk_api/isdk_api.hpp"

using isdk::api::IHandDataSource;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FDataSourceImpl
			{
			public:
				isdk_HandData HandData{};
			};
		}
	}
} // namespace isdk::api::helper

UIsdkHandDataSource::UIsdkHandDataSource()
{
  PrimaryComponentTick.bCanEverTick = true;

  DataSourceImpl = MakePimpl<isdk::api::helper::FDataSourceImpl>();
  HandData = CreateDefaultSubobject<UIsdkHandData>("HandData");
}

void UIsdkHandDataSource::BeginDestroy()
{
  Super::BeginDestroy();
  DataSourceImpl.Reset();
}

void UIsdkHandDataSource::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  LastTickedTime = GetWorld()->GetTimeSeconds();

  if (ShouldUpdateDataSource())
  {
    IHandDataSource* ApiInstance = GetApiIHandDataSource();
    if (ApiInstance)
    {
      auto RetVal = ApiInstance->updateData();
      EnsureDataSourceResult(static_cast<EIsdkDataSourceUpdateDataResult>(RetVal));
    }
  }
}

UIsdkHandData* UIsdkHandDataSource::GetHandData_Implementation()
{
  return HandData;
}

bool UIsdkHandDataSource::EnsureDataSourceResult(EIsdkDataSourceUpdateDataResult RetVal)
{
  return ensureMsgf(
      RetVal != EIsdkDataSourceUpdateDataResult::Failure_InvalidPrevDataSource,
      TEXT("Data source update failed due to a failure with the dependent data source."));
}

IHandDataSource* UIsdkHandDataSource::ValidateAndGetApiDataSource(
    UIsdkHandDataSource* InDataSource,
    FString&& ErrMsgFieldName)
{
  const auto InputDataSource = InDataSource;
  if (!ensureMsgf(InputDataSource, TEXT("%s must be set"), *ErrMsgFieldName))
  {
    return nullptr;
  }

  const auto ApiFromDataSource = InDataSource->GetApiIHandDataSource();
  if (!ensureMsgf(ApiFromDataSource, TEXT("%s could not be initialized"), *ErrMsgFieldName))
  {
    return nullptr;
  }

  return ApiFromDataSource;
}

void UIsdkHandDataSource::ReadHandJointsFromImpl()
{
  IHandDataSource* ApiInstance = GetApiIHandDataSource();
  if (ApiInstance)
  {
    auto RetVal = ApiInstance->updateData();
    EnsureDataSourceResult(static_cast<EIsdkDataSourceUpdateDataResult>(RetVal));

    int outCount{};
    isdk_IHandDataSource_getJoints(
        ApiInstance->getIHandDataSourceHandle(),
        &DataSourceImpl->HandData.joints[0],
        HandData->GetNumJoints(),
        &outCount);
    HandData->SetJoints(DataSourceImpl->HandData);
  }
}
