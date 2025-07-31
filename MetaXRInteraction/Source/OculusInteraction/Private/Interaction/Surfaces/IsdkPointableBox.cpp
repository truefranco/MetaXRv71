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

#include "Interaction/Surfaces/IsdkPointableBox.h"
#include "Templates/PimplPtr.h"
#include "StructTypesPrivate.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "IsdkChecks.h"

using isdk::api::IPose;
using isdk::api::PointableOrientedBox;
using isdk::api::PointableOrientedBoxPtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FPointableOrientedBoxImpl : public FApiImpl<PointableOrientedBox, PointableOrientedBoxPtr>
			{
			public:
				explicit FPointableOrientedBoxImpl(std::function<PointableOrientedBoxPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkPointableBox::UIsdkPointableBox()
{
  PrimaryComponentTick.bCanEverTick = false;
  bWantsOnUpdateTransform = true;

  PointableBoxImpl = MakePimpl<isdk::api::helper::FPointableOrientedBoxImpl>(
      [this]() -> PointableOrientedBoxPtr
      {
        ovrpPosef BoxOriginPose;
        StructTypesUtils::Copy(GetComponentTransform(), BoxOriginPose);

        // Create backing surface
        const ovrpVector3f ApiSize = StructTypesUtils::Convert((FVector3d)Size);
        UIsdkChecks::WarnIfApiSizeInvalid(Size, this, TEXT("Size"));

        auto Instance = PointableOrientedBox::createWithPose(&BoxOriginPose, &ApiSize);
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
        {
          return nullptr;
        }
        return Instance;
      });
}

void UIsdkPointableBox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  PointableBoxImpl->DestroyInstance();
  Super::EndPlay(EndPlayReason);
}

void UIsdkPointableBox::BeginDestroy()
{
  Super::BeginDestroy();
  PointableBoxImpl.Reset();
}

void UIsdkPointableBox::OnUpdateTransform(
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport)
{
  Super::OnUpdateTransform(UpdateTransformFlags, Teleport);

  auto Instance = GetApiPointableBox();
  if (Instance != nullptr)
  {
    ovrpPosef InteractablePose;
    StructTypesUtils::Copy(GetComponentTransform(), InteractablePose);
    Instance->setPose(&InteractablePose);
  }
}

IPose* UIsdkPointableBox::GetApiIPose()
{
  return GetApiPointableBox();
}

PointableOrientedBox* UIsdkPointableBox::GetApiPointableBox()
{
  return PointableBoxImpl->GetOrCreateInstance();
}

bool UIsdkPointableBox::IsApiInstanceValid() const
{
  return PointableBoxImpl->IsInstanceValid();
}

isdk::api::ISurface* UIsdkPointableBox::GetApiISurface()
{
  return GetApiPointableBox();
}

void UIsdkPointableBox::SetSize(FVector InSize)
{
  Size = InSize;
  if (PointableBoxImpl->IsInstanceValid())
  {
    const ovrpVector3f ApiSize = StructTypesUtils::Convert((FVector3d)Size);
    UIsdkChecks::WarnIfApiSizeInvalid(Size, this, TEXT("Size"));

    PointableBoxImpl->GetOrCreateInstance()->setSize(&ApiSize);
  }
}

void UIsdkPointableBox::GetDebugSegments(TArray<TPair<FVector, FVector>>& OutSegments) const
{
  const auto HalfSize3D = FVector(Size.X, Size.Y, Size.Z) / 2.f;
  FVector BoxPoints[8] = {
      GetComponentTransform().TransformPosition(HalfSize3D),
      GetComponentTransform().TransformPosition(HalfSize3D * FVector(1, 1, -1)),
      GetComponentTransform().TransformPosition(HalfSize3D * FVector(1, -1, -1)),
      GetComponentTransform().TransformPosition(HalfSize3D * FVector(1, -1, 1)),
      GetComponentTransform().TransformPosition(HalfSize3D) * FVector(-1, 1, -1),
      GetComponentTransform().TransformPosition(HalfSize3D * FVector(-1, 1, -1)),
      GetComponentTransform().TransformPosition(HalfSize3D * FVector(-1, -1, -1)),
      GetComponentTransform().TransformPosition(HalfSize3D * FVector(-1, -1, 1))};

  OutSegments.Reset(12);
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[0], BoxPoints[1]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[1], BoxPoints[2]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[2], BoxPoints[3]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[3], BoxPoints[0]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[0], BoxPoints[4]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[1], BoxPoints[5]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[2], BoxPoints[6]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[3], BoxPoints[7]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[4], BoxPoints[5]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[5], BoxPoints[6]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[6], BoxPoints[7]));
  OutSegments.Add(TPair<FVector, FVector>(BoxPoints[7], BoxPoints[4]));
}
