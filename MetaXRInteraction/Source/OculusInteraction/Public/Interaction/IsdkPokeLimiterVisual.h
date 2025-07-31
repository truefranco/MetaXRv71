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
#include "Components/ActorComponent.h"
#include "DataSources/IsdkSyntheticHand.h"
#include "Interaction/IsdkPokeInteractor.h"
#include "DataSources/IsdkHandDataSource.h"
#include "Subsystem/IsdkWorldSubsystem.h"
#include "IsdkPokeLimiterVisual.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class HandPokeLimiterVisual;

		namespace helper
		{
			class FPokeLimiterVisualImpl;
		}
	}
} // namespace isdk::api

/**
 * @class UIsdkPokeLimiterVisual
 * @brief An ActorComponent that, when present on a PokeInteractable, will limit the hand visual
 * from passing through the poked surface.
 *
 * The hand visual will be snapped to the poked surface regardless of how far past the user moves
 * their hand.
 * @see UIsdkPokeInteractable
 * @addtogroup InteractionSDK
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Poke Limiter Visual"))
class OCULUSINTERACTION_API UIsdkPokeLimiterVisual : public UActorComponent
{
  GENERATED_BODY()

 public:
  UIsdkPokeLimiterVisual();
  /**
   * @brief Called on EndPlay, overridden from UActorComponent. Unregisters the poke event handler.
   */
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  /**
   * @brief Called on BeginDestroy, overridden from UActorComponent. Resets the reference to this in
   * the API.
   */
  virtual void BeginDestroy() override;

  /**
   * @brief Returns the PokeInteractor currently engaged
   * @return UIsdkPokeInteractor* Reference to the current PokeInteractor this limiter is acting
   * upon
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkPokeInteractor* GetPokeInteractor() const
  {
    return PokeInteractor;
  }

  /**
   * @brief Returns reference to the SyntheticHand being used in the interaction
   * @return UIsdkSyntheticHand* Reference to the current synthetic hand (related to a
   * PokeInteractor) this limiter is acting upon
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkSyntheticHand* GetSyntheticHand() const
  {
    return SyntheticHand;
  }

  /**
   * @brief Returns the Hand Data source connected to the hand being used in the interaction
   * @return UIsdkHandDataSource* Reference to the current hand data source (related to a
   * PokeInteractor) this limiter is acting upon
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkHandDataSource* GetDataSource() const
  {
    return DataSource;
  }

  /**
   * @brief Retrieves the delegate broadcast when the limiter is engaged in an poke interaction
   * event
   * @return FIsdkIUpdateEventDelegate& Reference to the event updated delegate
   */
  FIsdkIUpdateEventDelegate& GetUpdatedEventDelegate()
  {
    return Updated;
  }

  /**
   * @brief Sets the PokeInteractor to be engaged with this limiter, will set dependencies after
   */
  UFUNCTION(BlueprintSetter, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetPokeInteractor(UIsdkPokeInteractor* InPokeInteractor);

  /**
   * @brief Sets the SyntheticHand to be used in the interaction, will set dependencies after
   */
  UFUNCTION(BlueprintSetter, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetSyntheticHand(UIsdkSyntheticHand* InSyntheticHand);

  /**
   * @brief Sets the Hand Data source connected to the hand being used in the interaction, will set
   * dependencies after
   */
  UFUNCTION(BlueprintSetter, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetDataSource(UIsdkHandDataSource* InHandDataSource);

 private:
  void SetupImplDependencies();

  // State that need cleaning up when this actor leaves play
  TPimplPtr<isdk::api::helper::FPokeLimiterVisualImpl> PokeLimiterVisualImpl;

  UPROPERTY(
      BlueprintGetter = GetPokeInteractor,
      BlueprintSetter = SetPokeInteractor,
      Category = InteractionSDK)
  UIsdkPokeInteractor* PokeInteractor;
  UPROPERTY(
      BlueprintGetter = GetSyntheticHand,
      BlueprintSetter = SetSyntheticHand,
      Category = InteractionSDK)
  UIsdkSyntheticHand* SyntheticHand;
  UPROPERTY(
      BlueprintGetter = GetDataSource,
      BlueprintSetter = SetDataSource,
      Category = InteractionSDK)
  UIsdkHandDataSource* DataSource;

  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkIUpdateEventDelegate Updated;
  int64 UpdateEventToken{};
};
