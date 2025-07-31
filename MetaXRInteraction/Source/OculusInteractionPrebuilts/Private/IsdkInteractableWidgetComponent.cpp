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

#include "IsdkInteractableWidgetComponent.h"
#include "IsdkContentAssetPaths.h"
#include "Interaction/Surfaces/IsdkPointablePlane.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "IsdkFunctionLibrary.h"

UIsdkInteractableWidgetComponent::UIsdkInteractableWidgetComponent()
{
  PrimaryComponentTick.bCanEverTick = true;

  SelectedHoveredAudio = CreateDefaultSubobject<UAudioComponent>(FName("Selected Hovered Audio"));
  SelectedHoveredAudio->SetupAttachment(this);
  SelectedHoveredAudio->SetAutoActivate(false);

  SelectedEmptyAudio = CreateDefaultSubobject<UAudioComponent>(FName("Selected Empty Audio"));
  SelectedEmptyAudio->SetupAttachment(this);
  SelectedEmptyAudio->SetAutoActivate(false);

  Widget = CreateDefaultSubobject<UWidgetComponent>(FName("Widget Component"));
  Widget->SetupAttachment(this);

  PointableWidget = CreateDefaultSubobject<UIsdkPointableWidget>(FName("Pointable Widget"));
  WidgetEventAudioPlayer =
      CreateDefaultSubobject<UIsdkWidgetEventAudioPlayer>(FName("Event Audio Player"));
  ClippedPlaneSurface =
      CreateDefaultSubobject<UIsdkClippedPlaneSurface>(FName("Clipped Plane Surface"));

  // Find Materials
  using MaterialFinder = ConstructorHelpers::FObjectFinder<UMaterialInterface>;
  using Materials = IsdkContentAssetPaths::Materials;

  static MaterialFinder WidgetMaterialMasked(Materials::Widget3DRoundedBox_Masked);
  if (ensureMsgf(
          WidgetMaterialMasked.Succeeded(), TEXT("Could not find Widget3DRoundedBox_Masked")))
  {
    MaskedMaterial = WidgetMaterialMasked.Object;
  }

  static MaterialFinder WidgetMaterialMaskedOneSided(Materials::Widget3DRoundedBox_Masked_OneSided);
  if (ensureMsgf(
          WidgetMaterialMaskedOneSided.Succeeded(),
          TEXT("Could not find Widget3DRoundedBox_Masked_OneSided")))
  {
    MaskedMaterialOneSided = WidgetMaterialMaskedOneSided.Object;
  }

  static MaterialFinder WidgetMaterialTransparent(Materials::Widget3DRoundedBox_Transparent);
  if (ensureMsgf(
          WidgetMaterialTransparent.Succeeded(),
          TEXT("Could not find Widget3DRoundedBox_Transparent")))
  {
    TransparentMaterial = WidgetMaterialTransparent.Object;
  }

  static MaterialFinder WidgetMaterialTransparentOneSided(
      Materials::Widget3DRoundedBox_Transparent_OneSided);
  if (ensureMsgf(
          WidgetMaterialTransparent.Succeeded(),
          TEXT("Could not find Widget3DRoundedBox_Transparent_OneSided")))
  {
    TransparentMaterialOneSided = WidgetMaterialTransparentOneSided.Object;
  }

  WidgetScale = 0.02;
  DrawSize = FVector2D(1000.0, 1000.0);
  bCreatePokeInteractable = true;
  bCreateRayInteractable = true;
}

