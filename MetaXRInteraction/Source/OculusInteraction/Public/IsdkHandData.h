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
#include "StructTypes.h"
#include "IsdkHandData.generated.h"

UENUM(BlueprintType)
enum class EIsdkHandBones : uint8
{
  HandStart = 0,
  HandPalm = HandStart + 0 UMETA(DisplayName = "Palm"),
  HandWristRoot = HandStart + 1 UMETA(DisplayName = "Wrist"),

  HandThumb1 = HandStart + 2 UMETA(DisplayName = "Thumb Metacarpal"),
  HandThumb2 = HandStart + 3 UMETA(DisplayName = "Thumb Proximal"),
  HandThumb3 = HandStart + 4 UMETA(DisplayName = "Thumb Distal"),
  HandThumbTip = HandStart + 5 UMETA(DisplayName = "Thumb Tip"),

  HandIndex0 = HandStart + 6 UMETA(DisplayName = "Index Metacarpal"),
  HandIndex1 = HandStart + 7 UMETA(DisplayName = "Index Proximal"),
  HandIndex2 = HandStart + 8 UMETA(DisplayName = "Index Intermediate"),
  HandIndex3 = HandStart + 9 UMETA(DisplayName = "Index Distal"),
  HandIndexTip = HandStart + 10 UMETA(DisplayName = "Index Tip"),

  HandMiddle0 = HandStart + 11 UMETA(DisplayName = "Middle Metacarpal"),
  HandMiddle1 = HandStart + 12 UMETA(DisplayName = "Middle Proximal"),
  HandMiddle2 = HandStart + 13 UMETA(DisplayName = "Middle Intermediate"),
  HandMiddle3 = HandStart + 14 UMETA(DisplayName = "Middle Distal"),
  HandMiddleTip = HandStart + 15 UMETA(DisplayName = "Middle Tip"),

  HandRing0 = HandStart + 16 UMETA(DisplayName = "Ring Metacarpal"),
  HandRing1 = HandStart + 17 UMETA(DisplayName = "Ring Proximal"),
  HandRing2 = HandStart + 18 UMETA(DisplayName = "Ring Intermediate"),
  HandRing3 = HandStart + 19 UMETA(DisplayName = "Ring Distal"),
  HandRingTip = HandStart + 20 UMETA(DisplayName = "Ring Tip"),

  HandPinky0 = HandStart + 21 UMETA(DisplayName = "Pinky Metacarpal"),
  HandPinky1 = HandStart + 22 UMETA(DisplayName = "Pinky Proximal"),
  HandPinky2 = HandStart + 23 UMETA(DisplayName = "Pinky Intermediate"),
  HandPinky3 = HandStart + 24 UMETA(DisplayName = "Pinky Distal"),
  HandPinkyTip = HandStart + 25 UMETA(DisplayName = "Pinky Tip"),

  HandEnd = HandStart + 26,
  EHandBones_MAX = HandEnd,
  Invalid,
};

struct FIsdkBoneChain
{
 public:
  FIsdkBoneChain(TArray<EIsdkHandBones> BonesIn, FLinearColor ColorIn)
      : Bones(BonesIn), Color(ColorIn)
  {
  }

  TArray<EIsdkHandBones> Bones;
  FLinearColor Color;
  bool GetJoint(size_t Index, EIsdkHandBones& OutJointId)
  {
    if (Index > 4)
    {
      return false;
    }
    auto JointId = Bones[Index];
    if (JointId == EIsdkHandBones::EHandBones_MAX)
    {
      return false;
    }
    OutJointId = JointId;
    return true;
  }
};

struct isdk_HandData_;
typedef isdk_HandData_ isdk_HandData;

/**
 * @class UIsdkHandData
 * @brief Hand Data class, critical container for hand pose information throughout the Interaction
 * SDK. Stores joint poses, radii, bone chains, bone mapping and methods and functions to set and
 * query these member variables.
 *
 * @addtogroup InteractionSDK
 */
