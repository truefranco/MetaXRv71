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

#include "IsdkHandPoseData.h"
#include "IsdkFunctionLibrary.h"
#include "IsdkHandMeshComponent.h"
#if INTEL_ISPC
#include "isdkHandPoseData.ispc.generated.h"
#endif

static void FillComponentSpaceTransforms(
	const USkeletalMesh* Skeleton,
	const TArray<FTransform>& InBoneSpaceTransforms, 
	const TArray<FBoneIndexType>& InFillComponentSpaceTransformsRequiredBones,
	TArray<FTransform>& OutComponentSpaceTransforms);

static FTransform GetComponentSpaceTransform(
	const UPoseAsset* Pose,
	FName BoneName, 
	const TArray<FTransform>& LocalTransforms);

UIsdkHandPoseData::UIsdkHandPoseData()
{
  HandData = CreateDefaultSubobject<UIsdkHandData>(TEXT("Hand Data"));

  HandJointMapping = CreateDefaultSubobject<UIsdkHandJointMappings>(TEXT("Hand Joint Mapping"));
  HandJointMapping->ThumbJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRThumbMapping();
  HandJointMapping->FingerJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRFingerMapping();

  JointNames[0] = FName("XRHand_Palm");
  JointNames[1] = FName("XRHand_Wrist");
  // ----- Thumb
  JointNames[2] = FName("XRHand_ThumbMetacarpal");
  JointNames[3] = FName("XRHand_ThumbProximal");
  JointNames[4] = FName("XRHand_ThumbDistal");
  JointNames[5] = FName("XRHand_ThumbTip");
  // ----- Index
  JointNames[6] = FName("XRHand_IndexMetacarpal");
  JointNames[7] = FName("XRHand_IndexProximal");
  JointNames[8] = FName("XRHand_IndexIntermediate");
  JointNames[9] = FName("XRHand_IndexDistal");
  JointNames[10] = FName("XRHand_IndexTip");
  // ----- Middle
  JointNames[11] = FName("XRHand_MiddleMetacarpal");
  JointNames[12] = FName("XRHand_MiddleProximal");
  JointNames[13] = FName("XRHand_MiddleIntermediate");
  JointNames[14] = FName("XRHand_MiddleDistal");
  JointNames[15] = FName("XRHand_MiddleTip");
  // ----- Ring
  JointNames[16] = FName("XRHand_RingMetacarpal");
  JointNames[17] = FName("XRHand_RingProximal");
  JointNames[18] = FName("XRHand_RingIntermediate");
  JointNames[19] = FName("XRHand_RingDistal");
  JointNames[20] = FName("XRHand_RingTip");
  // ----- Pinky
  JointNames[21] = FName("XRHand_LittleMetacarpal");
  JointNames[22] = FName("XRHand_LittleProximal");
  JointNames[23] = FName("XRHand_LittleIntermediate");
  JointNames[24] = FName("XRHand_LittleDistal");
  JointNames[25] = FName("XRHand_LittleTip");
}

void UIsdkHandPoseData::SetRotationFromSkeleton(USkeletalMesh* SkinnedAsset)
{
  FReferenceSkeleton& RefSkeleton = SkinnedAsset->GetRefSkeleton();
  auto& RefBonePoses = RefSkeleton.GetRefBonePose();

  TArray<FTransform> ComponentSpacePoses = TArray<FTransform>();
  ComponentSpacePoses.SetNum(RefBonePoses.Num());
  TArray<uint16> RequiredBones = TArray<uint16>();
  RequiredBones.SetNum(RefBonePoses.Num());

  for (int BoneNameIndex = 0; BoneNameIndex < RefBonePoses.Num(); ++BoneNameIndex)
  {
    RequiredBones[BoneNameIndex] = BoneNameIndex;
  }

  FillComponentSpaceTransforms(SkinnedAsset, RefBonePoses, RequiredBones, ComponentSpacePoses);

  TArray<FTransform>& HandJoints = HandData->GetJointPoses();
  for (int BoneNameIndex = 0; BoneNameIndex < static_cast<int>(EIsdkHandBones::EHandBones_MAX);
       ++BoneNameIndex)
  {
    auto BoneIndex = RefSkeleton.FindBoneIndex(JointNames[BoneNameIndex]);
    auto BonePose = ComponentSpacePoses[BoneIndex];
    HandJoints[BoneNameIndex] = BonePose;
  }
  SetDirty();
}

