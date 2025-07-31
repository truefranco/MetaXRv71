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

#include "RoundedBoxVisuals/IsdkInformationFrame.h"
#include "RoundedBoxVisuals/IsdkRoundedBoxFunctionLibrary.h"
#include "Materials/Material.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "IsdkContentAssetPaths.h"

AIsdkInformationFrame::AIsdkInformationFrame()
{
  InformationFrameRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
  SetRootComponent(InformationFrameRoot);

  LabelGroup = CreateDefaultSubobject<USceneComponent>(TEXT("Label"));
  LabelGroup->SetupAttachment(InformationFrameRoot);

  FloorOutline = CreateDefaultSubobject<UStaticMeshComponent>(FName("FloorOutline"));
  FloorOutline->SetupAttachment(InformationFrameRoot);

  LabelTextRender = CreateDefaultSubobject<UTextRenderComponent>(FName("LabelText"));
  LabelTextRender->SetupAttachment(LabelGroup);

  LabelBackground = CreateDefaultSubobject<UStaticMeshComponent>(FName("LabelBackground"));
  LabelBackground->SetupAttachment(LabelGroup);
  LabelBackground->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0, 0.0, 90.0)));

  FloorOutline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  LabelBackground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AIsdkInformationFrame::Initialize()
{
  auto Properties = FIsdkRoundedBoxProperties();
  Properties.OutlineColor = OutlineColor;
  Properties.FillColor = OutlineColor;
  Properties.FillColor.A = 0.0;
  Properties.InnerBorderRadius = OutlineWidth;
  Properties.CornerRadius =
      FVector4(OutlineCornerRadius, OutlineCornerRadius, OutlineCornerRadius, OutlineCornerRadius);
  Properties.Size = OutlineSize;
  UMaterialInstanceDynamic* FloorOutlineMaterial;
  UIsdkRoundedBoxFunctionLibrary::SetupRoundedBox(
      this, Properties, InformationFrameRoot, FloorOutline, FloorOutlineMaterial, false);

  auto TextMaterial = Cast<UMaterial>(StaticLoadObject(
      UMaterial::StaticClass(), nullptr, IsdkContentAssetPaths::Materials::TextUnlit));
  SetTextRenderProperties(LabelTextRender, PrimaryLabelProperties, TextMaterial);
  auto PrimarySize = LabelTextRender->GetTextLocalSize();
  auto TextSize = PrimarySize;

  if (!SecondaryLabelProperties.Text.IsEmpty())
  {
    SecondaryLabelTextRender = Cast<UTextRenderComponent>(AddComponentByClass(
        UTextRenderComponent::StaticClass(), true, FTransform::Identity, false));
    SecondaryLabelTextRender->AttachToComponent(
        LabelGroup, FAttachmentTransformRules::SnapToTargetIncludingScale);
    SetTextRenderProperties(SecondaryLabelTextRender, SecondaryLabelProperties, TextMaterial);
    auto SecondarySize = SecondaryLabelTextRender->GetTextLocalSize();
    TextSize.Y = FMath::Max(PrimarySize.Y, SecondarySize.Y);
    TextSize.Z += SecondarySize.Z + LabelMargin;

    SetTextPosition(SecondaryLabelTextRender, SecondaryLabelProperties, TextSize, -1.0);
  }
  SetTextPosition(LabelTextRender, PrimaryLabelProperties, TextSize, 1.0);

  auto BackgroundProperties = FIsdkRoundedBoxProperties();
  BackgroundProperties.FillColor = BackgroundColor;
  BackgroundProperties.OutlineColor = BackgroundColor;
  BackgroundProperties.OutlineColor.A = 0.0;
  BackgroundProperties.CornerRadius = FVector4(
      BackgroundCornerRadius,
      BackgroundCornerRadius,
      BackgroundCornerRadius,
      BackgroundCornerRadius);
  BackgroundProperties.Size = FVector2D(TextSize.Y, TextSize.Z) + BackgroundPadding;
  UMaterialInstanceDynamic* BackgroundMaterial;
  UIsdkRoundedBoxFunctionLibrary::SetupRoundedBox(
      this, BackgroundProperties, LabelGroup, LabelBackground, BackgroundMaterial, false);

  auto LabelLocation =
      FVector(OutlineSize.X * 0.5 - BackgroundProperties.Size.X * 0.5, -OutlineSize.Y * 0.5, 0.0);
  auto AngleRadians = FMath::DegreesToRadians(BackgroundAngle);
  LabelLocation.Y += BackgroundProperties.Size.Y * 0.5 * FMath::Cos(AngleRadians);
  LabelLocation.Z += BackgroundProperties.Size.Y * 0.5 * FMath::Sin(AngleRadians);

  LabelLocation.X *= LabelFrameAttachment.X;
  LabelLocation.Y *= LabelFrameAttachment.Y;

  LabelLocation += LabelOffset;

  LabelGroup->SetRelativeLocation(LabelLocation);
  LabelGroup->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0, -BackgroundAngle, -90.0)));

  FloorOutline->SetVisibility(bShowOutline);
  FloorOutline->SetHiddenInGame(!bShowOutline);
}

void AIsdkInformationFrame::OnConstruction(const FTransform& Transform)
{
  Super::OnConstruction(Transform);
  Initialize();
}

void AIsdkInformationFrame::PostInitializeComponents()
{
  Super::PostInitializeComponents();
  Initialize();
}

void AIsdkInformationFrame::SetTextPosition(
    UTextRenderComponent* TextRender,
    FIsdkInformationFrameLabelProps Properties,
    FVector ContainerSize,
    float VerticalMultiplier)
{
  const auto OffsetMultiplier = Properties.Alignment == EHorizTextAligment::EHTA_Center ? 0.0
      : Properties.Alignment == EHorizTextAligment::EHTA_Left                           ? 1.0
                                                                                        : -1.0;
  const auto Offset = ContainerSize.Y * 0.5 * OffsetMultiplier;
  FVector TextSize = TextRender->GetTextLocalSize();
  auto PrimaryRelativeLocation =
      FVector((TextSize.Z - ContainerSize.Z) * 0.5 * VerticalMultiplier, Offset, 0.1);
  TextRender->SetRelativeLocation(PrimaryRelativeLocation);
}

void AIsdkInformationFrame::SetTextRenderProperties(
    UTextRenderComponent* TextRender,
    FIsdkInformationFrameLabelProps Properties,
    UMaterial* Material)
{
  TextRender->SetRelativeLocation(FVector(0.0, 0.0, 0.01));
  TextRender->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0, 90.0, 0.0)));
  TextRender->SetText(Properties.Text);
  TextRender->SetWorldSize(Properties.Size);
  TextRender->SetTextRenderColor(Properties.Color.ToFColor(false));
  TextRender->SetHorizontalAlignment(Properties.Alignment);
  TextRender->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
  TextRender->SetMaterial(0, Material);
}

#if WITH_EDITOR
bool AIsdkInformationFrame::CanEditChange(const FProperty* InProperty) const
{
  if (InProperty)
  {
    FString PropertyName = InProperty->GetName();
    if (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(AIsdkInformationFrame, OutlineColor) ||
        PropertyName ==
            GET_MEMBER_NAME_STRING_CHECKED(AIsdkInformationFrame, OutlineCornerRadius) ||
        PropertyName == GET_MEMBER_NAME_STRING_CHECKED(AIsdkInformationFrame, OutlineWidth) ||
        PropertyName == GET_MEMBER_NAME_STRING_CHECKED(AIsdkInformationFrame, OutlineSize))
    {
      return bShowOutline;
    }
  }
  return Super::CanEditChange(InProperty);
}
#endif
