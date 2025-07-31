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

#include "Interaction/IsdkGrabberComponent.h"
#include "CoreMinimal.h"
#include "IsdkRuntimeSettings.h"
#include "Interaction/IsdkGrabbableComponent.h"
#include "Subsystem/IsdkWorldSubsystem.h"
#include "StructTypes.h"
#include "Interaction/Grabbable/IsdkGrabTransformerComponent.h"
#include "Interaction/GrabDetectors/IsdkHandGrabDetector.h"
#include "Interaction/GrabDetectors/IsdkDistanceGrabDetector.h"
#include "Interaction/GrabDetectors/IsdkRayGrabDetector.h"
#include "Interaction/Motion/IsdkGrabMotion.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Logging/LogVerbosity.h"
#include "VisualLogger/VisualLogger.h"

namespace isdk
{
extern TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_DebugInteractionVisuals;
}

UIsdkGrabberComponent::UIsdkGrabberComponent()
{
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
  InteractorTags.AddTag(IsdkGameplayTags::TAG_Isdk_Type_Interactor_Grab);

  HandGrabDetector = CreateDefaultSubobject<UIsdkHandGrabDetector>(TEXT("HandGrabDetector"));
  DistanceGrabDetector =
      CreateDefaultSubobject<UIsdkDistanceGrabDetector>(TEXT("DistanceGrabDetector"));
  RayGrabDetector = CreateDefaultSubobject<UIsdkRayGrabDetector>(TEXT("RayGrabDetector"));

  static ConstructorHelpers::FObjectFinder<UNiagaraSystem> RayGrabNiagaraSystemFinder =
      ConstructorHelpers::FObjectFinder<UNiagaraSystem>(
          TEXT("/OculusInteraction/FX/NS_RayVisuals"));
  static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DistanceGrabNiagaraSystemFinder =
      ConstructorHelpers::FObjectFinder<UNiagaraSystem>(
          TEXT("/OculusInteraction/FX/NS_DistanceGrabVisuals"));
  RayGrabNiagaraSystem = RayGrabNiagaraSystemFinder.Object;
  DistanceGrabNiagaraSystem = DistanceGrabNiagaraSystemFinder.Object;
}

void UIsdkGrabberComponent::BeginPlay()
{
  Super::BeginPlay();

  SetMobility(EComponentMobility::Movable);

  HandGrabDetector->Initialize(this);
  DistanceGrabDetector->Initialize(this);
  RayGrabDetector->Initialize(this);
}

void UIsdkGrabberComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  TickInteractors(DeltaTime);
  UpdateHoveredGrabbables();
  UpdateHoveredInteractor();

  if (IsValid(GrabbedComponent))
  {
    PostEvent(EIsdkPointerEventType::Move, GrabbedComponent);
  }
  else if (IsAnyInputMethodActive())
  {
    // Our GrabbedComponent is no longer valid, reset the state
    ResetGrabState();
  }

  UpdateRayGrabVisuals();
  UpdateDistanceGrabVisuals();
  UpdateInteractorState();
}

void UIsdkGrabberComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  HandGrabDetector->Deinitialize();
  DistanceGrabDetector->Deinitialize();
  RayGrabDetector->Deinitialize();
}

void UIsdkGrabberComponent::Deactivate()
{
  // If we are still grabbing something when the grabber goes offline, let go of it
  PalmRelease();
  PinchRelease();
  ClearActiveInputMethods();

  if (RayGrabNiagaraComponent)
  {
    RayGrabNiagaraComponent->DestroyComponent();
  }

  if (DistanceGrabNiagaraComponent)
  {
    DistanceGrabNiagaraComponent->DestroyComponent();
  }

  for (const auto& HoveredGrabbable : HoveredGrabbables)
  {
    if (IsValid(HoveredGrabbable))
    {
      PostEvent(EIsdkPointerEventType::Unhover, HoveredGrabbable);
      HoveredGrabbable->RemoveInteractor(this);
    }
  }
  HoveredGrabbables.Empty();

  Super::Deactivate();
}

void UIsdkGrabberComponent::PinchGrab()
{
  GrabByInputMethod(EIsdkGrabInputMethod::Pinch);
}

void UIsdkGrabberComponent::PalmGrab()
{
  GrabByInputMethod(EIsdkGrabInputMethod::Palm);
}