void UIsdkHandPoseData::SetRotationFromVisual(UIsdkHandMeshComponent* HandMesh)
{
  if (!IsValid(HandMesh))
  {
    UE_LOG(LogOculusInteraction, Warning, TEXT("Hand Mesh is invalid"));
    return;
  }
  auto InputDataSource = HandMesh->GetJointsDataSource();
  if (!IsValid(InputDataSource.GetObject()))
  {
    UE_LOG(LogOculusInteraction, Warning, TEXT("Joint data source is invalid"));
    return;
  }
  const UIsdkHandData* SourceHandData =
      InputDataSource->Execute_GetHandData(InputDataSource.GetObject());
  TArray<FTransform>& HandJointPoses = HandData->GetJointPoses();
  const TArray<FTransform>& SourceJointPoses = SourceHandData->GetJointPoses();
  for (int BoneIndex = 0; BoneIndex < static_cast<int>(EIsdkHandBones::EHandBones_MAX); ++BoneIndex)
  {
    HandJointPoses[BoneIndex] = SourceJointPoses[BoneIndex];
  }

  EIsdkHandedness VisualHandedness;
  HandMesh->GetHandednessFromDataSource(VisualHandedness);
  Handedness = VisualHandedness;

  SetDirty();
}

void UIsdkHandPoseData::SetRotationFromPoseWithName(UPoseAsset* Pose, FName Name)
{
  auto LocalPoseTransforms = TArray<FTransform>();
#if WITH_EDITOR
  auto PoseIndex = Pose->GetPoseIndexByName(Name);
  Pose->GetFullPose(PoseIndex, LocalPoseTransforms);
  TArray<FTransform>& HandJointPoses = HandData->GetJointPoses();
  for (int BoneNameIndex = 1; BoneNameIndex < static_cast<int>(EIsdkHandBones::EHandBones_MAX);
       ++BoneNameIndex)
  {
    auto BoneName = JointNames[BoneNameIndex];
    if (BoneName == NAME_None)
    {
      auto JointName = JointNames[BoneNameIndex].ToString();
      UE_LOG(LogOculusInteraction, Warning, TEXT("Could not find node %s"), *JointName);
      return;
    }
    auto BonePose = GetComponentSpaceTransform(Pose, BoneName, LocalPoseTransforms);
    HandJointPoses[BoneNameIndex] = BonePose;
  }
  HandJointPoses[0] = FTransform::Identity;
  SetDirty();
#endif
}

