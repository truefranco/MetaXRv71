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

#include "IsdkExternalHandDataSource.h"
#include "IsdkHandData.h"
#include "IsdkExternalHandDataModifier.generated.h"

/**
 * @class UIsdkExternalHandDataModifier
 * @brief Hand Data Source intended to take and modify an existing HandData Source
 *
 * External Hand Data modifiers are intended to be implemented downstream of an existing Hand Data
 * source, modifying it in some way and then passing the data further downstream
 *
 * @see UIsdkExternalHandDataSource
 * @see UIsdkHandDataSource
 * @addtogroup InteractionSDK
 */
UCLASS(
    ClassGroup = (InteractionSDK),
    Blueprintable,
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK External Hand Data Modifier"))
class OCULUSINTERACTION_API UIsdkExternalHandDataModifier : public UIsdkExternalHandDataSource
{
  GENERATED_BODY()
 public:
  /**
   * @brief Updates the hand tracking data every frame.
   * @param DeltaTime Time since the last frame.
   * @param TickType Type of update this tick represents.
   * @param ThisTickFunction Tick function details provided by Unreal Engine.
   * This method is automatically managed by the engine, ensuring that the latest hand data is
   * always available.
   */
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;
  virtual void OnModify_Implementation(UIsdkHandData* InputHandData);

  /**
   * @brief Get the Hand Data Source this Modifier should be taking in as input
   * @return UIsdkHandDataSource The Hand Data source object used as input for this modifier
   */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  UIsdkHandDataSource* GetInputDataSource() const
  {
    return InputDataSource;
  }

  /**
   * @brief Called when this ticks if all data sources are valid, broadcasts a OnHandDataModified
   * delegate
   * @param InputHandData The Hand Data being modified
   * Blueprint native event, called by TickComponent if the data source and hand data are valid.
   * This gives developers a hook to add additional logic in Blueprint after hand data has been set
   * per tick.
   */
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = InteractionSDK)
  void OnModify(UIsdkHandData* InputHandData);

  /**
   * @brief Sets the hand data source this modifier should use as input
   * @param InInputDataSource The new data source to use for this modifier
   *
   * Validates the new data source, removes tick prerequisites on the old one (if present) and adds
   * tick prerequisites to the new data source
   */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetInputDataSource(UIsdkHandDataSource* InInputDataSource);

  /* Delegate broadcast during Tick, if all data sources are valid */
  DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandDataModified, UIsdkHandDataSource*, InHandData);
  UPROPERTY(BlueprintAssignable)
  FOnHandDataModified OnHandDataModified;

  /* Hand Data Source used as input for this modifier */
  UPROPERTY(
      BlueprintGetter = GetInputDataSource,
      BlueprintSetter = SetInputDataSource,
      EditAnywhere,
      Category = InteractionSDK)
  UIsdkHandDataSource* InputDataSource = nullptr;
};