void UIsdkGrabberComponent::PinchRelease()
{
  ReleaseByInputMethod(EIsdkGrabInputMethod::Pinch);
}

void UIsdkGrabberComponent::PalmRelease()
{
  ReleaseByInputMethod(EIsdkGrabInputMethod::Palm);
}

float UIsdkGrabberComponent::GetPinchStrength() const
{
  return PinchStrength;
}

void UIsdkGrabberComponent::SetPinchStrength(float InPinchStrength)
{
  PinchStrength = InPinchStrength;
}

bool UIsdkGrabberComponent::IsGrabbing() const
{
  return IsValid(GrabbedComponent);
}

bool UIsdkGrabberComponent::HasInteractable() const
{
  return State == EIsdkInteractorState::Hover;
}

bool UIsdkGrabberComponent::HasSelectedInteractable() const
{
  return State == EIsdkInteractorState::Select;
}

void UIsdkGrabberComponent::TickInteractors(float DeltaTime)
{
  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::HandGrab))
  {
    HandGrabDetector->Tick(DeltaTime);
  }

  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::DistanceGrab))
  {
    DistanceGrabDetector->Tick(DeltaTime);
  }

  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::RayGrab))
  {
    RayGrabDetector->Tick(DeltaTime);
  }

#if !UE_BUILD_SHIPPING
  if (isdk::CVar_Meta_InteractionSDK_DebugInteractionVisuals.GetValueOnAnyThread())
  {
    DrawDebugVisuals();
  }
#endif

  if (IsValid(CurrentGrabMotion) && CurrentGrabMotion->IsActive())
  {
    CurrentGrabMotion->Tick(DeltaTime);
  }
}

void UIsdkGrabberComponent::UpdateHoveredGrabbables()
{
  TSet<UIsdkGrabbableComponent*> CurrentHoveredGrabbables;
  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::HandGrab))
  {
    CurrentHoveredGrabbables.Append(HandGrabDetector->GetHoveredGrabbables());
  }
  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::DistanceGrab))
  {
    CurrentHoveredGrabbables.Append(DistanceGrabDetector->GetHoveredGrabbables());
  }
  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::RayGrab) &&
      IsValid(RayGrabDetector->GetHoveredGrabbable()))
  {
    CurrentHoveredGrabbables.Add(RayGrabDetector->GetHoveredGrabbable());
  }

  // Hovered Grabbables from last tick may have become null - clear these out
  HoveredGrabbables.Remove(nullptr);

  // New hovers are in Current and not in Previous
  const auto& NewHoveredGrabbables = CurrentHoveredGrabbables.Difference(HoveredGrabbables);

  // Unhovers are in Previous and not in Current
  const auto UnHoveredGrabbables = HoveredGrabbables.Difference(CurrentHoveredGrabbables);

  // Update
  HoveredGrabbables = CurrentHoveredGrabbables;

  // Post hovered events
  for (const auto& HoveredGrabbable : NewHoveredGrabbables)
  {
    if (IsValid(HoveredGrabbable))
    {
      PostEvent(EIsdkPointerEventType::Hover, HoveredGrabbable);
      HoveredGrabbable->AddInteractor(this);
    }
  }

  // Post unhovered events
  for (const auto& UnhoveredGrabbable : UnHoveredGrabbables)
  {
    if (IsValid(UnhoveredGrabbable))
    {
      PostEvent(EIsdkPointerEventType::Unhover, UnhoveredGrabbable);
      UnhoveredGrabbable->RemoveInteractor(this);
    }
  }
}

void UIsdkGrabberComponent::UpdateInteractorState()
{
  EIsdkInteractorState NewState = EIsdkInteractorState::Normal;
  if (IsValid(GrabbedComponent))
  {
    NewState = EIsdkInteractorState::Select;
  }
  else if (!HoveredGrabbables.IsEmpty())
  {
    NewState = EIsdkInteractorState::Hover;
  }

  SetState(NewState);
}

