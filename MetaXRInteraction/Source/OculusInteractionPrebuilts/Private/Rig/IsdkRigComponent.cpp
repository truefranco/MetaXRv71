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

#include "Rig/IsdkRigComponent.h"

#include "UObject/UObjectIterator.h"
#include "Subsystem/IsdkWidgetSubsystem.h"
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "Interaction/IsdkRayInteractor.h"
#include "Rig/IsdkRigModifier.h"
#include "Engine/SkeletalMesh.h"
#include "CollisionShape.h"
#include "EnhancedInputComponent.h"
#include "IsdkHandMeshComponent.h"
#include "IsdkRuntimeSettings.h"
#include "OculusInteractionPrebuiltsLog.h"
#include "Interaction/IsdkPokeInteractor.h"
#include "Interaction/IsdkGrabberComponent.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "Rig/IsdkControllerVisualsRigComponent.h"
#include "Rig/IsdkGrabInteractionRigComponent.h"
#include "Rig/IsdkInputActionsRigComponent.h"
#include "Rig/IsdkInteractionGroupRigComponent.h"
#include "Rig/IsdkPokeInteractionRigComponent.h"
#include "Rig/IsdkRayInteractionRigComponent.h"

const FVector UIsdkRigComponent::ControllerPalmColliderOffset = FVector(-3.f, 0.f, -2.f);
const FVector UIsdkRigComponent::HandPalmColliderOffset = FVector(0.f, 3.f, -8.f);

// Sets default values
UIsdkRigComponent::UIsdkRigComponent()
{
  PrimaryComponentTick.bCanEverTick = false;

  RayInteractorSocket = EIsdkHandBones::HandWristRoot;
  PokeInteractorSocket = EIsdkHandBones::HandIndexTip;
  GrabberSocket = EIsdkHandBones::HandWristRoot;

  InputActions = CreateDefaultSubobject<UIsdkInputActionsRigComponent>(TEXT("InputActions"));
  RayInteraction = CreateDefaultSubobject<UIsdkRayInteractionRigComponent>(TEXT("RayInteraction"));
  PokeInteraction =
      CreateDefaultSubobject<UIsdkPokeInteractionRigComponent>(TEXT("PokeInteraction"));
  GrabInteraction =
      CreateDefaultSubobject<UIsdkGrabInteractionRigComponent>(TEXT("GrabInteraction"));
  InteractionGroup =
      CreateDefaultSubobject<UIsdkInteractionGroupRigComponent>(TEXT("InteractionGroup"));
}

void UIsdkRigComponent::BeginPlay()
{
  Super::BeginPlay();

  // Create data sources
  GetVisuals()->DataSourcesCreated.AddUObject(
      this, &UIsdkRigComponent::HandleVisualsDataSourcesCreatedEvent);

  InitializeHmdDataSource();

  // Register interactors to the widget system
  RegisterInteractorWidgetIndices();

  // Setup interaction groups
  CreateInteractionGroupConditionals();

  // Spawn & Initialize Rig Modifiers
  for (TSubclassOf<UIsdkRigModifier> RigModifierClass : RigModifiersToSpawn)
  {
    if (!IsValid(RigModifierClass))
    {
      continue;
    }
    UIsdkRigModifier* NewRigModifier = NewObject<UIsdkRigModifier>(this, RigModifierClass);
    NewRigModifier->InitializeRigModifier(this);
    ActiveRigModifiers.Add(NewRigModifier);
  }

  // Subscribe to input action events
  if (bAutoBindInputActions)
  {
    const auto FirstLocalPlayer = UGameplayStatics::GetPlayerController(this, 0);

    AActor* OwnerActor = GetOwner();
    OwnerActor->EnableInput(FirstLocalPlayer);
    const auto EnhancedInputComponent = OwnerActor->FindComponentByClass<UEnhancedInputComponent>();
    if (EnhancedInputComponent)
    {
      BindInputActions(EnhancedInputComponent);
    }
    else
    {
      UE_LOG(
          LogOculusInteractionPrebuilts,
          Error,
          TEXT("No EnhancedInputComponent found on Actor with a UIsdkRigComponent"))
    }

    for (UIsdkRigModifier* ThisRigModifier : ActiveRigModifiers)
    {
      InputActionBindingHandles.Append(ThisRigModifier->BindInputActions(EnhancedInputComponent));
    }
  }
}

void UIsdkRigComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  UnregisterInteractorWidgetIndices();

  // Shut Down Active Rig Modifiers
  for (UIsdkRigModifier* ThisRigModifier : ActiveRigModifiers)
  {
    ThisRigModifier->ShutdownRigModifier(this);
  }
  ActiveRigModifiers.Empty();

  // Remove bindings that were created.
  AActor* OwnerActor = GetOwner();
  const auto EnhancedInputComponent = Cast<UEnhancedInputComponent>(OwnerActor->InputComponent);
  if (IsValid(EnhancedInputComponent))
  {
    for (auto InputActionBindingHandle : InputActionBindingHandles)
    {
      EnhancedInputComponent->RemoveBindingByHandle(InputActionBindingHandle);
    }
    InputActionBindingHandles.Empty();
  }
}

bool UIsdkRigComponent::HasAnySockets() const
{
  return true;
}

void UIsdkRigComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
  Super::QuerySupportedSockets(OutSockets);
  OutSockets.Add(
      FComponentSocketDescription(NAME_isdk_PointerPoseSocket, EComponentSocketType::Type::Socket));
}

FTransform UIsdkRigComponent::GetSocketTransform(
    FName InSocketName,
    ERelativeTransformSpace TransformSpace) const
{
  FTransform SocketTransformWorld;
  bool bIsSocketTransformValid = false;

  const FIsdkTrackingDataSources& DataSources = GetVisuals()->GetDataSources();
  if (InSocketName == NAME_isdk_PointerPoseSocket && DataSources.bIsInitialized)
  {
    IIsdkIHandPointerPose::Execute_GetPointerPose(
        GetVisuals()->GetDataSources().HandPointerPose.GetObject(),
        SocketTransformWorld,
        bIsSocketTransformValid);
  }

  if (bIsSocketTransformValid)
  {
    switch (TransformSpace)
    {
      case RTS_World:
      {
        return SocketTransformWorld;
      }
      case RTS_Actor:
      {
        if (const AActor* Actor = GetOwner())
        {
          return SocketTransformWorld.GetRelativeTransform(Actor->GetTransform());
        }
        break;
      }
      case RTS_Component:
      {
        return SocketTransformWorld.GetRelativeTransform(GetComponentTransform());
      }
      default:
        // Unsupported transform space for hand pointer pose.
        break;
    }
  }

  // Fallback to base behavior if we couldn't derive a valid socket position.
  return Super::GetSocketTransform(InSocketName, TransformSpace);
}

bool UIsdkRigComponent::DoesSocketExist(FName InSocketName) const
{
  return InSocketName == NAME_isdk_PointerPoseSocket || Super::DoesSocketExist(InSocketName);
}

FName UIsdkRigComponent::GetPointerPoseSocketName()
{
  return NAME_isdk_PointerPoseSocket;
}

UIsdkPokeInteractor* UIsdkRigComponent::GetPokeInteractor() const
{
  return PokeInteraction->PokeInteractor;
}

UIsdkRayInteractor* UIsdkRigComponent::GetRayInteractor() const
{
  return RayInteraction->RayInteractor;
}

UIsdkPokeLimiterVisual* UIsdkRigComponent::GetPokeLimiterVisual() const
{
  return PokeInteraction->PokeLimiterVisual;
}

UIsdkGrabberComponent* UIsdkRigComponent::GetGrabber() const
{
  return GrabInteraction->Grabber;
}

UIsdkRayInteractionRigComponent* UIsdkRigComponent::GetRayInteraction() const
{
  return RayInteraction;
}

UIsdkPokeInteractionRigComponent* UIsdkRigComponent::GetPokeInteraction() const
{
  return PokeInteraction;
}

UIsdkGrabInteractionRigComponent* UIsdkRigComponent::GetGrabInteraction() const
{
  return GrabInteraction;
}

UIsdkInputActionsRigComponent* UIsdkRigComponent::GetInputActions() const
{
  return InputActions;
}

UIsdkInteractionGroupRigComponent* UIsdkRigComponent::GetInteractionGroup() const
{
  return InteractionGroup;
}

const FIsdkVirtualUserInfo& UIsdkRigComponent::GetWidgetVirtualUser() const
{
  return WidgetVirtualUser;
}

