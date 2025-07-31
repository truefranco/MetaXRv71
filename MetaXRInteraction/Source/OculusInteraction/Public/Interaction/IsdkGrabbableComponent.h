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
#include "Interaction/IsdkSceneInteractableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Pointable/IsdkIPointable.h"
#include "StructTypes.h"
#include "IsdkGrabbableComponent.generated.h"

class UIsdkGrabTransformerComponent;
class UIsdkGrabberComponent;
class UMaterialInterface;

/* @nolint */
/**
 * EIsdkGrabbableColliderMode is used to drive selection of GrabbableComponent's ColliderMode, which
 * determines the collider a grabbable uses to detect grabs.
 *
 * FindStaticMesh - Finds the first static mesh on the actor, and uses its collision as the grab collider.
 * FindByName - Finds a primitive object on the actor by the given name, and uses it as the grab collider.
 * Sphere - Uses a sphere with configurable radius as the grab collider.
 * Box - Uses a box with configurable radius as the grab collider.
 * CustomMesh - Uses a specified static mesh's collision as the grab collider
 * the SetCollider() method.
 */
UENUM()
enum class EIsdkGrabbableColliderMode : uint8
{
  FindByName,
  FindStaticMesh,
  Sphere,
  Box,
  CustomMesh
};
/* @nolint */

constexpr int GrabColliderTypeCount = static_cast<int>(EIsdkGrabInputMethod::Custom);

