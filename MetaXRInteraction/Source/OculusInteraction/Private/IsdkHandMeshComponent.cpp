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

#include "IsdkHandMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "OculusInteractionLog.h"
#include "StructTypesPrivate.h"
#include "DataSources/IsdkIRootPose.h"
#include "IsdkHandPoseData.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "VisualLogger/VisualLogger.h"

#if defined(ENGINE_MAJOR_VERSION) && ENGINE_MAJOR_VERSION == 5
#include "Engine/SkinnedAsset.h"
#endif

#include <array>
#include <vector>

#include "IsdkChecks.h"
#include "IsdkFunctionLibrary.h"
#include "IsdkRuntimeSettings.h"

using isdk::api::ExternalHandPositionFrame;
using isdk::api::ExternalHandPositionFramePtr;

namespace isdk
{
TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DebugHandVisuals(
    TEXT("Meta.InteractionSDK.DebugHandVisuals"),
    false,
    TEXT("Draws debug visuals for hands at runtime.  Also draws to visual logger."));
}

namespace isdk::api::helper
{
class FExternalHandPositionFrameImpl
    : public FApiImpl<ExternalHandPositionFrame, ExternalHandPositionFramePtr>
{
 public:
  explicit FExternalHandPositionFrameImpl(std::function<ExternalHandPositionFramePtr()> CreateFn)
      : FApiImpl(std::move(CreateFn))
  {
  }

  std::array<ovrpVector3f, UIsdkHandMeshComponent::MappedBoneCount> WristSpaceJointLocations{};
};
} // namespace isdk::api::helper

UIsdkHandMeshComponent::UIsdkHandMeshComponent()
{
  PrimaryComponentTick.bCanEverTick = true;

  PrimaryComponentTick.bStartWithTickEnabled = false;
  bTickInEditor = true;
  PrimaryComponentTick.TickGroup = TG_PrePhysics;

  ClearMappingState();

  MappedBoneNames.Init(NAME_None, MappedBoneCount);

  ExternalHandPositionFrameImpl =
      MakePimpl<isdk::api::helper::FExternalHandPositionFrameImpl, EPimplPtrMode::NoCopy>(
          [this]() -> ExternalHandPositionFramePtr
          {
            if (!ensureMsgf(
                    IsValid(JointsDataSource.GetObject()),
                    TEXT("%s: JointsDataSource must not be null"),
                    *GetFullName()))
            {
              return nullptr;
            }

            // Read Handedness
            auto Handedness = IIsdkIHandJoints::Execute_GetHandedness(JointsDataSource.GetObject());
            const auto ApiHandedness = static_cast<isdk_Handedness>(Handedness);

            // Read hand joint mappings
            const auto HandJointMappings =
                IIsdkIHandJoints::Execute_GetHandJointMappings(JointsDataSource.GetObject());
            std::vector<isdk_ExternalHandPositionFrame_ThumbJointMapping> ApiThumbJointMappings(
                HandJointMappings->ThumbJointMappings.Num());
            for (int Index = 0; Index < HandJointMappings->ThumbJointMappings.Num(); ++Index)
            {
              StructTypesUtils::Copy(
                  HandJointMappings->ThumbJointMappings[Index], ApiThumbJointMappings[Index]);
            }
            std::vector<isdk_ExternalHandPositionFrame_FingerJointMapping> ApiFingerJointMappings(
                HandJointMappings->FingerJointMappings.Num());
            for (int Index = 0; Index < HandJointMappings->FingerJointMappings.Num(); ++Index)
            {
              StructTypesUtils::Copy(
                  HandJointMappings->FingerJointMappings[Index], ApiFingerJointMappings[Index]);
            }

            // Get Hand Data
            const auto HandData =
                IIsdkIHandJoints::Execute_GetHandData(JointsDataSource.GetObject());
            if (!ensureMsgf(
                    HandData, TEXT("%s: JointsDataSource HandData must be valid."), *GetFullName()))
            {
              return nullptr;
            }

            // Read joint count
            const auto JointCount = HandData->GetNumJoints();
            if (!ensureMsgf(
                    JointCount > 0,
                    TEXT("%s: JointsDataSource HandData must have at least 1 joint defined."),
                    *GetFullName()))
            {
              return nullptr;
            }

            auto Instance = ExternalHandPositionFrame::create(
                ApiHandedness,
                ApiThumbJointMappings.data(),
                ApiThumbJointMappings.size(),
                ApiFingerJointMappings.data(),
                ApiFingerJointMappings.size(),
                JointCount);
            if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
            {
              return nullptr;
            }

            if (MappingState == EIsdkSkeletonMappingState::Valid)
            {
              UpdateApiHandPositionFrame(Instance.Get());
            }

            return Instance;
          });
}