void UIsdkRigComponent::SetWidgetVirtualUser(const FIsdkVirtualUserInfo& InWidgetVirtualUser)
{
  WidgetVirtualUser = InWidgetVirtualUser;
}

const TArray<UIsdkRigModifier*>& UIsdkRigComponent::GetActiveRigModifiers() const
{
  return ActiveRigModifiers;
}

bool UIsdkRigComponent::GetHmdDataSource(
    TScriptInterface<IIsdkIHmdDataSource>& HmdDataSourceOut) const
{
  HmdDataSourceOut = HmdDataSource;
  return IsValid(HmdDataSource.GetObject());
}

void UIsdkRigComponent::SetRigComponentDefaults()
{
  InputActions->SetSubobjectPropertyDefaults(Handedness);
  WidgetVirtualUser.VirtualUserIndex = Handedness == EIsdkHandedness::Left ? 0 : 1;

  const FNativeGameplayTag& Tag = Handedness == EIsdkHandedness::Left
      ? IsdkGameplayTags::TAG_Isdk_Type_Device_Left
      : IsdkGameplayTags::TAG_Isdk_Type_Device_Right;
  InteractionTags.AddTag(Tag);
}

UIsdkTrackedDataSourceRigComponent* UIsdkRigComponent::GetVisuals() const
{
  return nullptr;
}

EIsdkHandedness UIsdkRigComponent::GetHandedness() const
{
  return Handedness;
}

void UIsdkRigComponent::UpdateComponentDataSources()
{
  UIsdkTrackedDataSourceRigComponent* Visuals = GetVisuals();
  const FIsdkTrackingDataSources& DataSources = Visuals->GetDataSources();

  // UpdateComponentDataSources may be called before data sources have been initialized.
  if (!DataSources.bIsInitialized)
  {
    return;
  }

  FName InteractorAttachSocket = NAME_None;
  USceneComponent* InteractorAttachComponent = nullptr;

  // Wire up data sources to interactors that work with either controllers or hands
  Visuals->GetInteractorSocket(
      InteractorAttachComponent, InteractorAttachSocket, RayInteractorSocket);
  RayInteraction->BindDataSources(
      DataSources, HmdDataSource, InteractorAttachComponent, InteractorAttachSocket);

  Visuals->GetInteractorSocket(
      InteractorAttachComponent, InteractorAttachSocket, PokeInteractorSocket);
  PokeInteraction->BindDataSources(DataSources, InteractorAttachComponent, InteractorAttachSocket);

  Visuals->GetInteractorSocket(InteractorAttachComponent, InteractorAttachSocket, GrabberSocket);
  GrabInteraction->BindDataSources(
      DataSources, HmdDataSource, InteractorAttachComponent, InteractorAttachSocket);

  GrabInteraction->UpdateMeshDependencies(
      GetPalmColliderOffset(),
      GetPinchAttachMesh(),
      GetPinchColliderOffset(),
      GetThumbTipSocketName());
}

void UIsdkRigComponent::RegisterInteractorWidgetIndices()
{
  if (GetOwner()->HasActorBegunPlay() || GetOwner()->IsActorBeginningPlay())
  {
    // Registering the interactors with the ISDK Widget Subsystem will allow them to send
    // VirtualUserPointerEvents to Slate Widgets.
    UIsdkWidgetSubsystem& IsdkWidgetSubsystem = UIsdkWidgetSubsystem::Get(GetWorld());
    IsdkWidgetSubsystem.RegisterVirtualUserInfo(GetPokeInteractor(), WidgetVirtualUser);
    IsdkWidgetSubsystem.RegisterVirtualUserInfo(GetRayInteractor(), WidgetVirtualUser);
  }
}

void UIsdkRigComponent::UnregisterInteractorWidgetIndices()
{
  UIsdkWidgetSubsystem& IsdkWidgetSubsystem = UIsdkWidgetSubsystem::Get(GetWorld());
  IsdkWidgetSubsystem.UnregisterVirtualUserInfo(GetPokeInteractor());
  IsdkWidgetSubsystem.UnregisterVirtualUserInfo(GetRayInteractor());
}

FVector UIsdkRigComponent::GetPalmColliderOffset() const
{
  return FVector::ZeroVector;
}

FVector UIsdkRigComponent::GetPinchColliderOffset() const
{
  return FVector::ZeroVector;
}

