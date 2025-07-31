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
#include "Components/SceneComponent.h"
#include "IsdkHandPoseData.h"
#include "IsdkHandGrabPose.generated.h"

class UIsdkHandMeshComponent;

/* SceneComponent indicating where and with what pose a hand grab could snap to */
UCLASS(
    Blueprintable,
    Experimental,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent))
class OCULUSINTERACTION_API UIsdkHandGrabPose : public USceneComponent
{
  GENERATED_BODY()

 public:
  // Sets default values for this component's properties
  UIsdkHandGrabPose();

  /* Toggles ability to edit this hand grab pose in the view port. It is recommended to only have
   * one hand grab pose set to be edited at a time.*/
  UFUNCTION(CallInEditor, Category = "InteractionSDK")
  void ToggleHandPoseEditing()
  {
    bHandPoseEditingEnabled = !bHandPoseEditingEnabled;
  }

  UPROPERTY(SkipSerialization)
  bool bHandPoseEditingEnabled = false;

  /* Primary properties that dictate the behavior of the Hand Grab Pose */
  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "InteractionSDK",
      meta = (ShowOnlyInnerProperties))
  FIsdkHandGrabPoseProperties HandGrabPoseProperties;

  /* Whether or not to show the visualized hand pose data in Editor */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK")
  bool bShowDebugVisualInEditor = true;

  /* Whether or not to destroy this visualized hand pose when BeginPlay happens */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK")
  bool bRemoveDebugVisualOnPlay = true;

  /* Disable this Pose from Being Selected*/
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK")
  bool bPoseDisabled = false;

  /* Hand Pose Data asset to use for this Grab Pose */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK")
  UIsdkHandPoseData* HandPoseData;

  /* Generates a new debug hand visual (UIsdkHandMeshComponent), utilizing the set hand grab data.
   * Destroys the previous visual if it exists */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool InitializeDebugVisual();

  /* Destroys the generated debug visual, if instantiated */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void DestroyDebugVisual();

  /* Returns the hand pose data automatically generated as a mirror for this hand grab pose */
  UFUNCTION(BlueprintCallable, Category = "InteractionISDK")
  UIsdkHandPoseData* GetMirrorHandPoseData() const
  {
    return MirrorHandPoseData;
  }

  /* Returns the hand grab pose automatically generated as a mirror for this hand grab pose*/
  UFUNCTION(BlueprintCallable, Category = "InteractionISDK")
  UIsdkHandGrabPose* GetMirrorHandGrabPose() const
  {
    return ManualMirroredGrabPose;
  }
  /* Returns true if the Debug Visual HandMeshComponent is spawned and valid*/
  UFUNCTION(BlueprintCallable, Category = "InteractionISDK")
  bool IsDebugVisualActive()
  {
    return IsValid((UObject*)DebugHandVisual);
  }

  /* Returns reference to Debug Hand Visual */
  UFUNCTION(BlueprintCallable, Category = "InteractionISDK")
  UIsdkHandMeshComponent* GetDebugVisual()
  {
    return DebugHandVisual;
  }

 protected:
  virtual void OnComponentCreated() override;
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

 private:
  UPROPERTY()
  UIsdkHandMeshComponent* DebugHandVisual;

  UPROPERTY()
  EIsdkHandedness Handedness = EIsdkHandedness::Left;

  bool bVisualMirrored = false;

  UPROPERTY()
  UIsdkHandPoseData* MirrorHandPoseData;
  UPROPERTY()
  UIsdkHandGrabPose* ManualMirroredGrabPose;
};
