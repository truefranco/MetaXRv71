// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// @generated

#pragma once

#include "StructTypes.h"
#include "VectorTypes.h"
#include "Quaternion.h"
#include "isdk_api/isdk_api.hpp"

class OCULUSINTERACTION_API StructTypesUtils
{
 public:
  static void CreateStructTypesStatics();

  static void Copy(const FIsdkFilterPropertyBlock& src, isdk_FilterPropertyBlock& dest);
  static void Copy(const FIsdkInteractableStateChangeArgs& src, isdk_InteractableStateChangeArgs& dest);
  static void Copy(const FIsdkInteractorStateChangeArgs& src, isdk_InteractorStateChangeArgs& dest);
  static void Copy(const FIsdkPokeInteractable_PositionPinningConfig& src, isdk_PokeInteractable_PositionPinningConfig& dest);
  static void Copy(const FIsdkPokeInteractable_DragThresholdingConfig& src, isdk_PokeInteractable_DragThresholdingConfig& dest);
  static void Copy(const FIsdkPokeInteractable_MinThresholdsConfig& src, isdk_PokeInteractable_MinThresholdsConfig& dest);
  static void Copy(const FIsdkPokeInteractable_RecoilAssistConfig& src, isdk_PokeInteractable_RecoilAssistConfig& dest);
  static void Copy(const FIsdkPokeInteractable_Config& src, isdk_PokeInteractable_Config& dest);
  static void Copy(const FIsdkPokeInteractor_Config& src, isdk_PokeInteractor_Config& dest);
  static void Copy(const FIsdkAxisAlignedBox& src, isdk_AxisAlignedBox& dest);
  static void Copy(const FIsdkSyntheticHand_Config& src, isdk_SyntheticHand_Config& dest) {}
  static void Copy(const FIsdkRay& src, isdk_Ray& dest);
  static void Copy(const FIsdkSurfaceHit& src, isdk_SurfaceHit& dest);
  static void Copy(const FIsdkRayInteractor_Config& src, isdk_RayInteractor_Config& dest);
  static void Copy(const FIsdkRayInteractor_RayCandidateProperties& src, isdk_RayInteractor_RayCandidateProperties& dest) {}
  static void Copy(const FIsdkOptionalSurfaceHit& src, isdk_OptionalSurfaceHit& dest);
  static void Copy(const FIsdkDigitRangeParams& src, isdk_DigitRangeParams& dest);
  static void Copy(const FIsdkExternalHandPositionFrame_ThumbJointMapping& Src, isdk_ExternalHandPositionFrame_ThumbJointMapping& Dest);
  static void Copy(const FIsdkExternalHandPositionFrame_FingerJointMapping& Src, isdk_ExternalHandPositionFrame_FingerJointMapping& Dest);
  static void Copy(const FIsdkDigitRecognizer_ExpectedAngleValueRange& Src, isdk_DigitRecognizer_ExpectedAngleValueRange& Dest);
  static void Copy(const FIsdkDigitRecognizer_ExpectedDistanceValueRange& Src, isdk_DigitRecognizer_ExpectedDistanceValueRange& Dest);
  static void Copy(const FIsdkFingerRecognizer_ExpectedFingerValueRanges& Src, isdk_FingerRecognizer_ExpectedFingerValueRanges& Dest);
  static void Copy(const FIsdkThumbRecognizer_ExpectedThumbValueRanges& Src, isdk_ThumbRecognizer_ExpectedThumbValueRanges& Dest);
  static void Copy(const FVector3f& src, ovrpVector3f& dest);
  static void Copy(const FVector3d& src, ovrpVector3f& dest);
  static void Copy(const FQuat4f& src, ovrpQuatf& dest);
  static void Copy(const FQuat4d& src, ovrpQuatf& dest);
  static void Copy(const FTransform& Src, ovrpPosef& Dest);
  static void Copy(const FIsdkPosef& Src, ovrpPosef& Dest);

