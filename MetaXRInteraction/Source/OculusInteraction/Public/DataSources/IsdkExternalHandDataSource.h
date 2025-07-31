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
#include "UObject/Object.h"
#include "DataSources/IsdkHandDataSource.h"
#include "IsdkExternalHandDataSource.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class ExternalHandSource;

		namespace helper
		{
			class FExternalHandDataSourceImpl;
		}
	}
} // namespace isdk::api

/**
 * @class UIsdkExternalHandDataSource
 * @brief Hand Data Source, acts as a container for hand data, joint mappings, and other related
 * data for an external source
 *
 * Base class used by more specific external hand data sources for setting and querying hand data,
 * joint information, handedness and applying OpenXR support to older mappings
 *
 * @see UIsdkHandDataSource
 * @addtogroup InteractionSDK
 */
UCLASS(
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK External Hand Data Source"))
class OCULUSINTERACTION_API UIsdkExternalHandDataSource : public UIsdkHandDataSource
{
  GENERATED_BODY()

 public:
  UIsdkExternalHandDataSource();
  virtual void BeginPlay() override;
  virtual void BeginDestroy() override;

  /**
   * @brief Sets this Data Source's Hand Data object, passing in its current transform in worldspace
   * @param InHandData Hand data to set to this data source.
   * @param WorldTransform Current transform of the hand in worldspace
   *
   * Validates the passed in HandData, and then copies all joint information to HandData member
   * variable. Passes this along with world transform to API for further operations
   */
  UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, Category = InteractionSDK)
  void SetHandData(const UIsdkHandData* InHandData, const FTransform& WorldTransform);

  /**
   * @brief Sets the finger and thumb joint mappings used for this data source
   * @param ThumbJointMappings Thumb joint mappings to implement for this data source
   * @param FingerJointMappings Finger joint mappings to implement for this data source
   * @see UIsdkHandJointMappings
   *
   * Overrides local data with new mappings for both thumb and all finger joints,
   * expanding/contracting arrays as needed. Stored in the Hand Joint Mappings object
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetHandJointMappings(
      const TArray<FIsdkExternalHandPositionFrame_ThumbJointMapping>& ThumbJointMappings,
      const TArray<FIsdkExternalHandPositionFrame_FingerJointMapping>& FingerJointMappings);

  /**
   * @brief Returns the validity of the API instance associated with this data source
   * @return bool If the API instance associated with this data source is valid
   *
   * Provides the current validity state of the API instance, which is utilized to regularly push
   * and pull hand/HMD data for many operations.
   */
  virtual bool IsApiInstanceValid() const override;
  virtual isdk::api::IHandDataSource* GetApiIHandDataSource() override;
  virtual isdk::api::ExternalHandSource* GetApiExternalHandSource();

  /**
   * @brief Provides the handedness of this data source
   * @return EIsdkHandedness The handedness (left or right) assigned to this data source
   * @see IHandJoints
   */
  virtual EIsdkHandedness GetHandedness_Implementation() override
  {
    return Handedness;
  }

  /**
   * @brief Provides the Hand Joint Mappings object associated with this data source
   * @return UIsdkHandJointMappings The container for thumb and finger joint mappings
   * @see UIsdkHandJointMappings
   */
  virtual const UIsdkHandJointMappings* GetHandJointMappings_Implementation() override
  {
    return HandJointMappings;
  }

  /**
   * @brief Returns the current validity of the hand joint data set to this data source
   * @return bool Whether or not the hand joint data set to this data source is valid.
   *
   * This validation is implemented by children classes, this class will always return false.
   */
  virtual bool IsHandJointDataValid_Implementation() override
  {
    return bIsHandJointDataValid;
  }

  /* The handedness of this data source (left or right) */
  UPROPERTY(
      BlueprintReadWrite,
      EditAnywhere,
      Category = InteractionSDK,
      meta = (ExposeOnSpawn = true))
  EIsdkHandedness Handedness = EIsdkHandedness::Left;

  /**
   * @brief Generates the mapping between OVR and OpenXR bones
   *
   * Utilized to make sure OpenXR mapping is available for all external data sources, in the
   * instance of needing to map from OVR-style joint mappings.
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void PopulateOpenXRBoneMaps();

 protected:
  void SetImplHandData(const FTransform& Transform);

  TPimplPtr<isdk::api::helper::FExternalHandDataSourceImpl> ExternalHandDataSourceImpl;

  UPROPERTY()
  UIsdkHandJointMappings* HandJointMappings;

  UPROPERTY()
  bool bIsHandJointDataValid{false};
};
