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
#include "Templates/PimplPtr.h"
#include "IsdkInteractorComponent.h"
#include "StructTypes.h"
#include "DataSources/IsdkIHandPointerPose.h"
#include "IsdkRayInteractor.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class RayInteractor;

		namespace helper
		{
			class FSelectorImpl;
			class FRayInteractorImpl;
		} // namespace helper
	}
} // namespace isdk::api

/**
 * @class UIsdkRayInteractor
 * @brief IsdkRayInteractor drives the ability for a pawn to interact with actors that have an
 * IsdkRayInteractable attached to them.
 *
 * Interactions are initiated by pointing at them with a directed hand raycast or cursor. Passes
 * data to/from the API instance.
 *
 * @see UIsdkInteractorComponent
 * @addtogroup InteractionSDK
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Ray Interactor"))
class OCULUSINTERACTION_API UIsdkRayInteractor : public UIsdkInteractorComponent
{
  GENERATED_BODY()

 public:
  UIsdkRayInteractor();

  /**
   * @brief Called on EndPlay, overridden from USceneComponent. Destroys API instance and
   * unregisters this with World Subsystem.
   */
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  /**
   * @brief Called on BeginDestroy, overridden from USceneComponent. Checks if API instance was
   * destroyed during EndPlay.
   */
  virtual void BeginDestroy() override;

  /**
   * @brief With set data sources, updates the pointer pose of the ray, draws debug visuals if
   * enabled
   * @param DeltaTime Time since the last frame.
   * @param TickType Type of update this tick represents.
   * @param ThisTickFunction Tick function details provided by Unreal Engine.
   */
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  virtual bool IsApiInstanceValid() const override;
  virtual isdk::api::IInteractor* GetApiIInteractor() const override;
  isdk::api::RayInteractor* GetApiRayInteractor() const;

  /**
   * @brief Attempts to trigger a raycast selection with the given interactor position
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void Select();

  /**
   * @brief Triggers the deselection process
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void Unselect();

  /**
   * @brief Sets the cursor select strength to be used during ray interactions
   * @param InSelectStrength the new cursor select strength to use
   */
  void SetSelectStrength(float InSelectStrength);

  /* Returns the object implementing IIsdkIHandPointerPose that is the current pose used for
   * tracking the hand pointer pose */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  TScriptInterface<IIsdkIHandPointerPose> GetHandPointerPose() const
  {
    return HandPointerPose;
  }

  /* Returns the last collision surface hit information produced by the interactor raycasting */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  const FIsdkOptionalSurfaceHit& GetCollisionInfo() const
  {
    return CollisionInfo;
  }

  /* Returns the current ray interactor configuration struct */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  const FIsdkRayInteractor_Config& GetConfig() const
  {
    return Config;
  }

  /* Sets the object implementing IIsdkIHandPointerPose to be used for tracking hand pointer pose */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetHandPointerPose(const TScriptInterface<IIsdkIHandPointerPose>& InHandPointerPose);

  /* Sets the current ray interactor configuration via the passed struct */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetConfig(const FIsdkRayInteractor_Config& InConfig);

 protected:
  virtual bool ShouldInteractorBeEnabled() const override;
  virtual const TCHAR* GetInteractorTypeHint() const override
  {
    return InteractorTypeHint;
  }
  static constexpr const TCHAR* InteractorTypeHint = TEXT("ray");

  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  float SelectStrength = 0.f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool bDisableDebugVisuals = false;

 private:
  // State that need cleaning up when this actor leaves play
  TPimplPtr<isdk::api::helper::FRayInteractorImpl> RayInteractorImpl;
  TPimplPtr<isdk::api::helper::FSelectorImpl> SelectorImpl;

  // Properties
  UPROPERTY(
      BlueprintSetter = SetHandPointerPose,
      BlueprintGetter = GetHandPointerPose,
      EditInstanceOnly,
      Category = InteractionSDK)
  TScriptInterface<IIsdkIHandPointerPose> HandPointerPose;

  UPROPERTY(
      BlueprintGetter = GetConfig,
      BlueprintSetter = SetConfig,
      EditDefaultsOnly,
      Category = InteractionSDK)
  FIsdkRayInteractor_Config Config;

  UPROPERTY(BlueprintGetter = GetCollisionInfo, Category = InteractionSDK)
  FIsdkOptionalSurfaceHit CollisionInfo;

  void DrawDebugVisuals(const FTransform& PointerPose) const;
};
