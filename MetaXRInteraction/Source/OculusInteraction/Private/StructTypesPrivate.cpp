// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// @generated

#include "StructTypesPrivate.h"

#pragma region Default Structs
namespace
{
isdk_FilterPropertyBlock FilterPropertyBlock_Default = {};
isdk_InteractableStateChangeArgs InteractableStateChangeArgs_Default = {};
isdk_InteractorStateChangeArgs InteractorStateChangeArgs_Default = {};
isdk_PokeInteractable_PositionPinningConfig PokeInteractable_PositionPinningConfig_Default = {};
isdk_PokeInteractable_DragThresholdingConfig PokeInteractable_DragThresholdingConfig_Default = {};
isdk_PokeInteractable_MinThresholdsConfig PokeInteractable_MinThresholdsConfig_Default = {};
isdk_PokeInteractable_RecoilAssistConfig PokeInteractable_RecoilAssistConfig_Default = {};
isdk_PokeInteractable_Config PokeInteractable_Config_Default = {};
isdk_PokeInteractor_Config PokeInteractor_Config_Default = {};
isdk_PokeInteractor_SurfaceHitCache PokeInteractor_SurfaceHitCache_Default = {};
isdk_SyntheticHand_Config SyntheticHand_Config_Default = {};
isdk_AxisAlignedBox AxisAlignedBox_Default = {};
isdk_BoundsClipper BoundsClipper_Default = {};
isdk_Ray Ray_Default = {};
isdk_SurfaceHit SurfaceHit_Default = {};
isdk_PointerEvent PointerEvent_Default = {};
isdk_RayInteractor_Config RayInteractor_Config_Default = {};
isdk_RayInteractor_RayCandidateProperties RayInteractor_RayCandidateProperties_Default = {};
isdk_OptionalSurfaceHit OptionalSurfaceHit_Default = {};
isdk_DigitRangeParams isdk_DigitRangeParams_Default = {};
isdk_ExternalHandPositionFrame_FingerJointMapping FIsdkExternalHandPositionFrame_FingerJointMapping_Default = {};
isdk_ExternalHandPositionFrame_ThumbJointMapping FIsdkExternalHandPositionFrame_ThumbJointMapping_Default = {};
isdk_DigitRecognizer_ExpectedAngleValueRange FIsdkDigitRecognizer_ExpectedAngleValueRange_Default = {};
isdk_DigitRecognizer_ExpectedDistanceValueRange FIsdkDigitRecognizer_ExpectedDistanceValueRange_Default = {};
isdk_FingerRecognizer_ExpectedFingerValueRanges FIsdkFingerRecognizer_ExpectedFingerValueRanges_Default = {};
isdk_ThumbRecognizer_ExpectedThumbValueRanges FIsdkThumbRecognizer_ExpectedThumbValueRanges_Default = {};
}

void StructTypesUtils::CreateStructTypesStatics()
{
  isdk_FilterPropertyBlock_init(&FilterPropertyBlock_Default);
  isdk_InteractableStateChangeArgs_init(&InteractableStateChangeArgs_Default);
  isdk_InteractorStateChangeArgs_init(&InteractorStateChangeArgs_Default);
  isdk_PokeInteractable_PositionPinningConfig_init(&PokeInteractable_PositionPinningConfig_Default);
  isdk_PokeInteractable_DragThresholdingConfig_init(
      &PokeInteractable_DragThresholdingConfig_Default);
  isdk_PokeInteractable_MinThresholdsConfig_init(&PokeInteractable_MinThresholdsConfig_Default);
  isdk_PokeInteractable_RecoilAssistConfig_init(&PokeInteractable_RecoilAssistConfig_Default);
  isdk_PokeInteractable_Config_init(&PokeInteractable_Config_Default);
  isdk_PokeInteractor_Config_init(&PokeInteractor_Config_Default);
  isdk_PokeInteractor_SurfaceHitCache_init(&PokeInteractor_SurfaceHitCache_Default);
  isdk_SyntheticHand_Config_init(&SyntheticHand_Config_Default);
  isdk_AxisAlignedBox_init(&AxisAlignedBox_Default);
  isdk_BoundsClipper_init(&BoundsClipper_Default);
  isdk_Ray_init(&Ray_Default);
  isdk_SurfaceHit_init(&SurfaceHit_Default);
  isdk_PointerEvent_init(&PointerEvent_Default);
  isdk_RayInteractor_Config_init(&RayInteractor_Config_Default);
  isdk_RayInteractor_RayCandidateProperties_init(&RayInteractor_RayCandidateProperties_Default);
  isdk_OptionalSurfaceHit_init(&OptionalSurfaceHit_Default);
  isdk_DigitRangeParams_init(&isdk_DigitRangeParams_Default);
  isdk_ExternalHandPositionFrame_FingerJointMapping_init(&FIsdkExternalHandPositionFrame_FingerJointMapping_Default);
  isdk_ExternalHandPositionFrame_ThumbJointMapping_init(&FIsdkExternalHandPositionFrame_ThumbJointMapping_Default);
  isdk_DigitRecognizer_ExpectedAngleValueRange_init(&FIsdkDigitRecognizer_ExpectedAngleValueRange_Default);
  isdk_DigitRecognizer_ExpectedDistanceValueRange_init(&FIsdkDigitRecognizer_ExpectedDistanceValueRange_Default);
  isdk_FingerRecognizer_ExpectedFingerValueRanges_init(&FIsdkFingerRecognizer_ExpectedFingerValueRanges_Default);
  isdk_ThumbRecognizer_ExpectedThumbValueRanges_init(&FIsdkThumbRecognizer_ExpectedThumbValueRanges_Default);
}
#pragma endregion Default Structs

#pragma region Constructors
FIsdkFilterPropertyBlock::FIsdkFilterPropertyBlock()
{
  StructTypesUtils::Copy(FilterPropertyBlock_Default, *this);
}

FIsdkInteractableStateChangeArgs::FIsdkInteractableStateChangeArgs()
{
  StructTypesUtils::Copy(InteractableStateChangeArgs_Default, *this);
}