/**
 * @class UIsdkGrabbableComponent
*  Detects grab events and forwards them to a UIsdkGrabbable for processing.
*  UIsdkGrabbableComponent can be configured to only respect certain input methods (pinch, palm).
 *
 * @see UIsdkSceneInteractableComponent
 * @see UIsdkGrabTransformer
 * @see IIsdkIPointable
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Grabbable Component"))
class OCULUSINTERACTION_API UIsdkGrabbableComponent : public UIsdkSceneInteractableComponent,
                                                      public IIsdkIPointable
{
  GENERATED_BODY()

 public:
  UIsdkGrabbableComponent();

  virtual void BeginPlay() override;
  virtual void OnRegister() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFn) override;

  /**
   * Gets interactors that are hovered or selecting this grabbable.
   *
   * @param State
   * @param OutInteractors
   */
  virtual void GetInteractableStateRelationships(
      EIsdkInteractableState State,
      TArray<TScriptInterface<IIsdkIInteractorState>>& OutInteractors) const override;

  /**
   * Get a reference to the delegate that fires whenever this component detects a
   * grab related pointer event.
   */
  virtual FIsdkInteractionPointerEventDelegate& GetInteractionPointerEventDelegate() override;

  /**
   * Send a pointer event to this grabbable.  Expected to be called by grabbers.
   * @param Event the pointer event to send to this grabbable.
   */
  virtual void PostEvent(const FIsdkInteractionPointerEvent& Event);

  /**
   * Get the collider responsible for detecting grabs
   */
  UFUNCTION(BlueprintPure, Category = "InteractionSDK", meta = (ReturnDisplayName = "GrabCollider"))
  UPrimitiveComponent* GetGrabCollider() const;

  /**
   * Set the collider responsible for detecting grabs
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  void SetGrabCollider(UPrimitiveComponent* InGrabCollider);

  /**
   * Returns true if the collider is hovered by the given grabber.  Returns false if
   * the grabber is null, or not hovering the collider.
   */
  UFUNCTION(BlueprintPure, Category = "InteractionSDK|Interactable")
  bool IsHoveredBy(UIsdkGrabberComponent* Grabber) const;

  /**
   * Returns true if the collider is grabbed by the given grabber.  Returns false if
   * the grabber is null, or not grabbing the collider.
   */
  UFUNCTION(BlueprintPure, Category = "InteractionSDK|Interactable")
  bool IsGrabbedBy(const UIsdkGrabberComponent* Grabber) const;

  /**
   * Returns true if this grabbable respects the given input method.
   * @param InputMethod the type to change detection of
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  bool IsGrabInputMethodAllowed(EIsdkGrabInputMethod InputMethod);

  /**
   * Sets whether the given input method should be respected
   * @param InputMethod the type to change detection of
   * @param Allowed whether the input method should be respected or not
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  void SetGrabInputMethodAllowed(EIsdkGrabInputMethod InputMethod, bool Allowed);

  /**
   * @return The GrabTransformer component this grabbable is associated with
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkGrabTransformerComponent* GetGrabTransformer() const;

  /**
   * Sets the GrabTransformer component this grabbable is associated with.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetGrabTransformer(UIsdkGrabTransformerComponent* InGrabTransformer);

  /**
   * Sets the collider mode for this grabbable to utilize.
   @param NewColliderMode new collider mode to set. If using CustomMesh, SetCollider() will need to be called after
   @param NewSize When using sphere or box collider modes, this input will set the radius and extent respectively
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetColliderMode(EIsdkGrabbableColliderMode NewColliderMode, float NewSize);

  bool IsHandGrabAllowed() const;
  bool IsDistanceGrabAllowed() const;
  bool IsRayGrabAllowed() const;
  void SetupCollider();

protected:
  
  /**
   * AllowedGrabDetectorTypes drives which grab detectors can detect this grabbable
   */
  UPROPERTY(EditAnywhere, Category = InteractionSDK, meta=(Bitmask, BitmaskEnum="/Script/OculusInteraction.EIsdkGrabDetectorType"))
  uint32 AllowedGrabDetectorTypes = static_cast<uint32>(EIsdkGrabDetectorType::HandGrab);
  
  /* @nolint */
  /**
   * ColliderMode determines the collider this grabbable uses to detect grabs.
   *
   * FindStaticMesh - Finds the first static mesh on the actor, and uses its collision as the grab collider.
   * FindByName - Finds a primitive object on the actor by the given name, and uses it as the grab collider.
   * Sphere - Uses a sphere with configurable radius as the grab collider.
   * Box - Uses a box with configurable radius as the grab collider.
   * CustomMesh - Uses a specified static mesh's collision as the grab collider
   * the SetCollider() method.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractionSDK")
  EIsdkGrabbableColliderMode ColliderMode = EIsdkGrabbableColliderMode::FindStaticMesh;
  /* @nolint */
  
  /**
   * Determines the radius of the collider.  Only valid if ColliderMode is set to Sphere.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      Category = "InteractionSDK",
      meta =
          (EditCondition = "ColliderMode==EIsdkGrabbableColliderMode::Sphere", EditConditionHides))
  float ColliderRadius = 10.f;

  /**
   * Determines the extent of the collider.  Only valid if ColliderMode is set to Box.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      Category = "InteractionSDK",
      meta = (EditCondition = "ColliderMode==EIsdkGrabbableColliderMode::Box", EditConditionHides))
  FVector ColliderBoxExtent = FVector(10.f, 10.f, 10.f);

  /**
   * Uses the default collision of the provided static mesh as the collider.  Only valid if
   * ColliderMode is set to CustomMesh.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      Category = "InteractionSDK",
      meta =
          (EditCondition = "ColliderMode==EIsdkGrabbableColliderMode::CustomMesh",
           EditConditionHides))
  UStaticMesh* CustomCollisionMesh;

  /**
   * Uses the default collision of the provided static mesh as the collider.  Only valid if
   * ColliderMode is set to CustomMesh.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="InteractionSDK", meta=(EditCondition = "ColliderMode==EIsdkGrabbableColliderMode::FindByName", EditConditionHides))
  FString ColliderName;
  
  /**
   * What input methods this component detects.
   */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "InteractionSDK")
  TSet<EIsdkGrabInputMethod> AllowedInputMethods{
      EIsdkGrabInputMethod::Palm,
      EIsdkGrabInputMethod::Pinch};

  virtual void SetState(EIsdkInteractableState NewState) override;
  
  UPROPERTY(
    BlueprintGetter=GetGrabTransformer,
    BlueprintSetter=SetGrabTransformer,
    meta = (ExposeOnSpawn = true),
    Category = "InteractionSDK")
  UIsdkGrabTransformerComponent* GrabTransformerComponent;

  /**
   * The collider we'll use to detect grabs.  If not set, we'll find the first
   * UPrimitiveComponent found in the scene component hierarchy.
   */
  UPROPERTY(BlueprintReadWrite, Category = "InteractionSDK")
  UPrimitiveComponent* GrabCollider;

  /**
   * A list of grabbers which are currently grabbing this grabbable component
   */
  TArray<UIsdkGrabberComponent*> SelectedGrabbers;

  /**
   * A list of grabbers which are currently hovering this grabbable component
   */
  TArray<UIsdkGrabberComponent*> HoveredGrabbers;

  /**
   * Delegate broadcast when pointer events are triggered
   */
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkInteractionPointerEventDelegate InteractionPointerEvent;

  UFUNCTION()
  void HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent);

  UPROPERTY()
  UMaterialInterface* CustomCollisionMaterial;
};
