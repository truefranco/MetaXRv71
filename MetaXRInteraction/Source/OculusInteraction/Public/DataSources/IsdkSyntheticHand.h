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
#include "IsdkIRootPoseLocker.h"
#include "DataSources/IsdkHandDataModifier.h"

#include "IsdkSyntheticHand.generated.h"

// Forward declarations of internal types
namespace isdk {
	namespace api
	{
		class SyntheticHand;

		namespace helper
		{
			class FSyntheticHandImpl;
		}
	}
} // namespace isdk::api

/* Hand Data Modifier that returns a HandDataSource driven by the API's Synthetic Hand */
UCLASS(
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Synthetic Hand"))
class OCULUSINTERACTION_API UIsdkSyntheticHand : public UIsdkHandDataModifier,
                                                 public IIsdkIRootPoseLocker
{
  GENERATED_BODY()

 public:
  UIsdkSyntheticHand();
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void BeginDestroy() override;

  virtual bool IsApiInstanceValid() const override;
  virtual isdk::api::IHandDataSource* GetApiIHandDataSource() override;
  virtual isdk::api::IHandDataModifier* GetApiIHandDataModifier() override;
  isdk::api::SyntheticHand* GetApiSyntheticHand();

  virtual void LockRootTransform_Implementation(
      const FTransform& WristPose,
      const float OverrideFactor = 1.f,
      EIsdkRootPoseLockMode LockMode = EIsdkRootPoseLockMode::Full,
      bool bSkipAnimation = false) override;

  virtual void LockRootLocation_Implementation(
      const FVector& Position,
      const float OverrideFactor = 1.f,
      bool bSkipAnimation = false) override;

  virtual void LockRootRotation_Implementation(
      const FQuat& Rotation,
      const float OverrideFactor = 1.f,
      bool bSkipAnimation = false) override;

  virtual void FreeRootTransform_Implementation(
      EIsdkRootPoseLockMode LockMode = EIsdkRootPoseLockMode::Full) override;

 protected:
  TPimplPtr<isdk::api::helper::FSyntheticHandImpl> SyntheticHandImpl;
};
