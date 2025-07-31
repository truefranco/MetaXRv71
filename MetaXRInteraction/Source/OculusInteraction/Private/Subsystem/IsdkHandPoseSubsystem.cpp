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

#include "Subsystem/IsdkHandPoseSubsystem.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/UObjectIterator.h"
#include "IsdkHandMeshComponent.h"
#include "IsdkFunctionLibrary.h"
#include "Interaction/Grabbable/IsdkHandGrabPose.h"
#include "OculusInteractionLog.h"
#include "IsdkHandData.h"
#include "IsdkHandPoseData.h"
#include "Utilities/IsdkXRUtils.h"
#include "IsdkChecks.h"
#include "Runtime/Launch/Resources/Version.h"

namespace isdk
{
TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_HandGrabPoses_DebugPoseTransforms(
    TEXT("Meta.InteractionSDK.HandGrabPoses.DebugPoseTransforms"),
    false,
    TEXT("Draws debug information for hand grab pose transforms at runtime"));

TAutoConsoleVariable<bool> CVar_Meta_InteractionSDK_HandGrabPoses_DebugPoseVectors(
    TEXT("Meta.InteractionSDK.HandGrabPoses.DebugPoseVectors"),
    false,
    TEXT("Draws debug information for hand grab pose vectors at runtime"));
} // namespace isdk
// namespace isdk

UIsdkHandPoseSubsystem::UIsdkHandPoseSubsystem()
{
  static FStaticObjectFinders StaticObjectFinders;
  HandMeshMaterial = StaticObjectFinders.HandMaterialFinder.Get();
  HandMeshRight = StaticObjectFinders.RightHandMeshFinder.Get();
  HandMeshLeft = StaticObjectFinders.LeftHandMeshFinder.Get();
}

void UIsdkHandPoseSubsystem::BeginDestroy()
{
  Super::BeginDestroy();
}

void UIsdkHandPoseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
  Super::Initialize(Collection);
}

void UIsdkHandPoseSubsystem::Tick(float InDeltaTime)
{
  Super::Tick(InDeltaTime);

  // Check if we have any hand grab poses to destroy
  if (HandGrabPoseDestroyQueue.Num() > 0)
  {
    for (int i = 0; i < HandGrabPoseDestroyQueue.Num(); i++)
    {
      USceneComponent* ComponentToDestroy = HandGrabPoseDestroyQueue[i];

      if (ComponentToDestroy->IsRegistered())
      {
        ComponentToDestroy->UnregisterComponent();
      }
      ComponentToDestroy->DestroyComponent();

#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5)
      HandGrabPoseDestroyQueue.RemoveAt(0, 1, EAllowShrinking::Yes);
#else
      HandGrabPoseDestroyQueue.RemoveAt(0, 1, true);
#endif

      i--;
    }
  }

  // Handle Pose Transform Debug Drawing when enabled (this is not currently intended to be
  // performant)
  if (isdk::CVar_Meta_InteractionSDK_HandGrabPoses_DebugPoseTransforms.GetValueOnAnyThread())
  {
    constexpr auto Depth = ESceneDepthPriorityGroup::SDPG_Foreground;
    for (TObjectIterator<AActor> Itr; Itr; ++Itr)
    {
      AActor* ThisActor = *Itr;
      if (IsValid(ThisActor) && ThisActor->HasActorBegunPlay())
      {
		  const FName ActorClassName = FName(*ThisActor->GetClass()->GetPackage()->GetName());
        // We only debug actors we have cached poses for

        if (ActorPoseCacheMap.Contains(ActorClassName))
        {
          if (ActorPoseCacheMap[ActorClassName].InstanceVariations.Contains(ThisActor))
          {
            DrawDebugCoordinateSystem(
                GetWorld(),
                ThisActor->GetActorLocation(),
                ThisActor->GetActorRotation(),
                1.f,
                false,
                0.0,
                Depth,
                0.1f);
            const FTransform ActorTransform = ThisActor->GetActorTransform();
            for (uint32 VariationIdx :
                 ActorPoseCacheMap[ActorClassName].InstanceVariations[ThisActor].Variations)
            {
              const FTransform ThisTransform =
                  ActorPoseCacheMap[ActorClassName].PoseDataCache.CachedTransforms[VariationIdx];

              FTransform RotatedGrabTransform;
              RotatedGrabTransform.SetRotation(
                  ActorTransform.GetRotation() * ThisTransform.GetRotation());
              RotatedGrabTransform.SetScale3D(ThisTransform.GetScale3D());
              FVector UpdatedGrabLoc =
                  ActorTransform.GetRotation().RotateVector(ThisTransform.GetLocation());
              RotatedGrabTransform.SetLocation(ActorTransform.GetLocation() + UpdatedGrabLoc);

              const FVector Origin = RotatedGrabTransform.GetLocation();

              DrawDebugCoordinateSystem(
                  GetWorld(),
                  Origin,
                  RotatedGrabTransform.GetRotation().Rotator(),
                  1.f,
                  false,
                  0.0,
                  Depth,
                  0.1f);
            }
          }
        }
      }
    }
  }
}

