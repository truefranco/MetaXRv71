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

#include "IsdkRoundedButtonComponent.h"
#include "IsdkContentAssetPaths.h"
#include "RoundedBoxVisuals/IsdkRoundedBoxFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Curves/CurveFloat.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "IsdkFunctionLibrary.h"

// Sets default values for this component's properties
UIsdkRoundedButtonComponent::UIsdkRoundedButtonComponent()
{
  PrimaryComponentTick.bCanEverTick = true;
  bAutoActivate = true;
  bTwoSidedMaterial = true;

  // Interactable Things
  PokeButtonVisual = CreateDefaultSubobject<UIsdkPokeButtonVisual>(TEXT("Poke Button Visual"));
  PokeButtonVisual->SetupAttachment(this);

  PokeInteractable = CreateDefaultSubobject<UIsdkPokeInteractable>(TEXT("Poke Interactable"));
  PokeInteractable->SetupAttachment(this);
  PokeInteractable->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0, 0.0, 180.0)));

  PointablePlane = CreateDefaultSubobject<UIsdkPointablePlane>(TEXT("Pointable Plane"));
  PointablePlane->SetupAttachment(this);
  PointablePlane->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0, 0.0, 180.0)));

  ClippedPlaneSurface =
      CreateDefaultSubobject<UIsdkClippedPlaneSurface>(TEXT("Clipped Plane Surface"));
  //----------------------------------------

  InteractableColorVisual =
      CreateDefaultSubobject<UIsdkInteractableColorVisual>(TEXT("Interactable Color Visual"));

  ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button Mesh Visual"));
  ButtonMesh->SetupAttachment(PokeButtonVisual);
  ButtonMesh->bAlwaysCreatePhysicsState = false;
  ButtonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  ButtonMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

  BackplaneMesh =
      CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button Backplane Mesh Visual"));
  BackplaneMesh->SetupAttachment(this);
  ButtonMesh->bAlwaysCreatePhysicsState = false;
  BackplaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  BackplaneMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

  InteractionButtonPress = CreateDefaultSubobject<UAudioComponent>(TEXT("Button Press Audio"));
  InteractionButtonPress->SetupAttachment(this);
  InteractionButtonPress->SetAutoActivate(false);
  InteractionButtonRelease = CreateDefaultSubobject<UAudioComponent>(TEXT("Button Release Audio"));
  InteractionButtonRelease->SetupAttachment(this);
  InteractionButtonRelease->SetAutoActivate(false);
  PointerEventAudioPlayer =
      CreateDefaultSubobject<UIsdkPointerEventAudioPlayer>(TEXT("Pointer Event Audio Player"));

  ButtonLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Button Label"));
  ButtonLabel->SetupAttachment(PokeButtonVisual);
  bInitialized = false;

  SetDefaultValues();
}

void UIsdkRoundedButtonComponent::DestroyComponent(bool bPromoteChildren)
{
  if (IsValid(PokeButtonVisual))
  {
    PokeButtonVisual->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(PokeInteractable))
  {
    PokeInteractable->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(PointablePlane))
  {
    PointablePlane->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(ClippedPlaneSurface))
  {
    ClippedPlaneSurface->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(InteractableColorVisual))
  {
    InteractableColorVisual->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(ButtonMesh))
  {
    ButtonMesh->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(BackplaneMesh))
  {
    BackplaneMesh->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(InteractionButtonPress))
  {
    InteractionButtonPress->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(InteractionButtonRelease))
  {
    InteractionButtonRelease->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(PointerEventAudioPlayer))
  {
    PointerEventAudioPlayer->DestroyComponent(bPromoteChildren);
  }
  if (IsValid(ButtonLabel))
  {
    ButtonLabel->DestroyComponent(bPromoteChildren);
  }
  Super::DestroyComponent(bPromoteChildren);
}

