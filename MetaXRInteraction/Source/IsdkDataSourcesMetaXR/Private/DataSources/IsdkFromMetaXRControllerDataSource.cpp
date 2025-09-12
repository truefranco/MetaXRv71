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

#include "DataSources/IsdkFromMetaXRControllerDataSource.h"

#include "IsdkDataSourcesMetaXR.h"
#include "IsdkDataSourcesMetaXRLog.h"
#include "IsdkDataSourcesMetaXRSubsystem.h"
#include "IsdkOculusXRHelper.h"
#include "IsdkRuntimeSettings.h"
#include "Core/IsdkConditionalBool.h"
#include "Utilities/IsdkXRUtils.h"
#include "VisualLogger/VisualLogger.h"

UIsdkFromMetaXRControllerDataSource::UIsdkFromMetaXRControllerDataSource()
    : MotionController(nullptr)
{
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
  PrimaryComponentTick.TickGroup = TG_PrePhysics;
  PrimaryComponentTick.SetTickFunctionEnable(true);

  IsRootPoseConnected = NewObject<UIsdkConditionalBool>(this, TEXT("IsRootPoseConnected"));
  IsRootPoseHighConfidence =
      NewObject<UIsdkConditionalBool>(this, TEXT("IsRootPoseHighConfidence"));
}

UIsdkFromMetaXRControllerDataSource*
UIsdkFromMetaXRControllerDataSource::MakeMetaXRControllerDataSource(
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

  auto* DataSourceComponent = NewObject<UIsdkFromMetaXRControllerDataSource>(Actor);
  DataSourceComponent->MotionController = SourceMotionController;
  DataSourceComponent->AddTickPrerequisiteComponent(SourceMotionController);
  DataSourceComponent->Handedness = InHandedness;
  DataSourceComponent->bUpdateInTick = true;
  DataSourceComponent->RegisterComponent();

  const bool bLeftHandMismatch = InHandedness == EIsdkHandedness::Left &&
      SourceMotionController->GetTrackingMotionSource() != IMotionController::LeftHandSourceId;
  const bool bRightHandMismatch = InHandedness == EIsdkHandedness::Right &&
      SourceMotionController->GetTrackingMotionSource() != IMotionController::RightHandSourceId;
  if (bLeftHandMismatch || bRightHandMismatch)
  {
    UE_LOG(
        LogIsdkDataSourcesMetaXR,
        Warning,
        TEXT(
            "UIsdkFromMetaXRControllerDataSource created with mismatching Handedness \"%s\" and MotionController MotionSource \"%s\""),
        *StaticEnum<EIsdkHandedness>()->GetValueAsString(EIsdkHandedness::Left),
        *SourceMotionController->GetTrackingMotionSource().ToString())
  }

  return DataSourceComponent;
}

UMotionControllerComponent* UIsdkFromMetaXRControllerDataSource::GetMotionController() const
{
  return MotionController;
}

// Called every frame
void UIsdkFromMetaXRControllerDataSource::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  bool bIsHoldingAController = FIsdkOculusXRHelper::IsHoldingAController(GetWorld());

  // Checking if the motion controller is being tracked solves the "static in mid air" problem
  // when switching from hands to controllers, when only one controller has connected.
  IsRootPoseConnected->SetValue(
      MotionController && bIsHoldingAController && MotionController->IsTracked());

  if (IsRootPoseConnected->GetResolvedValue())
  {
    LastGoodRootPose = MotionController->GetComponentTransform();
    bIsLastGoodRootPoseValid = true;
    bIsLastGoodPointerPoseValid = true;

    ReadControllerData();
  }
  else
  {
    IsRootPoseHighConfidence->SetValue(false);
  }

#if !UE_BUILD_SHIPPING
  DebugLog();
#endif
}

void UIsdkFromMetaXRControllerDataSource::ReadControllerData()
{
  // `FIsdkOculusXRHelper::GetPointerPose` always returns the **hand** pointer pose
  // and after the switch the last known pointer pose.
  // The validity check using `UOculusXRInputFunctionLibrary::IsPointerPoseValid` returns
  // either "is the palm pointing towards the HMD" or the value before the hand tracking was lost.
  // Which means it is currently unrealiable, and that is why it is not used here.

  RelativePointerPose = IsdkXRUtils::OVR::ControllerRelativePointerTransform;
  IsRootPoseHighConfidence->SetValue(true);
}