void UIsdkHandMeshComponent::BeginPlay()
{
  Super::BeginPlay();
  InitializeSkeleton();
  if (ExternalHandPositionFrameImpl->IsInstanceValid() &&
      MappingState == EIsdkSkeletonMappingState::Valid)
  {
    UpdateApiHandPositionFrame(ExternalHandPositionFrameImpl->GetInstanceChecked());
  }
}

void UIsdkHandMeshComponent::BeginDestroy()
{
  Super::BeginDestroy();

  ExternalHandPositionFrameImpl.Reset();
}

bool UIsdkHandMeshComponent::IsApiInstanceValid() const
{
  return ExternalHandPositionFrameImpl->IsInstanceValid();
}

isdk::api::IHandPositionFrame* UIsdkHandMeshComponent::TryGetApiIHandPositionFrame() const
{
  const bool bIsDataSourceValid = IsDataSourceJointsValid();
  const bool bIsDataSourceRootPoseValid = IsDataSourceRootPoseValid();
  const bool bRequiredBonesValid = RequiredBones.IsValid();
  if (bIsDataSourceValid && bIsDataSourceRootPoseValid && bRequiredBonesValid)
  {
    return ExternalHandPositionFrameImpl->GetOrCreateInstance();
  }
  return nullptr;
}

UObject* UIsdkHandMeshComponent::GetSkeletalMesh() const
{
#if defined(ENGINE_MAJOR_VERSION) && ENGINE_MAJOR_VERSION == 4
  return Cast<UObject>(SkeletalMesh);
#else
  return Cast<UObject>(GetSkinnedAsset());
#endif
}

void UIsdkHandMeshComponent::InitializeSkeleton()
{
  // map bones to indices for quicker lookup
  for (int BoneNameIndex = 0; BoneNameIndex < static_cast<int>(EIsdkHandBones::EHandBones_MAX);
       ++BoneNameIndex)
  {
    int BoneIndex = GetBoneIndex(MappedBoneNames[BoneNameIndex]);
    if (BoneIndex == INDEX_NONE)
    {
      SetInvalidMappingState(GetSkeletalMesh());
      UE_LOG(
          LogOculusInteraction,
          Error,
          TEXT("Bone %s has no valid bone name mapped to the skeleton."),
          *MappedBoneNames[BoneNameIndex].ToString());
      return;
    }
    MappedBoneIndices[BoneNameIndex] = BoneIndex;
  }
  SetValidMappingState(GetSkeletalMesh());
}

void UIsdkHandMeshComponent::SetJointsDataSource(
    TScriptInterface<IIsdkIHandJoints> InJointsDataSource)
{
  UActorComponent* JointsActorComponent = Cast<UActorComponent>(JointsDataSource.GetObject());
  if (IsValid(JointsActorComponent))
  {
    RemoveTickPrerequisiteComponent(JointsActorComponent);
  }
  JointsDataSource = InJointsDataSource;

  // Ensure that this component ticks after the input component
  JointsActorComponent = Cast<UActorComponent>(JointsDataSource.GetObject());
  if (IsValid(JointsActorComponent))
  {
    AddTickPrerequisiteComponent(JointsActorComponent);
  }
}

