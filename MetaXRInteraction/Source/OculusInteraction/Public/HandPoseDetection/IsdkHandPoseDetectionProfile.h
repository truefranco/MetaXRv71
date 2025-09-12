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
#include "GameplayTagContainer.h"
#include "IsdkHandData.h"
#include "StructTypes.h"
#include "IsdkHandPoseDetectionProfile.generated.h"

class UIsdkHandMeshComponent;

/*
 */

/* */
USTRUCT(BlueprintType)
struct FIsdkHandPoseDetectionFingerTarget
{
  GENERATED_BODY()

  // The Target values for this finger
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  TMap<EIsdkDetection_FingerCalcType, float> FingerCalcTargets;

  //
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  TMap<EIsdkDetection_FingerCalcType, float> FingerCalcTolerances;
};

/* */
USTRUCT(BlueprintType)
struct FIsdkHandPoseDetectionThumbTarget
{
  GENERATED_BODY()

  // The Target values for this finger
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  TMap<EIsdkDetection_ThumbCalcType, float> ThumbCalcTargets;

  //
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  TMap<EIsdkDetection_ThumbCalcType, float> ThumbCalcTolerances;
};

/* A delta profile is compared against current hand pose to produce a result*/
/**
 * @class UIsdkHandPoseDetectionProfile
 * @brief Data Asset that stores information used for detecting hand poses, primarily the target
 * hand shapes required to be detected by the hand pose subsystem. Includes semantic information as
 * well as target tolerannces.
 *
 * @see UIsdkHandPoseSubsystem
 * @addtogroup InteractionSDK
 *
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (DisplayName = "ISDK Hand Pose Detection Profile"))
class OCULUSINTERACTION_API UIsdkHandPoseDetectionProfile : public UPrimaryDataAsset
{
  GENERATED_BODY()
 public:
  UIsdkHandPoseDetectionProfile();

  /**
   * @brief Name given to this profile upon creation, used to distinguish it from other profiles,
   * giving developers a quick way to discern between multiple returned delegates
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  FName PoseDetectionName;

  /**
   * @brief Gameplay tag container that can be used to distinguish between multiple broadcasted
   * detection profiles.
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  FGameplayTagContainer GameplayTags;

  /**
   * @brief Map of Finger Type to Finger Targets, which contain the target and tolerance data
   * required to establish a pose detection
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  TMap<EIsdkFingerType, FIsdkHandPoseDetectionFingerTarget> ProfileFingerTargets;

  /**
   * @brief Thumb targets for this pose detection profile, including target hand shape data and
   * tolerances used to establish a pose detection.
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  FIsdkHandPoseDetectionThumbTarget ProfileThumbTarget;
};
