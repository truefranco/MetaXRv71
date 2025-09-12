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
#include "Components/PoseableMeshComponent.h"
#include "IsdkHandPoseData.h"
#include "StructTypes.h"
#include "IsdkHandMeshComponent.generated.h"

class IIsdkIRootPose;

// Forward declarations of internal types
namespace isdk::api
{
class IHandPositionFrame;
class ExternalHandPositionFrame;

namespace helper
{
class FExternalHandPositionFrameImpl;
}
} // namespace isdk::api

UENUM()
enum class EIsdkSkeletonMappingState : uint8
{
  // Possible to map
  None,
  // Tried to map but failed
  Invalid,
  // Mapping was successful
  Valid
};

UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Hand Mesh Component"))
class OCULUSINTERACTION_API UIsdkHandMeshComponent : public UPoseableMeshComponent
{
  GENERATED_BODY()

 public:
  UIsdkHandMeshComponent();
  virtual void BeginPlay() override;
  virtual void BeginDestroy() override;

  bool IsApiInstanceValid() const;

  // If this component's dependencies are ready, try and create/get the native hand position frame
  // instance. Otherwise, return nullptr.
  isdk::api::IHandPositionFrame* TryGetApiIHandPositionFrame() const;

  /* Returns the joints data source member variable (IsdkIHandJoints interface) */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  TScriptInterface<IIsdkIHandJoints> GetJointsDataSource() const
  {
    return JointsDataSource;
  }

  /* Returns the root pose data source member variable (IsdkIRootPose interface) */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  TScriptInterface<IIsdkIRootPose> GetRootPoseDataSource() const
  {
    return RootPoseDataSource;
  }

  /* Sets the joints data source member variable (IsdkIHandJoints interface) */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetJointsDataSource(TScriptInterface<IIsdkIHandJoints> InJointsDataSource);

  /* Sets the root pose data source member variable (IsdkIRootPose interface) */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetRootPoseDataSource(TScriptInterface<IIsdkIRootPose> InRootPoseDataSource);

  bool IsDataSourceRootPoseValid() const;
  bool IsDataSourceJointsValid() const;

  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  inline static constexpr size_t MappedBoneCount =
      static_cast<size_t>(EIsdkHandBones::EHandBones_MAX);

  /* Array of bone names that have been mapped (in indexed order) */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TArray<FName> MappedBoneNames;

  /* Get Handedness from JointDataSource */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  bool GetHandednessFromDataSource(EIsdkHandedness& HandednessOut) const
  {
    if (!IsValid(JointsDataSource.GetObject()))
    {
      return false;
    }
    HandednessOut = IIsdkIHandJoints::Execute_GetHandedness(JointsDataSource.GetObject());
    return true;
  }

  /* Set each value of the MappedBoneNames array to defaults for the provided hand mesh, for the
   given hand type. */
  UFUNCTION(CallInEditor, BlueprintCallable, Category = InteractionSDK)
  void SetMappedBoneNamesAsDefault();

  /* Set handedness and each value of the MappedBoneNames array using the joint source member
   * variable */
  UFUNCTION(CallInEditor, BlueprintCallable, Category = InteractionSDK)
  void SetMappedBoneNamesFromJointSourceHandedness()
  {
    if (!IsValid(JointsDataSource.GetObject()))
    {
      return;
    }
    EIsdkHandedness Handedness =
        IIsdkIHandJoints::Execute_GetHandedness(JointsDataSource.GetObject());
    SetMappedBoneNamesAsDefault();
  }

  /* Returns the current mapping state enum*/
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  EIsdkSkeletonMappingState GetMappingState() const
  {
    return MappingState;
  }

  /* Resets the mapping state and clears the previously set skeleton */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void ClearMappingState()
  {
    ResetAllBoneTransforms();
    MappingState = EIsdkSkeletonMappingState::None;
    MappedSkeleton = nullptr;
  }

  /* Sets an override with a given HandPoseData, will start a lerp in (if enabled on the data) */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetHandPoseOverride(
      UIsdkHandPoseData* PoseDataIn,
      FTransform RootOffsetIn,
      FIsdkHandGrabPoseProperties& PosePropertiesIn,
      AActor*& InteractableActorIn);

  /* Removes the current hand pose override, will start a lerp out (if enabled on the data)*/
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void ResetHandPoseOverride();

  /* */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  FTransform GetRootWorldTransform();

  /* Sets new value for whether or not the HandMeshComponent should push updates to the skeleton on
   * tick */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetPauseSkeletonUpdates(bool bNewSkeletonPauseState)
  {
    bPauseSkeletonUpdates = bNewSkeletonPauseState;
  }

