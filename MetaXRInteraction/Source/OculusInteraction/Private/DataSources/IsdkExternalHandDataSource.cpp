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

#include "DataSources/IsdkExternalHandDataSource.h"
#include "Templates/PimplPtr.h"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "StructTypesPrivate.h"

using isdk::api::ExternalHandSource;
using isdk::api::ExternalHandSourcePtr;
using isdk::api::IHandDataSource;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FExternalHandDataSourceImpl : public FApiImpl<ExternalHandSource, ExternalHandSourcePtr>
			{
			public:
				explicit FExternalHandDataSourceImpl(std::function<ExternalHandSourcePtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}

				isdk_HandData ScratchHandData{};
			};
		}
	}
} // namespace isdk::api::helper

UIsdkExternalHandDataSource::UIsdkExternalHandDataSource()
{
  HandJointMappings = CreateDefaultSubobject<UIsdkHandJointMappings>(TEXT("HandJointMappings"));
  ExternalHandDataSourceImpl =
      MakePimpl<isdk::api::helper::FExternalHandDataSourceImpl>(
          [this]() -> ExternalHandSourcePtr
          {
            auto Instance =
                ExternalHandSource::create(static_cast<int>(EIsdkHandBones::EHandBones_MAX));
            if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
            {
              return nullptr;
            }

            return Instance;
          });
  PopulateOpenXRBoneMaps();
}

void UIsdkExternalHandDataSource::BeginPlay()
{
  Super::BeginPlay();

  // Always reset to identity, in case invalid data was persisted on the HandData instance.
  HandData->SetJointsToIdentity();
}

void UIsdkExternalHandDataSource::SetHandData(
    const UIsdkHandData* InHandData,
    const FTransform& WorldTransform)
{
  // Update the HandData on the base class, so that values from GetHandData are correct
  if (ensureMsgf(IsValid(InHandData), TEXT("InHandData must not be null")))
  {
    HandData->SetJoints(*InHandData);

    SetImplHandData(WorldTransform);
  }
}

void UIsdkExternalHandDataSource::SetHandJointMappings(
    const TArray<FIsdkExternalHandPositionFrame_ThumbJointMapping>& ThumbJointMappings,
    const TArray<FIsdkExternalHandPositionFrame_FingerJointMapping>& FingerJointMappings)
{
  const auto ThumbJointMappingCount = ThumbJointMappings.Num();
  const auto FingerJointMappingCount = FingerJointMappings.Num();
  HandJointMappings->ThumbJointMappings.SetNum(ThumbJointMappingCount);
  HandJointMappings->FingerJointMappings.SetNum(FingerJointMappingCount);

  for (auto Index = 0; Index < ThumbJointMappingCount; ++Index)
  {
    HandJointMappings->ThumbJointMappings[Index] = ThumbJointMappings[Index];
  }
  for (auto Index = 0; Index < FingerJointMappingCount; ++Index)
  {
    HandJointMappings->FingerJointMappings[Index] = FingerJointMappings[Index];
  }
}

bool UIsdkExternalHandDataSource::IsApiInstanceValid() const
{
  return ExternalHandDataSourceImpl->IsInstanceValid();
}

IHandDataSource* UIsdkExternalHandDataSource::GetApiIHandDataSource()
{
  return GetApiExternalHandSource();
}

ExternalHandSource* UIsdkExternalHandDataSource::GetApiExternalHandSource()
{
  return ExternalHandDataSourceImpl->GetOrCreateInstance();
}

void UIsdkExternalHandDataSource::SetImplHandData(const FTransform& WorldTransform)
{
  // Update the joints on the base instance.
  const auto Instance = ExternalHandDataSourceImpl->GetOrCreateInstance();
  if (Instance != nullptr)
  {
    HandData->ReadJoints(ExternalHandDataSourceImpl->ScratchHandData);
    ExternalHandDataSourceImpl->ScratchHandData.root = StructTypesUtils::Convert(WorldTransform);
    isdk_DataSourceUpdateDataResult RetVal =
        GetApiExternalHandSource()->setData(&ExternalHandDataSourceImpl->ScratchHandData);

    EnsureDataSourceResult(static_cast<EIsdkDataSourceUpdateDataResult>(RetVal));
  }
}

void UIsdkExternalHandDataSource::BeginDestroy()
{
  Super::BeginDestroy();
  ExternalHandDataSourceImpl.Reset();
}

void UIsdkExternalHandDataSource::PopulateOpenXRBoneMaps()
{
  /*
  Mapping between OVR/Native and OpenXR
  There are bones not mapped in OpenXR: HandIndex0, HandMiddle0, HandRing0
  and one bone not mapped in OVR/Native: HandThumb0
  */
  InboundBoneMap = {
      {2, 3}, // HandThumb1 (Metacarpal)
      {3, 4}, // HandThumb2 (Proximal)
      {4, 5}, // HandThumb3 (Distal)
      {5, 19}, // HandThumbTip
      {7, 6}, // HandIndex1 (Proximal)
      {8, 7}, // HandIndex2 (Intermediate)
      {9, 8}, // HandIndex3 (Distal)
      {10, 20}, // HandIndexTip
      {12, 9}, // HandMiddle1 (Proximal)
      {13, 10}, // HandMiddle2 (Intermediate)
      {14, 11}, // HandMiddle3 (Distal)
      {15, 21}, // HandMiddleTip
      {17, 12}, // HandRing1 (Proximal)
      {18, 13}, // HandRing2 (Intermediate)
      {19, 14}, // HandRing3 (Distal)
      {20, 22}, // HandRingTip
      {21, 15}, // HandPinky0 (Metacarpal)
      {22, 16}, // HandPinky1 (Proximal)
      {23, 17}, // HandPinky2 (Intermediate)
      {24, 18}, // HandPinky3 (Distal)
      {25, 23} // HandPinkyTip
  };

  // Invert the mapping for Outbound
  for (auto& Elem : InboundBoneMap)
  {
    OutboundBoneMap.Add(Elem.Value, Elem.Key);
  }
  PropagateOutboundBoneMap();
}
