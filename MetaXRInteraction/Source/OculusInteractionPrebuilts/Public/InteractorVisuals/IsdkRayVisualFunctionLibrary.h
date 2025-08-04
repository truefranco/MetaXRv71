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
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DataSources/IsdkIHmdDataSource.h"
#include "StructTypes.h"
#include "IsdkHandData.h"
#include "IsdkRayVisual.h"
#include "IsdkRayVisualFunctionLibrary.generated.h"

class UIsdkRayVisualComponent;
class UIsdkRayInteractor;

/**
 * @class UIsdkRayVisualFunctionLibrary
 * @brief Blueprint Function Library used to support support Ray Visuals (cursor and pinch arrows)

 * @see UIsdkRayVisualComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(BlueprintType)
class OCULUSINTERACTIONPREBUILTS_API UIsdkRayVisualFunctionLibrary final
    : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()

 public:
  /**
   * @brief Determine the appropriate scale for the ray visual cursor given its distance from the
   * HMD
   * @param CursorPosition The position of the cursor in worldspace
   * @param ReferenceDistance Distance at which the scale of the cursor is 1.0
   * @param TScriptInterface<IIsdkIHmdDataSource>& Currently referenced HMD Data source
   * @return float Distance in unreal units
   */
  static float GetCursorScaleFromDistanceToHmd(
      const FVector CursorPosition,
      const float ReferenceDistance,
      const TScriptInterface<IIsdkIHmdDataSource>& HmdDataSource);

  /**
   * @brief With a given collision, determine the ideal transform for the ray visual cursor
   * @param CollisionInfo Struct containing the generated FIsdkSurfaceHit
   * @return FTransform The proposed new transform for the ray visual cursor
   */
  static FTransform GetCursorTransformFromCollision(FIsdkOptionalSurfaceHit CollisionInfo);

  /**
   * @brief With a given handedness, return the current pinch select strength, as defined in Content
   * Asset paths
   * @param Handedness Handedness for the desired pinch strength
   * @return UInputAction* Data asset containing input parameters, including pinch strength
   */
  static FName GetPinchStrengthIA(EIsdkHandedness Handedness);

  /**
   * @brief Propagate state changes from the Ray Interactor to the Ray Visual, updating state and
   * visibility as needed
   * @param RayVisual Pointer to the Ray Visual component (cursor and PinchArrow) associated with
   * the Ray Interactor
   * @param RayInteractor Interactor component driving the changes in state
   * @param RaySelectStrength Select strength to apply to the visual
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static void UpdateRayVisualWithRayInteractor(
      UIsdkRayVisualComponent* RayVisual,
      UIsdkRayInteractor* RayInteractor,
      float RaySelectStrength);
};