void UIsdkGrabberComponent::UpdateRayGrabVisuals()
{
  // If we don't have a distance grab interactor or anything to show, exit
  if (!IsValid(RayGrabDetector) || !IsValid(RayGrabNiagaraSystem) ||
      !IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::RayGrab))
  {
    if (IsValid(RayGrabNiagaraComponent))
    {
      RayGrabNiagaraComponent->DestroyInstance();
      RayGrabNiagaraComponent->DestroyComponent();
      RayGrabNiagaraComponent = nullptr;
    }
    return;
  }

  FHitResult RayHit;
  RayGrabDetector->GetHit(RayHit);

  // To display, we must have a valid candidate, distance grab must be the active interactor,
  // and we shouldn't be inside the grabbable
  const auto HoveredGrabbable = RayGrabDetector->GetHoveredGrabbable();
  const bool bRayGrabberActive =
      RayGrabDetector == CurrentHoverDetector || RayGrabDetector == CurrentGrabDetector;
  const bool bShouldDisplayRayVisual =
      !RayHit.bStartPenetrating && IsValid(HoveredGrabbable) && bRayGrabberActive;

  // Either create a visual, destroy a visual, or continue in whatever state we're already in
  if (bShouldDisplayRayVisual && !IsValid(RayGrabNiagaraComponent))
  {
    RayGrabNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
        RayGrabNiagaraSystem,
        this,
        NAME_None,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::Type::SnapToTarget,
        true);
  }
  else if (!bShouldDisplayRayVisual && IsValid(RayGrabNiagaraComponent))
  {
    RayGrabNiagaraComponent->DestroyInstance();
    RayGrabNiagaraComponent->DestroyComponent();
    RayGrabNiagaraComponent = nullptr;
  }

  // If we're not displaying anything, all done here
  if (!bShouldDisplayRayVisual || !IsValid(RayGrabNiagaraComponent))
  {
    return;
  }

  // Update niagara system parameters so that the system can display itself correctly
  FTransform PointerTransform = FTransform::Identity;
  GetPointerTransform(PointerTransform);
  RayGrabNiagaraComponent->SetVectorParameter(
      FName("User.PointerLocation"), PointerTransform.GetLocation());
  RayGrabNiagaraComponent->SetVectorParameter(
      FName("User.PointerVector"), PointerTransform.GetRotation().GetForwardVector());
  RayGrabNiagaraComponent->SetVectorParameter(FName("User.HitLocation"), RayHit.Location);
  RayGrabNiagaraComponent->SetVectorParameter(FName("User.HitNormal"), RayHit.Normal);
  RayGrabNiagaraComponent->SetFloatParameter(FName("User.PinchStrength"), PinchStrength);
}