FIsdkInteractorStateChangeArgs::FIsdkInteractorStateChangeArgs()
{
  StructTypesUtils::Copy(InteractorStateChangeArgs_Default, *this);
}

FIsdkPokeInteractable_PositionPinningConfig::FIsdkPokeInteractable_PositionPinningConfig()
{
  StructTypesUtils::Copy(PokeInteractable_PositionPinningConfig_Default, *this);
}

FIsdkPokeInteractable_DragThresholdingConfig::FIsdkPokeInteractable_DragThresholdingConfig()
{
  StructTypesUtils::Copy(PokeInteractable_DragThresholdingConfig_Default, *this);
}

FIsdkPokeInteractable_MinThresholdsConfig::FIsdkPokeInteractable_MinThresholdsConfig()
{
  StructTypesUtils::Copy(PokeInteractable_MinThresholdsConfig_Default, *this);
}

FIsdkPokeInteractable_RecoilAssistConfig::FIsdkPokeInteractable_RecoilAssistConfig()
{
  StructTypesUtils::Copy(PokeInteractable_RecoilAssistConfig_Default, *this);
}

FIsdkPokeInteractable_Config::FIsdkPokeInteractable_Config()
{
  StructTypesUtils::Copy(PokeInteractable_Config_Default, *this);
}

FIsdkPokeInteractor_Config::FIsdkPokeInteractor_Config()
{
  StructTypesUtils::Copy(PokeInteractor_Config_Default, *this);
}

FIsdkPokeInteractor_SurfaceHitCache::FIsdkPokeInteractor_SurfaceHitCache() = default;

FIsdkSyntheticHand_Config::FIsdkSyntheticHand_Config() = default;

FIsdkAxisAlignedBox::FIsdkAxisAlignedBox()
{
  StructTypesUtils::Copy(AxisAlignedBox_Default, *this);
}

FIsdkBoundsClipper::FIsdkBoundsClipper()
{
  PoseProvider = nullptr;
  FVector3f TempPosition;
  FVector3f TempSize;
  StructTypesUtils::Copy(BoundsClipper_Default.position, TempPosition);
  StructTypesUtils::Copy(BoundsClipper_Default.size, TempSize);
  Position = (FVector)TempPosition;
  Size = (FVector)TempSize;

}

FIsdkRay::FIsdkRay()
{
  StructTypesUtils::Copy(Ray_Default, *this);
}

FIsdkSurfaceHit::FIsdkSurfaceHit()
{
  StructTypesUtils::Copy(SurfaceHit_Default, *this);
}

FIsdkPointerEvent::FIsdkPointerEvent()
{
  Identifier = PointerEvent_Default.identifier;
  Type = static_cast<EIsdkPointerEventType>(PointerEvent_Default.type);
  FVector3f TempPosition;
  FQuat4f TempOrientation;
  StructTypesUtils::Copy(PointerEvent_Default.pose.Position, TempPosition);
  StructTypesUtils::Copy(PointerEvent_Default.pose.Orientation, TempOrientation);
  Pose.Position = (FVector)TempPosition;
  Pose.Orientation = (FQuat)TempOrientation;
}

FIsdkRayInteractor_Config::FIsdkRayInteractor_Config()
{
  StructTypesUtils::Copy(RayInteractor_Config_Default, *this);
}

FIsdkRayInteractor_RayCandidateProperties::FIsdkRayInteractor_RayCandidateProperties()
{
  StructTypesUtils::Copy(RayInteractor_RayCandidateProperties_Default, *this);
}

FIsdkOptionalSurfaceHit::FIsdkOptionalSurfaceHit()
{
  StructTypesUtils::Copy(OptionalSurfaceHit_Default, *this);
}
FIsdkDigitRangeParams::FIsdkDigitRangeParams()
{
  StructTypesUtils::Copy(isdk_DigitRangeParams_Default, *this);
}
FIsdkExternalHandPositionFrame_FingerJointMapping::FIsdkExternalHandPositionFrame_FingerJointMapping()
{
  StructTypesUtils::Copy(FIsdkExternalHandPositionFrame_FingerJointMapping_Default, *this);
}
FIsdkExternalHandPositionFrame_ThumbJointMapping::FIsdkExternalHandPositionFrame_ThumbJointMapping()
{
  StructTypesUtils::Copy(FIsdkExternalHandPositionFrame_ThumbJointMapping_Default, *this);
}

FIsdkDigitRecognizer_ExpectedAngleValueRange::FIsdkDigitRecognizer_ExpectedAngleValueRange() {
  StructTypesUtils::Copy(FIsdkDigitRecognizer_ExpectedAngleValueRange_Default, *this);
}
FIsdkDigitRecognizer_ExpectedDistanceValueRange::FIsdkDigitRecognizer_ExpectedDistanceValueRange() {
  StructTypesUtils::Copy(FIsdkDigitRecognizer_ExpectedDistanceValueRange_Default, *this);
}
FIsdkFingerRecognizer_ExpectedFingerValueRanges::FIsdkFingerRecognizer_ExpectedFingerValueRanges() {
  StructTypesUtils::Copy(FIsdkFingerRecognizer_ExpectedFingerValueRanges_Default, *this);
}
FIsdkThumbRecognizer_ExpectedThumbValueRanges::FIsdkThumbRecognizer_ExpectedThumbValueRanges() {
  StructTypesUtils::Copy(FIsdkThumbRecognizer_ExpectedThumbValueRanges_Default, *this);
}
#pragma endregion Constructors

#pragma region Copy
void StructTypesUtils::Copy(const FIsdkFilterPropertyBlock& src, isdk_FilterPropertyBlock& dest)
{
  dest.minCutOff = src.MinCutOff;
  dest.beta = src.Beta;
  dest.deltaMinCutOff = src.DeltaMinCutOff;
}

void StructTypesUtils::Copy(
    const FIsdkInteractableStateChangeArgs& src,
    isdk_InteractableStateChangeArgs& dest)
{
  dest.previousState = static_cast<isdk_InteractableState>(src.PreviousState);
  dest.newState = static_cast<isdk_InteractableState>(src.NewState);
}

