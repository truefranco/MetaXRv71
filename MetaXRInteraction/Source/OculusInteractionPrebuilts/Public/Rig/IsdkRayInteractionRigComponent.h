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
#include "Core/IsdkConditionalComponentIsActive.h"
#include "Core/IsdkConditionalGroupAll.h"
#include "InteractorVisuals/IsdkRayVisual.h"
#include "IsdkRayInteractionRigComponent.generated.h"

class UIsdkRayInteractor;
class UInputAction;
class UIsdkHandMeshComponent;
struct FIsdkTrackingDataSources;
class UIsdkRigComponent;

/**
 * @class UIsdkRayInteractionRigComponent
 * @brief Actor component that stores references to ray-related interaction components.
 *
 * Data sources and visualizers.
 * @see UIsdkRigComponent
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(ClassGroup = (InteractionSDK))
class OCULUSINTERACTIONPREBUILTS_API UIsdkRayInteractionRigComponent : public UActorComponent
{
  GENERATED_BODY()

 public:
  UIsdkRayInteractionRigComponent();

  /**
   * @brief Overridden from UActorComponent, called when component is initialized. Sets up
   * Conditional Groups and a reference to and tick pre-requisite for the parent RigComponent
   */
  virtual void InitializeComponent() override;

  /**
   * @brief Overridden from UActorComponent, called on BeginPlay. Sets up Pinch Select/Unselect
   * delegates
   */
  virtual void BeginPlay() override;

  /**
   * @brief Overridden from UActorComponent, called on tick. Propagates SelectStrength from the
   * parent Rig Component to the Ray Interactor component
   */
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  /**
   * @brief Attaches given USceneComponent to the RayInteractor, binds the data sources for joints
   * and HMD to it
   * @param DataSources Data source for active hand/joint poses
   * @param HmdDataSourceIn Data source for active HMD
   * @param AttachToComponent Component to attach to
   * @param AttachToComponentSocket Component socket name to attach to on given component
   */
  UFUNCTION()
  void BindDataSources(
      const FIsdkTrackingDataSources& DataSources,
      TScriptInterface<IIsdkIHmdDataSource>& HmdDataSourceIn,
      USceneComponent* AttachToComponent,
      FName AttachToComponentSocket);

  /**
   * @brief Propagates state from Ray Interactor component to the Ray Visual component, along with
   * pinch strength from parent RigComponent
   * @see UIsdkRayVisualFunctionLibrary#UpdateRayVisualWithRayInteractor
   */
  UFUNCTION()
  void UpdateRayVisual();

  /**
   * @brief Ray Interactor component reference, utilized for generating events and routing state
   * from to other related components
   */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = InteractionSDK)
  UIsdkRayInteractor* RayInteractor;

  /**
   * @brief Retrieves the Conditional Group for if this component is enabled.
   * @return UIsdkConditionalGroupAll* Conditional Group. All constituent conditionals of the group
   * must be true for it to resolve true.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkConditionalGroupAll* GetEnabledConditional() const
  {
    return EnabledConditionalGroup;
  }

  /**
   * @brief Reference to current Ray Visual component used to spawn and update the cursor and
   * PinchArrow subcomponents
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkRayVisualComponent* RayVisual;

  /**
   * @brief Subclass utilized to spawn the Ray Visual Component, may be derived class or Unreal
   * blueprint
   */
  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "InteractionSDK|Ray Customization")
  TSubclassOf<UIsdkRayVisualComponent> RayVisualClass;

  /**
   * @brief HMD Data source to pull data from for current state and pose information for the active
   * headset
   */
  UPROPERTY()
  TScriptInterface<IIsdkIHmdDataSource> HmdDataSource;

 protected:
  /**
   * @brief Reference to the parent RigComponent
   */
  UPROPERTY()
  UIsdkRigComponent* RigComponent;

 private:
  void CreateRayVisual(AActor* AttachParent);

  void HandleIsEnabledConditionalChanged(bool bIsEnabled);

  UPROPERTY(BlueprintGetter = GetEnabledConditional, Category = InteractionSDK)
  UIsdkConditionalGroupAll* EnabledConditionalGroup;

  UPROPERTY()
  UIsdkConditionalComponentIsActive* IsActiveConditional;

  UFUNCTION()
  void HandleRigPinchSelect(UIsdkRigComponent* IsdkRigComponent);

  UFUNCTION()
  void HandleRigPinchUnselect(UIsdkRigComponent* IsdkRigComponent);
};
