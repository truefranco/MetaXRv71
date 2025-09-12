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

#include "Interaction/GrabDetectors/IsdkHandGrabDetector.h"

#include "IsdkFunctionLibrary.h"
#include "OculusInteractionLog.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Interaction/IsdkGrabbableComponent.h"
#include "Interaction/IsdkGrabberComponent.h"
#include "Utilities/IsdkDebugUtils.h"
#include "DrawDebugHelpers.h"
#include "Logging/LogVerbosity.h"
#include "VisualLogger/VisualLogger.h"
#include "Components/SkinnedMeshComponent.h"

void UIsdkHandGrabDetector::Initialize(UIsdkGrabberComponent* InGrabberComponent)
{
  Super::Initialize(InGrabberComponent);

  // Initialize Default Palm Collider
  const auto PalmColliderAsSphere = NewObject<USphereComponent>(this, TEXT("Palm Collider"));
  PalmColliderAsSphere->InitSphereRadius(PalmColliderRadius);
  PalmGrabCollider = PalmColliderAsSphere;
  PalmGrabCollider->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
  PalmGrabCollider->SetCollisionObjectType(CollisionObjectType);
  PalmGrabCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
  PalmGrabCollider->SetMobility(EComponentMobility::Movable);
  PalmGrabCollider->AttachToComponent(
      GrabberComponent, FAttachmentTransformRules::KeepRelativeTransform);
  PalmGrabCollider->RegisterComponent();
  PalmGrabCollider->OnComponentBeginOverlap.AddDynamic(
      this, &UIsdkHandGrabDetector::HandleBeginOverlap);
  PalmGrabCollider->OnComponentEndOverlap.AddDynamic(
      this, &UIsdkHandGrabDetector::HandleEndOverlap);
  PalmGrabCollider->SetRelativeLocation(DefaultPalmColliderOffset);

  // Initialize Default Pinch Collider
  const auto PinchColliderAsSphere = NewObject<USphereComponent>(this, TEXT("Pinch Collider"));
  PinchColliderAsSphere->InitSphereRadius(PinchColliderRadius);
  PinchCollider = PinchColliderAsSphere;
  PinchCollider->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
  PinchCollider->SetCollisionObjectType(CollisionObjectType);
  PinchCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
  PinchCollider->SetMobility(EComponentMobility::Movable);
  PinchCollider->AttachToComponent(
      GrabberComponent, FAttachmentTransformRules::KeepRelativeTransform);
  PinchCollider->RegisterComponent();
  PinchCollider->OnComponentBeginOverlap.AddDynamic(
      this, &UIsdkHandGrabDetector::HandleBeginOverlap);
  PinchCollider->OnComponentEndOverlap.AddDynamic(this, &UIsdkHandGrabDetector::HandleEndOverlap);
}

void UIsdkHandGrabDetector::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  ComputeGrabCandidates();
}

const TSet<TObjectPtr<UIsdkGrabbableComponent>>& UIsdkHandGrabDetector::GetHoveredGrabbables() const
{
  return HoveredGrabbables;
}

UIsdkGrabbableComponent* UIsdkHandGrabDetector::GetGrabCandidate(EIsdkGrabInputMethod InputMethod)
{
  switch (InputMethod)
  {
    case EIsdkGrabInputMethod::Pinch:
      return PinchGrabCandidate;
    case EIsdkGrabInputMethod::Palm:
      return PalmGrabCandidate;
    default:
      return nullptr;
  }
}

EIsdkGrabDetectorType UIsdkHandGrabDetector::GetGrabDetectorType() const
{
  return EIsdkGrabDetectorType::HandGrab;
}

void UIsdkHandGrabDetector::AttachAndReplacePinchCollider(UPrimitiveComponent* Collider)
{
  AttachAndReplaceCollider(Collider, EIsdkGrabInputMethod::Pinch);
}

void UIsdkHandGrabDetector::AttachAndReplacePalmCollider(UPrimitiveComponent* Collider)
{
  AttachAndReplaceCollider(Collider, EIsdkGrabInputMethod::Palm);
}

void UIsdkHandGrabDetector::UpdatePalmOffset(const FVector& InPalmOffset)
{
  DefaultPalmColliderOffset = InPalmOffset;
  if (IsValid(PalmGrabCollider))
  {
    PalmGrabCollider->SetRelativeLocation(DefaultPalmColliderOffset);
  }
}