void StructTypesUtils::Copy(
    const FIsdkInteractorStateChangeArgs& src,
    isdk_InteractorStateChangeArgs& dest)
{
  dest.previousState = static_cast<isdk_InteractorState>(src.PreviousState);
  dest.newState = static_cast<isdk_InteractorState>(src.NewState);
}

void StructTypesUtils::Copy(
    const FIsdkPokeInteractable_PositionPinningConfig& src,
    isdk_PokeInteractable_PositionPinningConfig& dest)
{
  dest.enabled = src.Enabled ? 1 : 0;
  dest.maxPinDistance = src.MaxPinDistance;
}

void StructTypesUtils::Copy(
    const FIsdkPokeInteractable_DragThresholdingConfig& src,
    isdk_PokeInteractable_DragThresholdingConfig& dest)
{
  dest.enabled = src.Enabled ? 1 : 0;
  dest.dragNormal = src.DragNormal;
  dest.dragTangent = src.DragTangent;
}

void StructTypesUtils::Copy(
    const FIsdkPokeInteractable_MinThresholdsConfig& src,
    isdk_PokeInteractable_MinThresholdsConfig& dest)
{
  dest.enabled = src.Enabled ? 1 : 0;
  dest.minNormal = src.MinNormal;
}

void StructTypesUtils::Copy(
    const FIsdkPokeInteractable_RecoilAssistConfig& src,
    isdk_PokeInteractable_RecoilAssistConfig& dest)
{
  dest.enabled = src.Enabled ? 1 : 0;
  dest.useVelocityExpansion = src.UseVelocityExpansion ? 1 : 0;
  dest.velocityExpansionMinSpeed = src.VelocityExpansionMinSpeed;
  dest.velocityExpansionMaxSpeed = src.VelocityExpansionMaxSpeed;
  dest.velocityExpansionDistance = src.VelocityExpansionDistance;
  dest.velocityExpansionDecayRate = src.VelocityExpansionDecayRate;
  dest.useDynamicDecay = src.UseDynamicDecay ? 1 : 0;
  dest.exitDistance = src.ExitDistance;
  dest.reEnterDistance = src.ReEnterDistance;
}

void StructTypesUtils::Copy(
    const FIsdkPokeInteractable_Config& src,
    isdk_PokeInteractable_Config& dest)
{
  dest.enterHoverNormal = src.EnterHoverNormal;
  dest.enterHoverTangent = src.EnterHoverTangent;
  dest.exitHoverNormal = src.ExitHoverNormal;
  dest.exitHoverTangent = src.ExitHoverTangent;
  dest.cancelSelectNormal = src.CancelSelectNormal;
  dest.cancelSelectTangent = src.CancelSelectTangent;
  Copy(src.PositionPinningConfig, dest.positionPinningConfig);
  Copy(src.DragThresholdsConfig, dest.dragThresholdsConfig);
  Copy(src.MinThresholdsConfig, dest.minThresholdsConfig);
  Copy(src.RecoilAssistConfig, dest.recoilAssistConfig);
}

void StructTypesUtils::Copy(const FIsdkPokeInteractor_Config& src, isdk_PokeInteractor_Config& dest)
{
  dest.touchReleaseThreshold = src.TouchReleaseThreshold;
  dest.maxDeltaFromTouchPoint = src.MaxDeltaFromTouchPoint;
  dest.equalDistanceThreshold = src.EqualDistanceThreshold;
  dest.radius = src.Radius;
}

void StructTypesUtils::Copy(const FIsdkAxisAlignedBox& src, isdk_AxisAlignedBox& dest)
{
	FVector3f TempLowerBound = src.LowerBound;
	FVector3f TempUpperBound = src.UpperBound;
	FVector3f TempCentroid = src.Centroid;
	FVector3f TempHalfSize = src.HalfSize;
  Copy(TempLowerBound, dest.lowerBound);
  Copy(TempUpperBound, dest.upperBound);
  Copy(TempCentroid, dest.centroid);
  Copy(TempHalfSize, dest.halfSize);
}

void StructTypesUtils::Copy(const FIsdkRay& src, isdk_Ray& dest)
{
	FVector3f TempOrigin = src.Origin;
	FVector3f TempDirection = src.Direction;
  Copy(TempOrigin, dest.origin);
  Copy(TempDirection, dest.direction);
}

void StructTypesUtils::Copy(const FIsdkSurfaceHit& src, isdk_SurfaceHit& dest)
{
	FVector3f TempPoint = src.Point;
	FVector3f TempNormal = src.Normal;
  Copy(TempPoint, dest.point);
  Copy(TempNormal, dest.normal);
  dest.distance = src.Distance;
}

void StructTypesUtils::Copy(const FIsdkRayInteractor_Config& src, isdk_RayInteractor_Config& dest)
{
  dest.equalDistanceThreshold = src.EqualDistanceThreshold;
  dest.maxRayLength = src.MaxRayLength;
}

void StructTypesUtils::Copy(const FIsdkOptionalSurfaceHit& src, isdk_OptionalSurfaceHit& dest)
{
  dest.hasValue = src.HasValue ? 1 : 0;
}

void StructTypesUtils::Copy(const FIsdkDigitRangeParams& src, isdk_DigitRangeParams& dest)
{
  dest.lowerLimit = src.LowerLimit;
  dest.lowerTolerance = src.LowerTolerance;
  dest.upperLimit = src.UpperLimit;
  dest.upperTolerance = src.UpperTolerance;
  dest.minTimeToTransition = src.MinTimeToTransition;
}

void StructTypesUtils::Copy(
    const FIsdkExternalHandPositionFrame_ThumbJointMapping& Src,
    isdk_ExternalHandPositionFrame_ThumbJointMapping& Dest)
{
  Dest.joint = static_cast<isdk_ThumbJoint>(Src.Joint);
  Dest.index = Src.Index;
}

void StructTypesUtils::Copy(
    const FIsdkExternalHandPositionFrame_FingerJointMapping& Src,
    isdk_ExternalHandPositionFrame_FingerJointMapping& Dest)
{
  Dest.finger = static_cast<isdk_FingerType>(Src.Finger);
  Dest.joint = static_cast<isdk_FingerJoint>(Src.Joint);
  Dest.index = Src.Index;
}