void UIsdkGrabberComponent::UpdateDistanceGrabVisuals()
{
  // If we don't have a distance grab interactor or anything to show, exit
  if (!IsValid(DistanceGrabDetector) || !IsValid(DistanceGrabNiagaraSystem) ||
      !IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::DistanceGrab))
  {
    if (IsValid(DistanceGrabNiagaraComponent))
    {
      DistanceGrabNiagaraComponent->DestroyInstance();
      DistanceGrabNiagaraComponent->DestroyComponent();
      DistanceGrabNiagaraComponent = nullptr;
    }
    return;
  }

  // We must have a valid candidate, and distance grab must be the active hovering interactor
  UIsdkGrabbableComponent* Grabbable = nullptr;
  if (CurrentHoverDetector == DistanceGrabDetector)
  {
    Grabbable = DistanceGrabDetector->GetGrabCandidate(EIsdkGrabInputMethod::Unknown);
  }
  else if (CurrentGrabDetector == DistanceGrabDetector)
  {
    Grabbable = GrabbedComponent;
  }

  const bool bShouldDisplayDistanceGrabVisual = IsValid(Grabbable);

  // Either create a visual, destroy a visual, or continue in whatever state we're already in
  if (bShouldDisplayDistanceGrabVisual && !IsValid(DistanceGrabNiagaraComponent))
  {
    DistanceGrabNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
        DistanceGrabNiagaraSystem,
        this,
        NAME_None,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::Type::SnapToTarget,
        true);
  }
  else if (!bShouldDisplayDistanceGrabVisual && IsValid(DistanceGrabNiagaraComponent))
  {
    DistanceGrabNiagaraComponent->DestroyInstance();
    DistanceGrabNiagaraComponent->DestroyComponent();
    DistanceGrabNiagaraComponent = nullptr;
  }

  // If we're not displaying anything, all done here
  if (!bShouldDisplayDistanceGrabVisual || !IsValid(DistanceGrabNiagaraComponent))
  {
    return;
  }

  // Calculate some helpful values to pass along to the niagara system
  FTransform PointerTransform = FTransform::Identity;
  GetPointerTransform(PointerTransform);
  const auto GrabbableVector =
      (Grabbable->GetComponentLocation() - PointerTransform.GetLocation()).GetSafeNormal();
  const USceneComponent* GrabTarget = nullptr;
  FVector GrabTargetLocation = FVector::ZeroVector;
  FVector GrabTargetVector = FVector::ZeroVector;
  if (IsValid(Grabbable->GetGrabTransformer()))
  {
    GrabTarget = Grabbable->GetGrabTransformer()->GetTransformTarget();
  }
  if (IsValid(GrabTarget))
  {
    GrabTargetLocation = GrabTarget->GetComponentLocation();
    GrabTargetVector =
        (GrabTarget->GetComponentLocation() - PointerTransform.GetLocation()).GetSafeNormal();
  }

  // Update niagara system parameters so that the system can display itself correctly
  DistanceGrabNiagaraComponent->SetVectorParameter(
      FName("User.GrabLocation"), PointerTransform.GetLocation());
  DistanceGrabNiagaraComponent->SetVectorParameter(
      FName("User.GrabVector"), PointerTransform.GetRotation().GetForwardVector());
  DistanceGrabNiagaraComponent->SetVectorParameter(
      FName("User.GrabTargetLocation"), GrabTargetLocation);
  DistanceGrabNiagaraComponent->SetVectorParameter(
      FName("User.GrabTargetVector"), GrabTargetVector);
  DistanceGrabNiagaraComponent->SetVectorParameter(
      FName("User.GrabbableLocation"), Grabbable->GetComponentLocation());
  DistanceGrabNiagaraComponent->SetVectorParameter(FName("User.GrabbableVector"), GrabbableVector);
  DistanceGrabNiagaraComponent->SetBoolParameter(FName("User.IsGrabbing"), IsGrabbing());
  DistanceGrabNiagaraComponent->SetBoolParameter(
      FName("User.IsGrabTargetValid"), IsValid(GrabTarget));
  DistanceGrabNiagaraComponent->SetFloatParameter(FName("User.PinchStrength"), PinchStrength);
  DistanceGrabNiagaraComponent->SetBoolParameter(
      FName("User.IsPinching"), IsInputMethodActive(EIsdkGrabInputMethod::Pinch));
}

void UIsdkGrabberComponent::UpdateHoveredInteractor()
{
  // If we're grabbing, we aren't hovering
  if (IsValid(CurrentGrabDetector))
  {
    CurrentHoverDetector = nullptr;
    return;
  }

  // Prioritize Hand > Ray > Distance
  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::HandGrab) &&
      !HandGrabDetector->GetHoveredGrabbables().IsEmpty())
  {
    CurrentHoverDetector = HandGrabDetector;
  }
  else if (
      IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::RayGrab) &&
      IsValid(RayGrabDetector->GetHoveredGrabbable()))
  {
    CurrentHoverDetector = RayGrabDetector;
  }
  else if (
      IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::DistanceGrab) &&
      !DistanceGrabDetector->GetHoveredGrabbables().IsEmpty())
  {
    CurrentHoverDetector = DistanceGrabDetector;
  }
}

FIsdkInteractionRelationshipCounts UIsdkGrabberComponent::GetInteractorStateRelationshipCounts()
    const
{
  const int SelectCount = IsValid(GrabbedComponent) ? 1 : 0;
  return {HoveredGrabbables.Num() - SelectCount, SelectCount};
}

void UIsdkGrabberComponent::GetInteractorStateRelationships(
    EIsdkInteractableState InState,
    TArray<TScriptInterface<IIsdkIInteractableState>>& OutInteractables) const
{
  if (InState == EIsdkInteractableState::Select)
  {
    OutInteractables.Add(GrabbedComponent);
  }
  else if (InState == EIsdkInteractableState::Hover)
  {
    for (UIsdkGrabbableComponent* HoveredGrabbable : HoveredGrabbables)
    {
      if (HoveredGrabbable != GrabbedComponent)
      {
        OutInteractables.Add(HoveredGrabbable);
      }
    }
  }
}