void UIsdkHandGrabDetector::UpdatePinchOffset(const FVector& InPinchOffset)
{
  DefaultPinchColliderOffset = InPinchOffset;
  if (IsValid(PinchCollider))
  {
    PinchCollider->SetRelativeLocation(DefaultPinchColliderOffset);
  }
}

void UIsdkHandGrabDetector::DrawDebugVisuals() const
{
  if (bDisableDebugVisuals)
  {
    return;
  }

  DrawDebugCollider(EIsdkGrabInputMethod::Pinch, PinchCollider);
  DrawDebugCollider(EIsdkGrabInputMethod::Palm, PalmGrabCollider);
}

void UIsdkHandGrabDetector::AttachAndReplaceCollider(
    UPrimitiveComponent* Collider,
    EIsdkGrabInputMethod InputMethod)
{
  // Validate input collider
  if (!IsValid(Collider))
  {
    return;
  }

  // Pick the correct collider to replace
  UPrimitiveComponent* ColliderToReplace = nullptr;
  if (InputMethod == EIsdkGrabInputMethod::Pinch)
  {
    ColliderToReplace = PinchCollider;
  }
  else if (InputMethod == EIsdkGrabInputMethod::Palm)
  {
    ColliderToReplace = PalmGrabCollider;
  }

  // If we don't have a collider to replace, this is an error
  if (ColliderToReplace == nullptr)
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandGrabDetector::AttachAndReplaceCollider failed due to invalid ColliderToReplace"));
    return;
  }

  // Attach the new collider
  const auto AttachParent = ColliderToReplace->GetAttachParent();
  const auto AttachSocket = ColliderToReplace->GetAttachSocketName();
  const auto RelativeTransform = ColliderToReplace->GetRelativeTransform();
  Collider->AttachToComponent(
      AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
  Collider->SetRelativeTransform(RelativeTransform);

  // Stop listening to old collider events
  ColliderToReplace->OnComponentBeginOverlap.RemoveDynamic(
      this, &UIsdkHandGrabDetector::HandleBeginOverlap);
  ColliderToReplace->OnComponentEndOverlap.RemoveDynamic(
      this, &UIsdkHandGrabDetector::HandleEndOverlap);

  // Start listening to new collider event
  Collider->OnComponentBeginOverlap.AddDynamic(this, &UIsdkHandGrabDetector::HandleBeginOverlap);
  Collider->OnComponentEndOverlap.AddDynamic(this, &UIsdkHandGrabDetector::HandleEndOverlap);

  // Cleanup & Finalize the replacement
  ColliderToReplace->DestroyComponent();
  if (InputMethod == EIsdkGrabInputMethod::Pinch)
  {
    PinchCollider = Collider;
  }
  else if (InputMethod == EIsdkGrabInputMethod::Palm)
  {
    PalmGrabCollider = Collider;
  }
}

