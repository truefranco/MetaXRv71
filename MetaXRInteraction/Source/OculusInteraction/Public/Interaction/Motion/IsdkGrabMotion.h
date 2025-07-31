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
#include "UObject/Object.h"
#include "IsdkGrabMotion.generated.h"

class UIsdkGrabTransformerComponent;
class UIsdkGrabMotion;
class UIsdkGrabbableComponent;
class UIsdkGrabberComponent;
class UCurveFloat;

/**
 * @brief UIsdkGrabMotion is an abstract base class allowing for different means of moving
 * grabbed objects relative to the grabber.
 *
 * @see UIsdkPullToHandGrabMotion
 * @see UIsdkRelativeToHandGrabMotion
 * @see UIsdkManipulateInPlaceGrabMotion
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class OCULUSINTERACTION_API UIsdkGrabMotion : public UObject
{
  GENERATED_BODY()

 public:
  /**
   * Start the motion.  This is expected to be called when a grab starts (ie, on select).
   * Note that all parameters are required to be valid, or grab motion will not start.
   * @param InGrabbableComponent The grabbable that motion is being applied to
   * @param InGrabberComponent The grabber that is applying the motion
   * @param InGrabTransformerComponent The grab transformer component associated with the grabbable
   * component
   */
  virtual void Start(
      UIsdkGrabbableComponent* InGrabbableComponent,
      UIsdkGrabberComponent* InGrabberComponent,
      UIsdkGrabTransformerComponent* InGrabTransformerComponent);

  /**
   * Stop the motion.  This is expected to be called when the grab ends (ie, on unselect)
   */
  virtual void Stop();

  /**
   * Tick the motion.  Expected to be called each frame by the grabber in order to facilitate
   * predictable ordering of ticking between grab motion and other grab logic.
   * @param DeltaTime the number of seconds that have elapsed since last tick
   */
  virtual void Tick(float DeltaTime) {}

  /**
   * @return true if the motion has started and has not yet stopped
   */
  virtual bool IsActive() const;

  /**
   * @return true if all dependencies are met and state is valid
   */
  virtual bool HasValidDependencies() const;

  /**
   * @return the current world space transform of the motion.  Valid immediately after Start() is
   * called.  It is expected that this is used as the transform for pointer events broadcast from
   * the motion's grabber while the motion is active.
   */
  const FTransform& GetTransform() const;

 protected:
  virtual UWorld* GetWorld() const override;

  // The current world space transform of the motion
  FTransform Transform;

  // Whether the motion has started and has not yet stopped
  bool bIsActive = false;

  // The grabbable that motion is being applied to
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabbableComponent* GrabbableComponent;

  // The grabber that is applying the motion
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabberComponent* GrabberComponent;

  // The grab transformer component associated with the grabbable component
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabTransformerComponent* GrabTransformerComponent;
};

/**
 * @brief UIsdkPullToHandGrabMotion implements a motion that pulls the grabbed object to the pinch
 * position or the palm grab position, depending on which method was used to start the grab.
 *
 * @see UIsdkGrabMotion
 */
UCLASS(CollapseCategories)
class OCULUSINTERACTION_API UIsdkPullToHandGrabMotion : public UIsdkGrabMotion
{
  GENERATED_BODY()

 public:
  virtual void Start(
      UIsdkGrabbableComponent* InGrabbableComponent,
      UIsdkGrabberComponent* InGrabberComponent,
      UIsdkGrabTransformerComponent* InGrabTransformerComponent) override;
  virtual void Tick(float DeltaTime) override;

  bool IsUsingAbsoluteTime() const;
  float GetPullSpeed() const;
  float GetPullTime() const;
  const FTransform& GetStartTransform() const;

 protected:
  /**
   * If bUseAbsoluteTime is true, this motion will move from its original position to the grabber's
   * position in a fixed amount of time, regardless of the distance between the two, as specified
   * by PullTime.  Otherwise, the grabbable will move at a speed described by PullSpeed.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  bool bUseAbsoluteTime = false;

  /**
   * The speed at which the grabbable will travel, if bUseAbsoluteTime is false.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      Category = InteractionSDK,
      meta = (EditConditionHides, EditCondition = "bUseAbsoluteTime==false"))
  float PullSpeed = 500.f;

  /**
   * The time for the grabbable will take to reach the grabber, if bUseAbsoluteTime is true.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      Category = InteractionSDK,
      meta = (EditConditionHides, EditCondition = "bUseAbsoluteTime==true"))
  float PullTime = 0.13f;

  /**
   * PullCurve can be used to modify the motion by which the grabbed object is pulled to hand.
   * If bUseAbsoluteTime is true, the x-axis is treated as the normalized time of travel, and the
   * y-axis should be a 0-1 value indicating distance from the grabber.
   * If bUseAbsoluteTime is false, the x-axis represents the elapsed time since motion started, in
   * seconds, and the y-axis represents a coefficient by which PullSpeed is multiplied.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  UCurveFloat* PullCurve;

  /**
   * When the motion started.
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  float StartTimestamp = 0.f;

  /**
   * The grabbable's world space transform when the grab started
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  FTransform StartTransform = FTransform::Identity;

  /**
   * The initial rotational offset between the grabber and the grabbable
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  FQuat StartRotationOffset = FQuat::Identity;

  /**
   * Whether the grabbable has reached the grabber yet.
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  bool bHasReachedTarget = false;
};

/**
 * @brief UIsdkRelativeToPointerGrabMotion implements a motion that moves the grabbed object
 * relative to the pointer pose's transform.  The grabbed object will keep its position relative to
 * the pointer, as if it were attached.
 *
 * @see UIsdkGrabMotion
 */
UCLASS(CollapseCategories)
class OCULUSINTERACTION_API UIsdkRelativeToPointerGrabMotion : public UIsdkGrabMotion
{
  GENERATED_BODY()

 public:
  virtual void Start(
      UIsdkGrabbableComponent* InGrabbableComponent,
      UIsdkGrabberComponent* InGrabberComponent,
      UIsdkGrabTransformerComponent* InGrabTransformerComponent) override;
  virtual void Tick(float DeltaTime) override;

 protected:
  // Transform from pointer to grabbed object space
  FTransform ToObjectFromPointer = FTransform::Identity;
};

/**
 * @brief UIsdkManipulateInPlaceGrabMotion implements a motion that moves the grabbed object as
 * if the grabber had started it's grab at the grabber's location.  Alternatively, it can be thought
 * of as applying the grabbers motion 1:1 to the grabbable, at whichever point the grabbable
 * was when it was grabbed.
 *
 * @see UIsdkGrabMotion
 */
UCLASS(CollapseCategories)
class OCULUSINTERACTION_API UIsdkManipulateInPlaceGrabMotion : public UIsdkGrabMotion
{
  GENERATED_BODY()

 public:
  virtual void Start(
      UIsdkGrabbableComponent* InGrabbableComponent,
      UIsdkGrabberComponent* InGrabberComponent,
      UIsdkGrabTransformerComponent* InGrabTransformerComponent) override;
  virtual void Tick(float DeltaTime) override;

  // Grabber world space transform at the start of the grab
  FTransform ToGrabberStartFromWorld = FTransform::Identity;

  // Grabbed object world space transform at the start of the grab
  FTransform ToObjectStartFromWorld = FTransform::Identity;

  // Transform from the grabber to the grabbed object at the start of the grab.
  FTransform ToObjectFromGrabber = FTransform::Identity;
};