UIsdkHandGrabDetector* UIsdkGrabberComponent::GetHandGrabDetector() const
{
  return HandGrabDetector;
}

UIsdkRayGrabDetector* UIsdkGrabberComponent::GetRayGrabDetector() const
{
  return RayGrabDetector;
}

UIsdkDistanceGrabDetector* UIsdkGrabberComponent::GetDistanceGrabDetector() const
{
  return DistanceGrabDetector;
}

bool UIsdkGrabberComponent::GetPointerTransform(FTransform& OutTransform) const
{
  if (!IsValid(HandPointerPose.GetObject()))
  {
    return false;
  }

  bool bIsValid;
  IIsdkIHandPointerPose::Execute_GetPointerPose(
      HandPointerPose.GetObject(), OutTransform, bIsValid);
  return bIsValid;
}

void UIsdkGrabberComponent::SetHandPointerPose(
    const TScriptInterface<IIsdkIHandPointerPose>& InHandPointerPose)
{
  HandPointerPose = InHandPointerPose;
}

void UIsdkGrabberComponent::GetGrabberTransform(FTransform& OutTransform) const
{
  if (InitialInputMethodForCurrentGrab == EIsdkGrabInputMethod::Pinch)
  {
    GetPinchGrabTransform(OutTransform);
    return;
  }

  if (InitialInputMethodForCurrentGrab == EIsdkGrabInputMethod::Palm)
  {
    GetPalmGrabTransform(OutTransform);
    return;
  }

  OutTransform = GetComponentTransform();
}

void UIsdkGrabberComponent::GetPinchGrabTransform(FTransform& OutTransform) const
{
  if (!HandGrabDetector->GetPinchCollider())
  {
    return;
  }
  OutTransform = HandGrabDetector->GetPinchCollider()->GetComponentTransform();
}

void UIsdkGrabberComponent::GetPalmGrabTransform(FTransform& OutTransform) const
{
  if (!HandGrabDetector->GetPalmGrabCollider())
  {
    return;
  }
  OutTransform = HandGrabDetector->GetPalmGrabCollider()->GetComponentTransform();
}

void UIsdkGrabberComponent::ClearActiveInputMethods()
{
  ActiveInputMethodFlags = EIsdkGrabInputMethod::Unknown;
}