void UIsdkHandMeshComponent::SetRootPoseDataSource(
    TScriptInterface<IIsdkIRootPose> InRootPoseDataSource)
{
  UActorComponent* RootPoseActorComponent = Cast<UActorComponent>(RootPoseDataSource.GetObject());
  if (IsValid(RootPoseActorComponent))
  {
    RemoveTickPrerequisiteComponent(RootPoseActorComponent);
  }
  RootPoseDataSource = InRootPoseDataSource;

  // Ensure that this component ticks after the input component
  RootPoseActorComponent = Cast<UActorComponent>(RootPoseDataSource.GetObject());
  if (IsValid(RootPoseActorComponent))
  {
    AddTickPrerequisiteComponent(RootPoseActorComponent);
  }
}

void UIsdkHandMeshComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  if (bPauseSkeletonUpdates)
  {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    return;
  }

  if (static_cast<uint8>(HandPoseLerpState) > 0 && HandPoseLerpTime > 0.f)
  {
    if (HandPoseLerpState == EIsdkLerpState::TransitioningTo)
    {
      HandPoseLerpTimeCount = FMath::Min(HandPoseLerpTimeCount + DeltaTime, HandPoseLerpTime);
    }
    else if (HandPoseLerpState == EIsdkLerpState::TransitioningAway)
    {
      HandPoseLerpTimeCount = FMath::Max(HandPoseLerpTimeCount - DeltaTime, 0.f);
    }
    HandPoseLerpAlpha = FMath::Min(HandPoseLerpTimeCount / HandPoseLerpTime, 1.f);
  }

  UpdateMappingState();
  UpdateSkeleton();
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  UpdateChildTransforms();

  // Only update the native instance if it exists.
  if (ExternalHandPositionFrameImpl->IsInstanceValid() &&
      MappingState == EIsdkSkeletonMappingState::Valid)
  {
    UpdateApiHandPositionFrame(ExternalHandPositionFrameImpl->GetInstanceChecked());
  }

  // Enable this to see debug visuals for the skeleton in world space based on the HandJoints data
  if (isdk::CVar_Meta_InteractionSDK_DebugHandVisuals.GetValueOnAnyThread())
  {
    DrawDebugSkeleton();
  }
}

void UIsdkHandMeshComponent::SetMappedBoneNamesAsDefault()
{
  MappedBoneNames[0] = FName("XRHand_Palm");
  MappedBoneNames[1] = FName("XRHand_Wrist");
  // ----- Thumb
  MappedBoneNames[2] = FName("XRHand_ThumbMetacarpal");
  MappedBoneNames[3] = FName("XRHand_ThumbProximal");
  MappedBoneNames[4] = FName("XRHand_ThumbDistal");
  MappedBoneNames[5] = FName("XRHand_ThumbTip");
  // ----- Index
  MappedBoneNames[6] = FName("XRHand_IndexMetacarpal");
  MappedBoneNames[7] = FName("XRHand_IndexProximal");
  MappedBoneNames[8] = FName("XRHand_IndexIntermediate");
  MappedBoneNames[9] = FName("XRHand_IndexDistal");
  MappedBoneNames[10] = FName("XRHand_IndexTip");
  // ----- Middle
  MappedBoneNames[11] = FName("XRHand_MiddleMetacarpal");
  MappedBoneNames[12] = FName("XRHand_MiddleProximal");
  MappedBoneNames[13] = FName("XRHand_MiddleIntermediate");
  MappedBoneNames[14] = FName("XRHand_MiddleDistal");
  MappedBoneNames[15] = FName("XRHand_MiddleTip");
  // ----- Ring
  MappedBoneNames[16] = FName("XRHand_RingMetacarpal");
  MappedBoneNames[17] = FName("XRHand_RingProximal");
  MappedBoneNames[18] = FName("XRHand_RingIntermediate");
  MappedBoneNames[19] = FName("XRHand_RingDistal");
  MappedBoneNames[20] = FName("XRHand_RingTip");
  // ----- Pinky
  MappedBoneNames[21] = FName("XRHand_LittleMetacarpal");
  MappedBoneNames[22] = FName("XRHand_LittleProximal");
  MappedBoneNames[23] = FName("XRHand_LittleIntermediate");
  MappedBoneNames[24] = FName("XRHand_LittleDistal");
  MappedBoneNames[25] = FName("XRHand_LittleTip");

  // Clear the mapping state, because the state was only valid for the
  // previous mapped names
  ClearMappingState();
}

