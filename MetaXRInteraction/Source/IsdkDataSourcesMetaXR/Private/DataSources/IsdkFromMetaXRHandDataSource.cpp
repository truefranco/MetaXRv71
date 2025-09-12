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

#include "DataSources/IsdkFromMetaXRHandDataSource.h"

#include "DrawDebugHelpers.h"
#include "IsdkDataSourcesMetaXR.h"
#include "IsdkDataSourcesMetaXRLog.h"
#include "IsdkOculusXRHelper.h"
#include "Core/IsdkConditionalBool.h"
#include "IsdkFunctionLibrary.h"
#include "Utilities/IsdkXRUtils.h"
#include "VisualLogger/VisualLogger.h"

UIsdkFromMetaXRHandDataSource::UIsdkFromMetaXRHandDataSource() : MotionController(nullptr)
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

UIsdkFromMetaXRHandDataSource* UIsdkFromMetaXRHandDataSource::MakeMetaXRHandDataSource(
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

  auto* DataSourceComponent = NewObject<UIsdkFromMetaXRHandDataSource>(Actor);
  DataSourceComponent->MotionController = SourceMotionController;
  DataSourceComponent->AddTickPrerequisiteComponent(SourceMotionController);
  DataSourceComponent->Handedness = InHandedness;
  DataSourceComponent->bUpdateInTick = true;
  DataSourceComponent->SetAllowInvalidTrackedData(false);
  DataSourceComponent->RegisterComponent();

  // MetaXR's OculusXRHandComponent is being used to help us generate the OXR shaped data while
  // the Skeleton V2 API is not available in Unreal. We could have transformed the current OVR
  // data into OXR by transforming the rotations but we would still need the joint length
  // information plus transform from joint local space to wrist space ourselves, but this is
  // mostly taken care of by using this component.
  const auto ComponentClass =
      TSubclassOf<UActorComponent>(FindFirstObjectSafe<UClass>(TEXT("OculusXRHandComponent")));
  if (ComponentClass)
  {
    auto* OculusHandComponent = Cast<UPoseableMeshComponent>(
        Actor->AddComponentByClass(ComponentClass, false, FTransform::Identity, true));
    uint8 HandIndex = static_cast<uint8>(DataSourceComponent->Handedness) + 1;
    SetUintEnumProperty(OculusHandComponent, "SkeletonType", HandIndex);
    SetUintEnumProperty(OculusHandComponent, "MeshType", HandIndex);
    OculusHandComponent->SetVisibility(false);
    OculusHandComponent->bHiddenInGame = true;
    Actor->FinishAddComponent(OculusHandComponent, true, FTransform::Identity);

    DataSourceComponent->OculusXrHandComponent = OculusHandComponent;
  }
  else
  {
    ensureMsgf(
        ComponentClass,
        TEXT("OculusXRHandComponent must be defined, or hand tracking will not work"));
  }

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
            "UIsdkFromMetaXRHandDataSource created with mismatching Handedness \"%s\" and MotionController MotionSource \"%s\""),
        *StaticEnum<EIsdkHandedness>()->GetValueAsString(EIsdkHandedness::Left),
        *SourceMotionController->GetTrackingMotionSource().ToString())
  }

  return DataSourceComponent;
}

void UIsdkFromMetaXRHandDataSource::InitializeComponent()
{
  Super::InitializeComponent();

  const auto ThumbJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRThumbMapping();
  const auto FingerJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRFingerMapping();

  SetHandJointMappings(ThumbJointMappings, FingerJointMappings);
}

void UIsdkFromMetaXRHandDataSource::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  const auto& MetaXRModule = FIsdkDataSourcesMetaXRModule::GetChecked();

  // Hand data may be used for posing ControllerHands when using OVR, so we may need to read
  // hand data even if we are currently holding a controller
  const auto PoseType = MetaXRModule.Input_GetControllerDrivenHandPoses();
  const bool bIsControllerHeld = FIsdkOculusXRHelper::IsHoldingAController(GetWorld());
  const bool bShouldReadHandJointData =
      !bIsControllerHeld || PoseType != EIsdkXRControllerDrivenHandPoseType::None;

  if (bShouldReadHandJointData && IsValid(MotionController))
  {
    ReadHandData();
  }
  else
  {
    IsRootPoseHighConfidence->SetValue(false);
    bIsLastGoodPointerPoseValid = false;
    bIsLastGoodRootPoseValid = false;
  }

  // Checking if the motion controller is being tracked solves the "static in mid air" problem
  // when switching from hands to controllers, when only one controller has connected.
  IsRootPoseConnected->SetValue(
      MotionController && !bIsControllerHeld && MotionController->IsTracked());

#if !UE_BUILD_SHIPPING
  DebugLog();
#endif
}

