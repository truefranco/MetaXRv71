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
#include "Components/ActorComponent.h"
#include "../Pointable/IsdkInteractionPointerEvent.h"
#include "IsdkITransformer.h"
#include "IsdkGrabPoseCollection.h"
#include "../IsdkThrowable.h"
#include "IsdkGrabTransformerComponent.generated.h"

class UIsdkTransformer;
class UIsdkGrabMotion;

UENUM(Blueprintable)
enum class TransformEvent : uint8
{
  BeginTransform,
  UpdateTransform,
  EndTransform
};

/* State of the current Interactor Movement Override and original information needed */
USTRUCT()
struct FIsdkInteractorMoveSnapState
{
  GENERATED_USTRUCT_BODY()

  FTransform StartingTransform;

  float StartTime = 0.f;
  float StopTime = 0.f;
  float SecondsDuration = 0.f;

  void ResetState()
  {
    StartTime = 0.f;
    StopTime = 0.f;
    SecondsDuration = 0.f;
    StartingTransform = FTransform::Identity;
  }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FIsdkGrabTransformerEventDelegate,
    TransformEvent,
    Event,
    const UIsdkGrabTransformerComponent*,
    GrabTransformer);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FIsdkCancelGrabEventDelegate,
    int,
    InteractorID,
    UIsdkGrabTransformerComponent*,
    GrabTransformer);

/**
 * UIsdkGrabTransformer handles transform and throw behavior for grabbable components.  It does not
 * provide or handle the detection of grab events.  It works only if there is at least one
 * UIsdkGrabbableComponent to detect and pass along grab events.
 *
 * @see UIsdkGrabbableComponent
 */
UCLASS(
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "Grab Transformer Component"))
class OCULUSINTERACTION_API UIsdkGrabTransformerComponent : public UActorComponent
{
  GENERATED_BODY()

 public:
  // Sets default values for this component's properties
  UIsdkGrabTransformerComponent();

  FIsdkCancelGrabEventDelegate* GetCancelGrabEventDelegate();
  void ProcessPointerEvent(const FIsdkInteractionPointerEvent& Event);
  UIsdkGrabMotion* GetDistanceGrabMotion() const;

  /**
   * Sends a cancel event to all interactors that are currently grabbing this grab transformer
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void ForceCancel();

  /**
   * Set the transformer responsible for handling transform logic when a single
   * grabber is interacting with this grab transformer.
   *
   * Deprecated v74.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = InteractionSDK,
      meta =
          (DeprecatedFunction, DeprecationMessage = "Use SetSingleGrabTransformerObject instead"))
  void SetSingleGrabTransformer(TScriptInterface<IIsdkITransformer> Transformer);

  /**
   * Set the transformer responsible for handling transform logic when a single
   * grabber is interacting with this grab transformer.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetSingleGrabTransformerObject(UIsdkTransformer* Transformer);

  /**
   * Get the transformer responsible for handling transform logic when a single
   * grabber is interacting with this grab transformer.
   *
   * Deprecated v74.
   */
  UFUNCTION(
      BlueprintPure,
      Category = InteractionSDK,
      meta =
          (DeprecatedFunction,
           ReturnDisplayName = "SingleGrabTransformer",
           DeprecationMessage = "Use GetSingleGrabTransformerObject instead"))
  TScriptInterface<IIsdkITransformer> GetSingleGrabTransformer() const;

  /**
   * Get the transformer responsible for handling transform logic when a single
   * grabber is interacting with this grab transformer.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkTransformer* GetSingleGrabTransformerObject() const;

  /**
   * Set the transformer responsible for handling transform logic when multiple
   * grabbers are interacting with this grab transformer.
   *
   * Deprecated v74.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = InteractionSDK,
      meta = (DeprecatedFunction, DeprecationMessage = "Use SetMultiGrabTransformerObject instead"))
  void SetMultiGrabTransformer(TScriptInterface<IIsdkITransformer> Transformer);

  /**
   * Set the transformer responsible for handling transform logic when multiple
   * grabbers are interacting with this grab transformer.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetMultiGrabTransformerObject(UIsdkTransformer* Transformer);

  /**
   * Get the transformer responsible for handling transform logic when multiple
   * grabbers are interacting with this grab transformer.
   *
   * Deprecated v74.
   */
  UFUNCTION(
      BlueprintPure,
      Category = InteractionSDK,
      meta =
          (DeprecatedFunction,
           ReturnDisplayName = "MultiGrabTransformer",
           DeprecationMessage = "Use GetMultiGrabTransformerObject instead"))
  TScriptInterface<IIsdkITransformer> GetMultiGrabTransformer() const;

  /**
   * Get the transformer responsible for handling transform logic when multiple
   * grabbers are interacting with this grab transformer.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkTransformer* GetMultiGrabTransformerObject() const;

  /**
   * Set the target component around which transforms are calculated.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetTransformTarget(USceneComponent* NewTarget);

  /**
   * Get the target component around which transforms are calculated.
   */
  UFUNCTION(
      BlueprintPure,
      Category = InteractionSDK,
      meta = (ReturnDisplayName = "TransformTarget"))
  virtual USceneComponent* GetTransformTarget() const;

  /**
   * Gets a list of grab poses, which represent the points at which this grab transformer
   * is being grabbed.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK, meta = (ReturnDisplayName = "GrabPoses"))
  const FIsdkGrabPoseCollection& GetGrabPoses() const;

  /**
   * Get the transformer which is currently driving transform logic
   */
  UFUNCTION(
      BlueprintPure,
      Category = InteractionSDK,
      meta = (ReturnDisplayName = "ActiveGrabTransformer"))
  TScriptInterface<IIsdkITransformer> GetActiveGrabTransformer() const;

