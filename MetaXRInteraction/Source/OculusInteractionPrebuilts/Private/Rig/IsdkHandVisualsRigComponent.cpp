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

#include "Rig/IsdkHandVisualsRigComponent.h"

#include "IsdkContentAssetPaths.h"
#include "IsdkHandMeshComponent.h"
#include "DataSources/IsdkSyntheticHand.h"

#include "Components/SceneComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"

UIsdkHandVisualsRigComponent::UIsdkHandVisualsRigComponent()
{
  PrimaryComponentTick.bCanEverTick = false;
  bWantsInitializeComponent = true;

  SyntheticHand = CreateDefaultSubobject<UIsdkSyntheticHand>(TEXT("SyntheticHand"));
  TrackedHandVisual = CreateDefaultSubobject<UIsdkHandMeshComponent>(TEXT("TrackedHandVisual"));
  SyntheticHandVisual = CreateDefaultSubobject<UIsdkHandMeshComponent>(TEXT("SyntheticHandVisual"));

  // Initialize Bones to default values. If a custom hand mesh is used, these can be overridden by
  // a user in the details panel of the actor instance in the editor.
  TrackedHandVisual->SetMappedBoneNamesAsDefault();
  SyntheticHandVisual->SetMappedBoneNamesAsDefault();

  // By default, hide the tracked hand visual so that only synthetic hand is displayed
  TrackedHandVisual->SetVisibility(false);
}

void UIsdkHandVisualsRigComponent::InitializeComponent()
{
  Super::InitializeComponent();

  // Set data sources to default state, which will de-activate the components.
  OnDataSourcesCreated();
}

USceneComponent* UIsdkHandVisualsRigComponent::GetTrackedVisual() const
{
  return TrackedHandVisual;
}

USceneComponent* UIsdkHandVisualsRigComponent::GetSyntheticVisual() const
{
  return SyntheticHandVisual;
}

void UIsdkHandVisualsRigComponent::GetInteractorSocket(
    USceneComponent*& OutSocketComponent,
    FName& OutSocketName,
    EIsdkHandBones HandBone) const
{
  OutSocketComponent = GetTrackedVisual();
  OutSocketName = NAME_None;
  if (HandBone != EIsdkHandBones::Invalid && TrackedHandVisual)
  {
    OutSocketName = TrackedHandVisual->MappedBoneNames[static_cast<size_t>(HandBone)];
  }
}

FTransform UIsdkHandVisualsRigComponent::GetCurrentSyntheticTransform()
{
  return IIsdkIRootPose::Execute_GetRootPose(SyntheticHand);
}

void UIsdkHandVisualsRigComponent::OnDataSourcesCreated()
{
  Super::OnDataSourcesCreated();

  // Wire up data sources to poke limiting
  SyntheticHand->SetInputDataSource(LocalDataSources.DataSourceComponent);

  // Wire up data sources to hand visuals
  TrackedHandVisual->SetJointsDataSource(LocalDataSources.DataSourceComponent);
  TrackedHandVisual->SetRootPoseDataSource(LocalDataSources.DataSourceComponent);

  SyntheticHandVisual->SetJointsDataSource(SyntheticHand);
  SyntheticHandVisual->SetRootPoseDataSource(SyntheticHand);

  const bool bHasValidDataSource = IsValid(LocalDataSources.DataSourceComponent);
  SyntheticHand->bUpdateInTick = bHasValidDataSource;
}

UIsdkHandVisualsRigComponentLeft::UIsdkHandVisualsRigComponentLeft()
{
  Handedness = EIsdkHandedness::Left;

  static ConstructorHelpers::FObjectFinder<USkeletalMesh> LeftHandMeshFinder =
      IsdkContentAssetPaths::Models::Hand::OpenXRLeftHand;
  TrackedHandVisual->SetSkeletalMesh(LeftHandMeshFinder.Object);
  SyntheticHandVisual->SetSkeletalMesh(LeftHandMeshFinder.Object);
}

UIsdkHandVisualsRigComponentRight::UIsdkHandVisualsRigComponentRight()
{
  Handedness = EIsdkHandedness::Right;

  static ConstructorHelpers::FObjectFinder<USkeletalMesh> RightHandMeshFinder =
      IsdkContentAssetPaths::Models::Hand::OpenXRRightHand;
  TrackedHandVisual->SetSkeletalMesh(RightHandMeshFinder.Object);
  SyntheticHandVisual->SetSkeletalMesh(RightHandMeshFinder.Object);
}
