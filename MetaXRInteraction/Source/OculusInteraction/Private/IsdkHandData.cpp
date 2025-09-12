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

#include "IsdkHandData.h"

#include "StructTypesPrivate.h"
#include "isdk_api/isdk_api.h"

UIsdkHandData::UIsdkHandData()
{
  SetJointsToIdentity();
}

void UIsdkHandData::SetJoints(const isdk_HandData& HandData)
{
  if (InboundBoneMapping.Num() > 0)
  {
    for (int i = 0; i < JointPoses.Num(); i++)
    {
      FQuat NewRotation;

      JointPoses[i].SetLocation(JointPosesCache[i].GetLocation());
      int32* BoneIdx = InboundBoneMapping.Find(i);
      if (BoneIdx != nullptr)
      {
        StructTypesUtils::Copy(HandData.joints[*BoneIdx], NewRotation);
        JointPoses[i].SetRotation(NewRotation);
      }
      else
      {
        JointPoses[i].SetRotation(JointPosesCache[i].GetRotation());
      }
    }
  }
}

void UIsdkHandData::SetJoints(const UIsdkHandData& InHandData)
{
  const TArray<FTransform>& InJointPoses = InHandData.GetJointPoses();
  const TArray<float>& InJointRadii = InHandData.GetJointRadii();
  const auto DestJointCount = JointPoses.Num();
  checkf(
      InJointPoses.Num() == JointPoses.Num(),
      TEXT("Source joint count (%d) must match dest joint count (%d)"),
      InJointPoses.Num(),
      DestJointCount);

  for (int I = 0; I < DestJointCount; ++I)
  {
    JointPoses[I] = InJointPoses[I];
  }
  for (int I = 0; I < DestJointCount; ++I)
  {
    JointRadii[I] = InJointRadii[I];
  }
}

void UIsdkHandData::ReadJoints(isdk_HandData& HandData)
{
  StructTypesUtils::Copy(JointPoses[(uint8)EIsdkHandBones::HandWristRoot], HandData.root);
  const int32 NumJoints = std::size(HandData.joints);

  if (OutboundBoneMapping.Num() > 0)
  {
    for (int i = 0; i < NumJoints; i++)
    {
      int32* BoneIdx = OutboundBoneMapping.Find(i);
      if (BoneIdx != nullptr)
      {
        StructTypesUtils::Copy(JointPoses[*BoneIdx].GetRotation(), HandData.joints[i]);
      }
      else
      {
        StructTypesUtils::Copy(FQuat::Identity, HandData.joints[i]);
      }
    }
  }
}

void UIsdkHandData::SetJointsToIdentity()
{
  const auto DestJointCount = GetNumJoints();
  JointPoses.SetNum(DestJointCount);
  JointPosesCache.SetNum(DestJointCount);
  for (auto& Pose : JointPoses)
  {
    Pose = FTransform::Identity;
  }
  for (auto& Pose : JointPosesCache)
  {
    Pose = FTransform::Identity;
  }
  JointRadii.SetNum(DestJointCount);
  for (auto& Radius : JointRadii)
  {
    Radius = 0.0f;
  }
}

EIsdkHandBones UIsdkHandData::GetParentBoneInChain(const EIsdkHandBones BoneIn) const
{
  for (FIsdkBoneChain ThisChain : BoneChains)
  {
    const int32 BoneIdx = ThisChain.Bones.IndexOfByKey(BoneIn);
    if (BoneIdx != INDEX_NONE)
    {
      if (BoneIdx - 1 < 0)
      {
        return EIsdkHandBones::HandWristRoot;
      }
      else
      {
        return ThisChain.Bones[BoneIdx - 1];
      }
    }
  }
  return EIsdkHandBones::EHandBones_MAX;
}

EIsdkHandBones UIsdkHandData::GetChildBoneInChain(const EIsdkHandBones BoneIn) const
{
  for (FIsdkBoneChain ThisChain : BoneChains)
  {
    const int32 BoneIdx = ThisChain.Bones.IndexOfByKey(BoneIn);
    if (BoneIdx != INDEX_NONE)
    {
      if (BoneIdx + 1 > 4)
      {
        return EIsdkHandBones::EHandBones_MAX;
      }
      else
      {
        return ThisChain.Bones[BoneIdx + 1];
      }
    }
  }
  return EIsdkHandBones::EHandBones_MAX;
}

const FTransform& UIsdkHandData::GetFingerJointPose(
    const EIsdkFingerType FingerType,
    const EIsdkFingerJoint FingerJoint) const
{
  const uint32 FingerJointIdx =
      (uint32)EIsdkHandBones::HandIndex0 + ((uint32)FingerType * 5) + (uint32)FingerJoint;
  if (JointPoses.IsValidIndex(FingerJointIdx))
  {
    return JointPoses[FingerJointIdx];
  }
  return FTransform::Identity;
}

const FTransform& UIsdkHandData::GetThumbJointPose(const EIsdkFingerJoint ThumbJoint) const
{
  if (ThumbJoint == EIsdkFingerJoint::Intermediate)
  {
    // UELOG
    return FTransform::Identity;
  }
  const uint32 ThumbJointIdx = (uint32)EIsdkHandBones::HandThumb1 + (uint32)ThumbJoint -
      (ThumbJoint > EIsdkFingerJoint::Intermediate ? 1 : 0);
  if (JointPoses.IsValidIndex(ThumbJointIdx))
  {
    return JointPoses[ThumbJointIdx];
  }
  return FTransform::Identity;
}
