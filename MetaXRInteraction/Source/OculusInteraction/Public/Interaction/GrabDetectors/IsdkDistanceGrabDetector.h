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
#include "IsdkDistanceGrabDetector.generated.h"

class USphereComponent;

/**
 * @brief UIsdkDistanceGrabDetector is used by a grab interactor to detect grabbables at
 * a distance.  The current mechanism for detection is to check for grabbables within a cone
 * from the grabber's pointer pose.
 *
 * @see UIsdkGrabDetector
 * @see UIsdkGrabberComponent
 * @see UIsdkGrabbableComponent
 */
UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories)
class OCULUSINTERACTION_API UIsdkDistanceGrabDetector : public UIsdkGrabDetector
{
  GENERATED_BODY()

 public:
  virtual void Initialize(UIsdkGrabberComponent* InGrabberComponent) override;
  virtual void Deinitialize() override {}
  virtual void Tick(float DeltaTime) override;

  const TSet<UIsdkGrabbableComponent*>& GetHoveredGrabbables() const;
  virtual UIsdkGrabbableComponent* GetGrabCandidate(EIsdkGrabInputMethod InputMethod) override;
  void DrawDebugVisuals() const;
  virtual EIsdkGrabDetectorType GetGrabDetectorType() const override;

 protected:
  /**
   * The radius of the cone/frustum used to detect grabbables.  Note that under the hood, we're
   * using a sphere collider with radius equal to FrustumRadius to detect grab candidates, so very
   * large values may not be performant.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float FrustumRadius = 1000.f;

  /**
   * The angle of the cone/frustum (apex angle) used to detect grabbables.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  float FrustumAngle = 25.f;

  /**
   * If true, visuals for this detector will be disabled when the
   * "Meta.InteractionSDK.DebugInteractionVisuals" console variable is enabled.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool bDisableDebugVisuals = false;

  /**
   * The collision object type of the sphere component used to detect grabbables
   */
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InteractionSDK)
  TEnumAsByte<ECollisionChannel> CollisionObjectType = ECC_WorldDynamic;

  /**
   * HoveredGrabbables contains the set of all grabbables which are currently hovered by this
   * distance grab detector
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  TSet<UIsdkGrabbableComponent*> HoveredGrabbables;

  /**
   * CandidateGrabbable is the grabbable which will be grabbed if a grab is initiated.
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  UIsdkGrabbableComponent* CandidateGrabbable;

  /**
   * DistanceGrabCollider is the sphere collider used to detect all potential distance grab targets.
   * The set of overlapping grabbables is then further filtered by angle between the pointer pose
   * and the nearest point on the grabbable's bounding box to the pointer.
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  USphereComponent* DistanceGrabCollider;
};
