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

#include "GameFramework/Actor.h"
#include "Templates/PimplPtr.h"
#include "CoreMinimal.h"
#include "IsdkHandData.h"
#include "IsdkIHandJoints.h"
#include "StructTypes.h"
#include "IsdkHandDataSource.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class IHandDataSource;

		namespace helper
		{
			class FDataSourceImpl;
		}
	}
} // namespace isdk::api

/**
 * @class UIsdkHandDataSource
 * @brief Abstract class, ActorComponent representing a Hand Data Source, acts as a container for
 * hand data, joint mappings, and other related data for a source
 *
 * @see IIsdkIHandJoints
 * @addtogroup InteractionSDK
 */
UCLASS(Abstract)
class OCULUSINTERACTION_API UIsdkHandDataSource : public UActorComponent, public IIsdkIHandJoints
{
  GENERATED_BODY()

 public:
  UIsdkHandDataSource();

  /**
   * @brief Called on BeginDestroy, overridden from UActorComponent. Resets the reference to this in
   * the implementation API to prevent references to garbage memory.
   */
  virtual void BeginDestroy() override;

  /**
   * @brief Updates the hand tracking data every frame, records last ticked time, and checks if
   * enough ticks have elapsed to update the data source
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

  virtual bool IsApiInstanceValid() const
      PURE_VIRTUAL(UIsdkHandDataSource::IsApiInstanceValid, return false;);

  virtual isdk::api::IHandDataSource* GetApiIHandDataSource()
      PURE_VIRTUAL(UIsdkHandDataSource::GetApiIHandDataSource, return nullptr;);

  /**
   * @brief Returns the HandData from this data source
   * @return UIsdkHandData* Reference to the active hand data object associated with this data
   * source. Implemented from IIsdkIHandJoints.
   */
  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  virtual UIsdkHandData* GetHandData_Implementation() override;

  /**
   * @brief Checks that the passed in EIsdkDataSourceUpdateDataResult is not in a failure state.
   * Throws an ensure if this is not the case
   * @param RetVal The state of the last data update for this data source, negative values equate to
   * a failed update
   * @return bool Whether or not this data source returned a clean result after being updated
   */
  static bool EnsureDataSourceResult(EIsdkDataSourceUpdateDataResult RetVal);

  static isdk::api::IHandDataSource* ValidateAndGetApiDataSource(
      UIsdkHandDataSource* InDataSource,
      FString&& ErrMsgFieldName);

  /**
   *  @brief Sets the Inbound Hand Data reference, and propagates the bone map data down to it
   * @param HandDataIn The HandData to set as a reference for this Data Source. It is validated, and
   * if clean, PropagateInboundBoneMap() is called.
   */
  void SetInboundHandData(UIsdkHandData* HandDataIn)
  {
    if (IsValid(HandDataIn))
    {
      HandDataInbound = HandDataIn;
      PropagateInboundBoneMap();
    }
  }
  /**
   * @brief Pushes down Inbound Bone Map to Inbound Hand Data
   */
  void PropagateInboundBoneMap()
  {
    if (IsValid(HandDataInbound))
    {
      HandDataInbound->SetInboundBoneMap(InboundBoneMap);
    }
  }

  /**
   * @brief Pushes down Outbound Bone Map to Outbound Hand Data
   */
  void PropagateOutboundBoneMap()
  {
    if (IsValid(HandData))
    {
      HandData->SetOutboundBoneMap(OutboundBoneMap);
    }
  }
  /**
   * @brief Return the Inbound and Outbound bone maps currently set in this data source
   * @param OutboundMapOut The map of integers representing the outbound bone mapping
   * @param InboundMapOut The map of integers representing the inbound bone mapping
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void GetBoneMaps(TMap<int32, int32>& OutboundMapOut, TMap<int32, int32>& InboundMapOut)
  {
    OutboundMapOut = OutboundBoneMap;
    InboundMapOut = InboundBoneMap;
  }

  /**
   * @brief Whether or not this data source should be updated during tick. Can be disabled if
   * per-frame bone updates are undesirable.
   */
  UPROPERTY(EditAnywhere, Category = InteractionSDK)
  bool bUpdateInTick = false;

  /**
   * @brief The last time this hand data source successfully ticked. Value is updated in
   * TickComponent and set by GetWorld()->GetTimeSeconds()
   */
  double LastTickedTime = 0.0f;

 protected:
  void ReadHandJointsFromImpl();

  /**
   * @brief Virtual function to determine whether or not this Hand Data Source should be updated.
   * Base class sets this to be equivalent to if it should tick.
   * @return bool Whether or not this data source should be continually updated
   */
  virtual bool ShouldUpdateDataSource()
  {
    return bUpdateInTick;
  }

  /**
   * @brief Active Hand Data pointer used in this data source for bone/joint information and mapping
   */
  UPROPERTY(BlueprintGetter = GetHandData_Implementation, Category = InteractionSDK)
  UIsdkHandData* HandData{};

  /**
   * @brief Hand data being set by something upstream of this data source, cached here instead of
   * directly overwriting the primary member variable
   */
  UPROPERTY()
  UIsdkHandData* HandDataInbound{};

  UPROPERTY()
  TMap<int32, int32> InboundBoneMap = {};

  UPROPERTY()
  TMap<int32, int32> OutboundBoneMap = {};

  TPimplPtr<isdk::api::helper::FDataSourceImpl> DataSourceImpl;
};
