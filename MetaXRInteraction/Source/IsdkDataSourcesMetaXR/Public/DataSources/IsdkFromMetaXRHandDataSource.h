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
#include "Core/IsdkConditionalBool.h"
#include "DataSources/IsdkExternalHandDataSource.h"
#include "DataSources/IsdkIHandPointerPose.h"
#include "DataSources/IsdkIRootPose.h"
#include "Components/PoseableMeshComponent.h"

#include "IsdkFromMetaXRHandDataSource.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class ExternalHandSource;
	}
}

USTRUCT()
struct FBoneOVRToOXRMap
{
  GENERATED_BODY()

  FName OVRBoneName;
  FVector OXRWristOffset;
  float OXRRotation;
  int OXRBoneIndex;

  static FBoneOVRToOXRMap MappedIndex(int Index, FName BoneName)
  {
    FBoneOVRToOXRMap Mapped;
    Mapped.OVRBoneName = BoneName;
    Mapped.OXRBoneIndex = Index;
    Mapped.OXRWristOffset = FVector::Zero();
    Mapped.OXRRotation = 0.0;
    return Mapped;
  }

  static FBoneOVRToOXRMap MappedOffset(int Index, FVector Offset, float Rotation)
  {
    FBoneOVRToOXRMap Mapped;
    Mapped.OVRBoneName = FName();
    Mapped.OXRBoneIndex = Index;
    Mapped.OXRWristOffset = Offset;
    Mapped.OXRRotation = Rotation;
    return Mapped;
  }
};

/**
 * @class UIsdkFromMetaXRHandDataSource
 * @brief Manages MetaXR plugin hand tracking data, integrating pointer and root poses into your VR
 * project.
 *
 * This class facilitates the integration of MetaXR plugin hand tracking data by providing access to
 * essential information such as the hand's pointer and root poses. It ensures that this data is
 * updated each frame, maintaining its relevance throughout the application's lifecycle. An instance
 * of this data source is created in the Interaction SDK's prebuilt rig component via
 * UIsdkTrackedDataSourceRigComponent.
 *
 * @see UIsdkExternalHandDataSource
 * @see IIsdkIHandPointerPose
 * @see IIsdkIRootPose
 * @addtogroup InteractionSDK
 */