ETickableTickType UIsdkHandPoseSubsystem::GetTickableTickType() const
{
  return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
}

TStatId UIsdkHandPoseSubsystem::GetStatId() const
{
  RETURN_QUICK_DECLARE_CYCLE_STAT(IsdkHandPoseSubsystem, STATGROUP_Tickables);
}

USkeletalMesh* UIsdkHandPoseSubsystem::GetHandMesh(EIsdkHandedness& Handedness)
{
  if (Handedness == EIsdkHandedness::Left)
  {
    return HandMeshLeft;
  }
  else
  {
    return HandMeshRight;
  }
}

UMaterial* UIsdkHandPoseSubsystem::GetHandMeshMaterial()
{
  return HandMeshMaterial;
}

FName UIsdkHandPoseSubsystem::GetActorClassNameFromComponent(USceneComponent* ComponentIn)
{
  if (!IsValid(ComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT("UIsdkHandPoseSubsystem::GetActorClassNameFromComponent - ComponentIn was invalid!"));
    return NAME_None;
  }
  AActor* OwningActor = ComponentIn->GetOwner();
  if (!IsValid(OwningActor))
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT("UIsdkHandPoseSubsystem::GetActorClassNameFromComponent - OwningActor was invalid!"));
    return NAME_None;
  }
  
  return FName(*OwningActor->GetClass()->GetPackage()->GetName());
}