void UIsdkGrabberComponent::GrabByInputMethod(EIsdkGrabInputMethod InputMethod)
{
  if (!IsActive())
  {
    return;
  }

  // We should ignore any attempts to grab with a currently active input method
  if (IsInputMethodActive(InputMethod))
  {
    return;
  }

  // If we're already grabbing something with this grabber, we should just update active input
  // methods and return.
  if (IsValid(GrabbedComponent))
  {
    // Only update the input method if it is allowed
    if (GrabbedComponent->IsGrabInputMethodAllowed(InputMethod))
    {
      UE_VLOG(
          GetOwner(),
          LogOculusInteraction,
          Log,
          TEXT(
              "Activated input method \"%s\" for Grabber \"%s\" , which was already grabbing \"%s\""),
          *StaticEnum<EIsdkGrabInputMethod>()->GetNameStringByValue(
              static_cast<int64>(InputMethod)),
          *GetNameSafe(this),
          *GetNameSafe(GrabbedComponent));
      ActiveInputMethodFlags |= InputMethod;
    }
    return;
  }

  // If there's nothing hovered, there's nothing to grab
  if (!IsValid(CurrentHoverDetector))
  {
    return;
  }

  // Get the grabbable component belonging to the hovered component
  UIsdkGrabbableComponent* GrabCandidate = CurrentHoverDetector->GetGrabCandidate(InputMethod);
  if (!IsValid(GrabCandidate))
  {
    return;
  }

  if (!GrabCandidate->IsGrabInputMethodAllowed(InputMethod))
  {
    return;
  }

  // Don't consider this a grab if we're already grabbing something
  // and the behavior is set to SingleGrabFirstRetained
  const auto GrabTransformer = GrabCandidate->GetGrabTransformer();
  if (IsValid(GrabTransformer))
  {
    const auto MultiGrabBehavior = GrabTransformer->GetMultiGrabBehavior();
    const auto NumGrabbers = GrabTransformer->GetNumGrabbers();
    if (NumGrabbers >= 1 && MultiGrabBehavior == EIsdkMultiGrabBehavior::SingleGrabFirstRetained)
    {
      return;
    }
  }

  // Transition from hover to grab
  CurrentGrabDetector = CurrentHoverDetector;
  CurrentHoverDetector = nullptr;

  // Store some info about what we're grabbing
  CurrentGrabDetectorType = CurrentGrabDetector->GetGrabDetectorType();
  GrabbedComponent = GrabCandidate;
  InitialInputMethodForCurrentGrab = InputMethod;
  ActiveInputMethodFlags |= InputMethod;

  UpdateInteractorState();

  // Let the interactor know we're grabbing
  CurrentGrabDetector->Select(InputMethod);

  const bool bIsHandGrabbing = CurrentGrabDetectorType == EIsdkGrabDetectorType::HandGrab;

  // When we grab something, we have some special handling to do in the case of multiple grabbers
  // Much of this logic is coupled to a grab transformer, which is the thing keeping track of how
  // many things are grabbing it.
  if (IsValid(GrabTransformer))
  {
    // Handle multiple grabs
    if (GrabTransformer->GetNumGrabbers() >= 1)
    {
      const auto MultiGrabBehavior = GrabTransformer->GetMultiGrabBehavior();
      if (MultiGrabBehavior == EIsdkMultiGrabBehavior::SingleGrabTransferToSecond)
      {
        // Cancel all current pointer events so only the new event is used
        GrabbedComponent->GetGrabTransformer()->ForceCancel();
      }
      else if (MultiGrabBehavior == EIsdkMultiGrabBehavior::MultiGrab && !bIsHandGrabbing)
      {
        // Multiple grabs currently only supported with hand grab.  Otherwise, we effectively behave
        // as if set to SingleGrabTransferToSecond.
        GrabbedComponent->GetGrabTransformer()->ForceCancel();
      }
    }

    // Listen for other grabbers triggering a cancel event (in which case, this grabber needs to
    // stop grabbing)
    GrabTransformer->GetCancelGrabEventDelegate()->AddUniqueDynamic(
        this, &UIsdkGrabberComponent::HandleGrabbableCancelEvent);

    // Currently, grab motion only works for non-hand grabs
    if (!bIsHandGrabbing)
    {
      CurrentGrabMotion = GrabTransformer->GetDistanceGrabMotion();
      if (IsValid(CurrentGrabMotion))
      {
        CurrentGrabMotion->Start(GrabbedComponent, this, GrabTransformer);
      }
    }
  }

  UE_VLOG(
      GetOwner(),
      LogOculusInteraction,
      Log,
      TEXT("Grabber \"%s\" grabbed \"%s\" by input method \"%s\""),
      *GetNameSafe(this),
      *GetNameSafe(GrabbedComponent),
      *StaticEnum<EIsdkGrabInputMethod>()->GetNameStringByValue(static_cast<int64>(InputMethod)));

  // If we got here, we're grabbing, and we should send a select pointer event to the grabbable
  PostEvent(EIsdkPointerEventType::Select, GrabbedComponent);
}

void UIsdkGrabberComponent::ReleaseByInputMethod(EIsdkGrabInputMethod InputMethod)
{
  if (!IsActive())
  {
    return;
  }

  // Negate the given input method
  ActiveInputMethodFlags &= ~InputMethod;

  // If we're still grabbing after the negation, another input method is active and we shouldn't
  // proceed to unselect
  if (IsAnyInputMethodActive())
  {
    return;
  }

  // If we aren't grabbing anything at all, escape
  if (!IsValid(GrabbedComponent))
  {
    return;
  }

  // At this point we can post the unselect pointer event
  PostEvent(EIsdkPointerEventType::Unselect, GrabbedComponent);

  // Stop listening to cancel events
  const auto GrabTransformer = GrabbedComponent->GetGrabTransformer();
  if (IsValid(GrabTransformer))
  {
    GrabTransformer->GetCancelGrabEventDelegate()->RemoveDynamic(
        this, &UIsdkGrabberComponent::HandleGrabbableCancelEvent);
  }

  ResetGrabState();
}

