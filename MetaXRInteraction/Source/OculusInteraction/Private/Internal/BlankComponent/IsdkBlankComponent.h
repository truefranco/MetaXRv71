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
#include "Interaction/IsdkISurfacePatch.h"
#include "IsdkBlankComponent.generated.h"

// Forward declarations of internal types, so they can be returned from GetApiXYZ functions.
namespace isdk {
	namespace api
	{
		class BlankComponent;

		namespace helper
		{
			class FIsdkBlankComponentImpl;
		}
	}
}// namespace isdk::api

/**
 * This is example component that derives from UActorComponent.
 * It demonstrates the pattern for taking a dependency on one other object (a ClippedPlaneSurface)
 * and one other basic property type (an FVector)
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Component"))
class OCULUSINTERACTION_API UIsdkBlankComponent
    // Use USceneComponent instead if this component requires a hierarchical transform
    : public UActorComponent

{
  GENERATED_BODY()

 public:
  UIsdkBlankComponent();

  // Tear down any state that exists on the internal opaque IsdkBlankComponentImpl instance.
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  // Delete the internal opaque IsdkBlankComponentImpl instance.
  virtual void BeginDestroy() override;

  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

  /**
   * @brief Is the Instance on the internal IsdkBlankComponentImpl object valid? Call this before
   * making a call to any of the GetApiXYZ methods, if you want to avoid lazy-creating an object.
   * @return `true` if the internal API instance has already been created.
   */
  virtual bool IsApiInstanceValid() const;

  /**
   * @brief Retrieves a handle object to the internal isdk native instance.
   * A call to this method will lazy-create the instance if it doesn't already exist, which will in
   * turn cause any dependencies to be lazy-created.
   * If any step of creation failed, this method will return null and no longer attempt creation on
   * future calls.
   */
  isdk::api::BlankComponent* GetApiIsdkBlankComponent();

  // Property Getters
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  const FVector& GetVectorOfSomething() const
  {
    return VectorOfSomething;
  }
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  TScriptInterface<IIsdkISurfacePatch> GetSurfacePatch() const
  {
    return SurfacePatch;
  }

  // Property Setters
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetVectorOfSomething(const FVector& InVectorOfSomething);
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetSurfacePatch(TScriptInterface<IIsdkISurfacePatch> InSurfacePatch);

 private:
  // Properties
  UPROPERTY(
      BlueprintGetter = GetVectorOfSomething,
      BlueprintSetter = SetVectorOfSomething,
      Category = InteractionSDK)
  FVector VectorOfSomething{};
  UPROPERTY(
      BlueprintGetter = GetSurfacePatch,
      BlueprintSetter = SetSurfacePatch,
      Category = InteractionSDK)
  TScriptInterface<IIsdkISurfacePatch> SurfacePatch{};

  // Opaque private implementation
  TPimplPtr<isdk::api::helper::FIsdkBlankComponentImpl> IsdkBlankComponentImpl;
};