void UIsdkHandPoseSubsystem::RegisterHandPoseData(
    UIsdkHandGrabPose* HandGrabPoseIn,
    bool bDestroyAfterRegistration)
{
  if (!IsValid(HandGrabPoseIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::RegisterHandPoseData - HandGrabPoseIn was invalid!"));
    return;
  }

  AActor* OwningActor = HandGrabPoseIn->GetOwner();
  if (!IsValid(OwningActor))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::RegisterHandPoseData - Owning Actor was invalid!"));
    return;
  }

  const FName OuterActorClassName = GetActorClassNameFromComponent(HandGrabPoseIn);

  if (OuterActorClassName == "")
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT("UIsdkHandPoseSubsystem::RegisterHandPoseData - Null Class Name from Outer Actor!"));
    return;
  }

  if (IsValid(HandGrabPoseIn) && !HandGrabPoseIn->bPoseDisabled)
  {
    const FName HandGrabPoseName = HandGrabPoseIn->GetFName();

    // Check to see if we already have this data cached for this actor class
    int32 FoundVariation = -1;
    if (!ShouldCreateNewActorPoseVariation(HandGrabPoseIn, FoundVariation))
    {
      AddExistingVariationToActorCacheMap(OuterActorClassName, OwningActor, FoundVariation);
    }
    else
    {
      // If not, let's make a new variation and cache it
      UIsdkHandPoseData* OriginalHandPoseData = HandGrabPoseIn->HandPoseData;
      if (!UIsdkChecks::ValidateDependency(
              OriginalHandPoseData,
              HandGrabPoseIn,
              TEXT("HandPoseData"),
              ANSI_TO_TCHAR(__FUNCTION__),
              nullptr))
      {
        return;
      }

      uint32 NewVariationIndex = -1;
      uint32 NewVariationMirrorIndex = -1;

      FIsdkHandPoseDataGroup NewHandPoseDataGroup;
      const EIsdkHandedness HandPoseHandedness = OriginalHandPoseData->Handedness;
      NewHandPoseDataGroup.CachedHandPoses.Add(HandPoseHandedness, OriginalHandPoseData);

      // Check if we need to automatically generate a mirrored version
      if (HandGrabPoseIn->HandGrabPoseProperties.MirroringMode ==
          EIsdkHandGrabPoseMirror::Automatic)
      {
        UIsdkHandPoseData* MirroredHandPoseData = nullptr;
        if (GenerateMirroredHandPoseData(OriginalHandPoseData, MirroredHandPoseData))
        {
          // If we're mirroring the location, make a separate cached entry for the transform
          if (HandGrabPoseIn->HandGrabPoseProperties.bMirrorLocationAndRotation)
          {
            FTransform NewTransform = HandGrabPoseIn->GetRelativeTransform();
            NewTransform.Mirror(HandGrabPoseIn->HandGrabPoseProperties.MirrorAxis, EAxis::Type::X);

            FIsdkHandGrabPoseProperties NewMirroredProperties;
            FIsdkHandPoseDataGroup NewMirroredHandPoseDataGroup;
            const EIsdkHandedness MirroredPoseHandedness = MirroredHandPoseData->Handedness;
            NewMirroredHandPoseDataGroup.CachedHandPoses.Add(
                MirroredPoseHandedness, MirroredHandPoseData);

            NewVariationMirrorIndex = AddNewVariationToActorCacheMap(
                OuterActorClassName,
                OwningActor,
                NewTransform,
                NewMirroredHandPoseDataGroup,
                MirroredHandPoseData->GetFName(),
                HandGrabPoseIn->HandGrabPoseProperties);
          }
          // Otherwise, just add it as a handedness option for this transform
          else
          {
            NewHandPoseDataGroup.CachedHandPoses.Add(
                MirroredHandPoseData->Handedness, MirroredHandPoseData);
          }
        }
      }
      // If we're manual, go find the manual counterpart and cache it separately
      else if (
          HandGrabPoseIn->HandGrabPoseProperties.MirroringMode == EIsdkHandGrabPoseMirror::Manual)
      {
        UIsdkHandGrabPose* MirroredReferenceGrabPose = nullptr;
        bool bMirrorReferenceFound = false;
        TArray<USceneComponent*> Children;
        HandGrabPoseIn->GetChildrenComponents(true, Children);
        for (USceneComponent*& ThisComponent : Children)
        {
          UIsdkHandGrabPose* AsHandGrabPose = Cast<UIsdkHandGrabPose>(ThisComponent);
          if (IsValid(AsHandGrabPose))
          {
            if (bMirrorReferenceFound)
            {
              UE_LOG(
                  LogOculusInteraction,
                  Warning,
                  TEXT(
                      "UIsdkHandPoseSubsystem::RegisterHandPoseData() - Found multiple Mirror References for %s! Only first one will be used."),
                  *HandGrabPoseIn->GetName());
              break;
            }
            else
            {
              if (AsHandGrabPose->HandGrabPoseProperties.PoseMode ==
                  EIsdkHandGrabPoseMode::MirrorReference)
              {
                bMirrorReferenceFound = true;
                MirroredReferenceGrabPose = AsHandGrabPose;
              }
            }
          }
        }

        if (IsValid(MirroredReferenceGrabPose))
        {
          UIsdkHandPoseData* MirrorHandPoseData = nullptr;
          if (GenerateMirroredHandPoseData(OriginalHandPoseData, MirrorHandPoseData))
          {
            FIsdkHandPoseDataGroup NewMirroredHandPoseDataGroup;
            const EIsdkHandedness MirroredPoseHandedness = MirrorHandPoseData->Handedness;
            NewMirroredHandPoseDataGroup.CachedHandPoses.Add(
                MirroredPoseHandedness, MirrorHandPoseData);

            NewVariationMirrorIndex = AddNewVariationToActorCacheMap(
                OuterActorClassName,
                OwningActor,
                (MirroredReferenceGrabPose->GetRelativeTransform() *
                 HandGrabPoseIn->GetRelativeTransform()),
                NewMirroredHandPoseDataGroup,
                MirrorHandPoseData->GetFName(),
                HandGrabPoseIn->HandGrabPoseProperties);

            // Add this component to be destroyed
            if (bDestroyAfterRegistration)
            {
              HandGrabPoseDestroyQueue.Add(MirroredReferenceGrabPose);
            }
          }
        }
        else
        {
          UE_LOG(
              LogOculusInteraction,
              Error,
              TEXT(
                  "UIsdkHandPoseSubsystem::RegisterHandPoseData - MirrorHandGrabPose was invalid!"));
        }
      }
      NewVariationIndex = AddNewVariationToActorCacheMap(
          OuterActorClassName,
          OwningActor,
          HandGrabPoseIn->GetRelativeTransform(),
          NewHandPoseDataGroup,
          HandGrabPoseName,
          HandGrabPoseIn->HandGrabPoseProperties);

      if (NewVariationIndex != -1 && NewVariationMirrorIndex != -1)
      {
        // Cache the relationship between the base pose and the mirrored version
        ActorPoseCacheMap[OuterActorClassName].OriginalMirrorVariationMap.Add(
            NewVariationIndex, NewVariationMirrorIndex);
      }
    }
  }

  // Add this component to be destroyed
  if (bDestroyAfterRegistration)
  {
    HandGrabPoseDestroyQueue.Add(HandGrabPoseIn);
  }
}

