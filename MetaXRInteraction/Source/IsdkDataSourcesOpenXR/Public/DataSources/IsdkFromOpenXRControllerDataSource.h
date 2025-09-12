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
#include "MotionControllerComponent.h"
#include "Components/ActorComponent.h"
#include "Core/IsdkConditionalBool.h"
#include "DataSources/IsdkExternalHandDataSource.h"
#include "DataSources/IsdkIHandPointerPose.h"
#include "DataSources/IsdkIRootPose.h"
#include "IsdkFromOpenXRControllerDataSource.generated.h"

namespace isdk::api
{
class ExternalHandSource;
}

/**
 * @class UIsdkFromOpenXRControllerDataSource
 * @brief Manages OpenXR controller data, integrating OpenXR pointer and root poses into your VR
 * project.
 *
 * This class facilitates the integration of OpenXR hand tracking data by providing access to
 * essential information such as the controller's pointer and root poses. It ensures that this data
 * is updated each frame, maintaining its relevance throughout the application's lifecycle. An
 * instance of this data source is created in the Interaction SDK's prebuilt rig component via
 * UIsdkTrackedDataSourceRigComponent.
 *
 * @see UIsdkExternalHandDataSource
 * @see IIsdkIHandPointerPose
 * @see IIsdkIRootPose
 * @addtogroup InteractionSDK
 */
UCLASS(ClassGroup = (InteractionSDK), meta = (BlueprintSpawnableComponent))
class ISDKDATASOURCESOPENXR_API UIsdkFromOpenXRControllerDataSource
    : public UIsdkExternalHandDataSource,
      public IIsdkIHandPointerPose,
      public IIsdkIRootPose
{
  GENERATED_BODY()

 public:
  /**
   * @brief Constructor that sets default values for this component's properties.
   *
   * This constructor initializes the component with default settings, preparing it to manage and
   * process controller data from OpenXR.
   */
  UIsdkFromOpenXRControllerDataSource();

  static UIsdkFromOpenXRControllerDataSource* MakeOpenXrControllerDataSource(
      AActor* Actor,
      UMotionControllerComponent* SourceMotionController,
      EIsdkHandedness InHandedness);

  /**
   * @brief Updates the controller data every frame.
   * @param DeltaTime Time since the last frame.
   * @param TickType Type of tick for this frame.
   * @param ThisTickFunction Tick function information provided by Unreal Engine.
   *
   * This method ensures that the controller's position, orientation, and related data are updated
   * consistently every frame. It’s automatically handled by the engine.
   */
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  /**
   * @brief Retrieves the motion controller component associated with this data source.
   * @return UMotionControllerComponent* Pointer to the motion controller component.
   *
   * This method provides access to the motion controller component used by this data source,
   * allowing other systems to interact with or modify the controller if needed.
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UMotionControllerComponent* GetMotionController() const;

  /**
   * @brief Checks if the pointer pose is valid.
   * @return bool True if the pointer pose is valid, false otherwise.
   *
   * This method is used internally to validate the pointer pose data. It ensures that
   * the pointer pose can be trusted for accurate ray interactions.
   * @see UIsdkRayInteractor.
   */
  virtual bool IsPointerPoseValid_Implementation() override;

  /**
   * @brief Retrieves the current pointer pose.
   * @param PointerPose Out parameter to store the current pointer pose.
   * @param IsValid Out parameter indicating if the pointer pose is valid.
   *
   * This is where the system handles the pointer pose used for ray interactions. It’s part of the
   * internal data processing and doesn’t require manual intervention.
   * @see UIsdkRayInteractor.
   */
  virtual void GetPointerPose_Implementation(FTransform& PointerPose, bool& IsValid) override;

  /**
   * @brief Retrieves the pointer pose relative to the tracking space.
   * @param PointerRelativePose Out parameter for the relative pointer pose.
   * @param IsValid Out parameter indicating if the relative pointer pose is valid.
   *
   * This method provides the pointer pose within the context of the overall tracking space, which
   * is useful for understanding the controller’s position relative to the tracked environment.
   */
  virtual void GetRelativePointerPose_Implementation(FTransform& PointerRelativePose, bool& IsValid)
      override;

  /**
   * @brief Retrieves the current root pose.
   * @return FTransform The transform representing the controller's root position.
   *
   * The root pose is the base transform from which other poses, like the pointer pose, are derived.
   * This method returns the current root pose, which can be used for various interaction and
   * alignment tasks.
   */
  virtual FTransform GetRootPose_Implementation() override;

  /**
   * @brief Checks if the root pose data is valid.
   * @return bool True if the root pose data is valid, false otherwise.
   *
   * This method is used to verify that the root pose data is reliable and can be used in
   *  interactions. For example, this is used to check if the poke interactor should be enabled.
   * @see UIsdkPokeInteractor
   */
  virtual bool IsRootPoseValid_Implementation() override;

  /**
   * @brief Retrieves the status of the root pose connection.
   * @return UIsdkConditional* A pointer to the `UIsdkConditional` object that represents the
   * connection status of the root pose.
   *
   * This method checks if the root pose connection is currently active and valid. The returned
   * `UIsdkConditional` object encapsulates this status and can trigger events when the connection
   * status changes. For example, this is used to check if the controller should be visible and if
   * the interaction rig components should be enabled/disabled.
   * @see UIsdkTrackedDataSourceRigComponent
   * @see UIsdkPokeInteractionRigComponent
   * @see UIsdkGrabInteractionRigComponent
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  virtual UIsdkConditional* GetRootPoseConnectedConditional_Implementation() override;

 private:
  TArray<FVector> OutPositions;
  TArray<FQuat> OutRotations;
  TArray<float> OutRadii;

  UPROPERTY(
      BlueprintGetter = GetMotionController,
      Category = InteractionSDK,
      meta = (ExposeOnSpawn = true))
  TObjectPtr<UMotionControllerComponent> MotionController;

  void ReadControllerData();

#if !UE_BUILD_SHIPPING
  void DebugLog();
#endif

  FTransform LastGoodRootPose = FTransform::Identity;

  // Indicates whether the root pose was read from input (true), or if it is garbage (false)
  bool bIsLastGoodRootPoseValid = false;

  // Indicates whether the pointer pose was read from input (true), or if it is garbage (false)
  bool bIsLastGoodPointerPoseValid = false;

  UPROPERTY()
  TObjectPtr<UIsdkConditionalBool> IsRootPoseConnected;

  UPROPERTY()
  TObjectPtr<UIsdkConditionalBool> IsRootPoseHighConfidence;

  bool IsHandTrackingEnabled();
  bool GetGripPose(FTransform& OutGripPose);
};