#if WITH_EDITOR
void UIsdkRoundedButtonComponent::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent)
{
  const FName PropertyName =
      PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : FName();
  // Changing the bTwoSidedMaterial requires the material instance to be updated
  if (PropertyName ==
      GET_MEMBER_NAME_STRING_CHECKED(UIsdkRoundedButtonComponent, bTwoSidedMaterial))
  {
    bInitialized = false;
    ReregisterComponent();
  }
  Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UIsdkRoundedButtonComponent::SetDefaultValues()
{
  // Default curve transition values
  static ConstructorHelpers::FObjectFinder<UCurveFloat> TransitionFloatCurve(
      IsdkContentAssetPaths::Models::RoundedButton::DefaultTransitionCurve);

  if (ensureMsgf(TransitionFloatCurve.Succeeded(), TEXT("Could not get default Transition curve")))
  {
    auto VisualNormalColorState = FIsdkColorState();
    VisualNormalColorState.Color = FLinearColor(1.0, 1.0, 1.0, 0.15);
    VisualNormalColorState.TransitionCurve = TransitionFloatCurve.Object;
    VisualNormalColorState.TransitionDuration = 0.1;

    auto VisualHoverColorState = FIsdkColorState();
    VisualHoverColorState.Color = FLinearColor(1.0, 1.0, 1.0, 0.3);
    VisualHoverColorState.TransitionCurve = TransitionFloatCurve.Object;
    VisualHoverColorState.TransitionDuration = 0.1;

    auto VisualSelectColorState = FIsdkColorState();
    VisualSelectColorState.Color = FLinearColor(1.0, 1.0, 1.0, 0.5);
    VisualSelectColorState.TransitionCurve = TransitionFloatCurve.Object;
    VisualSelectColorState.TransitionDuration = 0.05;

    auto VisualDisabledColorState = FIsdkColorState();
    VisualDisabledColorState.Color = FLinearColor(0.5, 0.5, 0.5, 1.0);
    VisualDisabledColorState.TransitionCurve = TransitionFloatCurve.Object;
    VisualDisabledColorState.TransitionDuration = 0.1;

    InteractableColorVisual->SetColorStates(
        VisualNormalColorState,
        VisualHoverColorState,
        VisualSelectColorState,
        VisualDisabledColorState);
  }

  CornerRadius = 0.5;
  VisualOffset = 1.5;
  ButtonSize = FVector2D(10.0, 5.0);
  LabelSize = 2.5;
  LabelText = FString("Button");
  LabelColor = FLinearColor(1.0, 1.0, 1.0, 1.0);
}

void UIsdkRoundedButtonComponent::SetCornerRadius(float Radius)
{
  CornerRadius = Radius;
  UIsdkRoundedBoxFunctionLibrary::SetRoundedBoxMaterialCornerRadius(
	  Cast<UMaterialInstanceDynamic>(ButtonMesh->GetMaterial(0)), FVector4(1.0f,1.0f,1.0f,1.0f) * CornerRadius);

  if (!CreateBackplane)
    return;

  UIsdkRoundedBoxFunctionLibrary::SetRoundedBoxMaterialCornerRadius(
      Cast<UMaterialInstanceDynamic>(BackplaneMesh->GetMaterial(0)),
      FVector4(1.0f, 1.0f, 1.0f, 1.0f) * CornerRadius);
}

void UIsdkRoundedButtonComponent::SetBackplaneFillColor(FLinearColor Color)
{
  if (!CreateBackplane)
    return;
  BackplaneFillColor = Color;
  UIsdkRoundedBoxFunctionLibrary::SetRoundedBoxMaterialColor(
      Cast<UMaterialInstanceDynamic>(BackplaneMesh->GetMaterial(0)), BackplaneFillColor);
}

void UIsdkRoundedButtonComponent::SetCreateBackplane(bool Value)
{
  CreateBackplane = Value;
  if (CreateBackplane)
  {
    SetBackplaneFillColor(BackplaneFillColor);
    SetCornerRadius(CornerRadius);
  }
}

void UIsdkRoundedButtonComponent::SetupTextComponent()
{
  static UMaterial* TextMaterial = Cast<UMaterial>(StaticLoadObject(
      UMaterial::StaticClass(), nullptr, IsdkContentAssetPaths::Materials::TextUnlit));

  if (ensureMsgf(TextMaterial != nullptr, TEXT("TextUnlit Material could not be loaded.")))
  {
    ButtonLabel->SetMaterial(0, TextMaterial);
  }
}

void UIsdkRoundedButtonComponent::BeginPlay()
{
  Super::BeginPlay();
  Initialize();

  // Interactable Configuration
  PointablePlane->SetSize(ButtonSize);
  auto BoundsClippers =
      UIsdkFunctionLibrary::MakeBoundsClippersFromPose(PointablePlane, ButtonSize);
  ClippedPlaneSurface->SetBoundsClippers(BoundsClippers);
  ClippedPlaneSurface->SetPointablePlane(PointablePlane);
  PokeInteractable->SetSurfacePatch(ClippedPlaneSurface);
  PokeButtonVisual->SetPokeInteractable(PokeInteractable);

  FIsdkPokeInteractableConfigOffsets Offsets{};
  Offsets.NormalOffset = VisualOffset;
  PokeInteractable->SetConfigOffsets(Offsets);
  //-------------------------

  InteractableColorVisual->SetInteractable(PokeInteractable);
  PointerEventAudioPlayer->SetPointable(PokeInteractable);
  PointerEventAudioPlayer->SetSelectAudio(InteractionButtonPress);
  PointerEventAudioPlayer->SetUnselectAudio(InteractionButtonRelease);
}

void UIsdkRoundedButtonComponent::TickComponent(
    float DeltaTime,
    enum ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  // If the button mesh and the backplane are invisible then the interactable should not be
  // interactive.
  auto InteractableVisibility = ButtonMesh->IsVisible() || BackplaneMesh->IsVisible();
  if (PokeInteractable->IsVisible() != InteractableVisibility)
  {
    PokeInteractable->SetVisibility(InteractableVisibility);
  }
}

void UIsdkRoundedButtonComponent::Initialize()
{
  if (!bInitialized)
  {
    bInitialized = true;
    SetupTextComponent();

    UMaterialInstanceDynamic* ButtonVisualMaterial;
    UIsdkRoundedBoxFunctionLibrary::SetupRoundedBox(
        GetOwner(),
        FIsdkRoundedBoxProperties(),
        PokeButtonVisual,
        ButtonMesh,
        ButtonVisualMaterial,
        bTwoSidedMaterial);

    UMaterialInstanceDynamic* BackplaneMaterial;
    UIsdkRoundedBoxFunctionLibrary::SetupRoundedBox(
        GetOwner(),
        FIsdkRoundedBoxProperties(),
        this,
        BackplaneMesh,
        BackplaneMaterial,
        bTwoSidedMaterial);

    InteractableColorVisual->DynamicMaterial = ButtonVisualMaterial;
    InteractableColorVisual->ParameterName = "Color";

    ButtonLabel->AttachToComponent(
        PokeButtonVisual, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    ButtonLabel->SetRelativeLocation(FVector(0.01, 0.0, 0.0));
    ButtonLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    ButtonLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
  }
  UpdateProperties();
}

void UIsdkRoundedButtonComponent::UpdateProperties()
{
  ButtonLabel->SetWorldSize(LabelSize);
  ButtonLabel->SetText(FText::FromString(LabelText));
  ButtonLabel->SetTextRenderColor(LabelColor.ToFColor(false));
  ButtonLabel->SetVisibility(LabelSize > 0.0 && !LabelText.IsEmpty());

  auto ButtonVisualProps = FIsdkRoundedBoxProperties();
  ButtonVisualProps.Size = ButtonSize;
  ButtonVisualProps.CornerRadius = FVector4(1.0f, 1.0f, 1.0f, 1.0f) * CornerRadius;
  ButtonVisualProps.FillColor = InteractableColorVisual->GetNormalColorState().Color;
  ButtonVisualProps.OutlineColor = InteractableColorVisual->GetNormalColorState().Color;
  ButtonVisualProps.OutlineColor.A = 0.0;

  UIsdkRoundedBoxFunctionLibrary::SetRoundedBoxMaterialParameters(
      Cast<UMaterialInstanceDynamic>(ButtonMesh->GetMaterial(0)), ButtonVisualProps);
  UIsdkRoundedBoxFunctionLibrary::SetMeshComponentScale(ButtonMesh, ButtonSize, 0.0f);

  ButtonVisualProps.FillColor = BackplaneFillColor;
  ButtonVisualProps.OutlineColor = BackplaneFillColor;
  ButtonVisualProps.OutlineColor.A = 0.0;

  BackplaneMesh->SetVisibility(CreateBackplane);
  if (CreateBackplane)
  {
    UIsdkRoundedBoxFunctionLibrary::SetRoundedBoxMaterialParameters(
        Cast<UMaterialInstanceDynamic>(BackplaneMesh->GetMaterial(0)), ButtonVisualProps);
    UIsdkRoundedBoxFunctionLibrary::SetMeshComponentScale(BackplaneMesh, ButtonSize, 0.0f);
  }

  if (IsValid(PokeButtonVisual))
  {
    PokeButtonVisual->SetRelativeLocation(FVector(VisualOffset, 0.0, 0.0));
  }

  auto rotation = FQuat::MakeFromEuler(FVector(-90.0, 0.0, 90.0));
  ButtonMesh->SetRelativeRotation(rotation);
  BackplaneMesh->SetRelativeRotation(rotation);
  BackplaneMesh->SetRelativeLocation(FVector(-0.01, 0.0, 0.0));
}

void UIsdkRoundedButtonComponent::OnRegister()
{
  Super::OnRegister();
  Initialize();
}