void UIsdkGrabberComponent::PostEvent(EIsdkPointerEventType Type, UIsdkGrabbableComponent* Dest)
{
  if (!IsValid(Dest))
  {
    return;
  }

  FTransform GrabTransform;
  if (IsValid(CurrentGrabMotion) && CurrentGrabMotion->IsActive())
  {
    // If we're distance/ray grabbing, use motion object to drive the grab transform
    GrabTransform = CurrentGrabMotion->GetTransform();
  }
  else if (Type == EIsdkPointerEventType::Hover || Type == EIsdkPointerEventType::Unhover)
  {
    // If we're hovering or unhovering,
    GrabTransform = Dest->GetComponentTransform();
  }
  else
  {
    // Else use the appropriate collider
    GetGrabberTransform(GrabTransform);
  }

  FIsdkInteractionPointerEvent Evt;
  Evt.Type = Type;
  Evt.Pose.Orientation = GrabTransform.GetRotation();
  Evt.Pose.Position = static_cast<FVector>(GrabTransform.GetLocation());
  Evt.Interactor = this;
  Evt.Interactable = Dest;
  Evt.Identifier = GetID();
  Dest->PostEvent(Evt);
}

UIsdkGrabbableComponent* UIsdkGrabberComponent::GetGrabbedComponent() const
{
  return GrabbedComponent;
}

bool UIsdkGrabberComponent::IsGrabInputMethodAllowed(EIsdkGrabInputMethod InputMethod) const
{
  return AllowedInputMethods.Contains(InputMethod);
}

void UIsdkGrabberComponent::SetGrabInputMethodAllowed(
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

bool UIsdkGrabberComponent::IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType GrabType) const
{
  return AllowedGrabDetectors.Contains(GrabType);
}

void UIsdkGrabberComponent::SetGrabDetectionTypeAllowed(
    EIsdkGrabDetectorType GrabType,
    bool Allowed)
{
  const bool CurrentlyAllowed = AllowedGrabDetectors.Contains(GrabType);

  if (CurrentlyAllowed != Allowed)
  {
    if (!Allowed)
      AllowedGrabDetectors.Remove(GrabType);
    else
      AllowedGrabDetectors.Add(GrabType);
  }
}

void UIsdkGrabberComponent::DrawDebugVisuals() const
{
  // We may have a multiple grabbers attached for hands/controllers... we'll only debug the
  // active ones to reduce noise
  if (!IsActive())
  {
    return;
  }

  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::HandGrab))
  {
    HandGrabDetector->DrawDebugVisuals();
  }
  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::DistanceGrab))
  {
    DistanceGrabDetector->DrawDebugVisuals();
  }
  if (IsGrabDetectionTypeAllowed(EIsdkGrabDetectorType::RayGrab))
  {
    RayGrabDetector->DrawDebugVisuals();
  }
}

void UIsdkGrabberComponent::HandleGrabbableCancelEvent(
    int InteractorID,
    UIsdkGrabTransformerComponent* InGrabTransformerComponent)
{
  if (InteractorID == ID && IsGrabbing())
  {
    ClearActiveInputMethods();
    ReleaseByInputMethod(EIsdkGrabInputMethod::Unknown);
  }
}

void UIsdkGrabberComponent::ResetGrabState()
{
  // Clean up any grab motion if it was active
  if (IsValid(CurrentGrabMotion))
  {
    CurrentGrabMotion->Stop();
    CurrentGrabMotion = nullptr;
  }

  // Clean up the grab interactor
  if (IsValid(CurrentGrabDetector))
  {
    CurrentGrabDetector->Unselect();
    CurrentGrabDetector = nullptr;
  }

  // Update info related to our current grab state
  HoveredGrabbables.Remove(GrabbedComponent);
  GrabbedComponent = nullptr;
  InitialInputMethodForCurrentGrab = EIsdkGrabInputMethod::Unknown;
  ActiveInputMethodFlags = EIsdkGrabInputMethod::Unknown;
  UpdateInteractorState();
}

void UIsdkGrabberComponent::SetInteractorSnap(FTransform& TransformIn, float LerpRate)
{
  if (IsValid(GrabbedComponent))
  {
    const auto GrabTransformer = GrabbedComponent->GetGrabTransformer();
    if (IsValid(GrabTransformer))
    {
      GrabTransformer->SetInteractorSnap(TransformIn, LerpRate);
    }
  }
}
