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

#include "DataSources/IsdkFromOpenXRControllerDataSource.h"
#include "IHandTracker.h"
#include "IsdkDataSourcesOpenXRLog.h"
#include "IXRTrackingSystem.h"
#include "DataSources/IsdkOpenXRHelper.h"
#include "Features/IModularFeatures.h"
#include "Utilities/IsdkXRUtils.h"
#include "Runtime/Launch/Resources/Version.h"
#include "VisualLogger/VisualLogger.h"

// Sets default values for this component's properties
UIsdkFromOpenXRControllerDataSource::UIsdkFromOpenXRControllerDataSource()
    : MotionController(nullptr), bIsLastGoodRootPoseValid(false), bIsLastGoodPointerPoseValid(false)
{
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
  PrimaryComponentTick.TickGroup = TG_PrePhysics;
  PrimaryComponentTick.SetTickFunctionEnable(true);

  IsRootPoseConnected = NewObject<UIsdkConditionalBool>(this, TEXT("IsRootPoseConnected"));
  IsRootPoseHighConfidence =
      NewObject<UIsdkConditionalBool>(this, TEXT("IsRootPoseHighConfidence"));
}

UIsdkFromOpenXRControllerDataSource*
UIsdkFromOpenXRControllerDataSource::MakeOpenXrControllerDataSource(
    AActor* Actor,
    UMotionControllerComponent* SourceMotionController,
    EIsdkHandedness InHandedness)
{
  if (!Actor)
  {
    checkf(
        false,
        TEXT(
            "Invalid Actor provided to UIsdkFromOpenXRControllerDataSource::MakeOpenXrControllerDataSource"));
    return nullptr;
  }

  if (!SourceMotionController)
  {
    checkf(
        false,
        TEXT(
            "Invalid SourceMotionController provided to UIsdkFromOpenXRControllerDataSource::MakeOpenXrControllerDataSource"));
    return nullptr;
  }

  auto* DataSourceComponent = NewObject<UIsdkFromOpenXRControllerDataSource>(Actor);
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
        LogIsdkDataSourcesOpenXR,
        Warning,
        TEXT(
            "UIsdkFromOpenXRControllerDataSource created with mismatching Handedness \"%s\" and MotionController MotionSource \"%s\""),
        *StaticEnum<EIsdkHandedness>()->GetValueAsString(EIsdkHandedness::Left),
        *SourceMotionController->GetTrackingMotionSource().ToString())
  }

  return DataSourceComponent;
}

void UIsdkFromOpenXRControllerDataSource::GetPointerPose_Implementation(
    FTransform& PointerPose,
    bool& IsValid)
{
  IsValid = bIsLastGoodPointerPoseValid;
  IsValid &= IsRootPoseConnected->GetResolvedValue();
  if (IsValid)
  {
    const FTransform ToPointerFromRoot = IsdkXRUtils::OXR::ControllerRelativePointerTransform;
    PointerPose = ToPointerFromRoot * MotionController->GetComponentTransform();
  }
}

void UIsdkFromOpenXRControllerDataSource::GetRelativePointerPose_Implementation(
    FTransform& PointerRelativePose,
    bool& IsValid)
{
  IsValid = bIsLastGoodPointerPoseValid;
  IsValid &= IsRootPoseConnected->GetResolvedValue();
  PointerRelativePose = FTransform::Identity;
}

FTransform UIsdkFromOpenXRControllerDataSource::GetRootPose_Implementation()
{
  FTransform ToRootFromController = IsdkXRUtils::OXR::ControllerRelativeRootTransform;
  return ToRootFromController * MotionController->GetComponentTransform();
}

bool UIsdkFromOpenXRControllerDataSource::IsRootPoseValid_Implementation()
{
  return bIsLastGoodRootPoseValid;
}

UIsdkConditional*
UIsdkFromOpenXRControllerDataSource::GetRootPoseConnectedConditional_Implementation()
{
  return IsRootPoseConnected;
}

void UIsdkFromOpenXRControllerDataSource::ReadControllerData()
{
  FTransform GripPose;
  if (GetGripPose(GripPose))
  {
    IsRootPoseHighConfidence->SetValue(true);
  }
  else
  {
    IsRootPoseHighConfidence->SetValue(false);
  }
  bIsLastGoodPointerPoseValid = true;
}