bool UIsdkHandPoseSubsystem::CheckForHandPose(
    USceneComponent* InteractableIn,
    UIsdkHandMeshComponent* InteractingHandIn,
    const FIsdkInteractorStateEvent& StateEventIn,
    UIsdkHandPoseData*& HandPoseOut,
    FIsdkHandGrabPoseProperties& GrabPosePropertiesOut,
    FTransform& RootOffsetOut)
{
  if (!IsValid(InteractableIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT("UIsdkHandPoseSubsystem::CheckForHandPose - InteractableIn was invalid!."));
    return false;
  }
  if (!IsValid(InteractingHandIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT("UIsdkHandPoseSubsystem::CheckForHandPose - InteractingHandIn was invalid!."));
    return false;
  }

  EIsdkHandedness IncomingHandedness;
  InteractingHandIn->GetHandednessFromDataSource(IncomingHandedness);

  AActor* OuterActor = InteractableIn->GetOwner();
  if (!IsValid(OuterActor))
  {
    return false;
  }
  const FName OuterActorName = GetActorClassNameFromComponent(InteractableIn);

  // Find the PoseDataCache for this base blueprint class
  if (ActorPoseCacheMap.Contains(OuterActorName))
  {
    const FIsdkHandPoseDataActorCache ActorPoseDataCache = *ActorPoseCacheMap.Find(OuterActorName);

    // Find the specific variations for this AActor
    if (ActorPoseDataCache.InstanceVariations.Contains(OuterActor))
    {
      const FIsdkHandPoseDataActorVariations ActorVariations =
          ActorPoseDataCache.InstanceVariations[OuterActor];
      if (ActorVariations.Variations.Num() > 0)
      {
        int32 CurrentWinningIdx = -1;
        float LastWinningLocationRotationDelta = FLT_MAX;
        const FTransform ActorTransform =
            InteractableIn->GetOwner()->GetActorTransform();
        const FTransform HandTransform = InteractingHandIn->GetComponentTransform();
        const FTransform HandWorldTransform = InteractingHandIn->GetComponentToWorld();
        const FIsdkHandPoseDataCache PoseDataCache = ActorPoseDataCache.PoseDataCache;

        if (isdk::CVar_Meta_InteractionSDK_HandGrabPoses_DebugPoseVectors.GetValueOnAnyThread())
        {
          FVector HandForwardZ = HandTransform.GetUnitAxis(EAxis::Z);
          HandForwardZ.Normalize(1.f);
          DrawDebugDirectionalArrow(
              GetWorld(),
              HandWorldTransform.GetLocation(),
              HandWorldTransform.GetLocation() + (HandForwardZ * 15.f),
              5.f,
              FColor::Green,
              false,
              5.f,
              0,
              0.5f);
        }

        // The handpose with the smallest location delta + the smallest rotation delta will win
        FTransform ThisGrabTransform;

        // Iterate through all variations that have been registered for this specific actor
        for (uint32 VariationIdx : ActorVariations.Variations)
        {
          if (!PoseDataCache.CachedHandPoseGroups.IsValidIndex(VariationIdx))
          {
            continue;
          }

          // If the pose group doesn't have our handedness, move on
          if (!PoseDataCache.CachedHandPoseGroups[VariationIdx].CachedHandPoses.Contains(
                  IncomingHandedness))
          {
            continue;
          }
          // Properly position the grab's transform in world space
          ThisGrabTransform.SetRotation(
              ActorTransform.GetRotation() *
              PoseDataCache.CachedTransforms[VariationIdx].GetRotation());
          ThisGrabTransform.SetScale3D(PoseDataCache.CachedTransforms[VariationIdx].GetScale3D());
          FVector UpdatedGrabLoc = ActorTransform.GetRotation().RotateVector(
              PoseDataCache.CachedTransforms[VariationIdx].GetLocation());
          ThisGrabTransform.SetLocation(ActorTransform.GetLocation() + UpdatedGrabLoc);

          const FVector LocationDeltaVector =
              (HandTransform.GetLocation() - ThisGrabTransform.GetLocation());
          const FRotator RotationDeltaRotator = HandWorldTransform.GetRotation().Rotator() -
              ThisGrabTransform.GetRotation().Rotator();
          const float RotationDelta = FMath::Abs(RotationDeltaRotator.Yaw) +
              FMath::Abs(RotationDeltaRotator.Pitch) + FMath::Abs(RotationDeltaRotator.Roll);

          const float FinalDelta = LocationDeltaVector.SizeSquared() + RotationDelta;

          if (FinalDelta < LastWinningLocationRotationDelta)
          {
            LastWinningLocationRotationDelta = FinalDelta;
            CurrentWinningIdx = VariationIdx;
            RootOffsetOut = PoseDataCache.CachedTransforms[VariationIdx];
            GrabPosePropertiesOut = PoseDataCache.CachedProperties[VariationIdx];
          }
        }

        if (CurrentWinningIdx >= 0 &&
            PoseDataCache.CachedHandPoseGroups.IsValidIndex(CurrentWinningIdx))
        {
          HandPoseOut = PoseDataCache.CachedHandPoseGroups[CurrentWinningIdx]
                            .CachedHandPoses[IncomingHandedness];

          if (!UIsdkChecks::ValidateDependency(
                  HandPoseOut,
                  this,
                  TEXT("Selected Hand Pose"),
                  ANSI_TO_TCHAR(__FUNCTION__),
                  nullptr))
          {
            return false;
          }
          return true;
        }
      }
    }
  }
  return false;
}

