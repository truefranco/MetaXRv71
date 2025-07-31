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

#include "DataSources/IsdkFromOpenXRHandDataSource.h"
#include "DrawDebugHelpers.h"
#include "IHandTracker.h"
#include "XRMotionControllerBase.h"
#include "IsdkFunctionLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IsdkDataSourcesOpenXRLog.h"
#include "DataSources/IsdkOpenXRHelper.h"
#include "Features/IModularFeatures.h"
#include "Utilities/IsdkXRUtils.h"
#include "Runtime/Launch/Resources/Version.h"
#include "VisualLogger/VisualLogger.h"

// Sets default values for this component's properties
UIsdkFromOpenXRHandDataSource::UIsdkFromOpenXRHandDataSource()
{
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
  PrimaryComponentTick.TickGroup = TG_PrePhysics;
  PrimaryComponentTick.SetTickFunctionEnable(true);

  IsRootPoseConnected = NewObject<UIsdkConditionalBool>(this, TEXT("IsRootPoseConnected"));
  IsRootPoseHighConfidence =
      NewObject<UIsdkConditionalBool>(this, TEXT("IsRootPoseHighConfidence"));

  bIsLastGoodRootPoseValid = false;
  bIsLastGoodPointerPoseValid = false;
  bAllowLowConfidenceData = false;
  bHasLastKnownGood = false;
  bWantsInitializeComponent = true;

  DefaultJointRadii = UIsdkFunctionLibrary::GetDefaultJointRadii();
}

UIsdkFromOpenXRHandDataSource* UIsdkFromOpenXRHandDataSource::MakeOpenXRHandDataSource(
    AActor* Actor,
    UMotionControllerComponent* SourceMotionController,
    EIsdkHandedness InHandedness)
{
  if (!Actor)
  {
    checkf(
        false,
        TEXT(
            "Invalid Actor provided to UIsdkFromOpenXRHandDataSource::MakeOpenXrControllerDataSource"));
    return nullptr;
  }

  if (!SourceMotionController)
  {
    checkf(
        false,
        TEXT(
            "Invalid SourceMotionController provided to UIsdkFromOpenXRHandDataSource::MakeOpenXrControllerDataSource"));
    return nullptr;
  }

  auto* DataSourceComponent = NewObject<UIsdkFromOpenXRHandDataSource>(Actor);
  DataSourceComponent->MotionController = SourceMotionController;
  DataSourceComponent->AddTickPrerequisiteComponent(SourceMotionController);
  DataSourceComponent->Handedness = InHandedness;
  DataSourceComponent->bUpdateInTick = true;
  DataSourceComponent->SetAllowInvalidTrackedData(false);
  DataSourceComponent->RegisterComponent();

  const bool bLeftHandMismatch = InHandedness == EIsdkHandedness::Left &&
      SourceMotionController->MotionSource != FXRMotionControllerBase::LeftHandSourceId;
  const bool bRightHandMismatch = InHandedness == EIsdkHandedness::Right &&
      SourceMotionController->MotionSource != FXRMotionControllerBase::RightHandSourceId;
  if (bLeftHandMismatch || bRightHandMismatch)
  {
    UE_LOG(
        LogIsdkDataSourcesOpenXR,
        Warning,
        TEXT(
            "UIsdkFromOpenXRHandDataSource created with mismatching Handedness \"%s\" and MotionController MotionSource \"%s\""),
        *StaticEnum<EIsdkHandedness>()->GetValueAsString(EIsdkHandedness::Left),
        *SourceMotionController->MotionSource.ToString())
  }

  return DataSourceComponent;
}

void UIsdkFromOpenXRHandDataSource::InitializeComponent()
{
  Super::InitializeComponent();

  const auto ThumbJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRThumbMapping();
  const auto FingerJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRFingerMapping();

  SetHandJointMappings(ThumbJointMappings, FingerJointMappings);
}

// Called every frame
void UIsdkFromOpenXRHandDataSource::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (!IsValid(MotionController))
  {
    return;
  }

  auto& ModularFeatures = IModularFeatures::Get();
  if (!ModularFeatures.IsModularFeatureAvailable(IHandTracker::GetModularFeatureName()))
  {
    return;
  }
  auto& HandTracker =
      ModularFeatures.GetModularFeature<IHandTracker>(IHandTracker::GetModularFeatureName());
  bool bIsHandTrackingEnabled = HandTracker.IsHandTrackingStateValid();
  EControllerHand Hand =
      (Handedness == EIsdkHandedness::Left) ? EControllerHand::Left : EControllerHand::Right;
  OutPositions.Empty();
  OutRotations.Empty();
  OutRadii.Empty();

#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5)
  bool bIsTracked = false;
  bool bIsHandDataValid =
      HandTracker.GetAllKeypointStates(Hand, OutPositions, OutRotations, OutRadii, bIsTracked);
  bIsHandDataValid &= bIsTracked;