void StructTypesUtils::Copy(
    const FIsdkDigitRecognizer_ExpectedAngleValueRange& Src,
    isdk_DigitRecognizer_ExpectedAngleValueRange& Dest)
{
  Dest.minValue = Src.MinValue;
  Dest.maxValue = Src.MaxValue;
}

void StructTypesUtils::Copy(
    const FIsdkDigitRecognizer_ExpectedDistanceValueRange& Src,
    isdk_DigitRecognizer_ExpectedDistanceValueRange& Dest)
{
  Dest.minValue = Src.MinValue;
  Dest.maxValue = Src.MaxValue;
}

void StructTypesUtils::Copy(
    const FIsdkFingerRecognizer_ExpectedFingerValueRanges& Src,
    isdk_FingerRecognizer_ExpectedFingerValueRanges& Dest)
{
  Copy(Src.Curl, Dest.curl);
  Copy(Src.Flexion, Dest.flexion);
  Copy(Src.Abduction, Dest.abduction);
  Copy(Src.Opposition, Dest.opposition);
  Copy(Src.Grab, Dest.grab);
  Copy(Src.OppositionTangentPlane, Dest.oppositionTangentPlane);
  Copy(Src.OppositionNormal, Dest.oppositionNormal);
  Copy(Src.OppositionTopTwo, Dest.oppositionTopTwo);
  Copy(Src.OppositionTopThree, Dest.oppositionTopThree);
}

void StructTypesUtils::Copy(
    const FIsdkThumbRecognizer_ExpectedThumbValueRanges& Src,
    isdk_ThumbRecognizer_ExpectedThumbValueRanges& Dest)
{
  Copy(Src.Curl, Dest.curl);
  Copy(Src.Flexion, Dest.flexion);
}

void StructTypesUtils::Copy(const FVector3f& src, ovrpVector3f& dest)
{
  dest.x = src.X;
  dest.y = src.Y;
  dest.z = src.Z;
}

void StructTypesUtils::Copy(const FVector3d& src, ovrpVector3f& dest)
{
  dest.x = static_cast<float>(src.X);
  dest.y = static_cast<float>(src.Y);
  dest.z = static_cast<float>(src.Z);
}

void StructTypesUtils::Copy(const FQuat4f& src, ovrpQuatf& dest)
{
  dest.x = src.X;
  dest.y = src.Y;
  dest.z = src.Z;
  dest.w = src.W;
}

void StructTypesUtils::Copy(const FQuat4d& src, ovrpQuatf& dest)
{
  dest.x = src.X;
  dest.y = src.Y;
  dest.z = src.Z;
  dest.w = src.W;
}

void StructTypesUtils::Copy(const FTransform& Src, ovrpPosef& Dest)
{
  ovrpVector3f Position{};
  ovrpQuatf Orientation{};
  FVector3d TempLocation = Src.GetLocation();
  FQuat TempRotation = Src.GetRotation();
  Copy(TempLocation, Position);
  Copy((FQuat4d)TempRotation, Orientation);
  Dest = {Orientation, Position};
}

void StructTypesUtils::Copy(const FIsdkPosef& Src, ovrpPosef& Dest)
{
  ovrpVector3f Position{};
  ovrpQuatf Orientation{};
  FVector3f TempPosition = Src.Position;
  FQuat TempOrientation = Src.Orientation;
  Copy(TempPosition, Position);
  Copy((FQuat4d)TempOrientation, Orientation);
  Dest = {Orientation, Position};
}

void StructTypesUtils::Copy(const isdk_FilterPropertyBlock& src, FIsdkFilterPropertyBlock& dest)
{
  dest.MinCutOff = src.minCutOff;
  dest.Beta = src.beta;
  dest.DeltaMinCutOff = src.deltaMinCutOff;
}

void StructTypesUtils::Copy(
    const isdk_InteractableStateChangeArgs& src,
    FIsdkInteractableStateChangeArgs& dest)
{
  dest.PreviousState = static_cast<EIsdkInteractableState>(src.previousState);
  dest.NewState = static_cast<EIsdkInteractableState>(src.newState);
}

void StructTypesUtils::Copy(
    const isdk_InteractorStateChangeArgs& src,
    FIsdkInteractorStateChangeArgs& dest)
{
  dest.PreviousState = static_cast<EIsdkInteractorState>(src.previousState);
  dest.NewState = static_cast<EIsdkInteractorState>(src.newState);
}

void StructTypesUtils::Copy(
    const isdk_PokeInteractable_PositionPinningConfig& src,
    FIsdkPokeInteractable_PositionPinningConfig& dest)
{
  dest.Enabled = !!src.enabled;
  dest.MaxPinDistance = src.maxPinDistance;
}

void StructTypesUtils::Copy(
    const isdk_PokeInteractable_DragThresholdingConfig& src,
    FIsdkPokeInteractable_DragThresholdingConfig& dest)
{
  dest.Enabled = !!src.enabled;
  dest.DragNormal = src.dragNormal;
  dest.DragTangent = src.dragTangent;
}

void StructTypesUtils::Copy(
    const isdk_PokeInteractable_MinThresholdsConfig& src,
    FIsdkPokeInteractable_MinThresholdsConfig& dest)
{
  dest.Enabled = !!src.enabled;
  dest.MinNormal = src.minNormal;
}

void StructTypesUtils::Copy(
    const isdk_PokeInteractable_RecoilAssistConfig& src,
    FIsdkPokeInteractable_RecoilAssistConfig& dest)
{
  dest.Enabled = !!src.enabled;
  dest.UseVelocityExpansion = !!src.useVelocityExpansion;
  dest.VelocityExpansionMinSpeed = src.velocityExpansionMinSpeed;
  dest.VelocityExpansionMaxSpeed = src.velocityExpansionMaxSpeed;
  dest.VelocityExpansionDistance = src.velocityExpansionDistance;
  dest.VelocityExpansionDecayRate = src.velocityExpansionDecayRate;
  dest.UseDynamicDecay = !!src.useDynamicDecay;
  dest.ExitDistance = src.exitDistance;
  dest.ReEnterDistance = src.reEnterDistance;
}