void UIsdkHandMeshComponent::ResetAllBoneTransforms()
{
#if defined(ENGINE_MAJOR_VERSION) && ENGINE_MAJOR_VERSION == 4
  auto SkinnedMesh = SkeletalMesh;
#else
  auto SkinnedMesh = GetSkinnedAsset();
#endif
  if (!IsValid(SkinnedMesh) || BoneSpaceTransforms.IsEmpty())
  {
    return;
  }
  BoneSpaceTransforms = SkinnedMesh->GetRefSkeleton().GetRefBonePose();
  MarkRefreshTransformDirty();
}

void UIsdkHandMeshComponent::UpdateMappingState()
{
  // If the skeleton is not valid, then the mapping is invalid
  if (!IsValid(GetSkeletalMesh()))
  {
    SetInvalidMappingState(GetSkeletalMesh());
    return;
  }

  // Reset mapping state if skeletal mesh changed
  if (MappedSkeleton != GetSkeletalMesh())
  {
    ClearMappingState();
  }

  // Try to map the skeleton if there is no mapping and there is a valid joint data source
  if (MappingState == EIsdkSkeletonMappingState::None && IsValid(JointsDataSource.GetObject()))
  {
    if (!AreMappedBoneNamesValid())
    {
      SetMappedBoneNamesFromJointSourceHandedness();
    }
    InitializeSkeleton();
  }
}

void UIsdkHandMeshComponent::UpdateSkeleton()
{
  if (MappingState != EIsdkSkeletonMappingState::Valid)
  {
    return;
  }

  // Root pose
  if (!bIgnoreRootPose && IsDataSourceRootPoseValid())
  {
    const auto RootPose = IIsdkIRootPose::Execute_GetRootPose(RootPoseDataSource.GetObject());
    if (bOverrideRootPose && CurrentHandGrabPoseMode == EIsdkHandGrabPoseMode::SnapPoseToObject)
    {
      // Pose sticks with object
      SetWorldTransform(
          ApplyRootPoseOffset(RootPose), false, nullptr, ETeleportType::TeleportPhysics);
    }
    else
    {
      SetWorldTransform(RootPose, false, nullptr, ETeleportType::TeleportPhysics);
    }
  }

  if (!JointsDataSource.GetObject())
  {
    return;
  }

  UIsdkHandData* HandDataSource =
      IIsdkIHandJoints::Execute_GetHandData(JointsDataSource.GetObject());
  UIsdkHandData* HandDataOverride = nullptr;
  if (bHandPoseOverridden)
  {
    // Read Joints from Overriding HandPoseData
    if (IsValid(HandPoseDataOverride))
    {
      HandDataOverride = IIsdkIHandJoints::Execute_GetHandData(HandPoseDataOverride);
    }
  }

  if (IsValid(HandDataSource))
  {
    TArray<FTransform>& SourcePoses = HandDataSource->GetJointPoses();
    const bool bHandPoseOverrideValid = bHandPoseOverridden && IsValid(HandDataOverride);
    TArray<FTransform> OverridePoses = {};

    // Check if we're overriding the pose
    if (bHandPoseOverrideValid)
    {
      OverridePoses = HandDataOverride->GetJointPoses();
      // If we're not lerping, use the override pose array
      if (HandPoseLerpState == EIsdkLerpState::Inactive)
      {
        SourcePoses = HandDataOverride->GetJointPoses();
      }
    }

    constexpr auto WristSpace = EBoneSpaces::Type::ComponentSpace;
    const int BoneCount = UIsdkHandData::GetNumJoints();
    for (int BoneId = 0; BoneId < BoneCount; BoneId++)
    {
      const FName BoneName = MappedBoneNames[BoneId];

      FTransform BoneTransform = GetBoneTransformByName(BoneName, WristSpace);
      if ((uint8)HandPoseLerpState > 0)
      {
        BoneTransform.SetLocation(FMath::Lerp(
            SourcePoses[BoneId].GetLocation(),
            OverridePoses[BoneId].GetLocation(),
            HandPoseLerpAlpha));
        BoneTransform.SetRotation(FMath::Lerp(
            SourcePoses[BoneId].GetRotation(),
            OverridePoses[BoneId].GetRotation(),
            HandPoseLerpAlpha));
      }
      else
      {
        BoneTransform.SetLocation(SourcePoses[BoneId].GetLocation());
        BoneTransform.SetRotation(SourcePoses[BoneId].GetRotation());
      }
      SetBoneTransformByName(BoneName, BoneTransform, WristSpace);
    }

    // Check if we're done lerping in
    if (HandPoseLerpState == EIsdkLerpState::TransitioningTo && HandPoseLerpAlpha >= 1.f)
    {
      HandPoseLerpState = EIsdkLerpState::Inactive;
    }
    // Or if we're done lerping out
    else if (HandPoseLerpState == EIsdkLerpState::TransitioningAway && HandPoseLerpAlpha <= 0.f)
    {
      HandPoseLerpState = EIsdkLerpState::Inactive;
      bHandPoseOverridden = false;
      HandPoseDataOverride = nullptr;
      HandPoseLerpTime = 0.f;
    }

    MarkRefreshTransformDirty();
  }
}

