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
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "Components/ActorComponent.h"
#include "Core/IsdkConditionalGroupAll.h"
#include "IsdkTrackedDataSourceRigComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FIsdkRigDataSourceEvent);

UENUM()
enum class EIsdkRigHandVisibility : uint8
{
  // Do not automatically adjust visibility of the hand visuals, even if the device is disabled.
  // The synthetic hand visual will by default be visible, and tracked hand mesh invisible.
  Manual,

  // Hides all visuals when tracking source is disconnected. Only show Tracked hand, when connected.
  TrackedOnly,

  // Hides all visuals when tracking source is disconnected. Only show Synthetic hand, when
  // connected.
  SyntheticOnly,

  // When the synthetic hand has different data than the tracked hand, make that visible. Otherwise,
  // make the tracked hand mesh visible.
  SyntheticWhenRootChanged,
};

/**
 * @class UIsdkTrackedDataSourceRigComponent
 * @brief SceneComponent representing a base class for Rig Components utilizing tracked data.
 *
 * Rig Component that holds references to synthetic and tracked visuals driven by a set data source.
 * Provides methods to setup and modify these visuals as well as modify their visibility state
 *
 * @see UIsdkControllerVisualsRigComponent
 * @see UIsdkHandVisualsRigComponent
 * @addtogroup InteractionSDKPrebuiltsPrebuilts
 */
UCLASS(ClassGroup = (InteractionSDK), Abstract)
class OCULUSINTERACTIONPREBUILTS_API UIsdkTrackedDataSourceRigComponent : public USceneComponent
{
  GENERATED_BODY()

 public:
  // Sets default values for this component's properties
  UIsdkTrackedDataSourceRigComponent();

  /**
   * @brief Called on BeginPlay, overridden from USceneComponent.
   */
  virtual void BeginPlay() override;

  /**
   * @brief Called on EndPlay, overridden from USceneComponent.
   */
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  /**
   * @brief Returns reference to the TrackingDataSources struct used in this Rig Component
   * @return FIsdkTrackingDataSources Struct containing the data source, root/pointer poses and hand
   * joint information
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  const FIsdkTrackingDataSources& GetDataSources() const;

  /**
   * @brief Attempts to attach this to a given motion controller, setting it as parent
   * @param Target Motion Controller desired to be parented to
   * @return bool If the attachment attempt was successful
   */
  bool TryAttachToParentMotionController(USceneComponent* Target);

  /**
   * @brief Sets up the Data Sources on this Rig Component to be a tracked hand. Calls
   * OnDataSourcesCreated() afterward.
   */
  void CreateDataSourcesAsTrackedHand();

  /**
   * @brief Sets up the Data Sources on this Rig Component to be a tracked controller. Calls
   * OnDataSourcesCreated() afterward.
   */
  void CreateDataSourcesAsTrackedController();

  /**
   * @brief Calculates the visibility states of both the tracked and synthetic visuals, and then
   * sets that visibility on those components.
   */
  void UpdateComponentVisibility();

  /**
   * @brief Get the SocketComponent associated with this Rig Component, based on the Tracked Visual
   * @param OutSocketComponent Returned reference of the Socket Component as a USceneComponent
   * @param OutSocketName Returned reference of the socket FName
   * @param HandBone Bone name used to find the appropriate socket
   */
  virtual void GetInteractorSocket(
      USceneComponent*& OutSocketComponent,
      FName& OutSocketName,
      EIsdkHandBones HandBone = EIsdkHandBones::Invalid) const
  {
  }

  /**
   * @brief Optionally update the visibility of the hand visuals each frame based on the
   * state of the synthetic hand.
   */
  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      meta = (ExposeOnSpawn = true),
      Category = InteractionSDK)
  EIsdkRigHandVisibility HandVisibilityMode = EIsdkRigHandVisibility::SyntheticOnly;

  /**
   * Which hand this rig represents. This field is used to determine the handedness of the
   * default hand meshes.
   */
  UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = InteractionSDK)
  EIsdkHandedness Handedness;

  /**
   * Optional: Use this given subsystem instead of the default one that exists on the world.
   * The subsystem is used to create components that are able to read hand tracking data.
   */
  UPROPERTY()
  TScriptInterface<IIsdkITrackingDataSubsystem> CustomTrackingData;

  /**
   * Motion controller this Rig Component is currently attached to
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UMotionControllerComponent* AttachedToMotionController;

  /**
   * @brief Get the Tracked Visual SceneComponent (driven by data source) associated with this Rig
   * Component
   * @return USceneComponent Scene Component associated as a Tracked Visual
   */
  virtual USceneComponent* GetTrackedVisual() const
      PURE_VIRTUAL(UIsdkTrackedDataSourceRigComponent::GetTrackedVisual, return nullptr;);
  /**
   * @brief Get the Synthetic Visual SceneComponent (modified during runtime) associated with this
   * Rig Component
   * @return USceneComponent Scene Component associated as a Synthetic Visual
   */
  virtual USceneComponent* GetSyntheticVisual() const
      PURE_VIRTUAL(UIsdkTrackedDataSourceRigComponent::GetSyntheticVisual, return nullptr;);

  /**
   * @brief Gets a pointer to the All Conditional Group for disabling visibility, which is
   * ultimately set to true only when all of its constituent conditionals are true.
   * @see UIsdkConditionalGroupAll
   * @return UIsdkConditionalGroupAll Scene Component associated as a Synthetic Visual
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkConditionalGroupAll* GetForceOffVisibility() const;

  /**
   * Returns which tracking data subsystem is being used by this Rig Component.
   * @return TScriptInterface<IIsdkITrackingDataSubsystem> Entity implementing the
   * IIsdkITrackingDataSubsystem
   */
  TScriptInterface<IIsdkITrackingDataSubsystem> GetTrackingDataSubsystem();

  /**
   * Multicast delegate broadcast when the data sources are created
   */
  FIsdkRigDataSourceEvent DataSourcesCreated;

 protected:
  /**
   * @brief Getter for the current transform of the Synthetic Visual associated with this Rig
   * Component
   * @return FTransform Transform of the synthetic visual
   */
  virtual FTransform GetCurrentSyntheticTransform()
  {
    return {};
  }

  /**
   * @brief Called when Data Sources have been created and initialized on this Rig Component
   */
  virtual void OnDataSourcesCreated()
  {
    DataSourcesCreated.Broadcast();
  }

  /**
   * @brief Called when the visibility of the synthetic and tracked visuals are updated, along with
   * their current visibility state
   */
  virtual void OnVisibilityUpdated(bool bTrackedVisibility, bool bSyntheticVisibility) {}

  /**
   * @brief Called after all ISDK operations have been completed
   */
  UFUNCTION()
  void HandleIsdkFrameFinished();

  /**
   * Data Sources used by tracked hands/controllers
   */
  FIsdkTrackingDataSources LocalDataSources;

  /**
   * The Conditional Group representing if this rig component should be forced invisible, all
   * constituent conditionals must be true for this to be true.
   */
  UPROPERTY()
  UIsdkConditionalGroupAll* ForceOffVisibility;
};
