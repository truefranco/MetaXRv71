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

#pragma once

#include "CoreMinimal.h"
#include "IsdkISurface.h"
#include "Templates/PimplPtr.h"
#include "Components/SceneComponent.h"
#include "Debug/IsdkHasDebugSegments.h"
#include "Input/IsdkIPose.h"
#include "IsdkPointableBox.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class PointableOrientedBox;

		namespace helper
		{
			class FPointableOrientedBoxImpl;
		}
	}
} // namespace isdk::api

/**
 * UIsdkPointableBox is used to represent a box volume within which interaction takes place.  It
 * extends IIsdkISurface to provide polymorphic behavior, where any shape of surface may be valid
 * for interaction.
 *
 * @see IIsdkIPose
 * @see IIsdkISurface
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Pointable Box"))
class OCULUSINTERACTION_API UIsdkPointableBox : public USceneComponent,
                                                public IIsdkIPose,
                                                public IIsdkISurface,
                                                public IIsdkHasDebugSegments
{
  GENERATED_BODY()

 public:
  UIsdkPointableBox();
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void BeginDestroy() override;

  virtual void OnUpdateTransform(
      EUpdateTransformFlags UpdateTransformFlags,
      ETeleportType Teleport = ETeleportType::None) override;

  virtual isdk::api::IPose* GetApiIPose() override;
  isdk::api::PointableOrientedBox* GetApiPointableBox();

  // IIsdkISurface Implementation
  virtual bool IsApiInstanceValid() const override;
  virtual isdk::api::ISurface* GetApiISurface() override;

  // Getter
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  FVector GetSize() const
  {
    return Size;
  }

  // Setter
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetSize(FVector InSize);

  // IHasDebugSegments
  virtual void GetDebugSegments(TArray<TPair<FVector, FVector>>& OutSegments) const override;
  // ~IHasDebugSegments

 private:
  UPROPERTY(BlueprintGetter = GetSize, BlueprintSetter = SetSize, Category = InteractionSDK)
  FVector Size;

  TPimplPtr<isdk::api::helper::FPointableOrientedBoxImpl> PointableBoxImpl;
};
