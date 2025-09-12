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

#include "IsdkDataSourcesOpenXRSubsystem.h"

#include "DataSources/IsdkFromOpenXRControllerDataSource.h"
#include "DataSources/IsdkFromOpenXRHmdDataSource.h"
#include "Modules/ModuleManager.h"
#include "IsdkDataSourcesOpenXRLog.h"
#include "Engine/Engine.h"
#include "Utilities/IsdkXRUtils.h"

bool UIsdkDataSourcesOpenXRSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
  const auto World = Cast<UWorld>(Outer);
  if (!World)
  {
    return false;
  }

  // Disable this subsystem for cases like automated testing, cooking, etc
  if (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::Inactive ||
      GIsAutomationTesting)
  {
    return false;
  }

  return true;
}

FIsdkTrackingDataSources
UIsdkDataSourcesOpenXRSubsystem::GetOrCreateHandDataSourceComponent_Implementation(
    UMotionControllerComponent* SourceMotionController,
    EIsdkHandedness Handedness)
{
  check(SourceMotionController);
  const auto OwnerActor = SourceMotionController->GetOwner();
  if (!OwnerActor)
  {
    ensureMsgf(false, TEXT("Failed to find owner of AttachToMotionController"));
    return FIsdkTrackingDataSources();
  }

  UIsdkFromOpenXRHandDataSource* DataSourceComponent = nullptr;

  // First, see if we can find an existing data source component with matching handedness
  TArray<UActorComponent*> Components;
  OwnerActor->GetComponents(UIsdkFromOpenXRHandDataSource::StaticClass(), Components);
  for (const auto Component : Components)
  {
    const auto CastComponent = Cast<UIsdkFromOpenXRHandDataSource>(Component);
    if (!IsValid(CastComponent))
    {
      continue;
    }

    if (IIsdkIHandJoints::Execute_GetHandedness(CastComponent) == Handedness)
    {
      DataSourceComponent = CastComponent;
      break;
    }
  }

  // If we couldn't find a matching data source, create a new one
  if (!IsValid(DataSourceComponent))
  {
    DataSourceComponent = UIsdkFromOpenXRHandDataSource::MakeOpenXRHandDataSource(
        OwnerActor, SourceMotionController, Handedness);
  }

  CurrentControllerHandBehavior = EControllerHandBehavior::BothAnimated;

  return {DataSourceComponent, DataSourceComponent, DataSourceComponent, DataSourceComponent};
}

FIsdkTrackingDataSources
UIsdkDataSourcesOpenXRSubsystem::GetOrCreateControllerDataSourceComponent_Implementation(
    UMotionControllerComponent* SourceMotionController,
    EIsdkHandedness Handedness)
{
  check(SourceMotionController);
  const auto OwnerActor = SourceMotionController->GetOwner();
  if (!OwnerActor)
  {
    ensureMsgf(false, TEXT("Failed to find owner of AttachToMotionController"));
    return FIsdkTrackingDataSources();
  }

  UIsdkFromOpenXRControllerDataSource* DataSourceComponent = nullptr;

  // First, see if we can find an existing data source component with matching handedness
  TArray<UActorComponent*> Components;
  OwnerActor->GetComponents(UIsdkFromOpenXRControllerDataSource::StaticClass(), Components);
  for (const auto Component : Components)
  {
    const auto CastComponent = Cast<UIsdkFromOpenXRControllerDataSource>(Component);
    if (!IsValid(CastComponent))
    {
      continue;
    }

    if (IIsdkIHandJoints::Execute_GetHandedness(CastComponent) == Handedness)
    {
      DataSourceComponent = CastComponent;
      break;
    }
  }

  // If we couldn't find a matching data source, create a new one
  if (!IsValid(DataSourceComponent))
  {
    DataSourceComponent = UIsdkFromOpenXRControllerDataSource::MakeOpenXrControllerDataSource(
        OwnerActor, SourceMotionController, Handedness);
  }

  return {DataSourceComponent, DataSourceComponent, DataSourceComponent, DataSourceComponent};
}

TScriptInterface<IIsdkIHmdDataSource>
UIsdkDataSourcesOpenXRSubsystem::GetOrCreateHmdDataSourceComponent_Implementation(
    AActor* TrackingSpaceRoot)
{
  check(TrackingSpaceRoot);

  auto Component = TrackingSpaceRoot->FindComponentByClass<UIsdkFromOpenXRHmdDataSource>();
  if (!Component)
  {
    Component = NewObject<UIsdkFromOpenXRHmdDataSource>(
        TrackingSpaceRoot, UIsdkFromOpenXRHmdDataSource::StaticClass());
    Component->RegisterComponent();
  }

  return Component;
}

bool UIsdkDataSourcesOpenXRSubsystem::IsEnabled_Implementation()
{
  return IsdkXRUtils::IsOpenXrTrackingSystem();
}

EControllerHandBehavior UIsdkDataSourcesOpenXRSubsystem::GetControllerHandBehavior()
{
  return CurrentControllerHandBehavior;
}

void UIsdkDataSourcesOpenXRSubsystem::SetControllerHandBehavior(
    EControllerHandBehavior ControllerHandBehavior)
{
  // OpenXR does not support oculus-provided procedural hand animations,
  // so we fall back to the animated counterparts here.
  bool bResultModified = false;
  const EControllerHandBehavior OldBehavior = ControllerHandBehavior;
  if (ControllerHandBehavior == EControllerHandBehavior::BothProcedural)
  {
    ControllerHandBehavior = EControllerHandBehavior::BothAnimated;
    bResultModified = true;
  }
  else if (ControllerHandBehavior == EControllerHandBehavior::HandsOnlyProcedural)
  {
    ControllerHandBehavior = EControllerHandBehavior::HandsOnlyAnimated;
    bResultModified = true;
  }

  // Warn the developer that something potentially unexpected happened here.
  if (bResultModified)
  {
    FString OldString = StaticEnum<EControllerHandBehavior>()->GetValueAsString(OldBehavior);
    FString NewString =
        StaticEnum<EControllerHandBehavior>()->GetValueAsString(ControllerHandBehavior);
    UE_LOG(
        LogIsdkDataSourcesOpenXR,
        Warning,
        TEXT("ControllerHandBehavior \"%s\" not supported by OpenXR.  Changing to \"%s\""),
        *OldString,
        *NewString);
  }

  const auto PreviousBehavior = CurrentControllerHandBehavior;
  CurrentControllerHandBehavior = ControllerHandBehavior;

  if (PreviousBehavior != CurrentControllerHandBehavior)
  {
    ControllerHandsBehaviorChangedDelegate.Broadcast(
        this, PreviousBehavior, CurrentControllerHandBehavior);
  }
}

FIsdkTrackedDataSubsystem_ControllerHandsBehaviorChangedDelegate*
UIsdkDataSourcesOpenXRSubsystem::GetControllerHandBehaviorChangedDelegate()
{
  return &ControllerHandsBehaviorChangedDelegate;
}
