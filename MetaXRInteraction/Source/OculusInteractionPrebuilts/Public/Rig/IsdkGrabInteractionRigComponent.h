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
#include "IsdkGrabInteractionRigComponent.generated.h"

class UIsdkRigComponent;
class IIsdkIHandPointerPose;
class UIsdkHandMeshComponent;
class IIsdkIHmdDataSource;
class IIsdkIRootPose;
struct FIsdkTrackingDataSources;
class UIsdkConditionalComponentIsActive;
class UIsdkConditionalGroupAll;
class UIsdkGrabberComponent;
class UInputAction;
class UEnhancedInputComponent;
class USkinnedMeshComponent;

/**
 * IsdkGrabInteractionRigComponent is responsible for taking input from the player pawn and
 * piping it into an IsdkGrabberComponent.
 */
UCLASS(ClassGroup = (InteractionSDK))
class OCULUSINTERACTIONPREBUILTS_API UIsdkGrabInteractionRigComponent : public UActorComponent
{
  GENERATED_BODY()

 public:
  UIsdkGrabInteractionRigComponent();

  virtual void InitializeComponent() override;
  virtual void BeginPlay() override;

  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  void BindDataSources(
      const FIsdkTrackingDataSources& DataSources,
      const TScriptInterface<IIsdkIHmdDataSource>& HmdDataSourceIn,
      USceneComponent* AttachToComponent,
      FName AttachToComponentSocket);

  void UpdateMeshDependencies(
      const FVector& PalmColliderOffset,
      USkinnedMeshComponent* InPinchAttachMesh,
      const FVector& PinchColliderOffset,
      const FName& InThumbTipSocketName);

  // A reference to the grab component we'll bind input to
  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = InteractionSDK)
  TObjectPtr<UIsdkGrabberComponent> Grabber;

  /*
   * Gets the EnabledConditionalGroup, which allows adding conditions that drive whether
   * the grabber component is activated.
   */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  UIsdkConditionalGroupAll* GetEnabledConditional() const
  {
    return EnabledConditionalGroup;
  }

 protected:
  UPROPERTY()
  TObjectPtr<UIsdkRigComponent> RigComponent;

 private:
  UPROPERTY()
  TScriptInterface<IIsdkIHandPointerPose> HandPointerPose;

  UPROPERTY()
  TScriptInterface<IIsdkIHmdDataSource> HmdDataSource;

  UFUNCTION()
  void HandleIsEnabledConditionalChanged(bool bIsEnabled);

  // The name of the thumb tip bone or socket that pinch grab colliders should be attached to.
  UPROPERTY()
  FName ThumbTipSocketName;

  UPROPERTY()
  TObjectPtr<USkinnedMeshComponent> PinchAttachMesh;

  // Allows adding conditions that drive whether the grabber component is activated.
  UPROPERTY(BlueprintGetter = GetEnabledConditional, Category = InteractionSDK)
  TObjectPtr<UIsdkConditionalGroupAll> EnabledConditionalGroup;

  /*
   * A conditional that detects whether this component is active or not, is used by
   * EnabledConditionalGroup to drive whether the grabber is active.
   */
  UPROPERTY()
  TObjectPtr<UIsdkConditionalComponentIsActive> IsActiveConditional;

  bool bLastIsGrabbing = true;

  UFUNCTION()
  void HandleRigPinchSelect(UIsdkRigComponent* IsdkRigComponent);

  UFUNCTION()
  void HandleRigPinchUnselect(UIsdkRigComponent* IsdkRigComponent);

  UFUNCTION()
  void HandleRigPalmSelect(UIsdkRigComponent* IsdkRigComponent);

  UFUNCTION()
  void HandleRigPalmUnselect(UIsdkRigComponent* IsdkRigComponent);
};