bool UIsdkFromMetaXRHandDataSource::IsPointerPoseValid_Implementation()
{
  return bIsLastGoodPointerPoseValid && !FIsdkOculusXRHelper::IsHoldingAController(GetWorld());
}

bool UIsdkFromMetaXRHandDataSource::IsRootPoseValid_Implementation()
{
  return IsRootPoseConnected->GetResolvedValue();
}

void UIsdkFromMetaXRHandDataSource::ReadHandData()
{
  const auto& MetaXRModule = FIsdkDataSourcesMetaXRModule::GetChecked();

  const bool bIsHighConfidenceData = MetaXRModule.Input_IsTrackingHighConfidence(Handedness);

  bool CanUseHandData = MetaXRModule.Input_IsHandPositionValid(Handedness);
  CanUseHandData &= bAllowLowConfidenceData || bIsHighConfidenceData;
  if (CanUseHandData)
  {
    if (BoneMap.IsEmpty())
    {
      GenerateBoneMap();
    }

    bHasLastKnownGood = true;

    // Root Pose
    LastGoodRootPose = GetRootPose_Implementation();
    IsRootPoseHighConfidence->SetValue(bIsHighConfidenceData);
    bIsLastGoodRootPoseValid = true;

    // Pointer pose
    RelativePointerPose = FIsdkOculusXRHelper::GetPointerPose(Handedness, MotionController);
    bIsLastGoodPointerPoseValid = !RelativePointerPose.Equals(FTransform());

    // set the wrist bone pose
    TArray<FTransform>& JointPoses = HandData->GetJointPoses();

    // Required to fix the orientation of the hands
    // As far as I can tell can be removed when the skeleton V2 is shipped
    const bool bIsLeft = Handedness == EIsdkHandedness::Left;
    const FQuat OVRToOXRRotation =
        bIsLeft ? IsdkXRUtils::OVR::OVRToOXRLeft : IsdkXRUtils::OVR::OVRToOXRRight;
    const FQuat InvWristRotation = bIsLeft ? IsdkXRUtils::OVR::HandRootInvFixupRotationLeft
                                           : IsdkXRUtils::OVR::HandRootInvFixupRotationRight;
    const FQuat MetacarpalRotation =
        bIsLeft ? FQuat(FVector::UnitX(), HALF_PI) : FQuat(FVector::UnitX(), -HALF_PI);

    // set the bone poses
    FTransform WristPose = FTransform::Identity;
    JointPoses[1] = WristPose;

    for (const auto& Bone : BoneMap)
    {
      FTransform Pose{};
      if (!Bone.OVRBoneName.IsNone())
      {
        Pose = OculusXrHandComponent->GetBoneTransformByName(
            Bone.OVRBoneName, EBoneSpaces::Type::ComponentSpace);
        Pose.SetRotation(Pose.GetRotation() * OVRToOXRRotation);
        Pose.SetScale3D(FVector::One());
      }
      else
      {
        const FQuat YRot = FQuat(FVector::UnitY(), Bone.OXRRotation);
        Pose = FTransform(MetacarpalRotation * YRot, Bone.OXRWristOffset);
      }
      Pose = Pose * FTransform(InvWristRotation);
      JointPoses[Bone.OXRBoneIndex] = Pose;
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

FTransform UIsdkFromMetaXRHandDataSource::GetRootPose_Implementation()
{
  FTransform MotionTransform = MotionController->GetComponentTransform();
  const FQuat WristRotation = Handedness == EIsdkHandedness::Left
      ? IsdkXRUtils::OVR::HandRootFixupRotationLeft
      : IsdkXRUtils::OVR::HandRootFixupRotationRight;
  MotionTransform.SetRotation(MotionTransform.GetRotation() * WristRotation);
  return MotionTransform;
}

void UIsdkFromMetaXRHandDataSource::GetPointerPose_Implementation(
    FTransform& PointerPose,
    bool& IsValid)
{
  if (MotionController)
  {
    PointerPose = RelativePointerPose * MotionController->GetComponentTransform();
    IsValid = bIsLastGoodPointerPoseValid && !FIsdkOculusXRHelper::IsHoldingAController(GetWorld());
  }
  else
  {
    IsValid = false;
  }
}

void UIsdkFromMetaXRHandDataSource::GetRelativePointerPose_Implementation(
    FTransform& PointerRelativePose,
    bool& IsValid)
{
  PointerRelativePose = RelativePointerPose;
  IsValid = bIsLastGoodPointerPoseValid;
}

UMotionControllerComponent* UIsdkFromMetaXRHandDataSource::GetMotionController() const
{
  return MotionController;
}

bool UIsdkFromMetaXRHandDataSource::GetAllowInvalidTrackedData() const
{
  return bAllowLowConfidenceData;
}

UIsdkConditional* UIsdkFromMetaXRHandDataSource::GetRootPoseConnectedConditional_Implementation()
{
  return IsRootPoseConnected;
}

UIsdkConditional*
UIsdkFromMetaXRHandDataSource::GetRootPoseHighConfidenceConditional_Implementation()
{
  return IsRootPoseHighConfidence;
}

void UIsdkFromMetaXRHandDataSource::SetUintEnumProperty(
    UObject* Target,
    FName InName,
    uint8 EnumValue)
{
  FProperty* Property = Target->GetClass()->FindPropertyByName(InName);
  FEnumProperty* TypedProperty = CastField<FEnumProperty>(Property);
  FNumericProperty* EnumNumericProperty = TypedProperty->GetUnderlyingProperty();
  void* PropertyAddress = Property->ContainerPtrToValuePtr<void>(Target);
  EnumNumericProperty->SetIntPropertyValue(PropertyAddress, static_cast<uint64>(EnumValue));
}

void UIsdkFromMetaXRHandDataSource::GenerateBoneMap()
{
  TArray<FName> BoneNames = TArray<FName>();
  OculusXrHandComponent->GetBoneNames(BoneNames);

  checkf(
      BoneNames.Num() >= 24,
      TEXT(
          "UIsdkFromMetaXRHandDataSource::GenerateBoneMap() - BoneNames Length Should be >= 24, Found %d"),
      BoneNames.Num());

  BoneMap.Empty();
  // BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(FName("Wrist Root"), 1)); Already set manually
  const auto PalmOffset = Handedness == EIsdkHandedness::Left ? FVector(-0.145789, 5.974506, 1.9)
                                                              : FVector(0.311415, -6.273499, -1.9);
  BoneMap.Add(FBoneOVRToOXRMap::MappedOffset(0, PalmOffset, 0.0));

  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(2, BoneNames[3])); // HandThumb1 = 2
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(3, BoneNames[4])); // HandThumb2 = 3
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(4, BoneNames[5])); // HandThumb3 = 4
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(5, BoneNames[19])); // HandThumbTip = 5

  const float RotationMult = Handedness == EIsdkHandedness::Left ? -1.0 : 1.0;

  const auto Index0Offset = Handedness == EIsdkHandedness::Left
      ? FVector(2.004335, 3.778780, 0.284375)
      : FVector(-1.908977, -3.500033, -0.284375);
  const float IndexRotation = RotationMult * PI * 0.1;
  BoneMap.Add(FBoneOVRToOXRMap::MappedOffset(6, Index0Offset, IndexRotation)); // HandIndex0 = 6
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(7, BoneNames[6])); // HandIndex1 = 7
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(8, BoneNames[7])); // HandIndex2 = 8
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(9, BoneNames[8])); // HandIndex3 = 9
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(10, BoneNames[20])); // HandIndexTip = 10

  const auto Middle0Offset = Handedness == EIsdkHandedness::Left
      ? FVector(0.383252, 3.780457, -0.507344)
      : FVector(-0.295005, -3.552834, 0.507344);
  BoneMap.Add(FBoneOVRToOXRMap::MappedOffset(11, Middle0Offset, 0.0)); // HandMiddle0 = 11
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(12, BoneNames[9])); // HandMiddle1 = 12
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(13, BoneNames[10])); // HandMiddle2 = 13
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(14, BoneNames[11])); // HandMiddle3 = 14
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(15, BoneNames[21])); // HandMiddleTip = 15

  const auto Ring0Offset = Handedness == EIsdkHandedness::Left
      ? FVector(-1.035655, 3.796854, -0.203718)
      : FVector(0.993480, -3.424118, 0.203718);
  const float RingRotation = RotationMult * -PI * 0.1;
  BoneMap.Add(FBoneOVRToOXRMap::MappedOffset(16, Ring0Offset, RingRotation)); // HandRing0 = 16
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(17, BoneNames[12])); // HandRing1 = 17
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(18, BoneNames[13])); // HandRing2 = 18
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(19, BoneNames[14])); // HandRing3 = 19
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(20, BoneNames[22])); // HandRingTip = 20

  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(21, BoneNames[15])); // HandPinky0 = 21
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(22, BoneNames[16])); // HandPinky1 = 22
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(23, BoneNames[17])); // HandPinky2 = 23
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(24, BoneNames[18])); // HandPinky3 = 24
  BoneMap.Add(FBoneOVRToOXRMap::MappedIndex(25, BoneNames[23])); // HandPinkyTip = 25,
}

void UIsdkFromMetaXRHandDataSource::SetAllowInvalidTrackedData(bool bInAllowInvalidTrackedData)
{
  bAllowLowConfidenceData = bInAllowInvalidTrackedData;
}

#if !UE_BUILD_SHIPPING
void UIsdkFromMetaXRHandDataSource::DebugLog()
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
  UE_VLOG(GetOwner(), LogIsdkDataSourcesMetaXR, Verbose, TEXT("%s"), *RootPoseString);
}
#endif
