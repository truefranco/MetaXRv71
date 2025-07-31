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

#include "DataSources/IsdkSyntheticHand.h"
#include "Templates/PimplPtr.h"
#include "StructTypesPrivate.h"
#include "ApiImpl.h"
#include "IsdkChecks.h"

using isdk::api::IHandDataModifier;
using isdk::api::IHandDataSource;
using isdk::api::SyntheticHand;
using isdk::api::SyntheticHandPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FSyntheticHandImpl : public FApiImpl<SyntheticHand, SyntheticHandPtr>
			{
			public:
				explicit FSyntheticHandImpl(std::function<SyntheticHandPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};

			TOptional<isdk_SyntheticHand_WristLockMode> Convert(EIsdkRootPoseLockMode LockMode)
			{
				TOptional<isdk_SyntheticHand_WristLockMode> WristLockMode{};
				switch (LockMode)
				{
				case EIsdkRootPoseLockMode::Position:
					WristLockMode = isdk_SyntheticHand_WristLockMode_Position;
					break;
				case EIsdkRootPoseLockMode::Rotation:
					WristLockMode = isdk_SyntheticHand_WristLockMode_Rotation;
					break;
				case EIsdkRootPoseLockMode::Full:
					WristLockMode = isdk_SyntheticHand_WristLockMode_Full;
					break;
				default:
					// Do nothing. If the intent is to unlock, user must call FreeRootTransform explicitly.
					break;
				}
				return WristLockMode;
			}
		}
	}
} // namespace isdk::api::helper

UIsdkSyntheticHand::UIsdkSyntheticHand()
{
  SyntheticHandImpl = MakePimpl<isdk::api::helper::FSyntheticHandImpl>(
      [this]() -> SyntheticHandPtr
      {
        FIsdkSyntheticHand_Config Config{};
        isdk_SyntheticHand_Config IsdkConfig;
        StructTypesUtils::Copy(Config, IsdkConfig);

        // If FromDataSource is set, validate its handle. Otherwise, create our instance with no
        // 'From', with the expectation that it will be set in the future.
        isdk_IHandDataSource* FromDataSourceHandle{};
        TryGetApiFromDataSource(FromDataSourceHandle);

        auto Instance =
            SyntheticHand::create(FromDataSourceHandle, bRecursiveUpdate ? 1 : 0, IsdkConfig);
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
        {
          return nullptr;
        }

        return Instance;
      });
}

void UIsdkSyntheticHand::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  SyntheticHandImpl->DestroyInstance();
  Super::EndPlay(EndPlayReason);
}

void UIsdkSyntheticHand::BeginDestroy()
{
  Super::BeginDestroy();
  SyntheticHandImpl.Reset();
}

bool UIsdkSyntheticHand::IsApiInstanceValid() const
{
  return SyntheticHandImpl->IsInstanceValid();
}

IHandDataSource* UIsdkSyntheticHand::GetApiIHandDataSource()
{
  return GetApiSyntheticHand();
}

IHandDataModifier* UIsdkSyntheticHand::GetApiIHandDataModifier()
{
  return GetApiSyntheticHand();
}

SyntheticHand* UIsdkSyntheticHand::GetApiSyntheticHand()
{
  return SyntheticHandImpl->GetOrCreateInstance();
}

void UIsdkSyntheticHand::LockRootTransform_Implementation(
    const FTransform& Transform,
    const float OverrideFactor,
    EIsdkRootPoseLockMode LockMode,
    bool bSkipAnimation)
{
  const auto WristLockMode = isdk::api::helper::Convert(LockMode);
  if (WristLockMode.IsSet())
  {
    const auto ApiPose = StructTypesUtils::Convert(Transform);
    SyntheticHandImpl->GetOrCreateInstance()->lockWristPose(
        &ApiPose, OverrideFactor, WristLockMode.GetValue(), bSkipAnimation ? 1 : 0);
  }
}

void UIsdkSyntheticHand::LockRootLocation_Implementation(
    const FVector& Position,
    const float OverrideFactor,
    bool bSkipAnimation)
{
  const ovrpVector3f ApiPosition = StructTypesUtils::Convert((FVector3d)Position);
  SyntheticHandImpl->GetOrCreateInstance()->lockWristPosition(
      &ApiPosition, OverrideFactor, bSkipAnimation ? 1 : 0);
}

void UIsdkSyntheticHand::LockRootRotation_Implementation(
    const FQuat& Rotation,
    const float OverrideFactor,
    bool bSkipAnimation)
{
  const ovrpQuatf ApiRotation = StructTypesUtils::Convert((FQuat4d)Rotation);
  SyntheticHandImpl->GetOrCreateInstance()->lockWristRotation(
      &ApiRotation, OverrideFactor, bSkipAnimation ? 1 : 0);
}

void UIsdkSyntheticHand::FreeRootTransform_Implementation(EIsdkRootPoseLockMode LockMode)
{
  const auto WristLockMode = isdk::api::helper::Convert(LockMode);
  if (WristLockMode.IsSet())
  {
    SyntheticHandImpl->GetOrCreateInstance()->freeWrist(WristLockMode.GetValue());
  }
}
