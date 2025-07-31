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

#include "Interaction/Surfaces/IsdkPointablePlane.h"
#include "Templates/PimplPtr.h"
#include "StructTypesPrivate.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "IsdkChecks.h"

using isdk::api::IPose;
using isdk::api::PointablePlane;
using isdk::api::PointablePlanePtr;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FPointablePlaneImpl : public FApiImpl<PointablePlane, PointablePlanePtr>
			{
			public:
				explicit FPointablePlaneImpl(std::function<PointablePlanePtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkPointablePlane::UIsdkPointablePlane()
{
  PrimaryComponentTick.bCanEverTick = false;

  bWantsOnUpdateTransform = true;
  Size = FVector2D(1.0, 1.0);

  PointablePlaneImpl = MakePimpl<isdk::api::helper::FPointablePlaneImpl>(
      [this]() -> PointablePlanePtr
      {
        ovrpPosef PlaneOriginPose;
        StructTypesUtils::Copy(GetComponentTransform(), PlaneOriginPose);

        // Create backing surface
        const ovrpVector2f ApiSize = StructTypesUtils::Convert((FVector2f)Size);
        UIsdkChecks::WarnIfApiSizeInvalid(Size, this, TEXT("Size"));

        auto Instance = PointablePlane::createWithPose(&PlaneOriginPose, &ApiSize);
        if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
        {
          return nullptr;
        }
        return Instance;
      });
}

void UIsdkPointablePlane::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  PointablePlaneImpl->DestroyInstance();
  Super::EndPlay(EndPlayReason);
}

void UIsdkPointablePlane::BeginDestroy()
{
  Super::BeginDestroy();
  PointablePlaneImpl.Reset();
}

void UIsdkPointablePlane::OnUpdateTransform(
    EUpdateTransformFlags UpdateTransformFlags,
    ETeleportType Teleport)
{
  Super::OnUpdateTransform(UpdateTransformFlags, Teleport);

  auto Instance = GetApiPointablePlane();
  if (Instance != nullptr)
  {
    ovrpPosef InteractablePose;
    StructTypesUtils::Copy(GetComponentTransform(), InteractablePose);
    Instance->setPose(&InteractablePose);
  }
}

IPose* UIsdkPointablePlane::GetApiIPose()
{
  return GetApiPointablePlane();
}

PointablePlane* UIsdkPointablePlane::GetApiPointablePlane()
{
  return PointablePlaneImpl->GetOrCreateInstance();
}

bool UIsdkPointablePlane::IsApiInstanceValid() const
{
  return PointablePlaneImpl->IsInstanceValid();
}

isdk::api::ISurface* UIsdkPointablePlane::GetApiISurface()
{
  return GetApiPointablePlane();
}

FVector2D UIsdkPointablePlane::GetSize() const
{
  return Size;
}

void UIsdkPointablePlane::SetSize(FVector2D InSize)
{
  Size = InSize;
  if (PointablePlaneImpl->IsInstanceValid())
  {
    const ovrpVector2f ApiSize = StructTypesUtils::Convert((FVector2f)Size);
    UIsdkChecks::WarnIfApiSizeInvalid(Size, this, TEXT("Size"));

    PointablePlaneImpl->GetOrCreateInstance()->setSize(&ApiSize);
  }
}

void UIsdkPointablePlane::GetDebugSegments(TArray<TPair<FVector, FVector>>& OutSegments) const
{
  const auto Size3D = FVector(0, Size.X, Size.Y);
  FVector PlanePoints[4] = {
      GetComponentTransform().TransformPosition(Size3D),
      GetComponentTransform().TransformPosition(Size3D * FVector(0, 1, -1)),
      GetComponentTransform().TransformPosition(Size3D * FVector(0, -1, -1)),
      GetComponentTransform().TransformPosition(Size3D * FVector(0, -1, 1))};

  OutSegments.Reset(4);
  OutSegments.Add(TPair<FVector, FVector>(PlanePoints[0], PlanePoints[1]));
  OutSegments.Add(TPair<FVector, FVector>(PlanePoints[1], PlanePoints[2]));
  OutSegments.Add(TPair<FVector, FVector>(PlanePoints[2], PlanePoints[3]));
  OutSegments.Add(TPair<FVector, FVector>(PlanePoints[3], PlanePoints[0]));
}
