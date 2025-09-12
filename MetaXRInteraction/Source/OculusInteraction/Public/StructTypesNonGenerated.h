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
#include "UObject/Interface.h"

// Includes needed in StructTypes.h
class IIsdkIPose;
class IIsdkIActiveState;

#include "StructTypesNonGenerated.generated.h"

UENUM(BlueprintType)
enum class EIsdkLerpState : uint8
{
  Inactive = 0,
  TransitioningTo = 1,
  TransitioningAway = 2,
  RestingAtDestination = 3
};

UENUM(BlueprintType)
enum class EIsdkHandGrabPoseMode : uint8
{
  None,
  SnapPoseToObject,
  SnapObjectToPose,
  MirrorReference
};

/**
*  @nolint
 * EIsdkHandGrabPoseMirror the different modes a hand grab pose will utilize pose & transform
 * mirroring
   None - This pose will utilize no mirroring and only activate for the hand it is set to
 * Automatic - The plugin will automatically create a mirrored version of this hand pose, using
 * relevant properties
   Manual - The plugin will pair this pose with another placed as a child of
 * this one, and set to the "MirrorReference" mode
 */
UENUM(BlueprintType)
enum class EIsdkHandGrabPoseMirror : uint8
{
  None,
  Automatic,
  Manual
};

USTRUCT(BlueprintType)
struct OCULUSINTERACTION_API FIsdkPosef
{
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FQuat Orientation = FQuat::Identity;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FVector3f Position = FVector3f::ZeroVector;

  FTransform ToTransform() const
  {
    return FTransform(Orientation, FVector(Position));
  }

  void FromTransform(FTransform transform)
  {
    Orientation = transform.GetRotation();
    Position = (FVector3f)transform.GetLocation();
  }
};

USTRUCT(Category = InteractionSDK, BlueprintType)
struct OCULUSINTERACTION_API FIsdkInteractionRelationshipCounts
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  int NumHover{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  int NumSelect{};
};

class UIsdkStandardEntity;
class UIsdkStandardPayload;

/**
 * Dummy payload type - we don't use the unreal type below. Payloads are looked up manually from the
 * isdk_Payload handle in the CreatePointerEventConverter.
 */
UINTERFACE(NotBlueprintable)
class OCULUSINTERACTION_API UIsdkIPayload : public UInterface
{
  GENERATED_BODY()
};
class OCULUSINTERACTION_API IIsdkIPayload
{
  GENERATED_BODY()
};

/**
 * EIsdkControllerDrivenHandPoseType is the ISDK analogue to Meta XR's
 * EOculusXRControllerDrivenHandPoseTypes, and is intended only internal use, for cross-plugin
 * communication between ISDK and Meta XR.
 */
UENUM()
enum class EIsdkXRControllerDrivenHandPoseType : uint8
{
  None = 0, // Controllers do not generate any hand poses.
  Natural, // Controller button inputs will be used to generate a normal hand pose.
  Controller, // Controller button inputs will be used to generate a hand pose holding a controller.
};

/**
 * EIsdkXRControllerType is the ISDK analogue to Meta XR's EOculusXRControllerType,
 * and is intended only internal use, for cross-plugin communication between ISDK and Meta XR.
 */
UENUM()
enum class EIsdkXRControllerType : uint8
{
  None = 0,
  MetaQuestTouch = 1,
  MetaQuestTouchPro = 2,
  MetaQuestTouchPlus = 3,
  Unknown = 0x7f,
};

/**
 * EControllerHandBehavior drives how we should present the user's hands when they are holding a
 * controller.
 */
UENUM(BlueprintType)
enum class EControllerHandBehavior : uint8
{
  BothProcedural = 0 UMETA(DisplayName = "Controller and Hands (Procedural, Quest Only)"),
  BothAnimated UMETA(DisplayName = "Controller and Hands (Animated)"),
  ControllerOnly UMETA(DisplayName = "Controller Only"),
  HandsOnlyProcedural UMETA(DisplayName = "Hands Only (Procedural, Quest Only)"),
  HandsOnlyAnimated UMETA(DisplayName = "Hands Only (Animated)")
};

/**
 * @brief EIsdkHandGrabPoseMirror the different orientations of a hand grab pose, used for mirroring
 * purposes
 */
