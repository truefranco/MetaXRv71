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

#include "Engine/DataAsset.h"
#include "Templates/PimplPtr.h"
#include "StructTypes.h"
#include "IsdkInteractableComponent.h"
#include "IsdkISurfacePatch.h"
#include "Pointable/IsdkIPointable.h"

#include "IsdkPokeInteractable.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class PokeInteractable;

		namespace helper
		{
			class FPokeInteractableImpl;
		}
	}
} // namespace isdk::api

USTRUCT(BlueprintType, Category = InteractionSDK)
struct FIsdkPokeInteractableConfigOffsets
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK")
  float NormalOffset{};
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK")
  float TangentOffset{};
};

/**
 * @class UIsdkPokeInteractableConfigDataAsset
 * @brief Data asset that contain poke interactable configuration and methods for generating new
 * configuration defaults.
 *
 * @see UIsdkPokeInteractable
 * @addtogroup InteractionSDK
 */
UCLASS(
    BlueprintType,
    ClassGroup = (InteractionSDK),
    meta = (DisplayName = "ISDK Poke Interactable Config Data Asset"))
class OCULUSINTERACTION_API UIsdkPokeInteractableConfigDataAsset : public UDataAsset
{
  GENERATED_BODY()

 public:
  /**
   * @brief The structure containing the normal and tangent thresholds for start and end hover
   * events as well as select and cancel events.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK")
  FIsdkPokeInteractable_Config Config = CreateDefaultPanelConfig();

  /**
   * @brief Resets the Config member variable of this data asset to the generated defaults
   */
  UFUNCTION(BlueprintCallable, CallInEditor, Category = "InteractionSDK")
  void ResetToDefaultPanelConfig()
  {
    Config = CreateDefaultPanelConfig();
  }
  /**
   * @brief Creates a new generated set of poke interactable configurations (normal/tangent
   * thresholds for common poke events)
   * @return FIsdkPokeInteractable_Config The new set of default configurations generated
   */
  static FIsdkPokeInteractable_Config CreateDefaultPanelConfig();
};

/**
 * @class UIsdkPokeInteractable
 * @brief IsdkPokeInteractable drives the ability for an actor to be poked, initiated by a single
 * finger non-physics interaction. It is expected to be interacted with by a IsdkPokeInteractor on
 * the player pawn.
 *
 * @see UIsdkInteractableComponent
 * @see IIsdkIPointable
 * @addtogroup InteractionSDK
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Poke Interactable"))
class OCULUSINTERACTION_API UIsdkPokeInteractable : public UIsdkInteractableComponent,
                                                    public IIsdkIPointable
{
  GENERATED_BODY()

 public:
  UIsdkPokeInteractable();
  virtual void BeginPlay() override;
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void BeginDestroy() override;

  virtual bool IsApiInstanceValid() const override;
  virtual isdk::api::IInteractable* GetApiIInteractable() const override;
  isdk::api::PokeInteractable* GetApiPokeInteractable() const;

  /* Returns a struct containing normal and tangent offsets */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  const FIsdkPokeInteractableConfigOffsets& GetConfigOffsets() const;

  /* Returns the currently set configuration DataAsset (does not currently validate) */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkPokeInteractableConfigDataAsset* GetConfigAsset() const
  {
    return ConfigAsset;
  }

  /* Returns this PokeInteractable's current configuration (hover/select normals and tangents) as a
   * struct */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  FIsdkPokeInteractable_Config GetCurrentConfig() const;

  /* Returns the object implementing IIsdkISurfacePatch that this is utilizing for interactions */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  TScriptInterface<IIsdkISurfacePatch> GetSurfacePatch() const
  {
    return SurfacePatch;
  }

  /* Sets normal and tangent offsets via struct */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetConfigOffsets(const FIsdkPokeInteractableConfigOffsets& InOffsets);

  /* Sets the given DataAsset to this PokeInteractable and applies its configuration if valid */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetConfigAsset(UIsdkPokeInteractableConfigDataAsset* InConfigAsset);

  /* Sets the object implementing IIsdkISurfacePatch to be used by this PokeInteractable */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetSurfacePatch(const TScriptInterface<IIsdkISurfacePatch>& InSurfacePatch);

  // Event Getters
  virtual FIsdkInteractionPointerEventDelegate& GetInteractionPointerEventDelegate() override
  {
    return InteractorPointerEvent;
  }

 private:
  void ApplyConfigToInstance() const;

  // Event for PointerEvents
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkInteractionPointerEventDelegate InteractorPointerEvent;

  // Properties
  UPROPERTY(
      BlueprintGetter = GetConfigOffsets,
      BlueprintSetter = SetConfigOffsets,
      Category = InteractionSDK,
      EditAnywhere)
  FIsdkPokeInteractableConfigOffsets ConfigOffsets;

  UPROPERTY(
      BlueprintGetter = GetConfigAsset,
      BlueprintSetter = SetConfigAsset,
      Category = InteractionSDK,
      EditAnywhere)
  UIsdkPokeInteractableConfigDataAsset* ConfigAsset{};

  UPROPERTY(
      BlueprintGetter = GetSurfacePatch,
      BlueprintSetter = SetSurfacePatch,
      Category = InteractionSDK)
  TScriptInterface<IIsdkISurfacePatch> SurfacePatch{};

  TPimplPtr<isdk::api::helper::FPokeInteractableImpl> PokeInteractableImpl{};
  int64 PointerEventToken{};

  UFUNCTION()
  void HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent);
};
