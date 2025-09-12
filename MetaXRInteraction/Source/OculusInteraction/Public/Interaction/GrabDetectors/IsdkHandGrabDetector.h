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
#include "IsdkGrabDetector.h"
#include "IsdkHandGrabDetector.generated.h"

class USphereComponent;
enum class EIsdkGrabInputMethod : uint8;
enum class EIsdkInteractorState : uint8;

/**
 * @brief UIsdkHandGrabDetector is used by a grab interactor to detect grabbables at
 * close to the hand.  This grab detector features two sets of colliders used for
 * the separate detection of pinches and palm grabs.
 *
 * @see UIsdkGrabDetector
 * @see UIsdkGrabberComponent
 * @see UIsdkGrabbableComponent
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories)
class OCULUSINTERACTION_API UIsdkHandGrabDetector : public UIsdkGrabDetector
{
  GENERATED_BODY()

 public:
  virtual void Initialize(UIsdkGrabberComponent* InGrabberComponent) override;
  virtual void Tick(float DeltaTime) override;

  const TSet<TObjectPtr<UIsdkGrabbableComponent>>& GetHoveredGrabbables() const;
  virtual UIsdkGrabbableComponent* GetGrabCandidate(EIsdkGrabInputMethod InputMethod) override;
  virtual EIsdkGrabDetectorType GetGrabDetectorType() const override;

  /**
   * Attaches the given collider and replaces the existing pinch collider with it.  Collider must
   * be a valid object.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void AttachAndReplacePinchCollider(UPrimitiveComponent* Collider);

  /**
   * Attaches the given collider and replaces the existing palm collider with it.  Collider must
   * be a valid object.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void AttachAndReplacePalmCollider(UPrimitiveComponent* Collider);

  /**
   * @return the collider that is being used to select the currently selected grabbable.
   */
  UFUNCTION(
      BlueprintPure,
      Category = InteractionSDK,
      meta = (ReturnDisplayName = "GrabbingComponent"))
  UPrimitiveComponent* GetGrabbingComponent() const;

  /**
   * Set the amount by which the default palm grab collider should be
   * offset from its attach parent.  Used particularly to account for
   * discrepancies between hand and controller offsets from the motion
   * controller root.
   *
   * @param InPalmOffset - The amount by which to offset the default palm grab
   *   collider from its attach parent.
   */
  void UpdatePalmOffset(const FVector& InPalmOffset);

  /**
   * Set the amount by which the default pinch grab collider should be
   * offset from its attach parent.  Used particularly to account for
   * discrepancies between hand and controller offsets from the motion
   * controller root.
   *
   * @param InPinchOffset - The amount by which to offset the default pinch grab
   *   collider from its attach parent.
   */
  void UpdatePinchOffset(const FVector& InPinchOffset);

  /**
   * Update the attach parent of all pinch colliders.  Called whenever grab state changes.
   * This stabilizes the motion when grabbing, such that after being grabbed, motion continues
   * relative to the hand/controller root, rather than being relative to the thumb tip.
   */
  void UpdatePinchCollidersAttachment(
      USkinnedMeshComponent* InPinchAttachMesh,
      const FName& InThumbTipSocketName);

  void DrawDebugVisuals() const;

  /**
   * @return the set of colliders which detect pinch grabs
   */
  const UPrimitiveComponent* GetPinchCollider() const;

  /**
   * @return the set of colliders which detect palm grabs
   */
  const UPrimitiveComponent* GetPalmGrabCollider() const;

 protected:
  /**
   * The collider which is currently selecting the selected interactable
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TObjectPtr<UPrimitiveComponent> SelectingCollider;

  /* Radius of the to set when initializing for the pinch grab overlap collider */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = InteractionSDK)
  float PinchColliderRadius = 1.2f;

  /* Radius of the to set when initializing for the palm grab overlap collider */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = InteractionSDK)
  float PalmColliderRadius = 4.0f;

  FVector DefaultPalmColliderOffset = FVector::ZeroVector;
  FVector DefaultPinchColliderOffset = FVector::ZeroVector;

  /**
   * The collision object type of the default colliders used to detect grabbables
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  TEnumAsByte<ECollisionChannel> CollisionObjectType = ECC_WorldDynamic;

  // HoveredGrabbables is the set of all grabbables hovered by ray, distance, and hand grab.
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TSet<TObjectPtr<UIsdkGrabbableComponent>> HoveredGrabbables;

  // PinchGrabCandidate is the grabbable that will be grabbed if the user pinches
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TObjectPtr<UIsdkGrabbableComponent> PinchGrabCandidate;

  // PalmGrabCandidate is the grabbable that will be grabbed if the user palm grabs
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TObjectPtr<UIsdkGrabbableComponent> PalmGrabCandidate;

  // We use PinchColliders to track which colliders are being used for pinch grab.
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TObjectPtr<UPrimitiveComponent> PinchCollider;

  // We use PalmGrabColliders to track which colliders are being used for palm grab.
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TObjectPtr<UPrimitiveComponent> PalmGrabCollider;

  // Allows disabling debug visuals
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  bool bDisableDebugVisuals = false;

 private:
  void AttachAndReplaceCollider(UPrimitiveComponent* Collider, EIsdkGrabInputMethod InputMethod);

  void DrawDebugCollider(EIsdkGrabInputMethod InputMethod, const UPrimitiveComponent* GrabCollider)
      const;

  FVector GetSelectingColliderPosition() const;

  void ComputeGrabCandidates();

  UFUNCTION()
  void HandleBeginOverlap(
      UPrimitiveComponent* OverlappedComponent,
      AActor* OtherActor,
      UPrimitiveComponent* OtherComp,
      int32 OtherBodyIndex,
      bool bFromSweep,
      const FHitResult& SweepResult);

  UFUNCTION()
  void HandleEndOverlap(
      UPrimitiveComponent* OverlappedComponent,
      AActor* OtherActor,
      UPrimitiveComponent* OtherComp,
      int32 OtherBodyIndex);
};