bool UIsdkHandPoseSubsystem::GenerateMirroredHandPoseData(
    UIsdkHandPoseData* PreviousHandData,
    UIsdkHandPoseData*& MirroredHandData)
{
  if (!UIsdkChecks::ValidateDependency(
          PreviousHandData, this, TEXT("Previous Hand Data"), ANSI_TO_TCHAR(__FUNCTION__), nullptr))
  {
    return false;
  }

  const FString MirroredPoseName =
      PreviousHandData->GetName() + "_Mirror_" + FString::FromInt(LastHandPoseMirroredSuffix);
  LastHandPoseMirroredSuffix += 1;

  MirroredHandData = NewObject<UIsdkHandPoseData>(this, FName(MirroredPoseName));
  MirroredHandData->Handedness = PreviousHandData->Handedness == EIsdkHandedness::Left
      ? EIsdkHandedness::Right
      : EIsdkHandedness::Left;

  MirroredHandData->bIsMirrored = true;
  MirroredHandData->SetPoseLerpTime(PreviousHandData->GetPoseLerpTime());

  // Set a reference to the Hand Joint Poses we'll be setting in our new Hand Pose Data
  TArray<FTransform>& TargetHandJointPoses = MirroredHandData->HandData->GetJointPoses();

  const UIsdkHandData* SourceHandData = PreviousHandData->Execute_GetHandData(PreviousHandData);
  const TArray<FTransform>& SourceJointPoses = SourceHandData->GetJointPoses();

  for (int BoneIndex = 0; BoneIndex < static_cast<int>(EIsdkHandBones::EHandBones_MAX); ++BoneIndex)
  {
    TargetHandJointPoses[BoneIndex] =
        UIsdkFunctionLibrary::MirrorHandTransform(SourceJointPoses[BoneIndex]);
  }

  return IsValid(MirroredHandData);
}