  static void Copy(const isdk_FilterPropertyBlock& src, FIsdkFilterPropertyBlock& dest);
  static void Copy(const isdk_InteractableStateChangeArgs& src, FIsdkInteractableStateChangeArgs& dest);
  static void Copy(const isdk_InteractorStateChangeArgs& src, FIsdkInteractorStateChangeArgs& dest);
  static void Copy(const isdk_PokeInteractable_PositionPinningConfig& src, FIsdkPokeInteractable_PositionPinningConfig& dest);
  static void Copy(const isdk_PokeInteractable_DragThresholdingConfig& src, FIsdkPokeInteractable_DragThresholdingConfig& dest);
  static void Copy(const isdk_PokeInteractable_MinThresholdsConfig& src, FIsdkPokeInteractable_MinThresholdsConfig& dest);
  static void Copy(const isdk_PokeInteractable_RecoilAssistConfig& src, FIsdkPokeInteractable_RecoilAssistConfig& dest);
  static void Copy(const isdk_PokeInteractable_Config& src, FIsdkPokeInteractable_Config& dest);
  static void Copy(const isdk_PokeInteractor_Config& src, FIsdkPokeInteractor_Config& dest);
  static void Copy(const isdk_AxisAlignedBox& src, FIsdkAxisAlignedBox& dest);
  static void Copy(const isdk_SyntheticHand_Config& src, FIsdkSyntheticHand_Config& dest) {}
  static void Copy(const isdk_Ray& src, FIsdkRay& dest);
  static void Copy(const isdk_SurfaceHit& src, FIsdkSurfaceHit& dest);
  static void Copy(const isdk_RayInteractor_Config& src, FIsdkRayInteractor_Config& dest);
  static void Copy(const isdk_RayInteractor_RayCandidateProperties& src, FIsdkRayInteractor_RayCandidateProperties& dest) {}
  static void Copy(const isdk_OptionalSurfaceHit& src, FIsdkOptionalSurfaceHit& dest);
  static void Copy(const isdk_DigitRangeParams& src, FIsdkDigitRangeParams& dest);
  static void Copy(const isdk_ExternalHandPositionFrame_ThumbJointMapping& Src, FIsdkExternalHandPositionFrame_ThumbJointMapping& Dest);
  static void Copy(const isdk_ExternalHandPositionFrame_FingerJointMapping& Src, FIsdkExternalHandPositionFrame_FingerJointMapping& Dest);
  static void Copy(const isdk_DigitRecognizer_ExpectedAngleValueRange& Src, FIsdkDigitRecognizer_ExpectedAngleValueRange& Dest);
  static void Copy(const isdk_DigitRecognizer_ExpectedDistanceValueRange& Src, FIsdkDigitRecognizer_ExpectedDistanceValueRange& Dest);
  static void Copy(const isdk_FingerRecognizer_ExpectedFingerValueRanges& Src, FIsdkFingerRecognizer_ExpectedFingerValueRanges& Dest);
  static void Copy(const isdk_ThumbRecognizer_ExpectedThumbValueRanges& Src, FIsdkThumbRecognizer_ExpectedThumbValueRanges& Dest);
  static void Copy(const ovrpVector3f& src, FVector3f& dest);
  static void Copy(const ovrpVector3f& src, FVector3d& dest);
  static void Copy(const ovrpQuatf& src, FQuat4f& dest);
  static void Copy(const ovrpQuatf& src, FQuat4d& dest);
  static void Copy(const ovrpPosef& Src, FTransform& Dest);
  static void Copy(const ovrpPosef& Src, FIsdkPosef& Dest);

