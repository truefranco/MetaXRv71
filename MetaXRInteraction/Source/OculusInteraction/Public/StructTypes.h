// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
// @generated

#pragma once

#include "StructTypesNonGenerated.h"
#include "VectorTypes.h"
#include "StructTypes.generated.h"

UENUM(BlueprintType)
enum class EIsdkAxisAlignedBox_BoxSide : uint8 {
  Xmin = 0,
  Xmax = 1,
  Ymin = 2,
  Ymax = 3,
  Zmin = 4,
  Zmax = 5
};

UENUM(BlueprintType)
enum class EIsdkDataSourceUpdateAttributeResult : uint8 {
  Unknown = 0,
  Modified = 1,
  NotModified = 2,
  DoesNotExist = 3,
  WrongValueType = 4
};

UENUM()
enum class EIsdkDataSourceUpdateDataResult {
  Failure_InvalidPrevDataSource = -1,
  Unknown = 0,
  Modified = 1,
  NotModified = 2
};

UENUM(BlueprintType)
enum class EIsdkDetection_FingerCalcType : uint8 {
  Curl = 0,
  Flexion = 1,
  Abduction = 2,
  Opposition = 3,
  Grab = 4,
  OppositionTangentPlane = 5,
  OppositionNormal = 6,
  OppositionTopTwo = 7,
  OppositionTopThree = 8
};

UENUM(BlueprintType)
enum class EIsdkDetection_ThumbCalcType : uint8 { Curl = 0, Flexion = 1 };

UENUM()
enum class EIsdkDigitRecognizer_UpdateResult {
  Failure = -1,
  Unknown = 0,
  Success = 1,
  FrameDataUnavailable = 2,
  FrameDataIsStale = 3
};

UENUM(BlueprintType)
enum class EIsdkFingerJoint : uint8 {
  Metacarpal = 0,
  Proximal = 1,
  Intermediate = 2,
  Distal = 3,
  Tip = 4
};

UENUM(BlueprintType)
enum class EIsdkFingerType : uint8 { Index = 0, Middle = 1, Ring = 2, Pinky = 3 };

UENUM(BlueprintType)
enum class EIsdkHandedness : uint8 { Left = 0, Right = 1 };

UENUM(BlueprintType)
enum class EIsdkInteractableState : uint8 { Normal = 0, Hover = 1, Select = 2, Disabled = 3 };

UENUM(BlueprintType)
enum class EIsdkInteractorState : uint8 { Normal = 0, Hover = 1, Select = 2, Disabled = 3 };

UENUM(BlueprintType)
enum class EIsdkNormalFacing : uint8 { Any = 0, In = 1, Out = 2 };

UENUM(BlueprintType)
enum class EIsdkOneEuroHandFilter_AttributeId : uint8 {
  Unknown = 0,
  WristPosBeta = 1,
  WristPosMinCutOff = 2,
  WristRotBeta = 3,
  WristRotMinCutOff = 4,
  FingerRotBeta = 5,
  FingerRotMinCutOff = 6,
  Frequency = 7,
  WristPosDeltaMinCutOff = 8,
  WristRotDeltaMinCutOff = 9,
  FingerRotDeltaMinCutOff = 10
};

UENUM(BlueprintType)
enum class EIsdkPointerEventType : uint8 {
  Hover = 0,
  Unhover = 1,
  Select = 2,
  Unselect = 3,
  Move = 4,
  Cancel = 5
};

UENUM()
enum class EIsdkSyntheticHand_WristLockMode { Position = 1, Rotation = 2, Full = 3 };