void UIsdkHandGrabDetector::DrawDebugCollider(
    EIsdkGrabInputMethod InputMethod,
    const UPrimitiveComponent* GrabCollider) const
{
  auto InteractorState = EIsdkInteractorState::Normal;
  if (bIsSelecting)
  {
    InteractorState = EIsdkInteractorState::Select;
  }
  else if (!HoveredGrabbables.IsEmpty())
  {
    InteractorState = EIsdkInteractorState::Hover;
  }
  FColor DebugColor = UIsdkDebugUtils::GetInteractorStateDebugColor(InteractorState);

  // Instead of having a bunch of different colors, slightly dim the different grab collider color
  // depending on types.  This way don't bloat the debug color options and have some easy visual
  // differentiation.
  float ColorCoefficient;
  switch (InputMethod)
  {
    case EIsdkGrabInputMethod::Palm:
      ColorCoefficient = 0.9f;
      break;
    case EIsdkGrabInputMethod::Unknown:
      ColorCoefficient = 0.8f;
      break;
    case EIsdkGrabInputMethod::Pinch:
    default:
      ColorCoefficient = 1.f;
      break;
  }
  const auto FinalDebugColor = (ColorCoefficient * DebugColor).ToFColor(true);

  if (const auto ColliderAsSphere = Cast<USphereComponent>(GrabCollider))
  {
    // Draw a sphere if the collider is a sphere
    DrawDebugSphere(
        GetWorld(),
        ColliderAsSphere->GetComponentLocation(),
        ColliderAsSphere->GetScaledSphereRadius(),
        12,
        FinalDebugColor);
    UE_VLOG_WIRESPHERE(
        GrabberComponent->GetOwner(),
        LogOculusInteraction,
        Verbose,
        ColliderAsSphere->GetComponentLocation(),
        ColliderAsSphere->GetScaledSphereRadius(),
        FinalDebugColor,
        TEXT_EMPTY);
  }
  else if (const auto ColliderAsBox = Cast<UBoxComponent>(GrabCollider))
  {
    // Draw a box if the collider is a box
    const auto BoxLocation = ColliderAsBox->GetComponentLocation();
    const auto BoxExtent = ColliderAsBox->GetScaledBoxExtent();
    const auto Box = FBox(BoxLocation - BoxExtent, BoxLocation + BoxExtent);
    DrawDebugBox(
        GetWorld(),
        ColliderAsBox->GetComponentLocation(),
        ColliderAsBox->GetScaledBoxExtent(),
        ColliderAsBox->GetComponentQuat(),
        FinalDebugColor);
    UE_VLOG_WIREOBOX(
        GrabberComponent->GetOwner(),
        LogOculusInteraction,
        Verbose,
        Box,
        ColliderAsBox->GetComponentQuat().ToMatrix(),
        FinalDebugColor,
        TEXT_EMPTY);
  }
  else
  {
    // Draw a bounding box otherwise
    const auto BoxLocation = GrabCollider->GetComponentLocation();
    const auto BoxExtent = GrabCollider->GetLocalBounds().BoxExtent;
    const auto Box = FBox(BoxLocation - BoxExtent, BoxLocation + BoxExtent);
    DrawDebugBox(
        GetWorld(),
        ColliderAsBox->GetComponentLocation(),
        ColliderAsBox->GetScaledBoxExtent(),
        FinalDebugColor);
    UE_VLOG_WIREBOX(
        GrabberComponent->GetOwner(),
        LogOculusInteraction,
        Verbose,
        Box,
        FinalDebugColor,
        TEXT_EMPTY);
  }
}

const UPrimitiveComponent* UIsdkHandGrabDetector::GetPinchCollider() const
{
  return PinchCollider;
}

const UPrimitiveComponent* UIsdkHandGrabDetector::GetPalmGrabCollider() const
{
  return PalmGrabCollider;
}

UPrimitiveComponent* UIsdkHandGrabDetector::GetGrabbingComponent() const
{
  return SelectingCollider;
}

void UIsdkHandGrabDetector::HandleBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
  // We need to do a lookup to find the grabbable associated with this collider, if there is one
  const auto OverlappedGrabbable = UIsdkFunctionLibrary::FindGrabbableByComponent(OtherComp);
  if (!OverlappedGrabbable)
  {
    return;
  }

  // If the grabbable doesn't allow hand grabs, don't hover
  if (!OverlappedGrabbable->IsHandGrabAllowed())
  {
    return;
  }

  // Get the input method associated with the overlapped grabber
  EIsdkGrabInputMethod InputMethod;
  if (OverlappedComponent == PinchCollider)
  {
    InputMethod = EIsdkGrabInputMethod::Pinch;
  }
  else if (OverlappedComponent == PalmGrabCollider)
  {
    InputMethod = EIsdkGrabInputMethod::Palm;
  }
  else
  {
    return;
  }

  // If the grabbable doesn't allow the grabbers input method, don't hover
  if (!OverlappedGrabbable->IsGrabInputMethodAllowed(InputMethod))
  {
    return;
  }

  // If grabber doesn't allow pinches and this is a pinch collider, don't hover
  if (InputMethod == EIsdkGrabInputMethod::Pinch &&
      !GrabberComponent->IsGrabInputMethodAllowed(EIsdkGrabInputMethod::Pinch))
  {
    return;
  }

  // If grabber doesn't allow palm grabs and this is a palm grab collider, don't hover
  if (InputMethod == EIsdkGrabInputMethod::Palm &&
      !GrabberComponent->IsGrabInputMethodAllowed(EIsdkGrabInputMethod::Palm))
  {
    return;
  }

  HoveredGrabbables.Add(OverlappedGrabbable);
}