  /**
   * Sets whether this hand mesh should ignore root pose hand data when determining its world
   * position
   */
  void SetRootPoseIgnored(bool bRootPoseIgnored);

  // Returns true if this hand mesh is ignoring root pose hand data
  bool IsRootPoseIgnored() const;

  /* Show debug markers for hand skeleton bones */
  UPROPERTY(EditAnywhere, Category = InteractionSDK)
  bool bShowDebugSkeleton = false;

  /* Copies data from the poseable mesh back to the IsdkHandData source associated with this asset
   * (will override current data)*/
  void PushTransformsFromSkeletonToHandData();

 protected:
  /* Hand Pose is currently being overridden */
  UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = InteractionSDK)
  bool bHandPoseOverridden = false;

  /* If enabled, will ignore lerp values set on hand poses */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  bool bInhibitHandPoseLerping = false;

  // If true, root pose will be ignored.  Used for controller hands.
  bool bIgnoreRootPose = false;

 private:
  int MappedBoneIndices[MappedBoneCount];

  UPROPERTY(
      VisibleAnywhere,
      Transient,
      BlueprintGetter = GetMappingState,
      Category = InteractionSDK)
  EIsdkSkeletonMappingState MappingState;

  UPROPERTY(Transient)
  TObjectPtr<UObject> MappedSkeleton;

  void SetInvalidMappingState(UObject* Skeleton)
  {
    MappingState = EIsdkSkeletonMappingState::Invalid;
    MappedSkeleton = Skeleton;
  }
  void SetValidMappingState(UObject* Skeleton)
  {
    if (!IsValid(Skeleton))
    {
      MappingState = EIsdkSkeletonMappingState::Invalid;
      UE_LOG(LogOculusInteraction, Error, TEXT("SetValidMappingState() - MappedSkeleton is null!"));
      return;
    }
    MappingState = EIsdkSkeletonMappingState::Valid;
    MappedSkeleton = Skeleton;
  }
  void ResetAllBoneTransforms();

  // If there are no bones with NAME_None, assume it is valid
  bool AreMappedBoneNamesValid() const
  {
    for (auto Name : MappedBoneNames)
    {
      if (Name == NAME_None)
      {
        return false;
      }
    }
    return true;
  }

  UObject* GetSkeletalMesh() const;

  void InitializeSkeleton();
  void UpdateMappingState();
  void UpdateSkeleton();
  void UpdateApiHandPositionFrame(isdk::api::ExternalHandPositionFrame& ApiHandPositionFrame) const;

  void DrawTransformAxis(const FTransform& Pose) const;
  void DrawDebugSkeleton() const;
  FTransform ApplyRootPoseOffset(FTransform OriginalTransform);

  UPROPERTY(
      BlueprintReadWrite,
      BlueprintGetter = GetJointsDataSource,
      BlueprintSetter = SetJointsDataSource,
      Category = InteractionSDK,
      meta = (AllowPrivateAccess = true))
  TScriptInterface<IIsdkIHandJoints> JointsDataSource;

  UPROPERTY(
      BlueprintReadWrite,
      BlueprintGetter = GetRootPoseDataSource,
      BlueprintSetter = SetRootPoseDataSource,
      Category = InteractionSDK,
      meta = (AllowPrivateAccess = true))
  TScriptInterface<IIsdkIRootPose> RootPoseDataSource;

  TPimplPtr<isdk::api::helper::FExternalHandPositionFrameImpl> ExternalHandPositionFrameImpl;

  UPROPERTY()
  TObjectPtr<UIsdkHandPoseData> HandPoseDataOverride;

 private:
  EIsdkLerpState HandPoseLerpState = EIsdkLerpState::Inactive;
  float HandPoseLerpTime = 0.f;
  float HandPoseLerpTimeCount = 0.f;
  float HandPoseLerpAlpha = 0.f;

  EIsdkLerpState HandLocationLerpState = EIsdkLerpState::Inactive;
  FTransform HandLocationSnapStartTransform;
  float HandLocationSnapStartTime = 0.f;
  float HandLocationSnapDuration = 0.f;
  bool bOverrideRootPose = false;
  bool bPauseSkeletonUpdates = false;

  EIsdkHandGrabPoseMode CurrentHandGrabPoseMode = EIsdkHandGrabPoseMode::None;
  FTransform RootOffset;
  TObjectPtr<AActor> InteractableActor;
};
