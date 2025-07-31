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

#include "Components/ActorComponent.h"
#include "Templates/PimplPtr.h"
#include "CoreMinimal.h"
#include "Input/IsdkIPose.h"
#include "IsdkISurfacePatch.h"
#include "Debug/IsdkHasDebugSegments.h"

#include "IsdkClippedPlaneSurface.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class ClippedPlaneSurface;

		namespace helper
		{
			class FClippedPlaneSurfaceImpl;
		}
	}
} // namespace isdk::api

class UIsdkPointablePlane;

/**
 * Actor Component representing a surface plane with clipped boundaries, implements
 * IIsdkISurfacePatch and IIsdkHasDebugSegments
 *
 * @see IIsdkISurfacePatch
 * @see IIsdkHasDebugSegments
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Clipped Plane Surface"))
class OCULUSINTERACTION_API UIsdkClippedPlaneSurface : public UActorComponent,
                                                       public IIsdkISurfacePatch,
                                                       public IIsdkHasDebugSegments
{
  GENERATED_BODY()

 public:
  UIsdkClippedPlaneSurface();

  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void BeginDestroy() override;

  virtual isdk::api::ISurfacePatch* GetApiISurfacePatch() override;
  isdk::api::ClippedPlaneSurface* GetApiClippedPlaneSurface();

  /**
   * Returns an array of bounds clippers which determine the extent within which
   * this plane exists.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  const TArray<FIsdkBoundsClipper>& GetBoundsClippers() const;

  /**
   * Sets the array of bounds clippers which determine the extent within which
   * this plane exists.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetBoundsClippers(const TArray<FIsdkBoundsClipper>& InBoundsClippers);

  /**
   * Returns the pointable plane used for this surface
   */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  const UIsdkPointablePlane* GetPointablePlane() const;

  /**
   * Sets the pointable plane used for this surface
   */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetPointablePlane(UIsdkPointablePlane* InPointablePlane);

 protected:
  /**
   * The bounds that clip this plane, determining the extent within which it exists.
   */
  UPROPERTY(
      BlueprintGetter = GetBoundsClippers,
      BlueprintSetter = SetBoundsClippers,
      Category = InteractionSDK)
  TArray<FIsdkBoundsClipper> BoundsClippers;

  /* Pointable plane used for this surface */
  UPROPERTY(
      BlueprintGetter = GetPointablePlane,
      BlueprintSetter = SetPointablePlane,
      Category = InteractionSDK)
  UIsdkPointablePlane* PointablePlane;

  FDelegateHandle TransformUpdatedDelegateHandle;
  void UpdateNativeBoundsClipper();

  // IHasDebugSegments
  virtual void GetDebugSegments(TArray<TPair<FVector, FVector>>& OutSegments) const override;
  // ~IHasDebugSegments

 private:
  TPimplPtr<isdk::api::helper::FClippedPlaneSurfaceImpl> ClippedPlaneSurfaceImpl;
};