UENUM(BlueprintType)
enum class EIsdkThumbJoint : uint8 { Metacarpal = 0, Proximal = 1, Distal = 2, Tip = 3 };

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkDigitRangeParams {
  GENERATED_BODY()

  FIsdkDigitRangeParams();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float LowerLimit{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float LowerTolerance{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float UpperLimit{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float UpperTolerance{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MinTimeToTransition{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkDigitRecognizer_ExpectedAngleValueRange {
  GENERATED_BODY()

  FIsdkDigitRecognizer_ExpectedAngleValueRange();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MinValue{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MaxValue{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkDigitRecognizer_ExpectedDistanceValueRange {
  GENERATED_BODY()

  FIsdkDigitRecognizer_ExpectedDistanceValueRange();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MinValue{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MaxValue{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkExternalHandPositionFrame_FingerJointMapping {
  GENERATED_BODY()

  FIsdkExternalHandPositionFrame_FingerJointMapping();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkFingerType Finger{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkFingerJoint Joint{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  int Index{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkExternalHandPositionFrame_ThumbJointMapping {
  GENERATED_BODY()

  FIsdkExternalHandPositionFrame_ThumbJointMapping();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkThumbJoint Joint{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  int Index{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkFilterPropertyBlock {
  GENERATED_BODY()

  FIsdkFilterPropertyBlock();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MinCutOff{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float Beta{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float DeltaMinCutOff{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkFingerRecognizer_ExpectedFingerValueRanges {
  GENERATED_BODY()

  FIsdkFingerRecognizer_ExpectedFingerValueRanges();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedAngleValueRange Curl{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedAngleValueRange Flexion{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedAngleValueRange Abduction{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedDistanceValueRange Opposition{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedAngleValueRange Grab{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedDistanceValueRange OppositionTangentPlane{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedDistanceValueRange OppositionNormal{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedDistanceValueRange OppositionTopTwo{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedDistanceValueRange OppositionTopThree{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkInteractableStateChangeArgs {
  GENERATED_BODY()

  FIsdkInteractableStateChangeArgs();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkInteractableState PreviousState{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkInteractableState NewState{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkInteractorStateChangeArgs {
  GENERATED_BODY()

  FIsdkInteractorStateChangeArgs();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkInteractorState PreviousState{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkInteractorState NewState{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPokeInteractable_DragThresholdingConfig {
  GENERATED_BODY()

  FIsdkPokeInteractable_DragThresholdingConfig();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool Enabled{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float DragNormal{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float DragTangent{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPokeInteractable_MinThresholdsConfig {
  GENERATED_BODY()

  FIsdkPokeInteractable_MinThresholdsConfig();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool Enabled{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MinNormal{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPokeInteractable_PositionPinningConfig {
  GENERATED_BODY()

  FIsdkPokeInteractable_PositionPinningConfig();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool Enabled{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MaxPinDistance{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPokeInteractable_RecoilAssistConfig {
  GENERATED_BODY()

  FIsdkPokeInteractable_RecoilAssistConfig();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool Enabled{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool UseVelocityExpansion{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionMinSpeed{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionMaxSpeed{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionDistance{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionDecayRate{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool UseVelocityRetractExpansion{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionRetractMinSpeed{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionRetractMaxSpeed{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionRetractDistance{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float VelocityExpansionRetractDecayRate{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool UseDynamicDecay{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float ExitDistance{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float ReEnterDistance{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPokeInteractable_Config {
  GENERATED_BODY()

  FIsdkPokeInteractable_Config();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float EnterHoverNormal{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float EnterHoverTangent{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float ExitHoverNormal{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float ExitHoverTangent{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float CancelSelectNormal{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float CancelSelectTangent{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkPokeInteractable_PositionPinningConfig PositionPinningConfig{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkPokeInteractable_DragThresholdingConfig DragThresholdsConfig{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkPokeInteractable_MinThresholdsConfig MinThresholdsConfig{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkPokeInteractable_RecoilAssistConfig RecoilAssistConfig{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPokeInteractor_Config {
  GENERATED_BODY()

  FIsdkPokeInteractor_Config();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float TouchReleaseThreshold{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MaxDeltaFromTouchPoint{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float EqualDistanceThreshold{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float Radius{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool IgnorePositionPinning{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPokeInteractor_SurfaceHitCache {
  GENERATED_BODY()

  FIsdkPokeInteractor_SurfaceHitCache();
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkRayInteractor_Config {
  GENERATED_BODY()

  FIsdkRayInteractor_Config();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float MaxRayLength{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float EqualDistanceThreshold{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkRayInteractor_RayCandidateProperties {
  GENERATED_BODY()

  FIsdkRayInteractor_RayCandidateProperties();
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkSyntheticHand_Config {
  GENERATED_BODY()

  FIsdkSyntheticHand_Config();
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkThumbRecognizer_ExpectedThumbValueRanges {
  GENERATED_BODY()

  FIsdkThumbRecognizer_ExpectedThumbValueRanges();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedAngleValueRange Curl{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkDigitRecognizer_ExpectedAngleValueRange Flexion{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkAxisAlignedBox {
  GENERATED_BODY()

  FIsdkAxisAlignedBox();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector LowerBound{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector UpperBound{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector Centroid{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector HalfSize{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkBoundsClipper {
  GENERATED_BODY()

  FIsdkBoundsClipper();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  TScriptInterface<IIsdkIPose> PoseProvider{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector Position{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector Size{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkRay {
  GENERATED_BODY()

  FIsdkRay();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector Origin{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector Direction{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkSurfaceHit {
  GENERATED_BODY()

  FIsdkSurfaceHit();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector Point{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector Normal{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float Distance{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkOptionalSurfaceHit {
  GENERATED_BODY()

  FIsdkOptionalSurfaceHit();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool HasValue{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkSurfaceHit Value{};
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPointerEvent {
  GENERATED_BODY()

  FIsdkPointerEvent();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  int Identifier{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkPointerEventType Type{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkPosef Pose{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  TScriptInterface<IIsdkIPayload> Payload{};
};