  static isdk_FilterPropertyBlock Convert(const FIsdkFilterPropertyBlock& src);
  static isdk_InteractableStateChangeArgs Convert(const FIsdkInteractableStateChangeArgs& src);
  static isdk_InteractorStateChangeArgs Convert(const FIsdkInteractorStateChangeArgs& src);
  static isdk_PokeInteractable_PositionPinningConfig Convert(const FIsdkPokeInteractable_PositionPinningConfig& src);
  static isdk_PokeInteractable_DragThresholdingConfig Convert(const FIsdkPokeInteractable_DragThresholdingConfig& src);
  static isdk_PokeInteractable_MinThresholdsConfig Convert(const FIsdkPokeInteractable_MinThresholdsConfig& src);
  static isdk_PokeInteractable_RecoilAssistConfig Convert(const FIsdkPokeInteractable_RecoilAssistConfig& src);
  static isdk_PokeInteractable_Config Convert(const FIsdkPokeInteractable_Config& src);
  static isdk_PokeInteractor_Config Convert(const FIsdkPokeInteractor_Config& src);
  static isdk_AxisAlignedBox Convert(const FIsdkAxisAlignedBox& src);
  static isdk_SyntheticHand_Config Convert(const FIsdkSyntheticHand_Config& src) { return {}; }
  static isdk_Ray Convert(const FIsdkRay& src);
  static isdk_SurfaceHit Convert(const FIsdkSurfaceHit& src);
  static isdk_RayInteractor_Config Convert(const FIsdkRayInteractor_Config& src);
  static isdk_RayInteractor_RayCandidateProperties Convert(const FIsdkRayInteractor_RayCandidateProperties& src) { return {}; }
  static isdk_OptionalSurfaceHit Convert(const FIsdkOptionalSurfaceHit& src);
  static ovrpVector2f Convert(const FVector2f& src);
  static ovrpVector2f Convert(const FVector2d& src);
  static ovrpVector3f Convert(const FVector3f& src);
  static ovrpVector3f Convert(const FVector3d& src);
  static ovrpQuatf Convert(const FQuat4f& src);
  static ovrpQuatf Convert(const FQuat4d& src);
  static ovrpPosef Convert(const FTransform& src);
  static ovrpPosef Convert(const FIsdkPosef& src);

  static FIsdkFilterPropertyBlock Convert(const isdk_FilterPropertyBlock& src);
  static FIsdkInteractableStateChangeArgs Convert(const isdk_InteractableStateChangeArgs& src);
  static FIsdkInteractorStateChangeArgs Convert(const isdk_InteractorStateChangeArgs& src);
  static FIsdkPokeInteractable_PositionPinningConfig Convert(const isdk_PokeInteractable_PositionPinningConfig& src);
  static FIsdkPokeInteractable_DragThresholdingConfig Convert(const isdk_PokeInteractable_DragThresholdingConfig& src);
  static FIsdkPokeInteractable_MinThresholdsConfig Convert(const isdk_PokeInteractable_MinThresholdsConfig& src);
  static FIsdkPokeInteractable_RecoilAssistConfig Convert(const isdk_PokeInteractable_RecoilAssistConfig& src);
  static FIsdkPokeInteractable_Config Convert(const isdk_PokeInteractable_Config& src);
  static FIsdkPokeInteractor_Config Convert(const isdk_PokeInteractor_Config& src);
  static FIsdkAxisAlignedBox Convert(const isdk_AxisAlignedBox& src);
  static FIsdkSyntheticHand_Config Convert(const isdk_SyntheticHand_Config& src) { return {}; }
  static FIsdkRay Convert(const isdk_Ray& src);
  static FIsdkSurfaceHit Convert(const isdk_SurfaceHit& src);
  static FIsdkRayInteractor_Config Convert(const isdk_RayInteractor_Config& src);
  static FIsdkRayInteractor_RayCandidateProperties Convert(const isdk_RayInteractor_RayCandidateProperties& src) { return {}; }
  static FIsdkOptionalSurfaceHit Convert(const isdk_OptionalSurfaceHit& src);
  static FVector3f Convert(const ovrpVector3f& src);
  static FVector3d ConvertDouble(const ovrpVector3f& src);
  static FQuat4f Convert(const ovrpQuatf& src);
  static FQuat4d ConvertDouble(const ovrpQuatf& src);
  static FTransform ConvertTransform(const ovrpPosef& src);
  static FIsdkPosef Convert(const ovrpPosef& src);
};
