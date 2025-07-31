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
#include "IsdkInteractorComponent.h"
#include "DataSources/IsdkIRootPose.h"

#include "IsdkPokeInteractor.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class PokeInteractor;

		namespace helper
		{
			class FPokeInteractorImpl;
		}
	}
} // namespace isdk::api

/**
 * @class UIsdkPokeInteractor
 * @brief IsdkPokeInteractor drives the ability for a pawn to interact with actors that have an
 * IsdkPokeInteractable attached to them
 *
 * Initiated by a single finger non-physics interaction. Passes data to/from the API instance.
 *
 * @see UIsdkInteractorComponent
 * @addtogroup InteractionSDK
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Poke Interactor"))
class OCULUSINTERACTION_API UIsdkPokeInteractor : public UIsdkInteractorComponent
{
  GENERATED_BODY()

 public:
  UIsdkPokeInteractor();
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
   * @brief With set data sources, updates the poke point transform and draws debug visuals if
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
  isdk::api::PokeInteractor* GetApiPokeInteractor() const;

  /**
   * @brief Returns the root pose object (implementing IIsdkIRootPose) being utilized by this
   * PokeInteractor, not guaranteed to be valid if one has not been set.
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  TScriptInterface<IIsdkIRootPose> GetRootPose() const
  {
    return RootPose;
  }
  /**
   * @brief Returns a struct with the current threshold and radius configurations for this
   * PokeInteractor
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  const FIsdkPokeInteractor_Config& GetConfig() const
  {
    return Config;
  }

  /**
   * @brief Sets the root pose object (implementing IIsdkIRootPose) to be utilized by this
   * PokeInteractor (does not currently validate).
   */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetRootPose(const TScriptInterface<IIsdkIRootPose>& InRootPose);

  /**
   * @brief Overwrites the current threshold/radius configuration with the given struct
   */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetConfig(const FIsdkPokeInteractor_Config& InConfig);

 protected:
  /**
   * @brief When true, disables debug visuals from being drawn, regardless of console variables
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool bDisableDebugVisuals = false;

  /**
   * @brief Returns whether or not this interactor should be enabled, using its Active status as a
   * metric. Overridden from UIsdkInteractorComponent.
   * @return bool If this interactor should be enabled
   */
  virtual bool ShouldInteractorBeEnabled() const override;

  virtual const TCHAR* GetInteractorTypeHint() const override
  {
    return InteractorTypeHint;
  }
  static constexpr const TCHAR* InteractorTypeHint = TEXT("poke");

 private:
  bool IsRootPoseValid() const;
  void SetPointTransform(const FTransform& Transform);

  TPimplPtr<isdk::api::helper::FPokeInteractorImpl> PokeInteractorImpl;

  // Properties
  UPROPERTY(
      BlueprintGetter = GetConfig,
      BlueprintSetter = SetConfig,
      EditDefaultsOnly,
      Category = InteractionSDK)
  FIsdkPokeInteractor_Config Config;

  UPROPERTY(
      BlueprintSetter = SetRootPose,
      BlueprintGetter = GetRootPose,
      Category = InteractionSDK,
      EditInstanceOnly,
      meta = (ExposeOnSpawn = true))
  TScriptInterface<IIsdkIRootPose> RootPose;

  void DrawDebugVisuals() const;
};