#else
  bool bIsHandDataValid =
      HandTracker.GetAllKeypointStates(Hand, OutPositions, OutRotations, OutRadii);
#endif

  FName DesiredSourceName;
  if (bIsHandTrackingEnabled && bIsHandDataValid)
  {
    DesiredSourceName = Handedness == EIsdkHandedness::Left ? IsdkXRUtils::LeftSourceName
                                                            : IsdkXRUtils::RightSourceName;
    // Update the motion controller's tracking source if necessary
    if (MotionController->MotionSource != DesiredSourceName)
    {
      MotionController->SetTrackingMotionSource(DesiredSourceName);
    }
    ReadHandData();
  }
  else
  {
    DesiredSourceName = Handedness == EIsdkHandedness::Left ? IsdkXRUtils::LeftAimSourceName
                                                            : IsdkXRUtils::RightAimSourceName;
    if (MotionController->MotionSource != DesiredSourceName)
    {
      MotionController->SetTrackingMotionSource(DesiredSourceName);
    }
    IsRootPoseHighConfidence->SetValue(false);
    bIsLastGoodPointerPoseValid = false;
    bIsLastGoodRootPoseValid = false;
    bIsHandDataValid = false;
    bIsHandTrackingEnabled = false;
  }
  IsRootPoseConnected->SetValue(MotionController && bIsHandDataValid && bIsHandTrackingEnabled);

#if !UE_BUILD_SHIPPING
  DebugLog();
#endif
}

bool UIsdkFromOpenXRHandDataSource::IsPointerPoseValid_Implementation()
{
  return bIsLastGoodPointerPoseValid;
}

bool UIsdkFromOpenXRHandDataSource::IsRootPoseValid_Implementation()
{
  return bIsLastGoodRootPoseValid;
}

FTransform UIsdkFromOpenXRHandDataSource::GetRootPose_Implementation()
{
  FTransform MotionTransform = MotionController->GetComponentTransform();
  MotionTransform.SetRotation(
      MotionTransform.GetRotation() * IsdkXRUtils::OXR::HandRootFixupRotation);
  FTransform WristOffset = Handedness == EIsdkHandedness::Left
      ? FTransform(IsdkXRUtils::OXR::HandRootWristOffsetLeft)
      : FTransform(IsdkXRUtils::OXR::HandRootWristOffsetRight);
  return WristOffset * MotionTransform;
}

void UIsdkFromOpenXRHandDataSource::GetPointerPose_Implementation(
    FTransform& PointerPose,
    bool& IsValid)
{
  if (MotionController)
  {
    PointerPose = RelativePointerPose * MotionController->GetComponentTransform();
    IsValid = bIsLastGoodPointerPoseValid;
  }
  else
  {
    IsValid = false;
  }
}

void UIsdkFromOpenXRHandDataSource::GetRelativePointerPose_Implementation(
    FTransform& PointerRelativePose,
    bool& IsValid)
{
  PointerRelativePose = RelativePointerPose;
  IsValid = bIsLastGoodPointerPoseValid;
}

UMotionControllerComponent* UIsdkFromOpenXRHandDataSource::GetMotionController() const
{
  return MotionController;
}

bool UIsdkFromOpenXRHandDataSource::GetAllowInvalidTrackedData() const
{
  return bAllowLowConfidenceData;
}

UIsdkConditional* UIsdkFromOpenXRHandDataSource::GetRootPoseConnectedConditional_Implementation()
{
  return IsRootPoseConnected;
}

UIsdkConditional*
UIsdkFromOpenXRHandDataSource::GetRootPoseHighConfidenceConditional_Implementation()
{
  return IsRootPoseHighConfidence;
}

void UIsdkFromOpenXRHandDataSource::SetMotionController(
    UMotionControllerComponent* InMotionController)
{
  if (IsValid(MotionController))
  {
    RemoveTickPrerequisiteComponent(MotionController);
  }
  MotionController = InMotionController;

  // Ensure that this component ticks after the input component
  if (IsValid(MotionController))
  {
    AddTickPrerequisiteComponent(MotionController);
    ReadHandedness();
  }
}

void UIsdkFromOpenXRHandDataSource::SetAllowInvalidTrackedData(bool bInAllowInvalidTrackedData)
{
  bAllowLowConfidenceData = bInAllowInvalidTrackedData;
}

