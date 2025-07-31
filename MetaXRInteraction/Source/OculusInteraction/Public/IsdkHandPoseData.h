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
#include "Engine/DataAsset.h"
#include "Animation/PoseAsset.h"
#include "Animation/Skeleton.h"
#include "OculusInteractionLog.h"
#include "IsdkHandData.h"
#include "Engine/DataTable.h"
#include "DataSources/IsdkIHandJoints.h"
#include "IsdkHandPoseData.generated.h"

class UIsdkHandMeshComponent;

/* Mode of freedom for a given joint
 * Free - No restrictions
 * Constrained - Some constraints may restrict movement
 * Locked - Joint receives no movement
 */
UENUM(BlueprintType)
enum class EIsdkJointFreedom : uint8
{
  Free,
  Constrained,
  Locked
};

UCLASS(Blueprintable, ClassGroup = (InteractionSDK), meta = (DisplayName = "ISDK Hand Pose Data"))
class OCULUSINTERACTION_API UIsdkHandPoseData : public UPrimaryDataAsset, public IIsdkIHandJoints
{
  GENERATED_BODY()
 public:
  UIsdkHandPoseData();

  /* The assigned handedness for this pose data, will determine which hand skeleton is utilized */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK")
  EIsdkHandedness Handedness;
  /* Joint freedom mode for the thumb */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InteractionSDK|Joint Freedom")
  EIsdkJointFreedom Thumb;
  /* Joint freedom mode for the index finger */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InteractionSDK|Joint Freedom")
  EIsdkJointFreedom Index;
  /* Joint freedom mode for the middle finger */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InteractionSDK|Joint Freedom")
  EIsdkJointFreedom Middle;
  /* Joint freedom mode for the ring finger */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InteractionSDK|Joint Freedom")
  EIsdkJointFreedom Ring;
  /* Joint freedom mode for the pinky finger */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InteractionSDK|Joint Freedom")
  EIsdkJointFreedom Pinky;

  /* Hand Data object, storing joint & bone information */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  UIsdkHandData* HandData;

  /* Thumb and Finger Joint Mappings */
  UPROPERTY(Transient)
  UIsdkHandJointMappings* HandJointMapping;

  virtual UIsdkHandData* GetHandData_Implementation() override
  {
    return HandData;
  }
  virtual bool IsHandJointDataValid_Implementation() override
  {
    return HandData != nullptr;
  }
  virtual EIsdkHandedness GetHandedness_Implementation() override
  {
    return Handedness;
  }
  virtual const UIsdkHandJointMappings* GetHandJointMappings_Implementation() override
  {
    return HandJointMapping;
  }

  /* Array of Joint Names */
  UPROPERTY(VisibleAnywhere, Category = InteractionSDK)
  FName JointNames[static_cast<size_t>(EIsdkHandBones::EHandBones_MAX)];

  static const size_t GetJointCount()
  {
    return static_cast<size_t>(EIsdkHandBones::EHandBones_MAX);
  }

  /* Get the quaternion at a given joint index*/
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  FQuat GetJointRotation(EIsdkHandBones JointIndex) const
  {
    return HandData->GetJointPoses()[uint8(JointIndex)].GetRotation();
  }

  /* Set the quaternion of a given joint index*/
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetJointRotation(EIsdkHandBones JointIndex, FQuat Rotation)
  {
    HandData->GetJointPoses()[uint8(JointIndex)].SetRotation(Rotation);
  }

  /* Mark this asset as dirty and required to be saved */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetDirty()
  {
    Modify();
  }
  /* Sets the Hand Pose (all joints) from a given skeleton */
  UFUNCTION(CallInEditor, BlueprintCallable, Category = InteractionSDK)
  void SetRotationFromSkeleton(USkeletalMesh* SkinnedAsset);

  /* Sets the Hand Pose (all joints) from a given Hand Visual Component */
  UFUNCTION(CallInEditor, BlueprintCallable, Category = InteractionSDK)
  void SetRotationFromVisual(UIsdkHandMeshComponent* HandMesh);

  /* Sets the Hand Pose (all joints) from a given named Pose Asset */
  UFUNCTION(CallInEditor, BlueprintCallable, Category = InteractionSDK)
  void SetRotationFromPoseWithName(UPoseAsset* Pose, FName Name);

  /* Returns the pose lerp time */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  float GetPoseLerpTime() const
  {
    return PoseLerpTime;
  }

  /* Sets the pose lerp time */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetPoseLerpTime(const float LerpTimeIn)
  {
    PoseLerpTime = LerpTimeIn;
  }

  // If this HandPoseData was generated as a mirror of an original one
  bool bIsMirrored = false;

 protected:
  /* If positive, will lerp into/out of hand pose with the given timing */
  UPROPERTY(
      BlueprintGetter = GetPoseLerpTime,
      BlueprintSetter = SetPoseLerpTime,
      EditAnywhere,
      BlueprintReadWrite,
      Category = "InteractionSDK")
  float PoseLerpTime = 0.f;
};