  FIsdkGrabTransformerEventDelegate* GetGrabTransformerEventDelegate();
  EIsdkMultiGrabBehavior GetMultiGrabBehavior() const;
  int GetNumGrabbers() const;

  /* Overrides the current transform by aligning the relative SnapPoint to the rotation and location
   * of the Interactor*/
  void SetInteractorSnap(FTransform& RelativeSnapPoint, float MoveSnapDuration /* =0.f*/);

 protected:
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  void BeginTransform();
  void UpdateTransform();
  void EndTransform();

  FIsdkTargetTransform TargetInitialTransform;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabMotion* DistanceGrabMotion;

  /**
   * If specified, TransformTargetName is used to search all the components on this actor.
   * If a component with a matching name is found, it will be set as the TransformTarget.
   */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "InteractionSDK")
  FName TransformTargetName;

  /**
   * MultiGrabBehavior specifies how this grab transformer should respond to interaction with
   * multiple grabbers.
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK")
  EIsdkMultiGrabBehavior MultiGrabBehavior = EIsdkMultiGrabBehavior::MultiGrab;

  /**
   * If enabled, the grabbed object will have its velocity and angular velocity set
   * based on its current motion when released.
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InteractionSDK|Throwable")
  bool bIsThrowable = true;

  /**
   * If true, the grabbed object will retain the gravity settings on the mesh, otherwise it will set
   * the opposite value when thrown.
   */
  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      meta = (EditCondition = "bIsThrowable == true", EditConditionHides),
      Category = "InteractionSDK|Throwable")
  bool bRetainGravityWhenThrown = true;

  /**
   * The transformer responsible for handling single-grabber behavior
   */
  UPROPERTY(EditAnywhere, Category = InteractionSDK)
  UIsdkTransformer* SingleGrabTransformerObject;

  /**
   * The transformer responsible for handling multiple-grabber behavior
   */
  UPROPERTY(
      EditAnywhere,
      Category = InteractionSDK,
      meta =
          (EditCondition = "MultiGrabBehavior == EIsdkMultiGrabBehavior::MultiGrab",
           EditConditionHides))
  UIsdkTransformer* MultiGrabTransformerObject;

 private:
  /**
   * GrabTransformerEvent fires when any transform has taken place
   */
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkGrabTransformerEventDelegate GrabTransformerEvent;

  /**
   * CancelGrabEvent fires when a grab has been forcibly canceled
   */
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkCancelGrabEventDelegate CancelGrabEvent;

  /**
   * The transformer responsible for handling single-grabber behavior.  Deprecated v74.
   */
  UPROPERTY(
      BlueprintGetter = GetSingleGrabTransformer,
      BlueprintSetter = SetSingleGrabTransformer,
      Category = InteractionSDK,
      meta = (DeprecatedProperty, DeprecationMessage = "Use SingleGrabTransformerObject instead"))
  TScriptInterface<IIsdkITransformer> SingleGrabTransformer_DEPRECATED;

  /**
   * The transformer responsible for handling multiple-grabber behavior.  Deprecated v74.
   */
  UPROPERTY(
      BlueprintGetter = GetMultiGrabTransformer,
      BlueprintSetter = SetMultiGrabTransformer,
      Category = InteractionSDK,
      meta = (DeprecatedProperty, DeprecationMessage = "Use MultiGrabTransformerObject instead"))
  TScriptInterface<IIsdkITransformer> MultiGrabTransformer_DEPRECATED;

  /**
   * The target component around which transforms take place
   */
  UPROPERTY(
      BlueprintSetter = SetTransformTarget,
      BlueprintGetter = GetTransformTarget,
      meta = (ExposeOnSpawn = true),
      Category = "InteractionSDK")
  USceneComponent* TransformTarget;

  /**
   * A collection of information about current grabs
   */
  UPROPERTY(BlueprintGetter = GetGrabPoses, Category = "InteractionSDK")
  FIsdkGrabPoseCollection GrabPoses;

  /**
   * The transformer that is actively driving the transformation of this grab transformer
   */
  TScriptInterface<IIsdkITransformer> ActiveGrabTransformer = nullptr;

  /**
   * Settings that drive the throw behavior of this grab transformer
   */
  UPROPERTY(EditAnywhere, Category = "InteractionSDK|Throwable")
  FIsdkThrowableSettings ThrowSettings;

  UPROPERTY()
  UIsdkThrowable* ThrowableComponent;

  bool bWasSimulatingPhysics = false;
  bool bHadGravity = false;
  bool bWasPhysicsCached = false;

  bool bInteractorSnapEnabled = false;
  bool bInteractorMoveSnapEnabled = false;
  bool bFirstSnapFrame = false;

  bool bQueuedInteractorSnap = false;
  FTransform QueuedInteractorSnapPoint = FTransform::Identity;
  float QueuedMoveSnapDuration = 0.f;

  // Resets transformer state
  void RestartTransformer();
  void UpdateTransformerConstraints();

  FTransform CalculateInteractorSnapTransform(
      FTransform& TargetTransformIn,
      FTransform& InteractorTransformIn);

  void ExecuteInteractorSnap(FTransform& RelativeSnapPoint, float MoveSnapDuration);

  UPROPERTY()
  USceneComponent* LastInteractor = nullptr;
  UPROPERTY()
  FTransform InteractorSnapOffset = FTransform::Identity;
  UPROPERTY()
  FIsdkInteractorMoveSnapState MoveSnapState;
};