bool UIsdkFromMetaXRControllerDataSource::IsPointerPoseValid_Implementation()
{
  return bIsLastGoodPointerPoseValid && FIsdkOculusXRHelper::IsHoldingAController(GetWorld());
}

void UIsdkFromMetaXRControllerDataSource::GetPointerPose_Implementation(
    FTransform& PointerPose,
    bool& IsValid)
{
  IsValid = bIsLastGoodPointerPoseValid;
  IsValid &= IsRootPoseConnected->GetResolvedValue();
  IsValid &= FIsdkOculusXRHelper::IsHoldingAController(GetWorld());
  if (IsValid)
  {
    const auto& MetaXRModule = FIsdkDataSourcesMetaXRModule::GetChecked();
    const auto PoseType = MetaXRModule.Input_GetControllerDrivenHandPoses();

    if (PoseType == EIsdkXRControllerDrivenHandPoseType::None)
    {
      PointerPose = RelativePointerPose * MotionController->GetComponentTransform();
    }
    else
    {
      FTransform ToControllerFromWorld = MotionController->GetComponentTransform();
      FTransform ToPointerFromController =
          FIsdkOculusXRHelper::GetPointerPose(Handedness, MotionController);
      PointerPose = ToPointerFromController * ToControllerFromWorld;
    }
  }
}

void UIsdkFromMetaXRControllerDataSource::GetRelativePointerPose_Implementation(
    FTransform& PointerRelativePose,
    bool& IsValid)
{
  IsValid = bIsLastGoodPointerPoseValid;
  IsValid &= IsRootPoseConnected->GetResolvedValue();
  if (IsValid)
  {
    PointerRelativePose = RelativePointerPose;
  }
}

FTransform UIsdkFromMetaXRControllerDataSource::GetRootPose_Implementation()
{
  const auto& MetaXRModule = FIsdkDataSourcesMetaXRModule::GetChecked();
  const auto PoseType = MetaXRModule.Input_GetControllerDrivenHandPoses();

  if (PoseType == EIsdkXRControllerDrivenHandPoseType::None)
  {
    const auto ToRootFromController = IsdkXRUtils::OVR::ControllerRelativeRootTransform;
    return ToRootFromController * MotionController->GetComponentTransform();
  }

  FTransform ToControllerFromWorld = MotionController->GetComponentTransform();
  const auto ToPointerFromController =
      FIsdkOculusXRHelper::GetPointerPose(Handedness, MotionController);
  const FTransform ToRootFromPointer = {
      FRotator(0.f, 0.f, 0.f),
      -IsdkXRUtils::OVR::ControllerRelativePointerOffset,
      FVector::OneVector};

  return ToRootFromPointer * ToPointerFromController * ToControllerFromWorld;
}

bool UIsdkFromMetaXRControllerDataSource::IsRootPoseValid_Implementation()
{
  return bIsLastGoodRootPoseValid;
}

UIsdkConditional*
UIsdkFromMetaXRControllerDataSource::GetRootPoseConnectedConditional_Implementation()
{
  return IsRootPoseConnected;
}

#if !UE_BUILD_SHIPPING

void UIsdkFromMetaXRControllerDataSource::DebugLog()
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
          "(%s Controller) IsRootPoseConnected->GetResolvedValue = %d \n MotionController->IsTracked() = %d \n IsPointerPoseValid() = %d \n IsRootPoseValid() = %d \n GetPointerPose() (IsValid) = %d \n IsHandJointDataValid() = %d \n IsValid(HandData) = %d \n IsValid(GetHandData()) = %d"),
      *HandString,
      _bIsRootPoseConnected ? 1 : 0,
      _bIsMotionControllerTracked ? 1 : 0,
      _bIsPointerPoseValid ? 1 : 0,
      _bIsRootPoseValid ? 1 : 0,
      _bPointerPose_IsValid ? 1 : 0,
      _bIsHandJointMappingValid ? 1 : 0,
      _bIsValid_HandData ? 1 : 0,
      _bIsValid_GetHandData ? 1 : 0);
  UE_VLOG(GetOwner(), LogIsdkDataSourcesMetaXR, Verbose, TEXT("%s"), *RootPoseString);
}
#endif