UCLASS(BlueprintType, DefaultToInstanced)
class OCULUSINTERACTION_API UIsdkHandData : public UObject
{
  GENERATED_BODY()
 public:
  UIsdkHandData();
  /**
   * @brief Array of bone chains, representing the chains of joints that make up the four fingers
   * and thumbs, utilizes the EIsdkHandBones (and thus, OpenXR) skeleton reference enum.
   */
  const FIsdkBoneChain BoneChains[5] = {
      FIsdkBoneChain(
          {EIsdkHandBones::HandThumb1,
           EIsdkHandBones::HandThumb2,
           EIsdkHandBones::HandThumb3,
           EIsdkHandBones::HandThumbTip,
           EIsdkHandBones::EHandBones_MAX},
          FLinearColor::Red),
      FIsdkBoneChain(
          {EIsdkHandBones::HandIndex0,
           EIsdkHandBones::HandIndex1,
           EIsdkHandBones::HandIndex2,
           EIsdkHandBones::HandIndex3,
           EIsdkHandBones::HandIndexTip},
          FLinearColor::Green),
      FIsdkBoneChain(
          {
              EIsdkHandBones::HandMiddle0,
              EIsdkHandBones::HandMiddle1,
              EIsdkHandBones::HandMiddle2,
              EIsdkHandBones::HandMiddle3,
              EIsdkHandBones::HandMiddleTip,
          },
          FLinearColor::Blue),
      FIsdkBoneChain(
          {
              EIsdkHandBones::HandRing0,
              EIsdkHandBones::HandRing1,
              EIsdkHandBones::HandRing2,
              EIsdkHandBones::HandRing3,
              EIsdkHandBones::HandRingTip,
          },
          FLinearColor::Yellow),
      FIsdkBoneChain(
          {EIsdkHandBones::HandPinky0,
           EIsdkHandBones::HandPinky1,
           EIsdkHandBones::HandPinky2,
           EIsdkHandBones::HandPinky3,
           EIsdkHandBones::HandPinkyTip},
          FColor::Turquoise)};

  /**
   * @brief Returns the parent bone of a given bone in the defined chains for this HandData.
   * EHandBones_MAX returned if the passed bone is invalid.
   * @param BoneIn Child bone to retrieve the parent for
   * @return EIsdkHandBones The parent bone given the passed in child bone
   */
  EIsdkHandBones GetParentBoneInChain(const EIsdkHandBones BoneIn) const;

  /**
   * @brief Returns the child bone of a given bone in the defined chains for this HandData.
   * EHandBones_MAX returned if the passed bone is invalid.
   * @param BoneIn Parent bone to retrieve the child for
   * @return EIsdkHandBones The child bone given the passed in parent bone
   */
  EIsdkHandBones GetChildBoneInChain(const EIsdkHandBones BoneIn) const;

  /**
   * @brief Copies joint data from local member variable into the given HandData object, normally
   * used for API boundary hand data communications.
   * @param isdk_HandData& API defined version of the Hand Data container
   */
  void ReadJoints(isdk_HandData& HandData);

  /**
   * @brief Retrieves the number of joints utilized by the hand this data is intended to represent.
   * Normally used to iterate through all bones indices by various workflows in the SDK.
   * @return int32 The number of joints this HandData supports (defined here by
   * EIsdkHandBones::EHandBones_MAX)
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  static int32 GetNumJoints()
  {
    return static_cast<int32>(EIsdkHandBones::EHandBones_MAX);
  }

  /**
   * @brief Retrieves all of the current joint poses as index of transforms. Const, UFUNCTION
   * version of GetJointPoses()
   * @return TArray<FTransform>& Joint poses as transforms, in bone index order.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  const TArray<FTransform>& GetJointPoses() const
  {
    return JointPoses;
  }

  /**
   * @brief Retrieves all of the current joint poses as index of transforms. Non-const version of
   * GetJointPoses() intended for CPP uses only.
   * @return TArray<FTransform>& Joint poses as transforms, in bone index order.
   */
  TArray<FTransform>& GetJointPoses()
  {
    return JointPoses;
  }

