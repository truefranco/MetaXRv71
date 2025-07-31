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
#include "IsdkGrabDetector.h"
#include "Engine/EngineTypes.h"
#include "IsdkRayGrabDetector.generated.h"

struct FHitResult;

/**
 * @brief UIsdkRayGrabDetector is used by a grab interactor to detect grabbables at
 * along the path of a ray cast out from the grabber's pointer pose.
 *
 * @see UIsdkGrabDetector
 * @see UIsdkGrabberComponent
 * @see UIsdkGrabbableComponent
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories)
class OCULUSINTERACTION_API UIsdkRayGrabDetector : public UIsdkGrabDetector
{
  GENERATED_BODY()

 public:
  virtual void Tick(float DeltaTime) override;
  virtual void Select(EIsdkGrabInputMethod InputMethod) override;
  virtual void Unselect() override;

  UIsdkGrabbableComponent* GetHoveredGrabbable();
  void GetHit(FHitResult& OutHit) const;
  virtual UIsdkGrabbableComponent* GetGrabCandidate(EIsdkGrabInputMethod InputMethod) override;
  virtual EIsdkGrabDetectorType GetGrabDetectorType() const override;
  void DrawDebugVisuals() const;

 protected:
  /**
   * If true, visuals for this detector will be disabled when the
   * "Meta.InteractionSDK.DebugInteractionVisuals" console variable is enabled.
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  bool bDisableDebugVisuals = false;

  // The length of the ray used to detect grabbables
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  float RayLength = 1000.f;

  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabbableComponent* HoveredGrabbable;

  // The object types ray grab uses to detect grabbables.  If empty, will detect all object types.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  TArray<TEnumAsByte<EObjectTypeQuery>> ObjectQueryTypes;

  // The hit result recorded this frame
  FHitResult CurrentHit;

  // The hit result recorded at time of selection
  FHitResult SelectHit;

  // Transform from grabbed object space to hit space
  FTransform SelectHitTransform = FTransform::Identity;
};