void UIsdkHandMeshComponent::UpdateApiHandPositionFrame(
    ExternalHandPositionFrame& ApiHandPositionFrame) const
{
  // Set Wrist Position
  const auto ApiWristPosition = StructTypesUtils::Convert(GetComponentLocation());
  ApiHandPositionFrame.setWristData(&ApiWristPosition);

  // Set Joint Positions
  auto& ApiJointLocations = ExternalHandPositionFrameImpl->WristSpaceJointLocations;

  FA2CSPose CSPose;
  CSPose.AllocateLocalPoses(RequiredBones, GetBoneSpaceTransforms());
  for (int BoneId = 0; BoneId < MappedBoneCount; ++BoneId)
  {
    const int BoneIndex = MappedBoneIndices[BoneId];
    const auto WristSpaceTransform = CSPose.GetComponentSpaceTransform(BoneIndex);

    ApiJointLocations[BoneId] = StructTypesUtils::Convert(WristSpaceTransform.GetLocation());
  }

  ApiHandPositionFrame.setJointData(ApiJointLocations.data(), ApiJointLocations.size());
}

bool UIsdkHandMeshComponent::IsDataSourceRootPoseValid() const
{
  UObject* Obj = RootPoseDataSource.GetObject();
  return IsValid(Obj) && IIsdkIRootPose::Execute_IsRootPoseValid(Obj);
}

bool UIsdkHandMeshComponent::IsDataSourceJointsValid() const
{
  UObject* Obj = JointsDataSource.GetObject();
  return IsValid(Obj) && IIsdkIHandJoints::Execute_IsHandJointDataValid(Obj);
}

void UIsdkHandMeshComponent::DrawTransformAxis(const FTransform& Pose) const
{
  static const FColor XColor = FColor::Red;
  static const FColor YColor = FColor::Green;
  static const FColor ZColor = FColor::Blue;

  const FVector JointOrigin = Pose.GetLocation();
  const FVector XAxisOffset = JointOrigin + Pose.GetUnitAxis(EAxis::X);
  const FVector YAxisOffset = JointOrigin + Pose.GetUnitAxis(EAxis::Y);
  const FVector ZAxisOffset = JointOrigin + Pose.GetUnitAxis(EAxis::Z);

  constexpr auto Depth = ESceneDepthPriorityGroup::SDPG_Foreground;
  DrawDebugLine(GetWorld(), JointOrigin, XAxisOffset, XColor, false, 0.0, Depth, 0.3);
  DrawDebugLine(GetWorld(), JointOrigin, YAxisOffset, YColor, false, 0.0, Depth, 0.3);
  DrawDebugLine(GetWorld(), JointOrigin, ZAxisOffset, ZColor, false, 0.0, Depth, 0.3);

  UE_VLOG_SEGMENT(
      GetOwner(), LogOculusInteraction, Verbose, JointOrigin, XAxisOffset, XColor, TEXT_EMPTY);
  UE_VLOG_SEGMENT(
      GetOwner(), LogOculusInteraction, Verbose, JointOrigin, YAxisOffset, YColor, TEXT_EMPTY);
  UE_VLOG_SEGMENT(
      GetOwner(), LogOculusInteraction, Verbose, JointOrigin, ZAxisOffset, ZColor, TEXT_EMPTY);
}