void StructTypesUtils::Copy(
    const isdk_PokeInteractable_Config& src,
    FIsdkPokeInteractable_Config& dest)
{
  dest.EnterHoverNormal = src.enterHoverNormal;
  dest.EnterHoverTangent = src.enterHoverTangent;
  dest.ExitHoverNormal = src.exitHoverNormal;
  dest.ExitHoverTangent = src.exitHoverTangent;
  dest.CancelSelectNormal = src.cancelSelectNormal;
  dest.CancelSelectTangent = src.cancelSelectTangent;
  Copy(src.positionPinningConfig, dest.PositionPinningConfig);
  Copy(src.dragThresholdsConfig, dest.DragThresholdsConfig);
  Copy(src.minThresholdsConfig, dest.MinThresholdsConfig);
  Copy(src.recoilAssistConfig, dest.RecoilAssistConfig);
}

void StructTypesUtils::Copy(const isdk_PokeInteractor_Config& src, FIsdkPokeInteractor_Config& dest)
{
  dest.TouchReleaseThreshold = src.touchReleaseThreshold;
  dest.MaxDeltaFromTouchPoint = src.maxDeltaFromTouchPoint;
  dest.EqualDistanceThreshold = src.equalDistanceThreshold;
  dest.Radius = src.radius;
}

void StructTypesUtils::Copy(const isdk_AxisAlignedBox& src, FIsdkAxisAlignedBox& dest)
{
	FVector3f TempLowerBound, TempUpperBound, TempCentroid, TempHalfSize;
  Copy(src.lowerBound, TempLowerBound);
  Copy(src.upperBound, TempUpperBound);
  Copy(src.centroid, TempCentroid);
  Copy(src.halfSize, TempHalfSize);
  dest.LowerBound = (FVector)TempLowerBound;
  dest.UpperBound = (FVector)TempUpperBound;
  dest.Centroid = (FVector)TempCentroid;
  dest.HalfSize = (FVector)TempHalfSize;
}

void StructTypesUtils::Copy(const isdk_Ray& src, FIsdkRay& dest)
{
	FVector3f TempOrigin, TempDirection;
  Copy(src.origin, TempOrigin);
  Copy(src.direction, TempDirection);
  dest.Origin = (FVector)TempOrigin;
  dest.Direction = (FVector)TempDirection;
}

void StructTypesUtils::Copy(const isdk_SurfaceHit& src, FIsdkSurfaceHit& dest)
{
	FVector3f TempPoint, TempNormal;
  Copy(src.point, TempPoint);
  Copy(src.normal, TempNormal);
  dest.Point = (FVector)TempPoint;
  dest.Normal = (FVector)TempNormal;
  dest.Distance = src.distance;
}

void StructTypesUtils::Copy(const isdk_RayInteractor_Config& src, FIsdkRayInteractor_Config& dest)
{
  dest.EqualDistanceThreshold = src.equalDistanceThreshold;
  dest.MaxRayLength = src.maxRayLength;
}

void StructTypesUtils::Copy(const isdk_OptionalSurfaceHit& src, FIsdkOptionalSurfaceHit& dest)
{
  dest.HasValue = !!src.hasValue;
  dest.Value.Distance = src.value.distance;
  FVector3f TempNormal, TempPoint;
  Copy(src.value.normal, TempNormal);
  Copy(src.value.point, TempPoint);
  dest.Value.Normal = (FVector)TempNormal;
  dest.Value.Point = (FVector)TempPoint;
}

void StructTypesUtils::Copy(const isdk_DigitRangeParams& src, FIsdkDigitRangeParams& dest)
{
  dest.LowerLimit = src.lowerLimit;
  dest.LowerTolerance = src.lowerTolerance;
  dest.UpperLimit = src.upperLimit;
  dest.UpperTolerance = src.upperTolerance;
  dest.MinTimeToTransition = src.minTimeToTransition;
}

void StructTypesUtils::Copy(
    const isdk_ExternalHandPositionFrame_ThumbJointMapping& Src,
    FIsdkExternalHandPositionFrame_ThumbJointMapping& Dest)
{
  Dest.Joint = static_cast<EIsdkThumbJoint>(Src.joint);
  Dest.Index = Src.index;
}

void StructTypesUtils::Copy(
    const isdk_ExternalHandPositionFrame_FingerJointMapping& Src,
    FIsdkExternalHandPositionFrame_FingerJointMapping& Dest)
{
  Dest.Finger = static_cast<EIsdkFingerType>(Src.finger);
  Dest.Joint = static_cast<EIsdkFingerJoint>(Src.joint);
  Dest.Index = Src.index;
}

void StructTypesUtils::Copy(
    const isdk_DigitRecognizer_ExpectedAngleValueRange& Src,
    FIsdkDigitRecognizer_ExpectedAngleValueRange& Dest)
{
  Dest.MinValue = Src.minValue;
  Dest.MaxValue = Src.maxValue;
}

void StructTypesUtils::Copy(
    const isdk_DigitRecognizer_ExpectedDistanceValueRange& Src,
    FIsdkDigitRecognizer_ExpectedDistanceValueRange& Dest)
{
  Dest.MinValue = Src.minValue;
  Dest.MaxValue = Src.maxValue;
}

void StructTypesUtils::Copy(
    const isdk_FingerRecognizer_ExpectedFingerValueRanges& Src,
    FIsdkFingerRecognizer_ExpectedFingerValueRanges& Dest)
{
  Copy(Src.curl, Dest.Curl);
  Copy(Src.flexion, Dest.Flexion);
  Copy(Src.abduction, Dest.Abduction);
  Copy(Src.opposition, Dest.Opposition);
  Copy(Src.grab, Dest.Grab);
  Copy(Src.oppositionTangentPlane, Dest.OppositionTangentPlane);
  Copy(Src.oppositionNormal, Dest.OppositionNormal);
  Copy(Src.oppositionTopTwo, Dest.OppositionTopTwo);
  Copy(Src.oppositionTopThree, Dest.OppositionTopThree);
}

