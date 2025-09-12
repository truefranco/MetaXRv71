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
#include "EnhancedInputComponent.h"
#include "IsdkRigComponent.h"
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "IsdkControllerRigComponent.generated.h"

enum class EControllerHandBehavior : uint8;

/**
 * @class UIsdkControllerRigComponent
 * @brief Drives all controller-based interaction behaviors
 *
 * Highest level ISDK scene component driving interaction
 * behavior while the user is holder a controller.  It is intended to be placed directly below a
 * motion controller in the scene hierarchy, one for each hand.  The component takes input events
 * from the controller and provides a means for interactors to read those events in a
 * controller/hand agnostic manner.
 *
 * @see UIsdkRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(Abstract, ClassGroup = ("InteractionSDK|Rig"), meta = (BlueprintSpawnableComponent))
class OCULUSINTERACTIONPREBUILTS_API UIsdkControllerRigComponent : public UIsdkRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkControllerRigComponent();
  UIsdkControllerRigComponent(EIsdkHandedness InHandedness);

  /**
   * @brief Called on BeginPlay, overridden from USceneComponent.
   */
  virtual void BeginPlay() override;

  /**
   * @brief Returns the Controller Visuals associated with this controller as a Visuals Rig
   * Component
   * @see UIsdkControllerRigComponent#GetVisuals
   * @return UIsdkControllerVisualsRigComponent The controller visuals component used by this
   * controller
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkControllerVisualsRigComponent* GetControllerVisuals() const;

  /**
   * @brief Returns the Controller Visuals associated with this controller as a tracked data source
   * @see UIsdkControllerRigComponent#GetControllerVisuals
   * @return UIsdkTrackedDataSourceRigComponent The tracked data source used by this
   * controller
   */
  virtual UIsdkTrackedDataSourceRigComponent* GetVisuals() const override;

  /**
   * @brief Returns the current pinch strength
   *
   * Overridden from UIsdkRigComponent. Checks if the FEnhancedInputActionValueBinding struct
   * associated with PinchStrength is valid, and returns its value as a float. Returns 0 if the
   * struct is invalid.
   *
   * @see UIsdkRigComponent#GetPinchStrength
   * @return float Current pinch strength associated with the FEnhancedInputActionValueBinding.
   * Should return 0 on controllers.
   */
  virtual float GetPinchStrength() const override;

 protected:
  /**
   * @brief Called when visuals are attached to this controller rig component
   *
   * Calls CreateDataSourcesAsTrackedController on the Controller Visuals Component
   */
  virtual void OnControllerVisualsAttached();

  /**
   * @brief Returns the current offset for the palm collider
   *
   * Referencing the ControllerHandBehavior state, returns the
   * local space offset for the collider used to detect palm grabs
   *
   * @return FVector Local space offset for palm grab collider
   */
  virtual FVector GetPalmColliderOffset() const override;

  /**
   * @brief Returns the current offset for the pinch collider
   *
   * Referencing the ControllerHandBehavior state, returns the
   * local space offset for the collider used to detect pinch grabs
   *
   * @return FVector Local space offset for pinch grab collider
   */
  virtual FVector GetPinchColliderOffset() const override;

  /**
   * @brief Returns the hand mesh associated with this controller rig
   * @return USkinnedMeshComponent Hand mesh associated with pinch interactions for this rig
   * component
   */
  virtual USkinnedMeshComponent* GetPinchAttachMesh() const override;

  /**
   * Pointer to the rig component used for the visuals associated with this controller rig
   */
  UPROPERTY(BlueprintGetter = GetControllerVisuals, EditAnywhere, Category = InteractionSDK)
  TObjectPtr<UIsdkControllerVisualsRigComponent> ControllerVisualsComponent;

  /**
   * Unreal Engine input binding struct used for PinchStrength. Normally nullptr on a controller.
   */
  FEnhancedInputActionValueBinding* PinchStrength = nullptr;

  /**
   * @brief Returns the FName of the bone representing the thumb tip.Used to attach pinch colliders
   *
   * @return FName Fully qualified name of the bone joint representing the thumb tip
   */
  virtual FName GetThumbTipSocketName() const override;

 private:
  virtual void BindInputActions(UEnhancedInputComponent* EnhancedInputComponent) override;
  virtual TSubclassOf<AActor> FindBPFromPath(const FString& Path);

  UFUNCTION()
  void HandleControllerHandBehaviorChanged(
      TScriptInterface<IIsdkITrackingDataSubsystem> IsdkITrackingDataSubsystem,
      EControllerHandBehavior ControllerHandBehavior,
      EControllerHandBehavior ControllerHandBehavior1);

  UFUNCTION()
  void HandlePinchGrabStartedInput();

  UFUNCTION()
  void HandlePinchGrabFinishedInput();

  UFUNCTION()
  void HandlePalmGrabStartedInput();

  UFUNCTION()
  void HandlePalmGrabFinishedInput();
};

/**
 * @class UIsdkControllerRigComponentLeft
 * @brief Rig Component Tracking Controller for Left Handedness
 *
 * @see UIsdkControllerRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(
    Blueprintable,
    ClassGroup = ("InteractionSDK|Rig"),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Controller Rig Left"))
class OCULUSINTERACTIONPREBUILTS_API UIsdkControllerRigComponentLeft
    : public UIsdkControllerRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkControllerRigComponentLeft();
};

/**
 * @class UIsdkControllerRigComponentLeft
 * @brief Rig Component Tracking Controller for Right Handedness
 *
 * @see UIsdkControllerRigComponent
 * @addtogroup InteractionSDKPrebuiltsPrebuilts
 */
UCLASS(
    Blueprintable,
    ClassGroup = ("InteractionSDK|Rig"),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Controller Rig Right"))
class OCULUSINTERACTIONPREBUILTS_API UIsdkControllerRigComponentRight
    : public UIsdkControllerRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkControllerRigComponentRight();
};