UENUM(BlueprintType)
enum class EIsdkHandGrabPoseOrientation : uint8
{
  Unknown,
  WristFacingDown,
  WristFacingIn,
  WristFacingUp
};

/**
 * 
 */
UENUM(BlueprintType)
enum class EIsdkDigitType : uint8
{
  Unknown,
  Index,
  Middle,
  Ring,
  Pinky,
  Thumb
};

USTRUCT(BlueprintType)
struct FIsdkHandGrabPoseProperties
{
  GENERATED_USTRUCT_BODY()

  /**
   * The different modes hand grab poses operate when applying a pose.
   * None - Apply no pose when grabbing
   * SnapPoseToObject - The hand will snap to the object in the given pose, regardless of it's
   * tracked position.
   * SnapObjectToPose - The object will instantly snap to the hand to match the set
   * pose.
   * MoveObjectToPose - The object will move (lerp) to the hand in a given position to match the set
   * pose
   * MirrorReference - Set only when this hand grab pose is intended to be targeted for a
   * manual pose mirror arrangement
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK")
  EIsdkHandGrabPoseMode PoseMode = EIsdkHandGrabPoseMode::SnapObjectToPose;

  /* How long (in seconds) it will take the pose/object to move to its final snap position. If zero,
   * this will snap instantly. Otherwise it will lerp over time. */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK")
  float SnapMoveDuration = 0.25f;

  /* Whether or not this pose can be mirrored to the other hand for which it was not authored */
  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = "InteractionSDK",
      meta =
          (EditCondition = "PoseMode != EIsdkHandGrabPoseMode::MirrorReference",
           EditConditionHides))
  EIsdkHandGrabPoseMirror MirroringMode = EIsdkHandGrabPoseMirror::None;

  /* If true, will automatically derive an appropriate mirrored location across an assigned plane. Otherwise the mirrored version will have the same position/rotation as the original.*/
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = InteractionSDK,
      meta =
          (EditCondition = "MirroringMode == EIsdkHandGrabPoseMirror::Automatic",
           EditConditionHides))
  bool bMirrorLocationAndRotation = false;

  /* The axis about which to mirror the hand pose.  Note, this corresponds to the axis of the hand
   * pose's attach parent (the component directly above the hand pose in the scene component
   * hierarchy, not the axis of the hand pose itself. */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = InteractionSDK,
      meta =
          (EditCondition = "(MirroringMode == EIsdkHandGrabPoseMirror::Automatic) && (bMirrorLocationAndRotation == true)",
           EditConditionHides))
  TEnumAsByte<EAxis::Type> MirrorAxis;

  bool operator==(const FIsdkHandGrabPoseProperties& other) const
  {
    return PoseMode == other.PoseMode && SnapMoveDuration == other.SnapMoveDuration &&
        bMirrorLocationAndRotation == bMirrorLocationAndRotation && MirrorAxis == other.MirrorAxis;
  }

  bool operator!=(const FIsdkHandGrabPoseProperties& other) const
  {
    return !operator==(other);
  }
};

/**
 * @brief EIsdkGrabDetectorType represents a method of detection for grabs.
 */
UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EIsdkGrabDetectorType : uint32
{
  None = 0 UMETA(Hidden),
  HandGrab = 1 << 0,
  DistanceGrab = 1 << 1,
  RayGrab = 1 << 2
};
ENUM_CLASS_FLAGS(EIsdkGrabDetectorType);

/**
 * EIsdkMultiGrabBehavior represents what happens when multiple grabbers grab a single grabbable.
 * SingleGrabFirstRetained - Only the first grabber is allowed to grab
 * SingleGrabTransferToSecond - The first grabber stops grabbing, the second grabber starts
 * MultiGrab - All grabbers can grab at the same time
 */
UENUM(BlueprintType)
enum class EIsdkMultiGrabBehavior : uint8
{
  SingleGrabFirstRetained,
  SingleGrabTransferToSecond,
  MultiGrab
};

/**
 * @brief EIsdkGrabInputMethod represents a method of input that can trigger a grab.
 */
UENUM(BlueprintType, meta = (Bitflags))
enum class EIsdkGrabInputMethod : uint8
{
  Unknown = 0 UMETA(DisplayName = "None"),
  Pinch = 1,
  Palm = 1 << 1,
  Custom = 1 << 2 UMETA(Hidden)
};
ENUM_CLASS_FLAGS(EIsdkGrabInputMethod)