UCLASS(
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK From MetaXR Hand Data Source"))
class ISDKDATASOURCESMETAXR_API UIsdkFromMetaXRHandDataSource : public UIsdkExternalHandDataSource,
                                                                public IIsdkIHandPointerPose,
                                                                public IIsdkIRootPose
{
  GENERATED_BODY()

 public:
  /**
   * @brief Constructor that sets default values for this component's properties.
   *
   * This constructor initializes the component with default settings, preparing it to manage and
   * process hand data from MetaXR plugin.
   */
  UIsdkFromMetaXRHandDataSource();

  static UIsdkFromMetaXRHandDataSource* MakeMetaXRHandDataSource(
      AActor* Actor,
      UMotionControllerComponent* SourceMotionController,
      EIsdkHandedness InHandedness);

  /**
   * @brief Initializes the component to start receiving hand tracking data.
   *
   * This method prepares the component to process hand tracking data immediately upon
   * initialization. It is automatically called during component setup and should not require manual
   * invocation.
   */
  virtual void InitializeComponent() override;

  /**
   * @brief Updates the hand tracking data every frame.
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
   * @brief Checks if the pointer pose data is valid.
   * @return bool True if the data is valid, false otherwise.
   *
   * This method is used internally to validate the pointer pose data. It ensures that
   * the pointer pose can be trusted for accurate ray interactions.
   * @see UIsdkRayInteractor.
   */
  virtual bool IsPointerPoseValid_Implementation() override;

  /**
   * @brief Checks if the root pose data is valid.
   * @return bool True if the data is valid, false otherwise.
   *
   * This is part of the internal validation process to ensure that the root pose is accurate before
   * it's used. For example, this is used to check if the poke interactor should be enabled.
   * @see UIsdkPokeInteractor
   */
  virtual bool IsRootPoseValid_Implementation() override;

  /**
   * @brief Retrieves the current root pose.
   * @return FTransform The transform representing the hand's base position.
   *
   * The root pose is managed internally and serves as the basis for all hand tracking data. You'll
   * likely only need to interact with this if you're doing something custom with the tracking data.
   */
  virtual FTransform GetRootPose_Implementation() override;

  /**
   * @brief Retrieves the current pointer pose.
   * @param PointerPose Out parameter to store the current pointer pose.
   * @param IsValid Out parameter indicating if the pointer pose is valid.
   *
   * This is where the system handles the pointer pose used for ray interactions. It's part of the
   * internal data processing and doesn't require manual intervention.
   * @see UIsdkRayInteractor.
   */
  virtual void GetPointerPose_Implementation(FTransform& PointerPose, bool& IsValid) override;

  /**
   * @brief Gets the pointer pose relative to the tracking space.
   * @param PointerRelativePose Out parameter for the relative pointer pose.
   * @param IsValid Out parameter indicating the validity of the relative pointer pose.
   *
   * This is used internally to provide context-sensitive pointer data based on the overall tracking
   * space.
   */
  virtual void GetRelativePointerPose_Implementation(FTransform& PointerRelativePose, bool& IsValid)
      override;

  /**
   * @brief Accesses the motion controller component used by this hand data source.
   * @return UMotionControllerComponent* Pointer to the motion controller component.
   *
   * This provides access to the motion controller tied to the hand data source, mainly for internal
   * use. You might use this if you're doing advanced modifications or debugging.
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UMotionControllerComponent* GetMotionController() const;

  /**
   * @brief Checks if low confidence hand data is allowed.
   * @return bool True if low confidence data is processed, false otherwise.
   *
   * The system defaults to high-confidence data for reliability. This setting is
   * configured internally but can be adjusted if needed for specific scenarios.
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  bool GetAllowInvalidTrackedData() const;

  /**
   * @brief Retrieves the status of the root pose connection.
   * @return UIsdkConditional* A pointer to the `UIsdkConditional` object that represents the
   * connection status of the root pose.
   *
   * This method checks if the root pose connection is currently active and valid. The returned
   * `UIsdkConditional` object encapsulates this status and can trigger events when the connection
   * status changes. For example, this is used to check if the hand should be visible and if the
   * interaction rig components should be enabled/disabled.
   * @see UIsdkTrackedDataSourceRigComponent
   * @see UIsdkPokeInteractionRigComponent
   * @see UIsdkGrabInteractionRigComponent
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  virtual UIsdkConditional* GetRootPoseConnectedConditional_Implementation() override;

  /**
   * @brief Gets the status of the root pose high confidence
   * @return UIsdkConditional* A pointer to the `UIsdkConditional` object that represents the
   * state of high confidence in the root pose.
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  virtual UIsdkConditional* GetRootPoseHighConfidenceConditional_Implementation() override;

  /**
   * @brief Sets whether to allow low confidence data.
   * @param bInAllowInvalidTrackedData True to process low confidence data, false otherwise.
   *
   * Adjusting this allows you to fine-tune how the system handles uncertain tracking data, trading
   * off between precision and responsiveness.
   */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetAllowInvalidTrackedData(bool bInAllowInvalidTrackedData);

 private:
  void ReadHandData();

  FTransform RelativePointerPose = FTransform::Identity;

  bool bIsLastGoodRootPoseValid = false;
  bool bIsLastGoodPointerPoseValid = false;

  UPROPERTY(
      BlueprintGetter = GetAllowInvalidTrackedData,
      BlueprintSetter = SetAllowInvalidTrackedData,
      Category = InteractionSDK)
  bool bAllowLowConfidenceData = false;
  bool bHasLastKnownGood = false;

  UPROPERTY(
      BlueprintGetter = GetMotionController,
      Category = InteractionSDK,
      meta = (ExposeOnSpawn = true))
  UMotionControllerComponent* MotionController;

  FTransform LastGoodRootPose = FTransform::Identity;

  UPROPERTY()
  UIsdkConditionalBool* IsRootPoseConnected;

  UPROPERTY()
  UIsdkConditionalBool* IsRootPoseHighConfidence;

  // Required to generate the "OpenXR" Data
  UPROPERTY()
  UPoseableMeshComponent* OculusXrHandComponent;

  static void SetUintEnumProperty(UObject* Target, FName InName, uint8 EnumValue);
  void GenerateBoneMap();

  TArray<FBoneOVRToOXRMap> BoneMap;
  TArray<float> DefaultJointRadii;

#if !UE_BUILD_SHIPPING
  // Log the state of all relevant data source state and interfaces.
  void DebugLog();
#endif
};
