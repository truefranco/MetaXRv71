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

#include "Interaction/IsdkGrabbableComponent.h"
#include "Subsystem/IsdkWorldSubsystem.h"
#include "CoreMinimal.h"
#include "IsdkRuntimeSettings.h"
#include "OculusInteractionLog.h"
#include "Interaction/IsdkGrabberComponent.h"
#include "Utilities/IsdkDebugUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "DrawDebugHelpers.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Interaction/Grabbable/IsdkGrabTransformerComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace isdk
{
extern TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DebugInteractionVisuals;
}

UIsdkGrabbableComponent::UIsdkGrabbableComponent()
{
  // Component tick is used only for debug purposes.  Disable for non-editor builds.
#if WITH_EDITOR
  PrimaryComponentTick.bCanEverTick = true;
#else
  PrimaryComponentTick.bCanEverTick = false;
#endif

  // Prioritize pre physics in order to reset velocities when releasing before physics solver to
  // avoid weird velocity issues
  PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
  InteractableTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Interactable_Grab);

  // Fetch the material applied to custom collision mesh for visualization purposes.
  struct FConstructorStatics
  {
    ConstructorHelpers::FObjectFinder<UMaterialInterface> CustomCollisionMeshMaterial;
    FConstructorStatics()
        : CustomCollisionMeshMaterial(TEXT("/OculusInteraction/Materials/CustomCollisionMaterial"))
    {
    }
  };
  static FConstructorStatics ConstructorStatics;

  CustomCollisionMaterial = ConstructorStatics.CustomCollisionMeshMaterial.Object;
}

void UIsdkGrabbableComponent::BeginPlay()
{
  SelectedGrabbers.Empty();
  HoveredGrabbers.Empty();

  GrabTransformerComponent = GetOwner()->FindComponentByClass<UIsdkGrabTransformerComponent>();
  if (!GrabTransformerComponent)
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkGrabbableComponent.GrabTransformer is null in %s.%s.  "
            "UIsdkGrabbableComponent must have a corresponding UIsdkGrabTransformerComponent to work."),
        *GetNameSafe(GetOwner()),
        *GetNameSafe(this))
  }

  // This is just for debugging, so let's turn it off in non-editor builds
#if WITH_EDITOR
  if (!GIsAutomationTesting)
  {
    InteractionPointerEvent.AddDynamic(this, &UIsdkGrabbableComponent::HandlePointerEvent);
  }
#endif

  Super::BeginPlay();
}

void UIsdkGrabbableComponent::OnRegister()
{
  Super::OnRegister();
  SetupCollider();
}

void UIsdkGrabbableComponent::SetupCollider()
{
  // If we already have a collider, set to null
  if (GrabCollider)
  {
    GrabCollider = nullptr;
  }

  switch (ColliderMode)
  {
    case EIsdkGrabbableColliderMode::FindStaticMesh:
    {
      GrabCollider = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
    }
    break;
    case EIsdkGrabbableColliderMode::FindByName:
    {
      const auto Components = GetOwner()->GetComponents();
      for (const auto Component : Components)
      {
        if (Component->GetName() != ColliderName)
        {
          continue;
        }

        const auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
        if (!PrimitiveComponent)
        {
          continue;
        }

        GrabCollider = PrimitiveComponent;
        break;
      }
    }
    break;
    case EIsdkGrabbableColliderMode::Sphere:
    {
      const auto SphereComponent =
          NewObject<USphereComponent>(this, NAME_None, RF_Transactional | RF_TextExportTransient);
      SphereComponent->SetSphereRadius(ColliderRadius);
      SphereComponent->SetHiddenInGame(true);
      GrabCollider = SphereComponent;
    }
    break;
    case EIsdkGrabbableColliderMode::Box:
    {
      const auto BoxComponent =
          NewObject<UBoxComponent>(this, NAME_None, RF_Transactional | RF_TextExportTransient);
      BoxComponent->SetBoxExtent(ColliderBoxExtent);
      BoxComponent->SetHiddenInGame(true);
      GrabCollider = BoxComponent;
    }
    break;
    case EIsdkGrabbableColliderMode::CustomMesh:
    {
      const auto StaticMeshComponent = NewObject<UStaticMeshComponent>(
          this, NAME_None, RF_Transactional | RF_TextExportTransient);
      StaticMeshComponent->SetStaticMesh(CustomCollisionMesh);
      StaticMeshComponent->SetCollisionProfileName("OverlapAllDynamic");
      StaticMeshComponent->SetHiddenInGame(true);

      for (int Index = 0; Index < StaticMeshComponent->GetNumMaterials(); Index++)
      {
        StaticMeshComponent->SetMaterial(Index, CustomCollisionMaterial);
      }
      GrabCollider = StaticMeshComponent;
    }
    break;
    default:
      break;
  }

  const bool bShouldSetupMesh = ColliderMode != EIsdkGrabbableColliderMode::FindStaticMesh &&
      ColliderMode != EIsdkGrabbableColliderMode::FindByName;
  if (IsValid(GrabCollider) && bShouldSetupMesh)
  {
    GrabCollider->SetupAttachment(this);
    GrabCollider->RegisterComponentWithWorld(GetWorld());
  }
}

void UIsdkGrabbableComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* TickFn)
{
  Super::TickComponent(DeltaTime, TickType, TickFn);

  if (!isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
    return;

  const FColor DebugColor = UIsdkDebugUtils::GetInteractableStateDebugColor(State);

  FBox BoundsBox = GetGrabCollider()->Bounds.GetBox();
  DrawDebugBox(GetWorld(), BoundsBox.GetCenter(), BoundsBox.GetExtent(), DebugColor, false);
  UE_VLOG_BOX(GetOwner(), LogOculusInteraction, Verbose, BoundsBox, DebugColor, TEXT_EMPTY);
}

void UIsdkGrabbableComponent::GetInteractableStateRelationships(
    EIsdkInteractableState InState,
    TArray<TScriptInterface<IIsdkIInteractorState>>& OutInteractors) const
{
  for (auto* Interactor : Interactors)
  {
    auto* Grabber = Cast<UIsdkGrabberComponent>(Interactor);
    const bool bIsSelected = IsValid(Grabber) && IsGrabbedBy(Grabber);

    const bool bWantsHovered = InState == EIsdkInteractableState::Hover;
    const bool bWantsSelected = InState == EIsdkInteractableState::Select;
    if ((!bIsSelected && bWantsHovered) || (bIsSelected && bWantsSelected))
    {
      OutInteractors.Add(Interactor);
    }
  }
}

FIsdkInteractionPointerEventDelegate& UIsdkGrabbableComponent::GetInteractionPointerEventDelegate()
{
  return InteractionPointerEvent;
}

void UIsdkGrabbableComponent::PostEvent(const FIsdkInteractionPointerEvent& Event)
{
  const auto& Grabber = Cast<UIsdkGrabberComponent>(Event.Interactor);
  if (!IsValid(Grabber))
  {
    return;
  }

  switch (Event.Type)
  {
    //----- HOVER
    case EIsdkPointerEventType::Hover:
      if (!IsHoveredBy(Grabber))
      {
        HoveredGrabbers.Add(Grabber);
      }
      break;
    case EIsdkPointerEventType::Unhover:
      if (IsHoveredBy(Grabber))
      {
        HoveredGrabbers.Remove(Grabber);
      }
      break;
    //----- SELECT
    case EIsdkPointerEventType::Select:
      if (!IsGrabbedBy(Grabber))
      {
        SelectedGrabbers.Add(Grabber);
      }
      break;
    case EIsdkPointerEventType::Unselect:
      if (IsGrabbedBy(Grabber))
      {
        SelectedGrabbers.Remove(Grabber);
      }
      break;
    //----- CANCEL
    case EIsdkPointerEventType::Cancel:
      if (IsHoveredBy(Grabber))
      {
        HoveredGrabbers.Remove(Grabber);
      }
      if (IsGrabbedBy(Grabber))
      {
        SelectedGrabbers.Remove(Grabber);
      }
      break;
    default:
      break;
  }

  if (SelectedGrabbers.Num() > 0)
  {
    SetState(EIsdkInteractableState::Select);
  }
  else if (HoveredGrabbers.Num() > 0)
  {
    SetState(EIsdkInteractableState::Hover);
  }
  else
  {
    SetState(EIsdkInteractableState::Normal);
  }

  InteractionPointerEvent.Broadcast(Event);
  if (IsValid(GrabTransformerComponent))
  {
    GrabTransformerComponent->ProcessPointerEvent(Event);
  }
}

UPrimitiveComponent* UIsdkGrabbableComponent::GetGrabCollider() const
{
  return GrabCollider;
}

void UIsdkGrabbableComponent::SetGrabCollider(UPrimitiveComponent* InGrabCollider)
{
  GrabCollider = InGrabCollider;
}

bool UIsdkGrabbableComponent::IsHoveredBy(UIsdkGrabberComponent* Grabber) const
{
  if (!IsValid(Grabber))
  {
    return false;
  }

  return HoveredGrabbers.Contains(Grabber);
}

bool UIsdkGrabbableComponent::IsGrabbedBy(const UIsdkGrabberComponent* Grabber) const
{
  if (!IsValid(Grabber))
  {
    return false;
  }

  return SelectedGrabbers.Contains(Grabber);
}

bool UIsdkGrabbableComponent::IsGrabInputMethodAllowed(EIsdkGrabInputMethod InputMethod)
{
  return AllowedInputMethods.Contains(InputMethod);
}

void UIsdkGrabbableComponent::SetGrabInputMethodAllowed(
    EIsdkGrabInputMethod InputMethod,
    bool Allowed)
{
  const bool CurrentlyAllowed = AllowedInputMethods.Contains(InputMethod);

  if (CurrentlyAllowed != Allowed)
  {
    if (!Allowed)
      AllowedInputMethods.Remove(InputMethod);
    else
      AllowedInputMethods.Add(InputMethod);
  }
}

UIsdkGrabTransformerComponent* UIsdkGrabbableComponent::GetGrabTransformer() const
{
  return GrabTransformerComponent;
}

void UIsdkGrabbableComponent::SetGrabTransformer(UIsdkGrabTransformerComponent* InGrabTransformer)
{
  GrabTransformerComponent = InGrabTransformer;
}

bool UIsdkGrabbableComponent::IsHandGrabAllowed() const
{
  return AllowedGrabDetectorTypes & static_cast<uint8>(EIsdkGrabDetectorType::HandGrab);
}

bool UIsdkGrabbableComponent::IsDistanceGrabAllowed() const
{
  return AllowedGrabDetectorTypes & static_cast<uint8>(EIsdkGrabDetectorType::DistanceGrab);
}

bool UIsdkGrabbableComponent::IsRayGrabAllowed() const
{
  return AllowedGrabDetectorTypes & static_cast<uint8>(EIsdkGrabDetectorType::RayGrab);
}

void UIsdkGrabbableComponent::SetState(EIsdkInteractableState NewState)
{
  const auto oldState = GetInteractableState();
  if (oldState == NewState)
  {
    return;
  }
  Super::SetState(NewState);
}

void UIsdkGrabbableComponent::HandlePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
{
  // Only debug draw if the appropriate cvar is on
  if (!isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
  {
    return;
  }

  // Don't draw move pointer events, they're too noisy
  if (PointerEvent.Type == EIsdkPointerEventType::Move)
  {
    return;
  }

  const FColor DebugColor = UIsdkDebugUtils::GetPointerEventDebugColor(PointerEvent.Type);

  const auto DebugLocation = FVector(PointerEvent.Pose.Position);
  const auto DebugRadius = GetDefault<UIsdkRuntimeSettings>()->PointerEventDebugRadius;
  const auto DebugDuration = GetDefault<UIsdkRuntimeSettings>()->PointerEventDebugDuration;
  DrawDebugSphere(GetWorld(), DebugLocation, DebugRadius, 12, DebugColor, false, DebugDuration);
  UE_VLOG_LOCATION(
      GetOwner(), LogOculusInteraction, Log, DebugLocation, DebugRadius, DebugColor, TEXT_EMPTY);
  UE_VLOG(
      GetOwner(),
      LogOculusInteraction,
      Log,
      TEXT("PointerEvent\nType: %s\nInteractor: %s\nInteractable: %s\nLocation: %s"),
      *UEnum::GetValueAsString(PointerEvent.Type),
      *GetFullNameSafe(PointerEvent.Interactor),
      *GetFullNameSafe(PointerEvent.Interactable.GetObject()),
      *PointerEvent.Pose.Position.ToString());
}

void UIsdkGrabbableComponent::SetColliderMode(
    EIsdkGrabbableColliderMode NewColliderMode,
    float NewSize)
{
  ColliderMode = NewColliderMode;
  if (ColliderMode == EIsdkGrabbableColliderMode::Sphere)
  {
    ColliderRadius = NewSize;
  }
  else if (ColliderMode == EIsdkGrabbableColliderMode::Box)
  {
    ColliderBoxExtent = FVector(NewSize);
  }
}