bool UIsdkHandPoseSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
  const auto World = Cast<UWorld>(Outer);
  if (!World)
  {
    return false;
  }

  return true;
}

uint32 UIsdkHandPoseSubsystem::AddNewVariationToActorCacheMap(
    FName OuterActorName,
    AActor* ActorIn,
    FTransform TransformIn,
    FIsdkHandPoseDataGroup PoseDataGroupIn,
    FName PoseNameIn,
    FIsdkHandGrabPoseProperties PosePropertiesIn)
{
  // If we're not already tracking this actor class, start doing so
  if (!ActorPoseCacheMap.Contains(OuterActorName))
  {
    FIsdkHandPoseDataActorCache NewActorCache;
    ActorPoseCacheMap.Add(OuterActorName, NewActorCache);
  }

  // If we're not already tracking the specific actor, start doing so
  if (!ActorPoseCacheMap[OuterActorName].InstanceVariations.Contains(ActorIn))
  {
    FIsdkHandPoseDataActorVariations NewVariationsContainer;
    ActorPoseCacheMap[OuterActorName].InstanceVariations.Add(ActorIn, NewVariationsContainer);
  }

  const uint32 NewVariationIdx =
      ActorPoseCacheMap[OuterActorName].PoseDataCache.CachedTransforms.Num();

  // Add it to the PoseDataCache
  ActorPoseCacheMap[OuterActorName].PoseDataCache.CachedTransforms.Add(TransformIn);
  ActorPoseCacheMap[OuterActorName].PoseDataCache.CachedHandPoseGroups.Add(PoseDataGroupIn);
  ActorPoseCacheMap[OuterActorName].PoseDataCache.CachedPoseNames.Add(PoseNameIn);
  ActorPoseCacheMap[OuterActorName].PoseDataCache.CachedProperties.Add(PosePropertiesIn);

  // And record a new variation
  ActorPoseCacheMap[OuterActorName].InstanceVariations[ActorIn].Variations.Add(NewVariationIdx);

  return NewVariationIdx;
}