void StructTypesUtils::Copy(
    const isdk_ThumbRecognizer_ExpectedThumbValueRanges& Src,
    FIsdkThumbRecognizer_ExpectedThumbValueRanges& Dest)
{
  Copy(Src.curl, Dest.Curl);
  Copy(Src.flexion, Dest.Flexion);
}

void StructTypesUtils::Copy(const ovrpVector3f& src, FVector3f& dest)
{
  dest.X = src.x;
  dest.Y = src.y;
  dest.Z = src.z;
}

void StructTypesUtils::Copy(const ovrpVector3f& src, FVector3d& dest)
{
  dest.X = static_cast<float>(src.x);
  dest.Y = static_cast<float>(src.y);
  dest.Z = static_cast<float>(src.z);
}

void StructTypesUtils::Copy(const ovrpQuatf& src, FQuat4f& dest)
{
  dest.X = src.x;
  dest.Y = src.y;
  dest.Z = src.z;
  dest.W = src.w;
}

void StructTypesUtils::Copy(const ovrpQuatf& src, FQuat4d& dest)
{
  dest.X = static_cast<float>(src.x);
  dest.Y = static_cast<float>(src.y);
  dest.Z = static_cast<float>(src.z);
  dest.W = static_cast<float>(src.w);
}

void StructTypesUtils::Copy(const ovrpPosef& Src, FTransform& Dest)
{
  FVector3d Position{};
  FQuat4d Orientation{};
  Copy(Src.Position, Position);
  Copy(Src.Orientation, Orientation);
  Dest = {(FQuat)Orientation, (FVector)Position};
}

void StructTypesUtils::Copy(const ovrpPosef& Src, FIsdkPosef& Dest)
{
	FVector3f TempPosition;
	FQuat4f TempOrientation;
    Copy(Src.Position, TempPosition);
    Copy(Src.Orientation, TempOrientation);
	Dest.Position = (FVector)TempPosition;
	Dest.Orientation = (FQuat)TempOrientation;
}
#pragma endregion Copy

#pragma region Convert
isdk_FilterPropertyBlock StructTypesUtils::Convert(const FIsdkFilterPropertyBlock& src)
{
  isdk_FilterPropertyBlock dest{};

  dest.minCutOff = src.MinCutOff;
  dest.beta = src.Beta;
  dest.deltaMinCutOff = src.DeltaMinCutOff;
  return dest;
}

isdk_InteractableStateChangeArgs StructTypesUtils::Convert(
    const FIsdkInteractableStateChangeArgs& src)
{
  isdk_InteractableStateChangeArgs dest{};

  dest.previousState = static_cast<isdk_InteractableState>(src.PreviousState);
  dest.newState = static_cast<isdk_InteractableState>(src.NewState);
  return dest;
}

isdk_InteractorStateChangeArgs StructTypesUtils::Convert(const FIsdkInteractorStateChangeArgs& src)
{
  isdk_InteractorStateChangeArgs dest{};

  dest.previousState = static_cast<isdk_InteractorState>(src.PreviousState);
  dest.newState = static_cast<isdk_InteractorState>(src.NewState);
  return dest;
}

isdk_PokeInteractable_PositionPinningConfig StructTypesUtils::Convert(
    const FIsdkPokeInteractable_PositionPinningConfig& src)
{
  isdk_PokeInteractable_PositionPinningConfig dest{};

  dest.enabled = src.Enabled ? 1 : 0;
  dest.maxPinDistance = src.MaxPinDistance;
  return dest;
}

isdk_PokeInteractable_DragThresholdingConfig StructTypesUtils::Convert(
    const FIsdkPokeInteractable_DragThresholdingConfig& src)
{
  isdk_PokeInteractable_DragThresholdingConfig dest{};

  dest.enabled = src.Enabled ? 1 : 0;
  dest.dragNormal = src.DragNormal;
  dest.dragTangent = src.DragTangent;
  return dest;
}

isdk_PokeInteractable_MinThresholdsConfig StructTypesUtils::Convert(
    const FIsdkPokeInteractable_MinThresholdsConfig& src)
{
  isdk_PokeInteractable_MinThresholdsConfig dest{};

  dest.enabled = src.Enabled ? 1 : 0;
  dest.minNormal = src.MinNormal;
  return dest;
}

isdk_PokeInteractable_RecoilAssistConfig StructTypesUtils::Convert(
    const FIsdkPokeInteractable_RecoilAssistConfig& src)
{
  isdk_PokeInteractable_RecoilAssistConfig dest{};

  dest.enabled = src.Enabled ? 1 : 0;
  dest.useVelocityExpansion = src.UseVelocityExpansion ? 1 : 0;
  dest.velocityExpansionMinSpeed = src.VelocityExpansionMinSpeed;
  dest.velocityExpansionMaxSpeed = src.VelocityExpansionMaxSpeed;
  dest.velocityExpansionDistance = src.VelocityExpansionDistance;
  dest.velocityExpansionDecayRate = src.VelocityExpansionDecayRate;
  dest.useDynamicDecay = src.UseDynamicDecay ? 1 : 0;
  dest.exitDistance = src.ExitDistance;
  dest.reEnterDistance = src.ReEnterDistance;
  return dest;
}

isdk_PokeInteractable_Config StructTypesUtils::Convert(const FIsdkPokeInteractable_Config& src)
{
  isdk_PokeInteractable_Config dest{};

  dest.enterHoverNormal = src.EnterHoverNormal;
  dest.enterHoverTangent = src.EnterHoverTangent;
  dest.exitHoverNormal = src.ExitHoverNormal;
  dest.exitHoverTangent = src.ExitHoverTangent;
  dest.cancelSelectNormal = src.CancelSelectNormal;
  dest.cancelSelectTangent = src.CancelSelectTangent;
  dest.positionPinningConfig = Convert(src.PositionPinningConfig);
  dest.dragThresholdsConfig = Convert(src.DragThresholdsConfig);
  dest.minThresholdsConfig = Convert(src.MinThresholdsConfig);
  dest.recoilAssistConfig = Convert(src.RecoilAssistConfig);
  return dest;
}