void UIsdkHandMeshComponent::DrawDebugSkeleton() const
{
  if (GetVisibleFlag() && IsDataSourceJointsValid())
  {
    const auto RootPose = IsDataSourceRootPoseValid()
        ? IIsdkIRootPose::Execute_GetRootPose(RootPoseDataSource.GetObject())
        : FTransform(FQuat(FVector::UnitX(), HALF_PI)) * GetComponentTransform();

    const TArray<FTransform>& JointPoses =
        IIsdkIHandJoints::Execute_GetHandData(JointsDataSource.GetObject())->GetJointPoses();

    const FTransform WristPose =
        JointPoses[static_cast<uint8>(EIsdkHandBones::HandWristRoot)] * RootPose;
    DrawTransformAxis(WristPose);

    const FTransform PalmPose =
        JointPoses[static_cast<uint8>(EIsdkHandBones::HandPalm)] * WristPose;
    DrawTransformAxis(PalmPose);

    for (int JointIndex = 2; JointIndex < JointPoses.Num(); JointIndex++)
    {
      const FTransform Pose = JointPoses[JointIndex] * WristPose;
      DrawTransformAxis(Pose);
    }

    const auto HandJointMappings =
        IIsdkIHandJoints::Execute_GetHandJointMappings(JointsDataSource.GetObject());
    if (!HandJointMappings)
    {
      return;
    }

    // Some macros just to make the code below a little easier to read
#define Thumb(Joint) \
  UIsdkFunctionLibrary::GetThumbMappingIndex(HandJointMappings, EIsdkThumbJoint::Joint)
#define Finger(Type, Joint)                    \
  UIsdkFunctionLibrary::GetFingerMappingIndex( \
      HandJointMappings, EIsdkFingerType::Type, EIsdkFingerJoint::Joint)

    // HandJointMappings only contains finger/thumb data, so we still have to reference wrist by
    // index
    constexpr int WristIndex = 1;

    // In addition to drawing axes, we'll draw connecting lines from the wrist through the
    // fingertips
    TArray<TPair<int, int>> Segments = {
        {WristIndex, Thumb(Metacarpal)},
        {Thumb(Metacarpal), Thumb(Proximal)},
        {Thumb(Proximal), Thumb(Distal)},
        {Thumb(Distal), Thumb(Tip)},
        {WristIndex, Finger(Index, Metacarpal)},
        {Finger(Index, Metacarpal), Finger(Index, Proximal)},
        {Finger(Index, Proximal), Finger(Index, Intermediate)},
        {Finger(Index, Intermediate), Finger(Index, Distal)},
        {Finger(Index, Distal), Finger(Index, Tip)},
        {WristIndex, Finger(Middle, Metacarpal)},
        {Finger(Middle, Metacarpal), Finger(Middle, Proximal)},
        {Finger(Middle, Proximal), Finger(Middle, Intermediate)},
        {Finger(Middle, Intermediate), Finger(Middle, Distal)},
        {Finger(Middle, Distal), Finger(Middle, Tip)},
        {WristIndex, Finger(Ring, Metacarpal)},
        {Finger(Ring, Metacarpal), Finger(Ring, Proximal)},
        {Finger(Ring, Proximal), Finger(Ring, Intermediate)},
        {Finger(Ring, Intermediate), Finger(Ring, Distal)},
        {Finger(Ring, Distal), Finger(Ring, Tip)},
        {WristIndex, Finger(Pinky, Metacarpal)},
        {Finger(Pinky, Metacarpal), Finger(Pinky, Proximal)},
        {Finger(Pinky, Proximal), Finger(Pinky, Intermediate)},
        {Finger(Pinky, Intermediate), Finger(Pinky, Distal)},
        {Finger(Pinky, Distal), Finger(Pinky, Tip)},
    };

    const auto RuntimeSettings = GetDefault<UIsdkRuntimeSettings>();
    for (const auto& Pair : Segments)
    {
      const auto BoneStartIndex = Pair.Key;
      const auto BoneEndIndex = Pair.Value;

      if (BoneStartIndex < 0 || BoneStartIndex >= JointPoses.Num() || BoneEndIndex < 0 ||
          BoneEndIndex >= JointPoses.Num())
      {
        continue;
      }

      const auto SegmentStart = (JointPoses[BoneStartIndex] * WristPose).GetLocation();
      const auto SegmentEnd = (JointPoses[BoneEndIndex] * WristPose).GetLocation();
      DrawDebugLine(
          GetWorld(),
          SegmentStart,
          SegmentEnd,
          RuntimeSettings->HandDebugColor,
          false,
          0.f,
          0.f,
          RuntimeSettings->HandDebugThickness);
      UE_VLOG_SEGMENT_THICK(
          GetOwner(),
          LogOculusInteraction,
          Verbose,
          SegmentStart,
          SegmentEnd,
          RuntimeSettings->HandDebugColor,
          RuntimeSettings->HandDebugThickness,
          TEXT_EMPTY);
    }
  }
}