bool UIsdkFromOpenXRControllerDataSource::IsHandTrackingEnabled()
{
  if (auto& ModularFeatures = IModularFeatures::Get();
      ModularFeatures.IsModularFeatureAvailable(IHandTracker::GetModularFeatureName()))
  {
    const auto& HandTracker =
        ModularFeatures.GetModularFeature<IHandTracker>(IHandTracker::GetModularFeatureName());
    return HandTracker.IsHandTrackingStateValid();
  }
  return false;
}

bool UIsdkFromOpenXRControllerDataSource::GetGripPose(FTransform& OutGripPose)
{
  auto XRTrackingSystem = GEngine->XRSystem.Get();
  if (!XRTrackingSystem)
  {
    return false;
  }
  FQuat Orientation;
  FVector Position;
  if (XRTrackingSystem->GetCurrentPose(
          XRTrackingSystem->GetTrackingOrigin(), Orientation, Position))
  {
    OutGripPose = FTransform(Orientation, Position);
    return true;
  }
  return false;
}

// Called every frame
void UIsdkFromOpenXRControllerDataSource::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  // We consider controller root pose as being "connected" if hand tracking isn't active, so we'll
  // need to pull information about hand tracking state here.
  bool bIsHandDataValid = false;
  auto& ModularFeatures = IModularFeatures::Get();
  if (ModularFeatures.IsModularFeatureAvailable(IHandTracker::GetModularFeatureName()))
  {
    auto& HandTracker =
        ModularFeatures.GetModularFeature<IHandTracker>(IHandTracker::GetModularFeatureName());
    EControllerHand Hand =
        (Handedness == EIsdkHandedness::Left) ? EControllerHand::Left : EControllerHand::Right;
    OutPositions.Empty();
    OutRotations.Empty();
    OutRadii.Empty();

#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5)
    bool bIsTracked = false;
    bIsHandDataValid =
        HandTracker.GetAllKeypointStates(Hand, OutPositions, OutRotations, OutRadii, bIsTracked);
    bIsHandDataValid &= bIsTracked;
#else
    bool bIsHandDataValid =
        HandTracker.GetAllKeypointStates(Hand, OutPositions, OutRotations, OutRadii);
#endif
  }

  // Update root pose connection
  IsRootPoseConnected->SetValue(
      MotionController && !bIsHandDataValid && MotionController->IsTracked());

  // Update data source values based on whether controllers are connected or not
  FName DesiredSourceName;
  if (IsRootPoseConnected->GetResolvedValue())
  {
    LastGoodRootPose = GetRootPose_Implementation();
    bIsLastGoodRootPoseValid = true;
    bIsLastGoodPointerPoseValid = true;
    // We need to set the tracking source to aim pose for openxr controllers
    if (!bIsHandDataValid)
    {
      DesiredSourceName = Handedness == EIsdkHandedness::Left ? IsdkXRUtils::LeftAimSourceName
                                                              : IsdkXRUtils::RightAimSourceName;
      if (MotionController->GetTrackingMotionSource() != DesiredSourceName)
      {
        MotionController->SetTrackingMotionSource(DesiredSourceName);
      }
    }
    ReadControllerData();
  }
  else
  {
    DesiredSourceName = Handedness == EIsdkHandedness::Left ? IsdkXRUtils::LeftSourceName
                                                            : IsdkXRUtils::RightSourceName;
    if (MotionController->GetTrackingMotionSource() != DesiredSourceName)
    {
      MotionController->SetTrackingMotionSource(DesiredSourceName);
    }
    IsRootPoseHighConfidence->SetValue(false);
  }

#if !UE_BUILD_SHIPPING
  DebugLog();
#endif
}

UMotionControllerComponent* UIsdkFromOpenXRControllerDataSource::GetMotionController() const
{
  return MotionController;
}

bool UIsdkFromOpenXRControllerDataSource::IsPointerPoseValid_Implementation()
{
  return bIsLastGoodPointerPoseValid;
}

#if !UE_BUILD_SHIPPING
void UIsdkFromOpenXRControllerDataSource::DebugLog()
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
  UE_VLOG(GetOwner(), LogIsdkDataSourcesOpenXR, Verbose, TEXT("%s"), *RootPoseString);
}
#endif