void UIsdkInteractableWidgetComponent::Initialize()
{
  auto AttachmentRule = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
  auto PointableRotation = FQuat::MakeFromEuler(FVector(0.0, 0.0, 180.0));

  if (bCreatePokeInteractable)
  {
    if (!IsValid(PointablePlane))
    {
      PointablePlane = Cast<UIsdkPointablePlane>(GetOwner()->AddComponentByClass(
          UIsdkPointablePlane::StaticClass(), true, FTransform::Identity, false));
      PointablePlane->AttachToComponent(this, AttachmentRule);
      PointablePlane->SetRelativeRotation(PointableRotation);
    }

    if (!IsValid(PokeInteractable))
    {
      PokeInteractable = Cast<UIsdkPokeInteractable>(GetOwner()->AddComponentByClass(
          UIsdkPokeInteractable::StaticClass(), true, FTransform::Identity, false));
      PokeInteractable->AttachToComponent(this, AttachmentRule);
    }
  }
  else
  {
    if (IsValid(PointablePlane))
    {
      GetOwner()->RemoveOwnedComponent(PointablePlane);
    }
    if (IsValid(PokeInteractable))
    {
      GetOwner()->RemoveOwnedComponent(PokeInteractable);
    }
  }

  if (bCreateRayInteractable)
  {
    if (!IsValid(RayPointablePlane))
    {
      RayPointablePlane = Cast<UIsdkPointablePlane>(GetOwner()->AddComponentByClass(
          UIsdkPointablePlane::StaticClass(), true, FTransform::Identity, false));
      RayPointablePlane->AttachToComponent(this, AttachmentRule);
      RayPointablePlane->SetRelativeRotation(PointableRotation);
    }

    if (!IsValid(RaySelectPlane))
    {
      RaySelectPlane = Cast<UIsdkPointablePlane>(GetOwner()->AddComponentByClass(
          UIsdkPointablePlane::StaticClass(), true, FTransform::Identity, false));
      RaySelectPlane->AttachToComponent(
          RayPointablePlane, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
      RaySelectPlane->SetRelativeRotation(FQuat::Identity);
    }

    if (!IsValid(RayInteractable))
    {
      RayInteractable = Cast<UIsdkRayInteractable>(GetOwner()->AddComponentByClass(
          UIsdkRayInteractable::StaticClass(), true, FTransform::Identity, false));
      RayInteractable->AttachToComponent(this, AttachmentRule);
    }
  }
  else
  {
    if (IsValid(RaySelectPlane))
    {
      GetOwner()->RemoveOwnedComponent(RaySelectPlane);
    }
    if (IsValid(RayPointablePlane))
    {
      GetOwner()->RemoveOwnedComponent(RayPointablePlane);
    }
    if (IsValid(RayInteractable))
    {
      GetOwner()->RemoveOwnedComponent(RayInteractable);
    }
  }
}

void UIsdkInteractableWidgetComponent::BeginPlay()
{
  Super::BeginPlay();
  Initialize();
  SetWidgetProperties();
  if (!bCreatePokeInteractable && !bCreateRayInteractable)
  {
    return;
  }
  auto WidgetSize = DrawSize * WidgetScale;

  if (bCreatePokeInteractable)
  {
    PointablePlane->SetSize(WidgetSize);
    auto WidgetClippers =
        UIsdkFunctionLibrary::MakeBoundsClippersFromPose(PointablePlane, WidgetSize);
    ClippedPlaneSurface->SetBoundsClippers(WidgetClippers);
    ClippedPlaneSurface->SetPointablePlane(PointablePlane);
    PokeInteractable->SetSurfacePatch(ClippedPlaneSurface);
    PointableWidget->Setup(Widget, PokeInteractable);
  }
  if (bCreateRayInteractable)
  {
    RayPointablePlane->SetSize(WidgetSize / 2.0);
    RayInteractable->SetSurface(RayPointablePlane);
    RaySelectPlane->SetSize(FVector2D(TNumericLimits<float>::Max()));
    RayInteractable->SetSelectSurface(RaySelectPlane);
    PointableWidget->Setup(Widget, RayInteractable);
  }

  WidgetEventAudioPlayer->SetSelectedHoveredAudio(SelectedHoveredAudio);
  WidgetEventAudioPlayer->SetSelectedEmptyAudio(SelectedEmptyAudio);

  PointableWidget->WidgetEventDelegate.AddUniqueDynamic(
      WidgetEventAudioPlayer, &UIsdkWidgetEventAudioPlayer::HandleWidgetStateChanged);
}

void UIsdkInteractableWidgetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  PointableWidget->WidgetEventDelegate.RemoveDynamic(
      WidgetEventAudioPlayer, &UIsdkWidgetEventAudioPlayer::HandleWidgetStateChanged);

  Super::EndPlay(EndPlayReason);
}

void UIsdkInteractableWidgetComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  auto IsWidgetVisible = Widget->IsVisible();
  if (IsValid(PokeInteractable))
  {
    PokeInteractable->SetVisibility(IsWidgetVisible);
  }
  if (IsValid(RayInteractable))
  {
    RayInteractable->SetVisibility(IsWidgetVisible);
  }
}

#if WITH_EDITOR

bool UIsdkInteractableWidgetComponent::CanEditChange(const FProperty* InProperty) const
{
  if (InProperty)
  {
    FString PropertyName = InProperty->GetName();
    if (PropertyName ==
            GET_MEMBER_NAME_STRING_CHECKED(UIsdkInteractableWidgetComponent, CornerRadius) ||
        PropertyName ==
            GET_MEMBER_NAME_STRING_CHECKED(UIsdkInteractableWidgetComponent, BackgroundColor) ||
        PropertyName ==
            GET_MEMBER_NAME_STRING_CHECKED(UIsdkInteractableWidgetComponent, BlendMode) ||
        PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UIsdkInteractableWidgetComponent, bTwoSided))
    {
      return bUseRoundedBoxMaterial;
    }
    if (PropertyName ==
            GET_MEMBER_NAME_STRING_CHECKED(
                UIsdkInteractableWidgetComponent, DefaultPokeInteractableConfigAsset) ||
        PropertyName ==
            GET_MEMBER_NAME_STRING_CHECKED(
                UIsdkInteractableWidgetComponent, DefaultPokeInteractableConfigOffsets))
    {
      return bCreatePokeInteractable;
    }
  }
  return Super::CanEditChange(InProperty);
}

