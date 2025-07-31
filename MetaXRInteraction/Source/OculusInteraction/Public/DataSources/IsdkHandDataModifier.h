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
#include "IsdkHandDataSource.h"
#include "IsdkIRootPose.h"

#include "IsdkHandDataModifier.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class IHandDataModifier;
	}
}
typedef struct isdk_IHandDataSource_ isdk_IHandDataSource;

/**
 * @class UIsdkHandDataModifier
 * @brief Abstract class representing a Hand Data Source intended to take and modify an existing
 * HandData Source, potentially chained to other HandDataModifiers or HandData Sources. Implements
 * IsdkIRootPose
 *
 * @see UIsdkHandDataSource
 * @see IIsdkIRootPose
 * @addtogroup InteractionSDK
 */
UCLASS(Abstract)
class OCULUSINTERACTION_API UIsdkHandDataModifier : public UIsdkHandDataSource,
                                                    public IIsdkIRootPose
{
  GENERATED_BODY()

 public:
  UIsdkHandDataModifier();

  /**
   * @brief Called on BeginDestroy, overridden from UActorComponent. Sets all joints to identity in
   * the hand data member variable.
   */
  virtual void BeginPlay() override;

  /**
   * @brief Updates the hand tracking data every frame, if allowed to update in tick, reads joint
   * data from API through Hand Data
   * @param DeltaTime Time since the last frame.
   * @param TickType Type of update this tick represents.
   * @param ThisTickFunction Tick function details provided by Unreal Engine.
   * This method is automatically managed by the engine, ensuring that the latest hand data is
   * always available.
   */
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  /**
   * @brief Get the Hand Data Source this Modifier should be taking in as input
   * @return UIsdkHandDataSource* The Hand Data Source this Modifier is currently taking as input
   */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  UIsdkHandDataSource* GetInputDataSource() const
  {
    return InputDataSource;
  }

  /**
   * @brief Sets the hand data source this modifier should use as input
   * @param InInputDataSource The hand data source (which could be a HandDataModifier) this modifier
   * should use as input
   */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetInputDataSource(UIsdkHandDataSource* InInputDataSource);

  /**
   * @brief Return whether or not this hand data modifier will recursively update its data sources
   * when it itself is updated
   * @return bool Whether or not this modifier recursively updates
   */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  bool GetRecursiveUpdate() const
  {
    return bRecursiveUpdate;
  }

  /**
   * @brief Set whether or not this hand data modifier will recursively update its data sources
   * @param bInRecursiveUpdate Whether or not this modifier should recursively update
   */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetRecursiveUpdate(bool bInRecursiveUpdate);

  virtual isdk::api::IHandDataModifier* GetApiIHandDataModifier()
      PURE_VIRTUAL(UIsdkHandDataSource::GetApiIHandDataModifier, return nullptr;);

  // From IIsdkRootPose
  /**
   * @brief Retrieves the Root Pose from the Hand Data as a transform. Implemented from
   * IIsdkRootPose. This returns the ModifiedRootPose member variable.
   * @return FTransform The current root pose from the hand data associated with this modifier
   */
  virtual FTransform GetRootPose_Implementation() override
  {
    return ModifiedRootPose;
  }

  /**
   * @brief Returns whether or not the Root Pose information from the Input Data source is valid.
   * Implemented from IIsdkRootPose.
   * @return bool The state of validity of the root pose information retrieved from the
   * InputDataSource. Checked via UObject::IsValid()
   */
  virtual bool IsRootPoseValid_Implementation() override;

  /**
   * @brief Retrieves the boolean Conditional state representing if the device this modifier is
   * representing is enabled. Implemented from IIsdkRootPose.
   * @return UIsdkConditional* Returns conditional of the state of whether or not the device
   * associated with this root pose is enabled in the system, and enumerated in the list of
   * connected devices.
   */
  virtual UIsdkConditional* GetRootPoseConnectedConditional_Implementation() override;

  /**
   * @brief Returns conditional of the state of if the device associated with this root pose is
   * connected, the root pose is valid and was set from tracking data is marked as high confidence.
   * If the conditional is true, then it implies that IsRootPoseConnected and IsRootPoseValid are
   * also true, so they don't need to be checked in addition to this. Implemented from
   * IIsdkRootPose.
   * @return UIsdkConditional* Conditional is false if IsRootPoseConnected or IsRootPoseValid is
   * false.
   */
  virtual UIsdkConditional* GetRootPoseHighConfidenceConditional_Implementation() override;

  /**
   * @brief Retrieves the handedness of the implementing object. Implemented from IIsdkIHandJoints.
   * @return EIsdkHandedness Handedness of this class (normally left or right)
   */
  virtual EIsdkHandedness GetHandedness_Implementation() override;

  /**
   * @brief Returns whether or not the hand joint data for this object is valid, validated by
   * UObject::IsValid(). Implemented from IIsdkIHandJoints.
   * @return bool Whether or not the hand joint data object is valid for use
   */
  virtual bool IsHandJointDataValid_Implementation() override;

  /**
   * @brief Returns an object containing the finger and thumb joint mappings for this object
   * Implemented from IIsdkIHandJoints
   * @return UIsdkHandJointMappings* Mappings object, containing finger joint mappings
   * (FIsdkExternalHandPositionFrame_FingerJointMapping) and thumb joint mappings
   * (FIsdkExternalHandPositionFrame_ThumbJointMapping)
   */
  virtual const UIsdkHandJointMappings* GetHandJointMappings_Implementation() override;

 protected:
  /**
   * @brief Virtual function to determine whether or not this Hand Data Source should be updated.
   * Base class sets this to be equivalent to if it should tick. Overridden from
   * UIsdkHandDataSource.
   * @return bool Whether or not this data source should be continually updated
   */
  virtual bool ShouldUpdateDataSource() override;
  bool TryGetApiFromDataSource(isdk_IHandDataSource*& OutFromDataSource) const;

  /**
   * @brief The Hand Data Source used as input for this modifier (which could also mean it might be
   * an HandDataSource, creating a chain)
   */
  UPROPERTY(
      BlueprintGetter = GetInputDataSource,
      BlueprintSetter = SetInputDataSource,
      EditAnywhere,
      Category = InteractionSDK)
  UIsdkHandDataSource* InputDataSource = nullptr;

  /**
   * @brief Whether or not this hand data modifier will recursively update its data sources when it
   * gets updated.
   */
  UPROPERTY(
      BlueprintGetter = GetRecursiveUpdate,
      BlueprintSetter = SetRecursiveUpdate,
      EditAnywhere,
      Category = InteractionSDK)
  bool bRecursiveUpdate = false;

  /**
   * @brief The modified root pose retrieved and then set through the API referencing the hand data
   * source
   */
  FTransform ModifiedRootPose;
};
