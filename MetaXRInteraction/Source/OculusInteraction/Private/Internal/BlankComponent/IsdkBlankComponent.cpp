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

#include "IsdkBlankComponent.h"
#include "Templates/PimplPtr.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "IsdkChecks.h"

// BEGIN Ignore this section - it's here only so this example compiles
namespace isdk {
	namespace api
	{
		typedef struct isdk_BlankComponent_ isdk_BlankComponent;

		class BlankComponent
		{
		public:
			explicit BlankComponent(isdk_BlankComponent* handle = nullptr) {}
			static void destroy(isdk_BlankComponent*) {}
			void setSurfacePatch(isdk_ISurfacePatch* surfacePatch) {}
			void setVectorOfSomething(const ovrpVector3f* vectorOfSomething) {}
		};

		using BlankComponentPtr = ObjectPtr<BlankComponent, isdk_BlankComponent>;
	}
} // namespace isdk::api

// END Ignore this section

namespace isdk {
	namespace api {
		namespace helper
		{
			class FIsdkBlankComponentImpl
				: public FApiImpl<isdk::api::BlankComponent, isdk::api::BlankComponentPtr>
			{
			public:
				explicit FIsdkBlankComponentImpl(std::function<isdk::api::BlankComponentPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkBlankComponent::UIsdkBlankComponent()
{
  // Good to explicitly set bCanEverTick in the constructor here, depending on whether or not
  // you need Tick().
  PrimaryComponentTick.bCanEverTick = true;

  // Create an instance of our opaque Impl class - IsdkBlankComponentImpl.
  // The `new` operator directly here MUST be matched by a `delete` in BeginDestroy().
  IsdkBlankComponentImpl =
      MakePimpl<isdk::api::helper::FIsdkBlankComponentImpl>(
          [this]() -> isdk::api::BlankComponentPtr
          {
            // Check that all dependencies are valid. This will lazy-create their own native
            // instances if this is the first time they have been accessed.
            isdk::api::ISurfacePatch* ApiSurfacePatch{};
            if (!IIsdkISurfacePatch::EnsureSurfacePatchValid(
                    SurfacePatch, this, TEXT("SurfacePatch"), ApiSurfacePatch))
            {
              return nullptr;
            }

            // Convert Unreal type to native type
            ovrpVector3f ApiVectorOfSomething{};
            // StructTypesUtils::Copy(VectorOfSomething, ApiVectorOfSomething);

            // Create the native BlankComponent instance by calling the C API create function. Pass
            // in any dependencies as C API types.
            isdk::api::BlankComponentPtr ApiInstance = nullptr;
            /*
            Instance = isdk::api::BlankComponent::create(
                ApiVectorOfSomething,
            SurfacePatch->GetApiISurfacePatch()->getISurfacePatchHandle());
             */
            UIsdkChecks::EnsureMsgfApiInstanceIsValid(ApiInstance.IsValid(), this);

            return ApiInstance;
          });
}

void UIsdkBlankComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UIsdkBlankComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  // Tear down any state on the Impl instance, but don't delete the Impl just yet.
  IsdkBlankComponentImpl->DestroyInstance();

  Super::EndPlay(EndPlayReason);
}

void UIsdkBlankComponent::BeginDestroy()
{
  Super::BeginDestroy();

  // BeginDestroy indicates that the component is being garbage collected - delete the Impl
  // instance.
  IsdkBlankComponentImpl.Reset();
}

bool UIsdkBlankComponent::IsApiInstanceValid() const
{
  return IsdkBlankComponentImpl->IsInstanceValid();
}

isdk::api::BlankComponent* UIsdkBlankComponent::GetApiIsdkBlankComponent()
{
  return IsdkBlankComponentImpl->GetOrCreateInstance();
}

void UIsdkBlankComponent::SetVectorOfSomething(const FVector& InVectorOfSomething)
{
  VectorOfSomething = InVectorOfSomething;

  // Only update the state on the Instance if it has already been created. Don't lazy-create
  // anything in setters, as they are often called during BeginPlay/Setup phases.
  if (IsdkBlankComponentImpl->IsInstanceValid())
  {
    // Convert unreal vector to native vector
    ovrpVector3f ApiVectorOfSomething{};
    // StructTypesUtils::Copy(VectorOfSomething, ApiVectorOfSomething);

    // Set property on the API instance
    IsdkBlankComponentImpl->GetOrCreateInstance()->setVectorOfSomething(&ApiVectorOfSomething);
  }
}

void UIsdkBlankComponent::SetSurfacePatch(TScriptInterface<IIsdkISurfacePatch> InSurfacePatch)
{
  SurfacePatch = InSurfacePatch;

  // Only update the state on the Instance if it has already been created. Don't lazy-create
  // anything in setters, as they are often called during BeginPlay/Setup phases.
  if (IsdkBlankComponentImpl->IsInstanceValid())
  {
    isdk::api::ISurfacePatch* ApiSurfacePatch{};
    if (IIsdkISurfacePatch::EnsureSurfacePatchValid(
            SurfacePatch, this, TEXT("SurfacePatch"), ApiSurfacePatch))
    {
      IsdkBlankComponentImpl->GetOrCreateInstance()->setSurfacePatch(
          ApiSurfacePatch->getISurfacePatchHandle());
    }
  }
}