void FillComponentSpaceTransforms(
	const USkeletalMesh* Skeleton,
	const TArray<FTransform>& InBoneSpaceTransforms,
	const TArray<FBoneIndexType>& InFillComponentSpaceTransformsRequiredBones,
	TArray<FTransform>& OutComponentSpaceTransforms)
{
	ANIM_MT_SCOPE_CYCLE_COUNTER(FillComponentSpaceTransforms, !IsInGameThread());

	// right now all this does is populate DestSpaceBases
	check(Skeleton->GetRefSkeleton().GetNum() == InBoneSpaceTransforms.Num());
	check(Skeleton->GetRefSkeleton().GetNum() == OutComponentSpaceTransforms.Num());

	const int32 NumBones = InBoneSpaceTransforms.Num();

	if (!NumBones)
	{
		return;
	}

#if DO_GUARD_SLOW
	/** Keep track of which bones have been processed for fast look up */
	TArray<uint8, TInlineAllocator<256>> BoneProcessed;
	BoneProcessed.AddZeroed(NumBones);
#endif

	const FTransform* LocalTransformsData = InBoneSpaceTransforms.GetData();
	FTransform* ComponentSpaceData = OutComponentSpaceTransforms.GetData();

	// First bone (if we have one) is always root bone, and it doesn't have a parent.
	{
		check(InFillComponentSpaceTransformsRequiredBones.Num() == 0 || InFillComponentSpaceTransformsRequiredBones[0] == 0);
		OutComponentSpaceTransforms[0] = InBoneSpaceTransforms[0];

#if DO_GUARD_SLOW
		// Mark bone as processed
		BoneProcessed[0] = 1;
#endif
	}

	if (INTEL_ISPC)
	{
#if INTEL_ISPC
		ispc::FillComponentSpaceTransforms(
			(ispc::FTransform*)&ComponentSpaceData[0],
			(ispc::FTransform*)&LocalTransformsData[0],
			InFillComponentSpaceTransformsRequiredBones.GetData(),
			(const uint8*)Skeleton->GetRefSkeleton().GetRefBoneInfo().GetData(),
			sizeof(FMeshBoneInfo),
			offsetof(FMeshBoneInfo, ParentIndex),
			InFillComponentSpaceTransformsRequiredBones.Num());
#endif
	}
	else
	{
		for (int32 i = 1; i < InFillComponentSpaceTransformsRequiredBones.Num(); i++)
		{
			const int32 BoneIndex = InFillComponentSpaceTransformsRequiredBones[i];
			FTransform* SpaceBase = ComponentSpaceData + BoneIndex;

			FPlatformMisc::Prefetch(SpaceBase);

#if DO_GUARD_SLOW
			// Mark bone as processed
			BoneProcessed[BoneIndex] = 1;
#endif
			// For all bones below the root, final component-space transform is relative transform * component-space transform of parent.
			const int32 ParentIndex = Skeleton->GetRefSkeleton().GetParentIndex(BoneIndex);
			FTransform* ParentSpaceBase = ComponentSpaceData + ParentIndex;
			FPlatformMisc::Prefetch(ParentSpaceBase);

#if DO_GUARD_SLOW
			// Check the precondition that Parents occur before Children in the RequiredBones array.
			checkSlow(BoneProcessed[ParentIndex] == 1);
#endif
			FTransform::Multiply(SpaceBase, LocalTransformsData + BoneIndex, ParentSpaceBase);

			SpaceBase->NormalizeRotation();

			checkSlow(SpaceBase->IsRotationNormalized());
			checkSlow(!SpaceBase->ContainsNaN());
		}
	}
}

FTransform GetComponentSpaceTransform(const UPoseAsset* Pose, FName BoneName, const TArray<FTransform>& LocalTransforms)
{
	const FReferenceSkeleton& RefSkel = Pose->GetSkeleton()->GetReferenceSkeleton();

	// Init component space transform with identity
	FTransform ComponentSpaceTransform = FTransform::Identity;

	// Start to walk up parent chain until we reach root (ParentIndex == INDEX_NONE)
	int32 BoneIndex = RefSkel.FindBoneIndex(BoneName);
	while (BoneIndex != INDEX_NONE)
	{
		BoneName = RefSkel.GetBoneName(BoneIndex);
		int32 TrackIndex = Pose->GetTrackIndexByName(BoneName);

		// If a track for parent, get local space transform from that
		// If not, get from ref pose
		FTransform BoneLocalTM = (TrackIndex != INDEX_NONE) ? LocalTransforms[TrackIndex] : RefSkel.GetRefBonePose()[BoneIndex];

		// Continue to build component space transform
		ComponentSpaceTransform = ComponentSpaceTransform * BoneLocalTM;

		// Now move up to parent
		BoneIndex = RefSkel.GetParentIndex(BoneIndex);
	}

	return ComponentSpaceTransform;
}