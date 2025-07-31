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

#include "RoundedBoxVisuals/IsdkRoundedBoxFunctionLibrary.h"
#include "IsdkContentAssetPaths.h"
#include "Engine/StaticMesh.h"

void UIsdkRoundedBoxFunctionLibrary::CreateRoundedBox(
    AActor* Target,
    FIsdkRoundedBoxProperties Properties,
    USceneComponent* Parent,
    UStaticMeshComponent*& MeshComponent,
    UMaterialInstanceDynamic*& RoundedBoxMaterial,
    bool TwoSided)
{
  MeshComponent = Cast<UStaticMeshComponent>(Target->AddComponentByClass(
      UStaticMeshComponent::StaticClass(), false, FTransform::Identity, false));
  MeshComponent->bAlwaysCreatePhysicsState = false;
  MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
  SetupRoundedBox(Target, Properties, Parent, MeshComponent, RoundedBoxMaterial, TwoSided);
}

void UIsdkRoundedBoxFunctionLibrary::SetupRoundedBox(
    AActor* Target,
    FIsdkRoundedBoxProperties Properties,
    USceneComponent* Parent,
    UStaticMeshComponent* MeshComponent,
    UMaterialInstanceDynamic*& RoundedBoxMaterial,
    bool TwoSided)
{
  UMaterialInterface* Material = nullptr;
  UMaterialInterface* RoundedBoxUnlit_TwoSided = Cast<UMaterialInterface>(StaticLoadObject(
      UMaterialInterface::StaticClass(),
      nullptr,
      IsdkContentAssetPaths::Materials::RoundedBoxUnlit_TwoSided));
  UMaterialInterface* RoundedBoxUnlit = Cast<UMaterialInterface>(StaticLoadObject(
      UMaterialInterface::StaticClass(),
      nullptr,
      IsdkContentAssetPaths::Materials::RoundedBoxUnlit));

  if (TwoSided)
  {
    Material = RoundedBoxUnlit_TwoSided;
  }
  else
  {
    Material = RoundedBoxUnlit;
  }

  if (!ensureMsgf(Material != nullptr, TEXT("RoundedBoxUnlit Material could not be loaded.")))
  {
    return;
  }

  auto MaterialInstance =
      UMaterialInstanceDynamic::Create(Material, Target, FName("Rounded Box Material"));
  auto PlaneMesh = Cast<UStaticMesh>(StaticLoadObject(
      UStaticMesh::StaticClass(), nullptr, IsdkContentAssetPaths::Engine::BasicShapes::Plane));

  if (!ensureMsgf(PlaneMesh != nullptr, TEXT("Plane mesh could not be loaded.")))
  {
    return;
  }
  if (!ensureMsgf(IsValid(MeshComponent), TEXT("Plane Mesh Component is invalid.")))
  {
    return;
  }
  MeshComponent->SetStaticMesh(PlaneMesh);
  MeshComponent->SetMaterial(0, MaterialInstance);
  SetRoundedBoxMaterialParameters(MaterialInstance, Properties);
  if (IsValid(Parent))
  {
    MeshComponent->AttachToComponent(
        Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    MeshComponent->SetRelativeLocation(FVector::Zero());
  }

  SetMeshComponentScale(MeshComponent, Properties.Size, Properties.OuterBorderRadius);
  RoundedBoxMaterial = MaterialInstance;
}

void UIsdkRoundedBoxFunctionLibrary::SetRoundedBoxMaterialParameters(
    UMaterialInstanceDynamic* RoundedBoxMaterial,
    FIsdkRoundedBoxProperties Properties)
{
  auto Dimensions = FVector4(
      Properties.Size,
      FVector2D(
          FMath::Max(Properties.InnerBorderRadius, 0.0f),
          FMath::Max(Properties.OuterBorderRadius, 0.0f)));
  SetRoundedBoxDimensions(
      RoundedBoxMaterial,
      Properties.Size,
      Properties.InnerBorderRadius,
      Properties.OuterBorderRadius);
  SetRoundedBoxMaterialColor(RoundedBoxMaterial, Properties.FillColor);
  SetRoundedBoxMaterialBorderColor(RoundedBoxMaterial, Properties.OutlineColor);
  SetRoundedBoxMaterialCornerRadius(RoundedBoxMaterial, Properties.CornerRadius);
}
