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

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "StructTypes.h"
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "Interaction/Grabbable/IsdkITransformer.h"
#include "IsdkFunctionLibrary.generated.h"

class UIsdkGrabbableComponent;
class UWidget;
enum class EControllerHandBehavior : uint8;

UCLASS(BlueprintType)
class OCULUSINTERACTION_API UIsdkFunctionLibrary final : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()

 public:
  // Helpers
  UFUNCTION(BlueprintPure, Category = InteractionSDK, meta = (NativeMakeFunc))
  static TArray<FIsdkBoundsClipper> MakeBoundsClippersFromPose(
      const TScriptInterface<IIsdkIPose>& InPose,
      const FVector2D Size);

  /**
   * @brief Finds the tracking subsystem for the given actor
   * @param World which world contains the subsystem
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK, meta = (NativeMakeFunc))
  static TScriptInterface<IIsdkITrackingDataSubsystem> FindTrackingDataSubsystem(UWorld* World);

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  static FIsdkExternalHandPositionFrame_ThumbJointMapping MapXrThumbJoint(
      EIsdkThumbJoint Joint,
      int Index)
  {
    FIsdkExternalHandPositionFrame_ThumbJointMapping Mapping;
    Mapping.Joint = Joint;
    Mapping.Index = Index;
    return Mapping;
  }

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  static FIsdkExternalHandPositionFrame_FingerJointMapping
  MapXrFingerJoint(EIsdkFingerType Finger, EIsdkFingerJoint Joint, int Index)
  {
    FIsdkExternalHandPositionFrame_FingerJointMapping Mapping;
    Mapping.Finger = Finger;
    Mapping.Joint = Joint;
    Mapping.Index = Index;
    return Mapping;
  }

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  static TArray<FIsdkExternalHandPositionFrame_ThumbJointMapping> GetDefaultOpenXRThumbMapping();

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  static TArray<FIsdkExternalHandPositionFrame_FingerJointMapping> GetDefaultOpenXRFingerMapping();

  /*
   * Get the index the given finger and joint combination in the provided hand joint mapping.
   */
  static int GetFingerMappingIndex(
      const UIsdkHandJointMappings* HandJointMappings,
      EIsdkFingerType Finger,
      EIsdkFingerJoint Joint);

  /*
   * Get the index the thumb joint in the provided hand joint mapping.
   */
  static int GetThumbMappingIndex(
      const UIsdkHandJointMappings* HandJointMappings,
      EIsdkThumbJoint Joint);

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  static TArray<float> GetDefaultJointRadii();

  /* Will iterate through every component of an actor, get all ISDK GameplayTags via interface, and
   * collate them into one container */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static bool GetISDKGameplayTagsFromActor(
      const AActor* ActorIn,
      FGameplayTagContainer& ContainerOut);

  /**
   * Get the current ControllerHandBehavior
   * @return the current ControllerHandBehavior
   */
  UFUNCTION(
      BlueprintCallable,
      Category = InteractionSDK,
      meta = (WorldContext = WorldContextObject, ReturnDisplayName = ControllerHandBehavior))
  static EControllerHandBehavior GetControllerHandBehavior(UObject* WorldContextObject);

  /**
   * Set the behavior and display of the controller and hands when holding a controller.  May
   * behave differently on the oculus runtime than on other runtimes.
   *
   * Controller and Hands (Animated) - Show both the controller and hands.  Hands will use an
   *   animation blueprint to drive their animations.
   *
   * Controller Only - Show only the controller.
   *
   * Hands Only (Procedural) - Show only the hands.  Hands will be posed by runtime bone transforms
   *   coming from the oculus runtime.
   *
   * Hands Only (Animated) - Show only the hands.  Hands will use an animation blueprint to drive
   *   their animations.
   *
   * Note that for non-oculus runtimes, the procedural variants are disabled and will
   * fall back to their animated counterparts.
   *
   * @param WorldContextObject A UObject from which to find the UWorld reference from
   * @param ControllerHandBehavior the ControllerHandBehavior to set
   */
  UFUNCTION(
      BlueprintCallable,
      Category = InteractionSDK,
      meta = (WorldContext = WorldContextObject))
  static void SetControllerHandBehavior(
      UObject* WorldContextObject,
      EControllerHandBehavior ControllerHandBehavior);

  static FIsdkTrackedDataSubsystem_ControllerHandsBehaviorChangedDelegate*
  GetControllerHandBehaviorDelegate(UObject* WorldContextObject);

  UFUNCTION(BlueprintPure, Category = InteractionSDK, meta = (BlueprintThreadSafe))
  static FIsdkGrabPose
  MakeGrabPoseStruct(int Identifier, const FVector& Location, const FQuat& Orientation)
  {
    auto Pose = FIsdkPosef();
    Pose.Position = FVector(Location);
    Pose.Orientation = Orientation;
    return FIsdkGrabPose(Identifier, Pose);
  }

  UFUNCTION(BlueprintPure, Category = InteractionSDK, meta = (BlueprintThreadSafe))
  static FIsdkTargetTransform MakeTargetTransformStruct(const USceneComponent* Target)
  {
    return FIsdkTargetTransform(Target);
  }

  /* For a given Transform representing a hand joint, mirror it (reflecting across the thumb side)
   * and return the new transform */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK, meta = (BlueprintThreadSafe))
  static FTransform MirrorHandTransform(FTransform HandTransformIn);

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static UIsdkGrabbableComponent* FindGrabbableByComponent(USceneComponent* Component);
};