void UIsdkHandMeshComponent::SetHandPoseOverride(
    UIsdkHandPoseData* PoseDataIn,
    FTransform RootOffsetIn,
    FIsdkHandGrabPoseProperties& PosePropertiesIn,
    AActor*& InteractableActorIn)
{
  if (IsValid(PoseDataIn))
  {
    HandPoseDataOverride = PoseDataIn;
    bHandPoseOverridden = true;

    if (IsValid(InteractableActorIn))
    {
      RootOffset = RootOffsetIn;
      bOverrideRootPose = true;
      InteractableActor = InteractableActorIn;
      CurrentHandGrabPoseMode = PosePropertiesIn.PoseMode;
    }
    // Setup the location lerp
    if (CurrentHandGrabPoseMode == EIsdkHandGrabPoseMode::SnapPoseToObject &&
        PosePropertiesIn.SnapMoveDuration > 0.f)
    {
      HandLocationSnapStartTime = GetWorld()->GetRealTimeSeconds();
      HandLocationSnapDuration = PosePropertiesIn.SnapMoveDuration;
      HandLocationSnapStartTransform =
          IIsdkIRootPose::Execute_GetRootPose(RootPoseDataSource.GetObject());
      HandLocationLerpState = EIsdkLerpState::TransitioningTo;
    }

    // Setup the lerp if enabled on the Hand Pose Data
    if (PoseDataIn->GetPoseLerpTime() > 0.f && !bInhibitHandPoseLerping)
    {
      HandPoseLerpTime = PoseDataIn->GetPoseLerpTime();
      HandPoseLerpState = EIsdkLerpState::TransitioningTo;
    }
  }
}

void UIsdkHandMeshComponent::ResetHandPoseOverride()
{
  // Setup the lerp out, after which we'll finish the override
  if (HandPoseLerpTime > 0.f)
  {
    HandPoseLerpState = EIsdkLerpState::TransitioningAway;
  }
  else
  {
    bHandPoseOverridden = false;
    CurrentHandGrabPoseMode = EIsdkHandGrabPoseMode::None;
  }

  // If we're going to lerp the hand location away, we'll set bOverrideRootPose to false once we're
  // done
  if (HandLocationLerpState == EIsdkLerpState::RestingAtDestination)
  {
    HandLocationSnapStartTransform =
        ApplyRootPoseOffset(IIsdkIRootPose::Execute_GetRootPose(RootPoseDataSource.GetObject()));
    HandLocationSnapStartTime = GetWorld()->GetRealTimeSeconds();

    HandLocationLerpState = EIsdkLerpState::TransitioningAway;
  }
  else
  {
    bOverrideRootPose = false;
  }
}