  /**
   * @brief Retrieves the transform of bone (using its index as a parameter). Checks the validity of
   * the index before retrieval.
   * @return FTransform& The transform of the given bone index. Returns identity if the index is
   * invalid or out of bounds.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  const FTransform& GetJointPose(uint8 JointIndex) const
  {
    if (JointPoses.IsValidIndex(JointIndex))
    {
      return JointPoses[JointIndex];
    }
    return FTransform::Identity;
  }

  //
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  const FTransform& GetFingerJointPose(
      const EIsdkFingerType FingerType,
      const EIsdkFingerJoint FingerJoint) const;

  //
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  const FTransform& GetThumbJointPose(const EIsdkFingerJoint ThumbJoint) const;

  /**
   * @brief Retrieves all of the current joint radii as index of floats. Const UFUNCTION version of
   * GetJointRadii() intended for Blueprints or other const use cases.
   * @return TArray<float>& Joint radii as floats, in bone index order.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  const TArray<float>& GetJointRadii() const
  {
    return JointRadii;
  }

  /**
   * @brief Retrieves all of the current joint radii as index of floats. Non-const version of
   * GetJointRadii() intended for CPP uses only.
   * @return TArray<float>& Joint radii as floats, in bone index order.
   */
  TArray<float>& GetJointRadii()
  {
    return JointRadii;
  }
  /**
   * @brief Sets the critical member variable joint pose array to the passed in parameter. Does not
   * validate or enforce array sizes. Completely overwrites existing joint pose information. This
   * signature of the function takes an array of Transforms
   * @param InJointPoses Array of Transforms to set joint poses to.
   */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetJoints(const TArray<FTransform>& InJointPoses)
  {
    JointPoses = InJointPoses;
  }
  /**
   * @brief Sets the critical member variable joint pose array to the passed in parameter. Does not
   * validate; completely overwrites existing joint pose information. This
   * signature of the function takes an API-defined HandData object.
   * @param HandData API-defined HandData object used to copy new joint pose data from.
   */
  void SetJoints(const isdk_HandData& HandData);

  /**
   * @brief Sets the critical member variable joint pose array to the passed in parameter. Does not
   * validate or enforce array sizes. Completely overwrites existing joint pose information. This
   * signature of the function takes another HandData object
   * @param HandData Other HandData object reference to copy data from
   */
  void SetJoints(const UIsdkHandData& HandData);

  /**
   * @brief Copies a specific joint pose to a local cache, allowing for caching of critical joint
   * information before it is overwritten by regular updates, if desired.
   * @param PoseIn The transform (in bone space) of the joint being cached
   * @param JointPoseIndex The bone index to cache. Index is validated.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetCachedJointPose(const FTransform& PoseIn, const int32 JointPoseIndex)
  {
    if (JointPosesCache.IsValidIndex(JointPoseIndex))
    {
      JointPosesCache[JointPoseIndex] = PoseIn;
    }
  }

  /**
   * @brief Copies all joint poses to a local cache, allowing for caching of critical joint
   * information before it is overwritten by regular updates, if desired.
   * @param PosesIn Array of transforms (in bone space) to set the cache to. Does not enforce array
   * size matching.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetCachedJointPoses(TArray<FTransform>& PosesIn)
  {
    JointPosesCache = PosesIn;
  }

  /**
   * @brief Sets all joint pose information (both main and cached) to identity, completely removing
   * all previous joint information
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetJointsToIdentity();

  /**
   * @brief Set Inbound Bone Mappings
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetInboundBoneMap(TMap<int32, int32>& InboundMap)
  {
    InboundBoneMapping = InboundMap;
  }

  /**
   * @brief Set Inbound Bone Mappings
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetOutboundBoneMap(TMap<int32, int32>& OutboundMap)
  {
    OutboundBoneMapping = OutboundMap;
  }

  UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = InteractionSDK)
  EIsdkHandedness Handedness = EIsdkHandedness::Left;

 private:
  UPROPERTY()
  TArray<FTransform> JointPoses{};
  UPROPERTY()
  TArray<float> JointRadii{};
  UPROPERTY()
  TArray<FTransform> JointPosesCache{};

  // Bone mapping to translate for sending to external skeleton
  UPROPERTY()
  TMap<int32, int32> OutboundBoneMapping = {};

  // Bone mapping to translate for receiving from external skeleton
  UPROPERTY()
  TMap<int32, int32> InboundBoneMapping = {};
};
