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
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "IsdkRoundedBoxFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct FIsdkRoundedBoxProperties
{
  GENERATED_BODY()
 public:
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  float InnerBorderRadius{};
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  float OuterBorderRadius{};
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  FVector2D Size = FVector2D::ZeroVector;
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  FVector4 CornerRadius = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  FLinearColor FillColor = FLinearColor::Black;
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  FLinearColor OutlineColor = FLinearColor::Black;
};

/**
 *
 */
UCLASS()
class OCULUSINTERACTIONPREBUILTS_API UIsdkRoundedBoxFunctionLibrary final
    : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()
 public:
  UFUNCTION(BlueprintCallable, Category = InteractionSDK, meta = (DefaultToSelf = Target))
  static void CreateRoundedBox(
      AActor* Target,
      FIsdkRoundedBoxProperties Properties,
      USceneComponent* Parent,
      UStaticMeshComponent*& MeshComponent,
      UMaterialInstanceDynamic*& RoundedBoxMaterial,
      bool TwoSided = true);
  UFUNCTION(BlueprintCallable, Category = InteractionSDK, meta = (DefaultToSelf = Target))
  static void SetupRoundedBox(
      AActor* Target,
      FIsdkRoundedBoxProperties Properties,
      USceneComponent* Parent,
      UStaticMeshComponent* MeshComponent,
      UMaterialInstanceDynamic*& RoundedBoxMaterial,
      bool TwoSided = true);

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static void SetRoundedBoxMaterialParameters(
      UMaterialInstanceDynamic* RoundedBoxMaterial,
      FIsdkRoundedBoxProperties Properties);

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static void SetMeshComponentScale(
      UStaticMeshComponent* MeshComponent,
      FVector2D Size,
      float OuterBorderRadius)
  {
    auto SizeOffset = FVector2D(FMath::Max(OuterBorderRadius, 0.0f) * 2.0);
    auto BoxSize =
        (Size + SizeOffset) / 100.0f; // Required because the default plane size is 100 units
    BoxSize = FVector2D::Max(FVector2D(0.001, 0.001), BoxSize);
    MeshComponent->SetRelativeScale3D(FVector(BoxSize, 1.0));
  }

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static void SetRoundedBoxDimensions(
      UMaterialInstanceDynamic* RoundedBoxMaterial,
      FVector2D Size,
      float InnerBorderRadius,
      float OuterBorderRadius)
  {
    auto Dimensions = FVector4(
        Size, FVector2D(FMath::Max(InnerBorderRadius, 0.0f), FMath::Max(OuterBorderRadius, 0.0f)));
    RoundedBoxMaterial->SetVectorParameterValue("Dimensions", FLinearColor(Dimensions));
  }

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static void SetRoundedBoxMaterialColor(
      UMaterialInstanceDynamic* RoundedBoxMaterial,
      FLinearColor Color)
  {
    RoundedBoxMaterial->SetVectorParameterValue("Color", Color);
  }

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static void SetRoundedBoxMaterialBorderColor(
      UMaterialInstanceDynamic* RoundedBoxMaterial,
      FLinearColor Color)
  {
    RoundedBoxMaterial->SetVectorParameterValue("BorderColor", Color);
  }

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  static void SetRoundedBoxMaterialCornerRadius(
      UMaterialInstanceDynamic* RoundedBoxMaterial,
      FVector4 Radii)
  {
    RoundedBoxMaterial->SetVectorParameterValue("Radii", FLinearColor(Radii));
  }
};