void UIsdkHandMeshComponent::SetRootPoseIgnored(bool bRootPoseIgnored)
{
  bIgnoreRootPose = bRootPoseIgnored;
}

bool UIsdkHandMeshComponent::IsRootPoseIgnored() const
{
  return bIgnoreRootPose;
}

FTransform UIsdkHandMeshComponent::GetRootWorldTransform()
{
  if (MappedBoneNames.Num() == 0)
  {
    return FTransform::Identity;
  }
  const FName RootBoneName = MappedBoneNames[0];
  return GetBoneTransformByName(RootBoneName, EBoneSpaces::WorldSpace);
}

FTransform UIsdkHandMeshComponent::ApplyRootPoseOffset(FTransform OriginalTransform)
{
  FTransform NewTransform;

  // Using the Actor + Offset
  FTransform ActorTransform = InteractableActor->GetActorTransform();
  FVector UpdatedCompLocation = ActorTransform.GetRotation().RotateVector(RootOffset.GetLocation());
  NewTransform.SetLocation(ActorTransform.GetLocation() + UpdatedCompLocation);
  NewTransform.SetRotation(ActorTransform.GetRotation() * RootOffset.GetRotation());
  NewTransform.SetScale3D(OriginalTransform.GetScale3D());

  if (HandLocationLerpState == EIsdkLerpState::TransitioningTo && HandLocationSnapDuration != 0.f)
  {
    const float MoveProgressAlpha = FMath::Min(
        (GetWorld()->GetRealTimeSeconds() - HandLocationSnapStartTime) / HandLocationSnapDuration,
        1.f);
    NewTransform =
        UKismetMathLibrary::TLerp(HandLocationSnapStartTransform, NewTransform, MoveProgressAlpha);

    if (MoveProgressAlpha >= 1.f)
    {
      HandLocationLerpState = EIsdkLerpState::RestingAtDestination;
    }
  }
  else if (
      HandLocationLerpState == EIsdkLerpState::TransitioningAway && HandLocationSnapDuration != 0.f)
  {
    const float MoveProgressAlpha = FMath::Min(
        (GetWorld()->GetRealTimeSeconds() - HandLocationSnapStartTime) / HandLocationSnapDuration,
        1.f);
    NewTransform = UKismetMathLibrary::TLerp(
        HandLocationSnapStartTransform,
        IIsdkIRootPose::Execute_GetRootPose(RootPoseDataSource.GetObject()),
        MoveProgressAlpha);

    if (MoveProgressAlpha >= 1.f)
    {
      bOverrideRootPose = false;
      HandLocationLerpState = EIsdkLerpState::Inactive;
    }
  }

  return NewTransform;
}

void UIsdkHandMeshComponent::PushTransformsFromSkeletonToHandData()
{
  UIsdkHandData* HandDataSource =
      IIsdkIHandJoints::Execute_GetHandData(JointsDataSource.GetObject());
  if (IsValid(HandDataSource))
  {
    TArray<FTransform>& SourcePoses = HandDataSource->GetJointPoses();

    constexpr auto WristSpace = EBoneSpaces::Type::ComponentSpace;
    const int BoneCount = UIsdkHandData::GetNumJoints();
    for (int BoneId = 0; BoneId < BoneCount; BoneId++)
    {
      const FName BoneName = MappedBoneNames[BoneId];

      const FTransform BoneTransform = GetBoneTransformByName(BoneName, WristSpace);
      SourcePoses[BoneId].SetLocation(BoneTransform.GetLocation());
      SourcePoses[BoneId].SetRotation(BoneTransform.GetRotation());
    }
    MarkRefreshTransformDirty();
  }
}