void UIsdkFromOpenXRHandDataSource::ReadHandData()
{
  FXRMotionControllerData MotionControllerData;
  EControllerHand Hand =
      (Handedness == EIsdkHandedness::Left) ? EControllerHand::Left : EControllerHand::Right;

  UHeadMountedDisplayFunctionLibrary::GetMotionControllerData(this, Hand, MotionControllerData);

  if (MotionControllerData.bValid && !MotionControllerData.HandKeyPositions.IsEmpty())
  {
    bHasLastKnownGood = true;
    LastGoodRootPose = GetRootPose_Implementation();
    IsRootPoseHighConfidence->SetValue(true);
    bIsLastGoodRootPoseValid = true;
    bIsLastGoodPointerPoseValid = true;
    // Offset for pointer pose
    FVector Offset = (Handedness == EIsdkHandedness::Left)
        ? IsdkXRUtils::OXR::HandRootWristOffsetLeft
        : IsdkXRUtils::OXR::HandRootWristOffsetRight;
    FTransform RootTransformInverse = FTransform(Offset).Inverse();
    RootTransformInverse.AddToTranslation(IsdkXRUtils::OXR::HandRelativePointerOffset);
    RelativePointerPose = FTransform(
        IsdkXRUtils::OXR::HandRelativePointerRotation, RootTransformInverse.GetTranslation());

    //  Update Joint Poses
    TArray<FTransform>& JointPoses = HandData->GetJointPoses();

    auto RootPose = GetRootPose_Implementation();
    auto RootPoseInverse = RootPose.Inverse();

    JointPoses[1] = FTransform::Identity;
    for (int32 Index = 0; Index < MotionControllerData.HandKeyPositions.Num(); ++Index)
    {
      if (Index == 1) // Wrist pose added above
      {
        continue;
      }

      auto AdjustedTransform = FTransform(
          MotionControllerData.HandKeyRotations[Index],
          MotionControllerData.HandKeyPositions[Index],
          FVector::One());
      AdjustedTransform = AdjustedTransform * RootPoseInverse;
      AdjustedTransform.SetRotation(
          AdjustedTransform.GetRotation() * IsdkXRUtils::OXR::HandJointFixupRotation);
      JointPoses[Index] = AdjustedTransform;
    }
    if (IsValid(HandDataInbound))
    {
      HandDataInbound->SetCachedJointPoses(JointPoses);
    }
    // set the bone radii
    TArray<float>& JointRadii = HandData->GetJointRadii();
    float HandScale = LastGoodRootPose.GetScale3D().X; // Scale should be the same for all axis
    for (int JointIndex = 0; JointIndex < JointRadii.Num(); JointIndex++)
    {
      JointRadii[JointIndex] = DefaultJointRadii[JointIndex] * HandScale;
    }
    SetImplHandData(LastGoodRootPose);
    bIsHandJointDataValid = true;
  }
  else if (bHasLastKnownGood)
  {
    // No high confidence data, keep state from last known good.
  }
  else
  {
    // No high confidence data, no available last known good.
    IsRootPoseHighConfidence->SetValue(false);
    bIsLastGoodRootPoseValid = false;
    bIsLastGoodPointerPoseValid = false;
    bIsHandJointDataValid = false;
    RelativePointerPose = {};
  }
}

void UIsdkFromOpenXRHandDataSource::ReadHandedness()
{
  Handedness = FIsdkOpenXRHelper::ReadHandedness(MotionController);
}

#if !UE_BUILD_SHIPPING
void UIsdkFromOpenXRHandDataSource::DebugLog()
{
  const auto HandString =
      StaticEnum<EIsdkHandedness>()->GetNameStringByValue(static_cast<int64>(Handedness));
  const bool _bIsRootPoseConnected = IsRootPoseConnected->GetResolvedValue();
  const bool _bIsMotionControllerTracked = MotionController && MotionController->IsTracked();
  const bool _bIsPointerPoseValid = IsPointerPoseValid_Implementation();
  const bool _bIsRootPoseValid = IsRootPoseValid_Implementation();
  bool _bPointerPose_IsValid;
  FTransform _PointerPose;
  GetPointerPose_Implementation(_PointerPose, _bPointerPose_IsValid);
  const bool _bIsHandJointMappingValid = Execute_IsHandJointDataValid(this);
  const bool _bIsValid_HandData = IsValid(HandData);
  const bool _bIsValid_GetHandData = IsValid(Execute_GetHandData(this));

  const FString RootPoseString = FString::Printf(
      TEXT(
          "(%s Hand) IsRootPoseConnected->GetResolvedValue = %d \n MotionController->IsTracked() = %d \n IsPointerPoseValid() = %d \n IsRootPoseValid() = %d \n GetPointerPose() (IsValid) = %d \n IsHandJointDataValid() = %d \n IsValid(HandData) = %d \n IsValid(GetHandData()) = %d"),
      *HandString,
      _bIsRootPoseConnected ? 1 : 0,
      _bIsMotionControllerTracked ? 1 : 0,
      _bIsPointerPoseValid ? 1 : 0,
      _bIsRootPoseValid ? 1 : 0,
      _bPointerPose_IsValid ? 1 : 0,
      _bIsHandJointMappingValid ? 1 : 0,
      _bIsValid_HandData ? 1 : 0,
      _bIsValid_GetHandData ? 1 : 0);
  UE_VLOG(GetOwner(), LogIsdkDataSourcesOpenXR, Verbose, TEXT("%s"), *RootPoseString);
}
#endif
