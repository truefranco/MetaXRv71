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
#include "IsdkTrackedDataSourceRigComponent.h"
#include "IsdkControllerVisualsRigComponent.generated.h"

class UIsdkHandMeshComponent;
class UIsdkXRRigSettingsComponent;
class UIsdkInputActionsRigComponent;
class UIsdkControllerMeshComponent;
class UQuestControllerAnimInstance;
class UPoseableMeshComponent;
enum class EControllerHandBehavior : uint8;

/**
 * @class UIsdkControllerVisualsRigComponent
 * @brief Rig Component Visualizing and Tracking Controllers (abstract class)
 *
 * Holds logic pertaining to the visualization and tracking
 * of controllers (and any corresponding controller + hand visualization) while controllers are
 * the active tracked input (vs, say, hands).
 *
 * @see UIsdkTrackedDataSourceRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(Abstract)
class OCULUSINTERACTIONPREBUILTS_API UIsdkControllerVisualsRigComponent
    : public UIsdkTrackedDataSourceRigComponent
{
  GENERATED_BODY()

  friend class FIsdkControllerRigComponentTestBase;

 public:
  UIsdkControllerVisualsRigComponent();

  /**
   * @brief Get the Tracked Visual SceneComponent associated with this Rig Component
   *
   * Overridden from UIsdkTrackedDataSourceRigComponent. Tracked Visual refers to a visual directly
   * driven by a data source
   *
   * @return USceneComponent Scene Component associated as a Tracked Visual
   */
  virtual USceneComponent* GetTrackedVisual() const override;

  /**
   * @brief Get the Synthetic Visual SceneComponent associated with this Rig Component
   *
   * Overridden from UIsdkTrackedDataSourceRigComponent. Synthetic Visual refers to a visual that is
   * originally derived from data but modified during runtime
   *
   * @return USceneComponent Scene Component associated as a Synthetic Visual (always nullptr for
   * UIsdkControllerVisualsRigComponent)
   */
  virtual USceneComponent* GetSyntheticVisual() const override
  {
    return nullptr;
  }
  /**
   * @brief Get the SocketComponent associated with this Rig Component, based on the Controller Hand
   * Behavior
   *
   * Overridden from UIsdkTrackedDataSourceRigComponent.
   *
   * @param OutSocketComponent Returned reference of the Socket Component as a USceneComponent
   * @param OutSocketName Returned reference of the socket FName
   * @param HandBone Bone name used to find the appropriate socket
   */
  virtual void GetInteractorSocket(
      USceneComponent*& OutSocketComponent,
      FName& OutSocketName,
      EIsdkHandBones HandBone) const override;

  /**
   * @brief Binds all of the controller input actions (A button, B button, etc.) to the given
   * EnhancedInputComponent
   * @param EnhancedInputComponent Component to bind the input actions to
   * @param InputActionsRigComponent The RigComponent from which the inputs are being bound
   */
  void BindInputActions(class UInputComponent* PlayerInputComponent);

  /**
   * @brief Returns a pointer to the Animated Mesh Component associated with this Rig Component
   * @return USkeletalMeshComponent Skeletal Mesh Component
   */
  USkeletalMeshComponent* GetAnimatedHandMeshComponent();

  /**
   * @brief Returns a pointer to the Hand Mesh Component associated with this Rig Component
   * @return UIsdkHandMeshComponent Hand Mesh Component (ISDK)
   */
  UIsdkHandMeshComponent* GetPoseableHandMeshComponent();

  /**
   * @brief Returns a pointer to the Animated Mesh Component associated with this Rig Component
   * @return UIsdkControllerMeshComponent Controller Mesh Component (ISDK)
   */
  UIsdkControllerMeshComponent* GetControllerMeshComponent();

 protected:
  // A skeletal mesh used to represent the game controller
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  UIsdkControllerMeshComponent* ControllerMeshComponent;

  // A skeletal mesh used to represent hands while holding a controller.  This mesh is driven
  // by an animations configured in-editor.
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  USkeletalMeshComponent* AnimatedHandMeshComponent;

  // A skeletal mesh used to represent hands while holding a controller.  This mesh is driven by
  // runtime-generated hand pose data.
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  UIsdkHandMeshComponent* PoseableHandMeshComponent;

  /**
   * @brief Returns the transform of the current Synthetic Visual
   * @return FTransform Current Synthetic Visual Transform (always identity with
   * UIsdkControllerVisualsRigComponent)
   */
  virtual FTransform GetCurrentSyntheticTransform() override
  {
    return FTransform::Identity;
  }

  /**
   * @brief Called when Data Sources are created. Overridden from
   * UIsdkTrackedDataSourceRigComponent.
   */
  virtual void OnDataSourcesCreated() override;

  /**
   * @brief Called when the visibility of is changed. Overridden from
   * UIsdkTrackedDataSourceRigComponent.
   */
  virtual void OnVisibilityUpdated(bool bTrackedVisibility, bool bSyntheticVisibility) override;

  //~ Begin Legacy Input Handlers
  void OnGripAxis(float Value);
  void OnTriggerAxis(float Value);
  void OnThumbstickX(float Value);
  void OnThumbstickY(float Value);
  void OnHandPinchSelectStrength(float value);

  void OnButtonAPressed();
  void OnButtonAReleased();
  void OnButtonBPressed();
  void OnButtonBReleased();
  void OnButtonXPressed();
  void OnButtonXReleased();
  void OnButtonYPressed();
  void OnButtonYReleased();

  void OnButtonATouched();
  void OnButtonAUnTouched();
  void OnButtonBTouched();
  void OnButtonBUnTouched();
  void OnButtonXTouched();
  void OnButtonXUnTouched();
  void OnButtonYTouched();
  void OnButtonYUnTouched();
  void OnTriggerTouched();
  void OnTriggerUnTouched();
  void OnThumbstickTouched();
  void OnThumbstickUnTouched();
  void OnPanelTouched();
  void OnPanelUnTouched();
  void OnHandPinchSelectPressed();
  void OnHandPinchSelectReleased();
  void OnHandPinchGrabPressed();
  void OnHandPinchGrabReleased();
  void OnHandPalmGrabPressed();
  void OnHandPalmGrabReleased();
  void OnMenuTogglePressed();
  void OnMenuToggleReleased();
  //~ End Legacy Input Handlers

	//~ Begin Input State Variables
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  float GripValue;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  float TriggerValue;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  float ThumbstickXValue;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  float ThumbstickYValue;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  float HandPinchSelectStrength;

  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonADown;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonBDown;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonXDown;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonYDown;

  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonATouched;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonBTouched;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonXTouched;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bButtonYTouched;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bTriggerTouched;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bThumbstickTouched;

  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bPanelTouched;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bHandPinchSelect;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bHandPinchGrab;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bHandPalmGrab;
  UPROPERTY(BlueprintReadOnly, Category = "Input State")
  bool bMenuToggle;


 private:

  void UpdateAnimatedHandMeshVisibility(
      bool bTrackedVisibility,
      EControllerHandBehavior ControllerHandBehavior);
  void UpdatePoseableHandMeshVisibility(
      bool bTrackedVisibility,
      EControllerHandBehavior ControllerHandBehavior);
  void UpdateControllerMeshVisibility(
      bool bTrackedVisibility,
      EControllerHandBehavior ControllerHandBehavior);
};

/**
 * @class UIsdkControllerVisualsRigComponentLeft
 * @brief Rig Component Visualizing and Tracking Controllers for Left Handedness
 *
 * @see UIsdkControllerVisualsRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(ClassGroup = InteractionSDK)
class OCULUSINTERACTIONPREBUILTS_API UIsdkControllerVisualsRigComponentLeft
    : public UIsdkControllerVisualsRigComponent
{
  GENERATED_BODY()

  UIsdkControllerVisualsRigComponentLeft();
};

/**
 * @class UIsdkControllerVisualsRigComponentRight
 * @brief Rig Component Visualizing and Tracking Controllers for Right Handedness
 *
 * @see UIsdkControllerVisualsRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(ClassGroup = InteractionSDK)
class OCULUSINTERACTIONPREBUILTS_API UIsdkControllerVisualsRigComponentRight
    : public UIsdkControllerVisualsRigComponent
{
  GENERATED_BODY()

  UIsdkControllerVisualsRigComponentRight();
};
