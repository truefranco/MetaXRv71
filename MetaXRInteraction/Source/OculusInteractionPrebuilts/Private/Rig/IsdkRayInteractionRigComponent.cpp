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

#include "Rig/IsdkRayInteractionRigComponent.h"
#include "OculusInteractionPrebuiltsLog.h"
#include "Interaction/IsdkRayInteractor.h"
#include "Core/IsdkConditionalGroupAll.h"
#include "InteractorVisuals/IsdkRayVisualFunctionLibrary.h"
#include "Rig/IsdkRigComponent.h"
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"

UIsdkRayInteractionRigComponent::UIsdkRayInteractionRigComponent()
{
  PrimaryComponentTick.bCanEverTick = true;
  bWantsInitializeComponent = true;
  bAutoActivate = true;

  RayInteractor = CreateDefaultSubobject<UIsdkRayInteractor>(TEXT("RayInteractor"));
  EnabledConditionalGroup =
      CreateDefaultSubobject<UIsdkConditionalGroupAll>(TEXT("RayEnabledConditionalAll"));
  IsActiveConditional =
      CreateDefaultSubobject<UIsdkConditionalComponentIsActive>(TEXT("RayIsActiveConditional"));

  // Update default ray length to "infinity"
  FIsdkRayInteractor_Config RayConfig = RayInteractor->GetConfig();
  RayConfig.MaxRayLength = 0;
  RayInteractor->SetConfig(RayConfig);

  RayVisualClass = UIsdkRayVisualComponent::StaticClass();
}

void UIsdkRayInteractionRigComponent::InitializeComponent()
{
  Super::InitializeComponent();

  IsActiveConditional->SetComponent(this);
  EnabledConditionalGroup->AddConditional(IsActiveConditional);
  EnabledConditionalGroup->ResolvedValueChanged.AddWeakLambda(
      this, [this](bool bIsEnabled) { HandleIsEnabledConditionalChanged(bIsEnabled); });

  RigComponent = Cast<UIsdkRigComponent>(GetOuter());
  ensureMsgf(
      IsValid(RigComponent),
      TEXT("UIsdkRayInteractionRigComponent must be a child of an UIsdkRigComponent"));
  AddTickPrerequisiteComponent(RigComponent);
}

void UIsdkRayInteractionRigComponent::BeginPlay()
{
  Super::BeginPlay();

  RigComponent->OnPinchSelectDelegate.AddUObject(
      this, &UIsdkRayInteractionRigComponent::HandleRigPinchSelect);
  RigComponent->OnPinchUnselectDelegate.AddUObject(
      this, &UIsdkRayInteractionRigComponent::HandleRigPinchUnselect);
}

void UIsdkRayInteractionRigComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  RayInteractor->SetSelectStrength(RigComponent->GetPinchStrength());
}

void UIsdkRayInteractionRigComponent::BindDataSources(
    const FIsdkTrackingDataSources& DataSources,
    TScriptInterface<IIsdkIHmdDataSource>& HmdDataSourceIn,
    USceneComponent* AttachToComponent,
    FName AttachToComponentSocket)
{
  check(AttachToComponent);

  RayInteractor->AttachToComponent(
      AttachToComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachToComponentSocket);
  RayInteractor->SetHandPointerPose(DataSources.HandPointerPose);

  if (IsValid(HmdDataSourceIn.GetObject()))
  {
    HmdDataSource = HmdDataSourceIn;
  }

  CreateRayVisual(AttachToComponent->GetOwner());
  if (IsValid(HmdDataSource.GetObject()) && IsValid(RayVisual))
  {
    RayVisual->HmdDataSource = HmdDataSource;
  }

  // Watch for the data sources becoming enabled/disabled.
  const auto IsConnectedConditional =
      IIsdkIRootPose::Execute_GetRootPoseConnectedConditional(DataSources.HandRootPose.GetObject());
  if (ensure(IsValid(IsConnectedConditional)))
  {
    EnabledConditionalGroup->AddConditional(IsConnectedConditional);
  }
}

void UIsdkRayInteractionRigComponent::UpdateRayVisual()
{
  UIsdkRayVisualFunctionLibrary::UpdateRayVisualWithRayInteractor(
      RayVisual, RayInteractor, RigComponent->GetPinchStrength());
}

void UIsdkRayInteractionRigComponent::CreateRayVisual(AActor* AttachParent)
{
  if (!IsValid(RayVisualClass) || !IsValid(AttachParent))
  {
    return;
  }

  if (IsValid(RayVisual))
  {
    RayVisual->DestroyComponent();
  }

  RayVisual = Cast<UIsdkRayVisualComponent>(
      GetOwner()->AddComponentByClass(RayVisualClass.Get(), true, FTransform::Identity, false));

  RayVisual->AttachToComponent(
      AttachParent->GetDefaultAttachComponent(),
      FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));

  RayInteractor->GetUpdatedEventDelegate().AddUniqueDynamic(
      this, &UIsdkRayInteractionRigComponent::UpdateRayVisual);
}

void UIsdkRayInteractionRigComponent::HandleIsEnabledConditionalChanged(bool bIsEnabled)
{
  UE_VLOG(
      GetOwner(),
      LogOculusInteractionPrebuilts,
      Log,
      TEXT("RayInteractionRigComponent for Rig \"%s\" Handled enabled changed: %d"),
      *GetNameSafe(RigComponent),
      bIsEnabled ? 1 : 0);

  if (!bIsEnabled)
  {
    RayInteractor->Deactivate();
    if (IsValid(RayVisual))
    {
      RayVisual->SetVisibility(false);
    }
  }
  else
  {
    RayInteractor->Activate();
  }
}

void UIsdkRayInteractionRigComponent::HandleRigPinchSelect(UIsdkRigComponent* IsdkRigComponent)
{
  if (!RayInteractor)
  {
    return;
  }

  RayInteractor->Select();
}

void UIsdkRayInteractionRigComponent::HandleRigPinchUnselect(UIsdkRigComponent* IsdkRigComponent)
{
  if (!RayInteractor)
  {
    return;
  }

  RayInteractor->Unselect();
}