void UIsdkHandGrabDetector::HandleEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
  TSet<UPrimitiveComponent*> PinchOverlaps;
  PinchCollider->GetOverlappingComponents(PinchOverlaps);

  TSet<UPrimitiveComponent*> PalmOverlaps;
  PalmGrabCollider->GetOverlappingComponents(PalmOverlaps);

  // Only unhover if the other component no longer overlaps BOTH pinch and palm colliders,
  // otherwise, we may unhover while still overlapping
  bool bShouldUnHover = !PinchOverlaps.Contains(OtherComp) && !PalmOverlaps.Contains(OtherComp);
  if (!bShouldUnHover)
  {
    return;
  }

  const auto OverlappedGrabbable = UIsdkFunctionLibrary::FindGrabbableByComponent(OtherComp);
  if (OverlappedGrabbable)
  {
    HoveredGrabbables.Remove(OverlappedGrabbable);
  }
}

FVector UIsdkHandGrabDetector::GetSelectingColliderPosition() const
{
  if (!IsValid(SelectingCollider))
  {
    return GrabberComponent->GetComponentLocation();
  }
  return SelectingCollider->GetComponentLocation();
}

void UIsdkHandGrabDetector::ComputeGrabCandidates()
{
  // Only update candidates if we are not currently selecting
  if (bIsSelecting)
  {
    return;
  }

  float NearestPinchDistance = FLT_MAX;
  float NearestPalmDistance = FLT_MAX;

  PinchGrabCandidate = nullptr;
  PalmGrabCandidate = nullptr;

  for (const auto& HoveredGrabbable : HoveredGrabbables)
  {
    const bool bIsPalmGrabbable =
        HoveredGrabbable->IsGrabInputMethodAllowed(EIsdkGrabInputMethod::Palm);
    const bool bIsPinchGrabbable =
        HoveredGrabbable->IsGrabInputMethodAllowed(EIsdkGrabInputMethod::Pinch);

    if (!bIsPalmGrabbable && !bIsPinchGrabbable)
    {
      continue;
    }

    const auto GrabbableCollider = HoveredGrabbable->GetGrabCollider();

    // Computer nearest pinch grabbable
    if (bIsPinchGrabbable && IsValid(PinchCollider))
    {
      const float PinchDistance =
          (PinchCollider->GetComponentLocation() - GrabbableCollider->GetComponentLocation())
              .SizeSquared();
      const bool bOverlapsPinchCollider =
          PinchCollider->IsOverlappingComponent(HoveredGrabbable->GetGrabCollider());

      if (bIsPinchGrabbable && bOverlapsPinchCollider && PinchDistance < NearestPinchDistance)
      {
        NearestPinchDistance = PinchDistance;
        PinchGrabCandidate = HoveredGrabbable;
      }
    }

    // Computer nearest palm grabbable
    if (bIsPalmGrabbable && IsValid(PalmGrabCollider))
    {
      const float PalmDistance =
          (PalmGrabCollider->GetComponentLocation() - GrabbableCollider->GetComponentLocation())
              .SizeSquared();
      const bool bOverlapsPalmCollider =
          PalmGrabCollider->IsOverlappingComponent(HoveredGrabbable->GetGrabCollider());

      if (bIsPalmGrabbable && bOverlapsPalmCollider && PalmDistance < NearestPalmDistance)
      {
        NearestPalmDistance = PalmDistance;
        PalmGrabCandidate = HoveredGrabbable;
      }
    }
  }
}

void UIsdkHandGrabDetector::UpdatePinchCollidersAttachment(
    USkinnedMeshComponent* InPinchAttachMesh,
    const FName& InThumbTipSocketName)
{
  if (!IsValid(InPinchAttachMesh) || !IsValid(PinchCollider))
  {
    return;
  }

  if (GrabberComponent->IsGrabbing())
  {
    // If the Grabber is grabbing, attach the PinchCollider to the Grabber and set its transform
    // to the thumb tip.  This keeps the transform of what is being grabbed constant relative to
    // the root of the controller / hand - small movement in the thumb will be ignored.
    PinchCollider->AttachToComponent(
        GrabberComponent, FAttachmentTransformRules::KeepRelativeTransform);

    const FTransform ThumbTipPosition = InPinchAttachMesh->GetBoneTransform(InThumbTipSocketName);
    PinchCollider->SetWorldTransform(ThumbTipPosition);
  }
  else
  {
    // If the Grabber is not grabbing, attach the PinchCollider to the thumb tip socket on the
    // HandVisual
    PinchCollider->AttachToComponent(
        InPinchAttachMesh,
        FAttachmentTransformRules::SnapToTargetIncludingScale,
        InThumbTipSocketName);
  }
}