void UIsdkInteractableWidgetComponent::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent)
{
  FProperty* Property = PropertyChangedEvent.MemberProperty;
  if (!Property)
  {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    return;
  }

  auto PropertyName = Property->GetFName();
  if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
  {
    if (PropertyName ==
        GET_MEMBER_NAME_STRING_CHECKED(UIsdkInteractableWidgetComponent, bUseRoundedBoxMaterial))
    {
      if (!bUseRoundedBoxMaterial)
      {
        Widget->EmptyOverrideMaterials();
      }
    }
  }
  if (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(UIsdkInteractableWidgetComponent, DrawSize))
  {
    if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
    {
      EditingSize = true;
    }
    else if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
    {
      EditingSize = false;
    }
  }
  Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif

void UIsdkInteractableWidgetComponent::SetRoundedMaterialProperties()
{
  if (!bUseRoundedBoxMaterial)
  {
    return;
  }
  UMaterialInterface* WidgetMaterial = nullptr;
  const FString MaterialName = TEXT("");
  switch (BlendMode)
  {
    case EIsdkWidgetBlendMode::Masked:
      WidgetMaterial = bTwoSided ? MaskedMaterial : MaskedMaterialOneSided;
      break;
    case EIsdkWidgetBlendMode::Transparent:
      WidgetMaterial = bTwoSided ? TransparentMaterial : TransparentMaterialOneSided;
      break;
  }
  Widget->SetMaterial(0, WidgetMaterial);
  UMaterialInstanceDynamic* DynamicMaterial = Widget->GetMaterialInstance();

  DynamicMaterial->SetVectorParameterValue(
      FName("Dimensions"),
      HasBegunPlay() ? FLinearColor(FVector4(PreviousDrawSize.X, PreviousDrawSize.Y, 0.0, 0.0))
                     : FLinearColor(FVector4(DrawSize.X, DrawSize.Y, 0.0, 0.0)));

  DynamicMaterial->SetVectorParameterValue(
	  FName("Radii"), EditingSize ? FLinearColor(FVector4(0.0f,0.0f,0.0f,0.0f)) : FLinearColor(CornerRadius));
  Widget->SetBackgroundColor(BackgroundColor);
  Widget->UpdateMaterialInstanceParameters();
  DynamicMaterial->SetVectorParameterValue(FName("BackColor"), BackgroundColor);
}

void UIsdkInteractableWidgetComponent::DestroyComponent(bool bPromoteChildren)
{
  if (IsValid(Widget))
  {
    Widget->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(SelectedHoveredAudio))
  {
    SelectedHoveredAudio->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(SelectedEmptyAudio))
  {
    SelectedEmptyAudio->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(ClippedPlaneSurface))
  {
    ClippedPlaneSurface->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(PointableWidget))
  {
    PointableWidget->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(WidgetEventAudioPlayer))
  {
    WidgetEventAudioPlayer->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(PokeInteractable))
  {
    PokeInteractable->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(RayInteractable))
  {
    RayInteractable->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(PointablePlane))
  {
    PointablePlane->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(RayPointablePlane))
  {
    RayPointablePlane->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(RaySelectPlane))
  {
    RaySelectPlane->DestroyComponent(bPromoteChildren);
  }
  Super::DestroyComponent(bPromoteChildren);
}

void UIsdkInteractableWidgetComponent::CreateWidgetInstance()
{
  if (IsValid(WidgetInstance) && IsValid(WidgetClass))
  {
    if (WidgetInstance->GetClass() == WidgetClass)
    {
      return;
    }
  }
  if (!IsValid(WidgetClass))
  {
    return;
  }
  WidgetInstance = nullptr;
  Widget->SetWidget(nullptr);
  WidgetInstance = CreateWidget(GetWorld(), WidgetClass);
}

void UIsdkInteractableWidgetComponent::SetWidgetProperties()
{
  CreateWidgetInstance();
  if (Widget->GetWidget() != WidgetInstance)
  {
    Widget->SetWidget(WidgetInstance);
  }
  Widget->SetDrawSize(DrawSize);

  // The draw size is not updated when you update it like a slider in the
  // details panel, so this is a workaround to still give the user a visual
  // indication of the size of the panel.
  auto DrawScale = DrawSize / PreviousDrawSize;
  Widget->SetWorldScale3D(
      EditingSize ? FVector(1.0, WidgetScale * DrawScale.X, WidgetScale * DrawScale.Y)
                  : FVector(WidgetScale));

  SetRoundedMaterialProperties();
}

void UIsdkInteractableWidgetComponent::OnRegister()
{
  Super::OnRegister();
  if (!EditingSize)
  {
    PreviousDrawSize = DrawSize;
  }
  Initialize();
  SetWidgetProperties();
  if (IsValid(PokeInteractable))
  {
    PokeInteractable->SetConfigAsset(DefaultPokeInteractableConfigAsset);
    PokeInteractable->SetConfigOffsets(DefaultPokeInteractableConfigOffsets);
  }
}
