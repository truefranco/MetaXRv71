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
#include "IsdkRigComponent.h"
#include "Components/ActorComponent.h"
#include "IsdkHandRigComponent.generated.h"

class UIsdkHandPalmGrabRecognizer;
struct FEnhancedInputActionValueBinding;

/**
 * @brief UIsdkHandRigComponent is the highest level ISDK scene component driving interaction
 * behavior while the user is not holding a controller.  It is intended to be placed directly below
 * a motion controller in the scene hierarchy, one for each hand.  The component takes input events
 * from the hand and provides a means for interactors to read those events in a controller/hand
 * agnostic manner.
 */
UCLASS(Abstract, ClassGroup = ("InteractionSDK|Rig"))
class OCULUSINTERACTIONPREBUILTS_API UIsdkHandRigComponent : public UIsdkRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkHandRigComponent();
  UIsdkHandRigComponent(EIsdkHandedness InHandedness);

  virtual void BeginPlay() override;
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;
  //virtual void BindInputActions(UInputComponent* InputComponent) override;

  virtual FVector GetPalmColliderOffset() const override;
  virtual USkinnedMeshComponent* GetPinchAttachMesh() const override;
  virtual void OnHandVisualsAttached();
  virtual void UpdateComponentDataSources() override;
  virtual float GetPinchStrength() const override;

  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkHandVisualsRigComponent* GetHandVisuals() const;

  virtual UIsdkTrackedDataSourceRigComponent* GetVisuals() const override;

 protected:
  // Properties for access of attached components.
  UPROPERTY(BlueprintGetter = GetHandVisuals, EditAnywhere, Category = InteractionSDK)
  UIsdkHandVisualsRigComponent* HandVisualsComponent;

  virtual FName GetThumbTipSocketName() const override;

  /*
   * PinchGrabRecognizer recognizes pinch motions in the user's hands, which we use to drive
   * pinch grab behavior in the grabber.
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkHandFingerPinchGrabRecognizer* PinchGrabRecognizer;

  /*
   * PalmGrabRecognizer recognizes palm grab motions in the user's hands, which we use to drive
   * palm grab behavior in the grabber.
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkHandPalmGrabRecognizer* PalmGrabRecognizer;

  FEnhancedInputActionValueBinding* PinchStrength = nullptr;

 private:
  UFUNCTION()
  void HandlePinchGrabStarted();

  UFUNCTION()
  void HandlePinchGrabFinished();

  UFUNCTION()
  void HandlePalmGrabStarted();

  UFUNCTION()
  void HandlePalmGrabFinished();
};

UCLASS(
    Blueprintable,
    ClassGroup = ("InteractionSDK|Rig"),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Hand Rig Left"))
class OCULUSINTERACTIONPREBUILTS_API UIsdkHandRigComponentLeft : public UIsdkHandRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkHandRigComponentLeft();
};

UCLASS(
    Blueprintable,
    ClassGroup = ("InteractionSDK|Rig"),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Hand Rig Right"))
class OCULUSINTERACTIONPREBUILTS_API UIsdkHandRigComponentRight : public UIsdkHandRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkHandRigComponentRight();
};
