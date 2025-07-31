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
#include "IsdkHandData.h"
#include "IsdkTrackedDataSourceRigComponent.h"
#include "Components/ActorComponent.h"

#include "IsdkHandVisualsRigComponent.generated.h"

class UIsdkHandMeshComponent;
class UIsdkSyntheticHand;
class USceneComponent;
class UMaterial;
class USkeletalMesh;

/**
 * @class UIsdkHandVisualsRigComponent
 * @brief Rig Component Visualizing and Tracking Hands (abstract class)
 *
 * Holds logic pertaining to the visualization of hands and tracking
 * of hands while hands are the active tracked input (vs, say, controllers).
 *
 * @see UIsdkTrackedDataSourceRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(Abstract)
class OCULUSINTERACTIONPREBUILTS_API UIsdkHandVisualsRigComponent
    : public UIsdkTrackedDataSourceRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkHandVisualsRigComponent();

  /**
   * @brief Called when the Component is Initialized. Overridden from
   * UIsdkTrackedDataSourceRigComponent.
   */
  virtual void InitializeComponent() override;

  /**
   * @brief Get the Tracked Visual SceneComponent associated with this Rig Component
   *
   * Overridden from UIsdkTrackedDataSourceRigComponent. Tracked Visual refers to a visual directly
   * driven by a data source
   *
   * @return USceneComponent Scene Component associated as a Tracked Visual
   */
  virtual USceneComponent* GetTrackedVisual() const override;

  /**
   * @brief Get the Synthetic Visual SceneComponent associated with this Rig Component
   *
   * Overridden from UIsdkTrackedDataSourceRigComponent. Synthetic Visual refers to a visual that is
   * originally derived from data but modified during runtime
   *
   * @return USceneComponent Scene Component associated as a Synthetic Visual
   */
  virtual USceneComponent* GetSyntheticVisual() const override;

  /**
   * @brief Get the SocketComponent associated with this Rig Component, based on the Tracked Visual
   *
   * Overridden from UIsdkTrackedDataSourceRigComponent.
   *
   * @param OutSocketComponent Returned reference of the Socket Component as a USceneComponent
   * @param OutSocketName Returned reference of the socket FName
   * @param HandBone Bone name used to find the appropriate socket
   */
  virtual void GetInteractorSocket(
      USceneComponent*& OutSocketComponent,
      FName& OutSocketName,
      EIsdkHandBones HandBone) const override;

  // A synthetic hand, which follows the tracked hand, but for which the positioning and posing may
  // be altered by interaction
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkSyntheticHand* SyntheticHand;

  // Visuals corresponding directly to the user's hand
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  UIsdkHandMeshComponent* TrackedHandVisual;

  // Visuals corresponding to the user's hand, for which the positioning and posing may be altered
  // by interaction
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  UIsdkHandMeshComponent* SyntheticHandVisual;

 protected:
  virtual void OnDataSourcesCreated() override;

 private:
  virtual FTransform GetCurrentSyntheticTransform() override;
};

/**
 * @class UIsdkHandVisualsRigComponentLeft
 * @brief Rig Component Visualizing and Tracking Hands for Left Handedness
 *
 * @see UIsdkHandVisualsRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(ClassGroup = InteractionSDK)
class OCULUSINTERACTIONPREBUILTS_API UIsdkHandVisualsRigComponentLeft
    : public UIsdkHandVisualsRigComponent
{
  GENERATED_BODY()

  UIsdkHandVisualsRigComponentLeft();
};

/**
 * @class UIsdkHandVisualsRigComponentRight
 * @brief Rig Component Visualizing and Tracking Hands for Right Handedness
 *
 * @see UIsdkHandVisualsRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(ClassGroup = InteractionSDK)
class OCULUSINTERACTIONPREBUILTS_API UIsdkHandVisualsRigComponentRight
    : public UIsdkHandVisualsRigComponent
{
  GENERATED_BODY()

  UIsdkHandVisualsRigComponentRight();
};
