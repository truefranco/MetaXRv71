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
#include "IsdkSceneInteractorComponent.h"
#include "DataSources/IsdkIHandPointerPose.h"
#include "Grabbable/IsdkGrabTransformerComponent.h"
#include "IsdkGrabberComponent.generated.h"

class UIsdkGrabDetector;
class UNiagaraSystem;
class UNiagaraComponent;
class UIsdkHandGrabDetector;
class UIsdkDistanceGrabDetector;
class UIsdkRayGrabDetector;
enum class EIsdkGrabInputMethod : uint8;
class UIsdkGrabbableComponent;
class USphereComponent;

/**
 * @class UIsdkGrabberComponent
 * IsdkGrabberComponent drives the ability for a pawn to interact with actors that have an
 * IsdkGrabbableComponent attached to them.  This component uses a number of configurable colliders
 * per input method (eg, pinch / palm grab) to drive detection of grabbables.  Selection/Unselection
 * is expected to be driven externally by an IsdkGrabInteractionRigComponent.
 * @see UIsdkSceneInteractorComponent
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Grabber Component"))
class OCULUSINTERACTION_API UIsdkGrabberComponent : public UIsdkSceneInteractorComponent
{
  GENERATED_BODY()

 public:
  UIsdkGrabberComponent();
  virtual void BeginPlay() override;
  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void Deactivate() override;

  /**
   * Grab (select) any interactables its pinch colliders are hovering over.
   */
  void PinchGrab();

  /**
   * Grab (select) any interactables its palm colliders are hovering over.
   */
  void PalmGrab();

  /**
   * Release (unselect) any interactables selected by pinch colliders.
   */
  void PinchRelease();

  /**
   * Release (unselect) any interactables selected by palm colliders.
   */
  void PalmRelease();

  /**
   * @return A 0-1 value (inclusive) indicating the closeness of the index finger and thumb.
   */
  float GetPinchStrength() const;

  /**
   * Sets the pinch strength.  This is intended to be set by the grab interaction rig component
   */
  void SetPinchStrength(float InPinchStrength);

  /**
   * @return true if this grabber has a valid grabbed component
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK, meta = (ReturnDisplayName = "bIsGrabbing"))
  bool IsGrabbing() const;

  /**
   * @return the component selected (grabbed) by this grabbable
   */
  UFUNCTION(
      BlueprintPure,
      Category = InteractionSDK,
      meta = (ReturnDisplayName = "GrabbedComponent"))
  UIsdkGrabbableComponent* GetGrabbedComponent() const;

  /**
   * Check to see if this grabber is hovering over anything for any input method
   * @return true if we are hovering over any interactables
   */
  virtual bool HasInteractable() const override;

  /**
   * Check to see if this grabber is selecting anything for any input method.
   * @return true if we are selecting any interactables
   */
  virtual bool HasSelectedInteractable() const override;

  /**
   * Get the number of components this grabber is hovering over and selecting.
   * @return the number of components this grabber is hovering over and selecting.
   */
  virtual FIsdkInteractionRelationshipCounts GetInteractorStateRelationshipCounts() const override;

  /**
   * Get the components this grabber is interacting with.
   * @param State - Whether to return hovered or selected interactables
   * @param OutInteractables The output interactables this grabber is interacting with.
   */
  virtual void GetInteractorStateRelationships(
      EIsdkInteractableState State,
      TArray<TScriptInterface<IIsdkIInteractableState>>& OutInteractables) const override;

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkHandGrabDetector* GetHandGrabDetector() const;

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkRayGrabDetector* GetRayGrabDetector() const;

  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkDistanceGrabDetector* GetDistanceGrabDetector() const;

  bool GetPointerTransform(FTransform& OutTransform) const;

  void SetHandPointerPose(const TScriptInterface<IIsdkIHandPointerPose>& InHandPointerPose);

  /**
   * Gets the transform representing the location of the grab.  If pinching, this will be the
   * location of the pinch.  If palm grabbing, this will be the location of the palm grab.
   * Otherwise, this will be the location of the grabber component.
   */
  void GetGrabberTransform(FTransform& OutTransform) const;

  void GetPinchGrabTransform(FTransform& OutTransform) const;
  void GetPalmGrabTransform(FTransform& OutTransform) const;

  void SetInteractorSnap(FTransform& TransformIn, float LerpRate);

  /**
   * Returns true if this grabber utilizes the given input method.
   * @param InputMethod the type to change utilization of
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  bool IsGrabInputMethodAllowed(EIsdkGrabInputMethod InputMethod) const;

  /**
   * Sets whether grabber should utilize the given input method
   * @param InputMethod the type to change utilization of
   * @param Allowed whether the input method should be respected or not
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  void SetGrabInputMethodAllowed(EIsdkGrabInputMethod InputMethod, bool Allowed);

  /**
   * Returns true if this grabber detects the given grab type.
   * @param GrabType the type to change detection of
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  bool IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType GrabType) const;

  /**
   * Sets whether grabber should detect the given grab type
   * @param GrabType the type to change detection of
   * @param Allowed whether the input method should be respected or not
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  void SetGrabDetectionTypeAllowed(EIsdkGrabDetectorType GrabType, bool Allowed);

  FORCEINLINE bool IsAnyInputMethodActive() const
  {
    return ActiveInputMethodFlags != EIsdkGrabInputMethod::Unknown;
  }
  FORCEINLINE bool IsInputMethodActive(EIsdkGrabInputMethod InputMethod) const
  {
    return (ActiveInputMethodFlags & InputMethod) != EIsdkGrabInputMethod::Unknown;
  }

 protected:
  void ClearActiveInputMethods();

  /**
   * Attempt to select a grabbable for the given input method
   * @param InputMethod
   */
  void GrabByInputMethod(EIsdkGrabInputMethod InputMethod);

  /**
   * Attempt to unselect any grabbables for the given input method
   * @param InputMethod
   */
  void ReleaseByInputMethod(EIsdkGrabInputMethod InputMethod);

  /**
   * What input methods this component is allowed to detect
   */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "InteractionSDK")
  TSet<EIsdkGrabInputMethod> AllowedInputMethods{
      EIsdkGrabInputMethod::Palm,
      EIsdkGrabInputMethod::Pinch};

  /**
   * What types of grabs this component is allowed to detect
   */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "InteractionSDK")
  TSet<EIsdkGrabDetectorType> AllowedGrabDetectors{
      EIsdkGrabDetectorType::HandGrab,
      EIsdkGrabDetectorType::DistanceGrab,
      EIsdkGrabDetectorType::RayGrab};

  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  float PinchStrength = 0.f;

  // HandGrabDetector detects pinch and palm grabs via colliders attached to the hand/controller
  UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Category = InteractionSDK)
  UIsdkHandGrabDetector* HandGrabDetector;

  // DistanceGrabDetector detects pinch and palm grabs via a frustum positioned relative to the
  // pointer transform
  UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Category = InteractionSDK)
  UIsdkDistanceGrabDetector* DistanceGrabDetector;

  // RayGrabDetector detects pinch and palm grabs via a line trace out from the pointer transform
  UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Category = InteractionSDK)
  UIsdkRayGrabDetector* RayGrabDetector;

  // A set of all grabbables hovered by the grabber this frame
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TSet<UIsdkGrabbableComponent*> HoveredGrabbables;

  // The grab motion which is currently being applied to the grabbed grabbable
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabMotion* CurrentGrabMotion;

  // The grab detector which detected the grabbable that is currently being grabbed
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabDetector* CurrentGrabDetector;

  // The grab detector which detected the grabbable that is currently being hovered.  Prioritized
  // in order of Hand > Ray > Distance
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabDetector* CurrentHoverDetector;

  // The niagara particle system used to display distance grab effects while hovering or grabbing
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  UNiagaraSystem* DistanceGrabNiagaraSystem;

  // The component that holds the DistanceGrabNiagaraSystem
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UNiagaraComponent* DistanceGrabNiagaraComponent;

  // The niagara particle system used to display ray grab effects
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  UNiagaraSystem* RayGrabNiagaraSystem;

  // The component that holds the RayGrabNiagaraSystem
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UNiagaraComponent* RayGrabNiagaraComponent;

  UPROPERTY()
  TScriptInterface<IIsdkIHandPointerPose> HandPointerPose;

 private:
  void TickInteractors(float DeltaTime);
  void UpdateHoveredGrabbables();
  virtual void UpdateInteractorState();
  void UpdateRayGrabVisuals();
  void UpdateDistanceGrabVisuals();
  void UpdateHoveredInteractor();

  void PostEvent(EIsdkPointerEventType Type, UIsdkGrabbableComponent* Dest);
  void ResetGrabState();

  int64 PointerEventToken = 0;

  UPROPERTY(Instanced)
  UIsdkGrabbableComponent* GrabbedComponent;

  /**
   * A set of flags indicating all currently active grab input methods.
   * @see EIsdkGrabInputMethod.
   */
  EIsdkGrabInputMethod ActiveInputMethodFlags = EIsdkGrabInputMethod::Unknown;

  /**
   * This value represents the input method that initialized the current grab.  Note that it might
   * not indicate a currently active input method, for instance, if a user initializes a grab with
   * a pinch, then palm grabs, then releases their pinch.  In this case, the initial input method
   * is pinch, and the only active input method is palm.
   */
  EIsdkGrabInputMethod InitialInputMethodForCurrentGrab = EIsdkGrabInputMethod::Unknown;
  EIsdkGrabDetectorType CurrentGrabDetectorType;

  void DrawDebugVisuals() const;

  UFUNCTION()
  void HandleGrabbableCancelEvent(
      int InteractorID,
      UIsdkGrabTransformerComponent* InGrabTransformerComponent);
};