isdk_PokeInteractor_Config StructTypesUtils::Convert(const FIsdkPokeInteractor_Config& src)
{
  isdk_PokeInteractor_Config dest{};

  dest.touchReleaseThreshold = src.TouchReleaseThreshold;
  dest.maxDeltaFromTouchPoint = src.MaxDeltaFromTouchPoint;
  dest.equalDistanceThreshold = src.EqualDistanceThreshold;
  dest.radius = src.Radius;
  return dest;
}

isdk_AxisAlignedBox StructTypesUtils::Convert(const FIsdkAxisAlignedBox& src)
{
  isdk_AxisAlignedBox dest{};
  FVector3f TempLowerBound = src.LowerBound;
  FVector3f TempUpperBound = src.UpperBound;
  FVector3f TempCentroid = src.Centroid;
  FVector3f TempHalfSize = src.HalfSize;
  dest.lowerBound = Convert(TempLowerBound);
  dest.upperBound = Convert(TempUpperBound);
  dest.centroid = Convert(TempCentroid);
  dest.halfSize = Convert(TempHalfSize);
  return dest;
}

isdk_Ray StructTypesUtils::Convert(const FIsdkRay& src)
{
  isdk_Ray dest{};
  FVector3f TempOrigin = src.Origin;
  FVector3f TempDirection = src.Direction;
  dest.origin = Convert(TempOrigin);
  dest.direction = Convert(TempDirection);
  return dest;
}

isdk_SurfaceHit StructTypesUtils::Convert(const FIsdkSurfaceHit& src)
{
  isdk_SurfaceHit dest{};
  FVector3f TempPoint = src.Point;
  FVector3f TempNormal = src.Normal;
  dest.point = Convert(TempPoint);
  dest.normal = Convert(TempNormal);
  dest.distance = src.Distance;
  return dest;
}

isdk_RayInteractor_Config StructTypesUtils::Convert(const FIsdkRayInteractor_Config& src)
{
  isdk_RayInteractor_Config dest{};

  dest.equalDistanceThreshold = src.EqualDistanceThreshold;
  dest.maxRayLength = src.MaxRayLength;
  return dest;
}

isdk_OptionalSurfaceHit StructTypesUtils::Convert(const FIsdkOptionalSurfaceHit& src)
{
  isdk_OptionalSurfaceHit dest{};

  dest.hasValue = src.HasValue ? 1 : 0;
  return dest;
}

ovrpVector2f StructTypesUtils::Convert(const FVector2f& src)
{
  return ovrpVector2f{
      src.X, src.Y
  };
}

ovrpVector2f StructTypesUtils::Convert(const FVector2d& src)
{
  return ovrpVector2f{
      static_cast<float>(src.X), static_cast<float>(src.Y)
  };
}

ovrpVector3f StructTypesUtils::Convert(const FVector3f& src)
{
  ovrpVector3f dest{};

  dest.x = src.X;
  dest.y = src.Y;
  dest.z = src.Z;
  return dest;
}

ovrpVector3f StructTypesUtils::Convert(const FVector3d& src)
{
  ovrpVector3f dest{};

  dest.x = static_cast<float>(src.X);
  dest.y = static_cast<float>(src.Y);
  dest.z = static_cast<float>(src.Z);
  return dest;
}

ovrpQuatf StructTypesUtils::Convert(const FQuat4f& src)
{
  ovrpQuatf dest{};

  dest.x = src.X;
  dest.y = src.Y;
  dest.z = src.Z;
  dest.w = src.W;
  return dest;
}

ovrpQuatf StructTypesUtils::Convert(const FQuat4d& src)
{
  ovrpQuatf dest{};

  dest.x = src.X;
  dest.y = src.Y;
  dest.z = src.Z;
  dest.w = src.W;
  return dest;
}

ovrpPosef StructTypesUtils::Convert(const FTransform& src)
{
	FQuat TempRotation = src.GetRotation();
	FVector3d TempLocation = src.GetLocation();
  return {Convert((FQuat4d)TempRotation), Convert(TempLocation)};
}

ovrpPosef StructTypesUtils::Convert(const FIsdkPosef& src)
{
	FQuat TempOrientation = src.Orientation;
	FVector3f TempPosition = src.Position;
  return {Convert((FQuat4d)TempOrientation), Convert(TempPosition)};
}

FIsdkFilterPropertyBlock StructTypesUtils::Convert(const isdk_FilterPropertyBlock& src)
{
  FIsdkFilterPropertyBlock dest{};

  dest.MinCutOff = src.minCutOff;
  dest.Beta = src.beta;
  dest.DeltaMinCutOff = src.deltaMinCutOff;
  return dest;
}

FIsdkInteractableStateChangeArgs StructTypesUtils::Convert(
    const isdk_InteractableStateChangeArgs& src)
{
  FIsdkInteractableStateChangeArgs dest{};

  dest.PreviousState = static_cast<EIsdkInteractableState>(src.previousState);
  dest.NewState = static_cast<EIsdkInteractableState>(src.newState);
  return dest;
}

FIsdkInteractorStateChangeArgs StructTypesUtils::Convert(const isdk_InteractorStateChangeArgs& src)
{
  FIsdkInteractorStateChangeArgs dest{};

  dest.PreviousState = static_cast<EIsdkInteractorState>(src.previousState);
  dest.NewState = static_cast<EIsdkInteractorState>(src.newState);
  return dest;
}

FIsdkPokeInteractable_PositionPinningConfig StructTypesUtils::Convert(
    const isdk_PokeInteractable_PositionPinningConfig& src)
{
  FIsdkPokeInteractable_PositionPinningConfig dest{};

  dest.Enabled = !!src.enabled;
  dest.MaxPinDistance = src.maxPinDistance;
  return dest;
}

FIsdkPokeInteractable_DragThresholdingConfig StructTypesUtils::Convert(
    const isdk_PokeInteractable_DragThresholdingConfig& src)
{
  FIsdkPokeInteractable_DragThresholdingConfig dest{};

  dest.Enabled = !!src.enabled;
  dest.DragNormal = src.dragNormal;
  dest.DragTangent = src.dragTangent;
  return dest;
}