bool UIsdkHandPoseSubsystem::AddExistingVariationToActorCacheMap(
    FName OuterActorName,
    AActor* ActorIn,
    int32 VariationIndexIn)
{
  // Validate the cache map is tracking this actor already (how else do we have a variation, then?)
  if (!ActorPoseCacheMap.Contains(OuterActorName))
  {
    return false;
  }

  // Check that it's a valid index (with transforms as the canary)
  if (VariationIndexIn < 0 ||
      !ActorPoseCacheMap[OuterActorName].PoseDataCache.CachedTransforms.IsValidIndex(
          VariationIndexIn))
  {
    return false;
  }

  // If we're not tracking this actor, start doing so
  if (!ActorPoseCacheMap[OuterActorName].InstanceVariations.Contains(ActorIn))
  {
    FIsdkHandPoseDataActorVariations NewVariationsContainer;
    ActorPoseCacheMap[OuterActorName].InstanceVariations.Add(ActorIn, NewVariationsContainer);
  }

  ActorPoseCacheMap[OuterActorName].InstanceVariations[ActorIn].Variations.Add(VariationIndexIn);

  // Do we have a mirrored version to bring over as well? If so, automatically add it as well
  if (ActorPoseCacheMap[OuterActorName].OriginalMirrorVariationMap.Contains(VariationIndexIn))
  {
    ActorPoseCacheMap[OuterActorName].InstanceVariations[ActorIn].Variations.Add(
        ActorPoseCacheMap[OuterActorName].OriginalMirrorVariationMap[VariationIndexIn]);
  }
  return true;
}

bool UIsdkHandPoseSubsystem::ShouldCreateNewActorPoseVariation(
    UIsdkHandGrabPose* HandGrabPoseIn,
    int32& VariationMatched)
{
  if (!IsValid(HandGrabPoseIn))
  {
    return true;
  }
  AActor* OwningActor = HandGrabPoseIn->GetOwner();
  if (!IsValid(OwningActor))
  {
    return true;
  }

  const FName OuterActorClassName = GetActorClassNameFromComponent(HandGrabPoseIn);

  // Check if we have the base actor class cached
  if (!ActorPoseCacheMap.Contains(OuterActorClassName))
  {
    return true;
  }
  else
  {
    // Check to see if this pose matches the "signature" of others cached for this actor
    VariationMatched = -1;

    for (int i = 0; i < ActorPoseCacheMap[OuterActorClassName].PoseDataCache.CachedTransforms.Num();
         i++)
    {
      uint32 FoundElements = 0; // bitmask, 1: transform, 2: properties, 4: PoseGroups
      // First make sure we're not dealing with a catastrophe
      if (!(ActorPoseCacheMap[OuterActorClassName].PoseDataCache.CachedProperties.IsValidIndex(i) &&
            ActorPoseCacheMap[OuterActorClassName].PoseDataCache.CachedHandPoseGroups.IsValidIndex(
                i) &&
            ActorPoseCacheMap[OuterActorClassName].PoseDataCache.CachedPoseNames.IsValidIndex(i)))
      {
        UE_LOG(
            LogOculusInteraction,
            Error,
            TEXT(
                "UIsdkHandPoseSubsystem::ShouldAddToPoseCache - PoseDataCache index mismatch for %s!"),
            *OuterActorClassName.ToString());
        ensure(true);
        return true;
      }

      // [1] Transform
      if (ActorPoseCacheMap[OuterActorClassName].PoseDataCache.CachedTransforms[i].Equals(
              HandGrabPoseIn->GetRelativeTransform()))
      {
        FoundElements |= 1;
      }

      // [2] Properties
      if (ActorPoseCacheMap[OuterActorClassName].PoseDataCache.CachedProperties[i] ==
          HandGrabPoseIn->HandGrabPoseProperties)
      {
        FoundElements |= 2;
      }

      // [4] PoseGroups (if we've already cached this hand pose for this handedness)
      for (auto& Elem : ActorPoseCacheMap[OuterActorClassName]
                            .PoseDataCache.CachedHandPoseGroups[i]
                            .CachedHandPoses)
      {
        UIsdkHandPoseData* FoundHandPoseData = Elem.Value;
        if (IsValid(FoundHandPoseData) && FoundHandPoseData == HandGrabPoseIn->HandPoseData &&
            Elem.Key == HandGrabPoseIn->HandPoseData->Handedness)
        {
          FoundElements |= 4;
          break;
        }
      }

      // We found an exact match of the relevant data, we can safely say this doesn't need to be
      // added. Return the index of the match
      if (FoundElements == 7)
      {
        VariationMatched = i;
        return false;
      }
    }
  }
  return true;
}
