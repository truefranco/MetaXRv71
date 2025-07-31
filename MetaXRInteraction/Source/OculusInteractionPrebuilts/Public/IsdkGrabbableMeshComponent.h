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
#include "GameFramework/Actor.h"
#include "Components/MeshComponent.h"
#include "Materials/Material.h"

#include "Interaction/IsdkGrabbableComponent.h"
#include "IsdkGrabbableMeshComponent.generated.h"

/**
 * @class UIsdkGrabbableMeshComponent
 * @brief
 *
 * @addtogroup InteractionSDKPrebuilts
 */
UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (DisplayName = "ISDK Grabbable Mesh Component", BlueprintSpawnableComponent))
class OCULUSINTERACTIONPREBUILTS_API UIsdkGrabbableMeshComponent : public USceneComponent
{
  GENERATED_BODY()

 public:
  UIsdkGrabbableMeshComponent();
  virtual void BeginDestroy() override;

  UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  USceneComponent* Root;

  UPROPERTY(
      Instanced,
      EditAnywhere,
      BlueprintReadWrite,
      Category = InteractionSDK,
      meta = (UseComponentPicker))

  UStaticMeshComponent* MeshComponent;

  UPROPERTY(
      Instanced,
      EditAnywhere,
      BlueprintReadOnly,
      Category = InteractionSDK,
      meta = (UseComponentPicker))

  UStaticMesh* Mesh;

  UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  UMaterial* Material;

  UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  UMaterialInstanceDynamic* MaterialInstance;

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetMesh(UStaticMesh* NewMesh);

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetCollision(UShapeComponent* Coll);

 private:
  virtual void BeginPlay() override;

  UPROPERTY(Instanced)
  UIsdkGrabbableComponent* Grabbable;

  UPROPERTY(Instanced)
  UPrimitiveComponent* Collision{};

  bool CreatedCollider{false};
};