FIsdkPokeInteractable_MinThresholdsConfig StructTypesUtils::Convert(
    const isdk_PokeInteractable_MinThresholdsConfig& src)
{
  FIsdkPokeInteractable_MinThresholdsConfig dest{};

  dest.Enabled = !!src.enabled;
  dest.MinNormal = src.minNormal;
  return dest;
}

FIsdkPokeInteractable_RecoilAssistConfig StructTypesUtils::Convert(
    const isdk_PokeInteractable_RecoilAssistConfig& src)
{
  FIsdkPokeInteractable_RecoilAssistConfig dest{};

  dest.Enabled = !!src.enabled;
  dest.UseVelocityExpansion = !!src.useVelocityExpansion;
  dest.VelocityExpansionMinSpeed = src.velocityExpansionMinSpeed;
  dest.VelocityExpansionMaxSpeed = src.velocityExpansionMaxSpeed;
  dest.VelocityExpansionDistance = src.velocityExpansionDistance;
  dest.VelocityExpansionDecayRate = src.velocityExpansionDecayRate;
  dest.UseDynamicDecay = !!src.useDynamicDecay;
  dest.ExitDistance = src.exitDistance;
  dest.ReEnterDistance = src.reEnterDistance;
  return dest;
}

FIsdkPokeInteractable_Config StructTypesUtils::Convert(const isdk_PokeInteractable_Config& src)
{
  FIsdkPokeInteractable_Config dest{};

  dest.EnterHoverNormal = src.enterHoverNormal;
  dest.EnterHoverTangent = src.enterHoverTangent;
  dest.ExitHoverNormal = src.exitHoverNormal;
  dest.ExitHoverTangent = src.exitHoverTangent;
  dest.CancelSelectNormal = src.cancelSelectNormal;
  dest.CancelSelectTangent = src.cancelSelectTangent;
  dest.PositionPinningConfig = Convert(src.positionPinningConfig);
  dest.DragThresholdsConfig = Convert(src.dragThresholdsConfig);
  dest.MinThresholdsConfig = Convert(src.minThresholdsConfig);
  dest.RecoilAssistConfig = Convert(src.recoilAssistConfig);
  return dest;
}

FIsdkPokeInteractor_Config StructTypesUtils::Convert(const isdk_PokeInteractor_Config& src)
{
  FIsdkPokeInteractor_Config dest{};

  dest.TouchReleaseThreshold = src.touchReleaseThreshold;
  dest.MaxDeltaFromTouchPoint = src.maxDeltaFromTouchPoint;
  dest.EqualDistanceThreshold = src.equalDistanceThreshold;
  dest.Radius = src.radius;
  return dest;
}

FIsdkAxisAlignedBox StructTypesUtils::Convert(const isdk_AxisAlignedBox& src)
{
  FIsdkAxisAlignedBox dest{};

  dest.LowerBound = (FVector)Convert(src.lowerBound);
  dest.UpperBound = (FVector)Convert(src.upperBound);
  dest.Centroid = (FVector)Convert(src.centroid);
  dest.HalfSize = (FVector)Convert(src.halfSize);
  return dest;
}

FIsdkRay StructTypesUtils::Convert(const isdk_Ray& src)
{
  FIsdkRay dest{};

  dest.Origin = (FVector)Convert(src.origin);
  dest.Direction = (FVector)Convert(src.direction);
  return dest;
}

FIsdkSurfaceHit StructTypesUtils::Convert(const isdk_SurfaceHit& src)
{
  FIsdkSurfaceHit dest{};

  dest.Point = (FVector)Convert(src.point);
  dest.Normal = (FVector)Convert(src.normal);
  dest.Distance = src.distance;
  return dest;
}

FIsdkRayInteractor_Config StructTypesUtils::Convert(const isdk_RayInteractor_Config& src)
{
  FIsdkRayInteractor_Config dest{};

  dest.EqualDistanceThreshold = src.equalDistanceThreshold;
  dest.MaxRayLength = src.maxRayLength;
  return dest;
}

FIsdkOptionalSurfaceHit StructTypesUtils::Convert(const isdk_OptionalSurfaceHit& src)
{
  FIsdkOptionalSurfaceHit dest{};

  dest.HasValue = !!src.hasValue;
  dest.Value.Distance = src.value.distance;
  FVector3f TempNormal, TempPoint;
  Copy(src.value.normal, TempNormal);
  Copy(src.value.point, TempPoint);
  dest.Value.Normal = (FVector)TempNormal;
  dest.Value.Point = (FVector)TempPoint;
  return dest;
}

FVector3f StructTypesUtils::Convert(const ovrpVector3f& src)
{
  FVector3f dest{};

  dest.X = src.x;
  dest.Y = src.y;
  dest.Z = src.z;
  return dest;
}

FVector3d StructTypesUtils::ConvertDouble(const ovrpVector3f& src)
{
  FVector3d dest{};

  dest.X = static_cast<float>(src.x);
  dest.Y = static_cast<float>(src.y);
  dest.Z = static_cast<float>(src.z);
  return dest;
}

FQuat4f StructTypesUtils::Convert(const ovrpQuatf& src)
{
  FQuat4f dest{};

  dest.X = src.x;
  dest.Y = src.y;
  dest.Z = src.z;
  dest.W = src.w;
  return dest;
}

FQuat4d StructTypesUtils::ConvertDouble(const ovrpQuatf& src)
{
  FQuat4d dest{};

  dest.X = static_cast<float>(src.x);
  dest.Y = static_cast<float>(src.y);
  dest.Z = static_cast<float>(src.z);
  dest.W = static_cast<float>(src.w);
  return dest;
}

FTransform StructTypesUtils::ConvertTransform(const ovrpPosef& src)
{
	FQuat4d TempOrientation = ConvertDouble(src.Orientation);
	FVector3d TempPosition = ConvertDouble(src.Position);
  return FTransform{ (FQuat)TempOrientation, (FVector)TempPosition };
}

FIsdkPosef StructTypesUtils::Convert(const ovrpPosef& src)
{
	FQuat4d TempOrientation = ConvertDouble(src.Orientation);
	FVector3f TempPosition = Convert(src.Position);
  return { (FQuat)TempOrientation, (FVector)TempPosition };
}
#pragma endregion Convert