USkinnedMeshComponent* UIsdkRigComponent::GetPinchAttachMesh() const
{
  return nullptr;
}

void UIsdkRigComponent::CreateInteractionGroupConditionals()
{
  constexpr auto PokeInteractorGroupBehavior = FIsdkInteractionGroupMemberBehavior{
      .bDisableOnOtherSelect = true, .bDisableOnOtherNearFieldHover = false, .bIsNearField = true};
  auto CalculateInteractorGroupMemberState = [](const FIsdkInteractorStateEvent& Event)
  {
    FIsdkInteractionGroupMemberState State{};
    if (IsValid(Event.Interactor.GetObject()))
    {
      State.bIsSelectStateBlocking = Event.Interactor->HasSelectedInteractable();
    }
    return State;
  };

  PokeInteraction->GetEnabledConditional()->AddConditional(InteractionGroup->AddInteractor(
      PokeInteraction->PokeInteractor,
      *PokeInteraction->PokeInteractor->GetInteractorStateChangedDelegate(),
      CalculateInteractorGroupMemberState,
      PokeInteractorGroupBehavior));

  constexpr auto RayInteractorGroupBehavior = FIsdkInteractionGroupMemberBehavior{
      .bDisableOnOtherSelect = true,
      .bDisableOnOtherNearFieldHover = true,
      .bIsNearField = false // false here means a ray wont prevent near-field interactors hovering.
  };
  RayInteraction->GetEnabledConditional()->AddConditional(InteractionGroup->AddInteractor(
      RayInteraction->RayInteractor,
      *RayInteraction->RayInteractor->GetInteractorStateChangedDelegate(),
      CalculateInteractorGroupMemberState,
      RayInteractorGroupBehavior));

  constexpr auto GrabInteractorGroupBehavior = FIsdkInteractionGroupMemberBehavior{
      .bDisableOnOtherSelect = true,
      .bDisableOnOtherNearFieldHover = true, // disable grab when poke is hovering
      .bIsNearField = true};
  GrabInteraction->GetEnabledConditional()->AddConditional(InteractionGroup->AddInteractor(
      GrabInteraction->Grabber,
      *GrabInteraction->Grabber->GetInteractorStateChangedDelegate(),
      CalculateInteractorGroupMemberState,
      GrabInteractorGroupBehavior));
}

void UIsdkRigComponent::InitializeHmdDataSource()
{
  // If our HMD Data Source is good, we're done
  if (IsValid(HmdDataSource.GetObject()))
  {
    return;
  }

  AActor* OwningActor = this->GetAttachParentActor();
  if (!ensureMsgf(
          IsValid(OwningActor),
          TEXT("UIsdkRigComponent::InitializeHmdDataSource() - OwningActor isn't valid!")))
  {
    return;
  }

  TArray<UIsdkRigComponent*> ActorRigComponents;
  OwningActor->GetComponents(UIsdkRigComponent::StaticClass(), ActorRigComponents);

  // Check every other RigComponent on this actor and if any of them already have an HMD Data
  // Source, grab a reference to that
  for (UIsdkRigComponent* RigComponent : ActorRigComponents)
  {
    if (RigComponent == this || !IsValid(RigComponent))
    {
      continue;
    }
    if (RigComponent->GetHmdDataSource(HmdDataSource))
    {
      break;
    }
  }

  // Our search was in vain, set up an HMD Data Source ourselves
  if (!IsValid(HmdDataSource.GetObject()))
  {
    const auto TrackingDataSubsystem = GetVisuals()->GetTrackingDataSubsystem();
    if (ensureMsgf(
            IsValid(TrackingDataSubsystem.GetObject()),
            TEXT(
                "UIsdkRigComponent::InitializeHmdDataSource() - Could not get the IsdkTrackingDataSubsystem.")))
    {
      HmdDataSource = IIsdkITrackingDataSubsystem::Execute_GetOrCreateHmdDataSourceComponent(
          TrackingDataSubsystem.GetObject(), GetOwner());
    }
  }
}

FName UIsdkRigComponent::GetThumbTipSocketName() const
{
  return NAME_None;
}

void UIsdkRigComponent::HandleVisualsDataSourcesCreatedEvent()
{
  UpdateComponentDataSources();

  if (GetVisuals()->GetDataSources().bIsInitialized)
  {
    DataSourcesReady.Broadcast();
  }
}
