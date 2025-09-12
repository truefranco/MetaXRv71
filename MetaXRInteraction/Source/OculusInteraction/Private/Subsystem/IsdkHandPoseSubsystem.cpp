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
#include "HandPoseDetection/IsdkHandFingerRecognizer.h"
#include "HandPoseDetection/IsdkHandThumbRecognizer.h"
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

  // Check for Hand Pose Detections
  DetectRegisteredPoses(InDeltaTime);

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
        const FName ActorClassName = FTopLevelAssetPath(ThisActor->GetClass()).GetPackageName();
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

USkinnedAsset* UIsdkHandPoseSubsystem::GetHandMesh(EIsdkHandedness& Handedness)
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
  AActor* OwningActor = ComponentIn->GetAttachParentActor();
  if (!IsValid(OwningActor))
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT("UIsdkHandPoseSubsystem::GetActorClassNameFromComponent - OwningActor was invalid!"));
    return NAME_None;
  }

  return FTopLevelAssetPath(OwningActor->GetClass()).GetPackageName();
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

  AActor* OwningActor = HandGrabPoseIn->GetAttachParentActor();
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
      UIsdkHandPoseData* OriginalHandPoseData = HandGrabPoseIn->HandPoseData.Get();
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

  AActor* OuterActor = InteractableIn->GetAttachParentActor();
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
            InteractableIn->GetAttachParentActor()->GetActorTransform();
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

          const float FinalDelta = LocationDeltaVector.SquaredLength() + RotationDelta;

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
  AActor* OwningActor = HandGrabPoseIn->GetAttachParentActor();
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

bool UIsdkHandPoseSubsystem::SetHandPoseDetectionProfileFromMesh(
    UIsdkHandPoseDetectionProfile* ProfileIn,
    UIsdkHandMeshComponent* MeshComponentIn,
    float BaseTolerances,
    TArray<EIsdkFingerType> FingersToIgnore,
    bool bIgnoreThumb,
    FName PoseName)
{
  if (!IsValid(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::SetHandPoseDetectionProfileFromMesh - Hand Mesh was invalid!"));
    return false;
  }
  // Register Hand Mesh
  if (!RegisterHandMeshForDetection(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::SetHandPoseDetectionProfileFromMesh - Hand Mesh registration failed!"));
    return false;
  }
  // Get Results
  if (!HandMeshComponentResults.Contains(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::SetHandPoseDetectionProfileFromMesh - Results struct not created for hand mesh!"));
    return false;
  }
  FIsdkHandPoseRecognizerResults& NewResults = HandMeshComponentResults[MeshComponentIn];
  GetHandMeshDetectionResults(MeshComponentIn, NewResults, 0.33f);

  // If no tolerances were set, use defined defaults
  if (BaseTolerances == 0.f)
  {
    BaseTolerances = HandPoseDetectionDigitCalcTolerance;
  }

  ProfileIn->PoseDetectionName = PoseName;
  ProfileIn->ProfileFingerTargets.Empty();

  // Fingers
  for (auto& OuterElem : NewResults.FingersResults)
  {
    const EIsdkFingerType& FingerType = OuterElem.Key;
    const FIsdkHandPoseRecognizerFingerResult& FingerResult = OuterElem.Value;
    if (FingersToIgnore.Contains(FingerType))
    {
      continue;
    }
    FIsdkHandPoseDetectionFingerTarget NewFingerTarget;
    for (auto& InnerElem : FingerResult.FingerResults)
    {
      const EIsdkDetection_FingerCalcType& CalcType = InnerElem.Key;
      const float CalcValue = InnerElem.Value;
      NewFingerTarget.FingerCalcTargets.Add(CalcType, CalcValue);
      NewFingerTarget.FingerCalcTolerances.Add(CalcType, BaseTolerances);
    }
    ProfileIn->ProfileFingerTargets.Add(FingerType, NewFingerTarget);
  }

  // Thumb
  if (!bIgnoreThumb)
  {
    FIsdkHandPoseDetectionThumbTarget NewThumbTarget;
    for (auto& OuterElem : NewResults.ThumbResults)
    {
      const EIsdkDetection_ThumbCalcType& CalcType = OuterElem.Key;
      const float CalcValue = OuterElem.Value;
      NewThumbTarget.ThumbCalcTargets.Add(CalcType, CalcValue);
      NewThumbTarget.ThumbCalcTolerances.Add(CalcType, BaseTolerances);
    }
    ProfileIn->ProfileThumbTarget = NewThumbTarget;
  }

  return true;
}

bool UIsdkHandPoseSubsystem::CreateHandPoseDetectionProfileFromMesh(
    UIsdkHandMeshComponent* MeshComponentIn,
    float BaseTolerances,
    TArray<EIsdkFingerType> FingersToIgnore,
    bool bIgnoreThumb,
    FName PoseName,
    UIsdkHandPoseDetectionProfile*& ProfileOut)
{
  if (!IsValid(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::CreateHandPoseDetectionProfileFromMesh - Hand Mesh was invalid!"));
    return false;
  }
  ProfileOut = NewObject<UIsdkHandPoseDetectionProfile>(this);
  return SetHandPoseDetectionProfileFromMesh(
      ProfileOut, MeshComponentIn, BaseTolerances, FingersToIgnore, bIgnoreThumb, PoseName);
}

bool UIsdkHandPoseSubsystem::RegisterHandPoseDetection(
    UIsdkHandPoseDetectionProfile* ProfileIn,
    UIsdkHandMeshComponent* MeshComponentIn)
{
  if (!IsValid(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::RegisterHandPoseDetection - MeshComponentIn was invalid"));
    return false;
  }

  if (!IsValid(ProfileIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::RegisterHandPoseDetection - ProfileIn was invalid"));
    return false;
  }

  uint32 ProfileIdx = RegisteredDetectionProfiles.IndexOfByKey(ProfileIn);

  if (ProfileIdx == INDEX_NONE)
  {
    RegisteredDetectionProfiles.Add(ProfileIn);
    ProfileIdx = RegisteredDetectionProfiles.Num() - 1;
    // Setup new results struct so we're not making a new one each tick
    FIsdkHandPoseRecognizerResults DeltaResults;
    DetectionProfileDeltaResults.Add(DeltaResults);
  }

  // Map Hand Mesh with Profile if not already done
  if (RegisteredMeshComponentProfileMap.Contains(MeshComponentIn))
  {
    if (!RegisteredMeshComponentProfileMap[MeshComponentIn].ProfileIndices.Contains(ProfileIdx))
    {
      RegisteredMeshComponentProfileMap[MeshComponentIn].ProfileIndices.Add(ProfileIdx);
    }
    else
    {
      // We've already registered this profile with this handmesh, and thus all the recognizers
      // should already be in place
      return true;
    }
  }
  else
  {
    FIsdkHandPoseDetectionMeshToProfile NewMeshProfile;
    NewMeshProfile.ProfileIndices.Add(ProfileIdx);
    RegisteredMeshComponentProfileMap.Add(MeshComponentIn, {NewMeshProfile});
  }

  MeshComponentIn->TryGetApiIHandPositionFrame();

  // Check if we need to create new Recognizers
  if (RegisteredMeshComponentMap.Contains(MeshComponentIn))
  {
    FIsdkHandPoseDetectionMeshGroup& ThisComponentGroup =
        RegisteredMeshComponentMap[MeshComponentIn];
    // Key: FingerType Value: FingerTarget
    for (auto& Elem : ProfileIn->ProfileFingerTargets)
    {
      const EIsdkFingerType FingerType = Elem.Key;
      if (!ThisComponentGroup.FingerRecognizerGroups.Contains(FingerType))
      {
        // Create recognizers for mesh and finger type
        CreateFingerRecognizers(MeshComponentIn, FingerType);
      }
    }
    if (ProfileIn->ProfileThumbTarget.ThumbCalcTargets.Num() > 0)
    {
      if (ThisComponentGroup.ThumbRecognizerGroup.ThumbCalcToRecognizerIndex.Num() == 0)
      {
        // create thumb recognizers for mesh
        CreateThumbRecognizers(MeshComponentIn);
      }
    }
  }
  else
  {
    // Brand new mesh, create all recognizers needed by the profile

    for (auto& Elem : ProfileIn->ProfileFingerTargets)
    {
      const EIsdkFingerType FingerType = Elem.Key;
      CreateFingerRecognizers(MeshComponentIn, FingerType);
    }
    if (ProfileIn->ProfileThumbTarget.ThumbCalcTargets.Num() > 0)
    {
      CreateThumbRecognizers(MeshComponentIn);
    }
  }
  bHandPoseDetectionEnabled = true;
  return true;
}

bool UIsdkHandPoseSubsystem::UnregisterHandPoseDetection(
    UIsdkHandPoseDetectionProfile* ProfileIn,
    UIsdkHandMeshComponent* MeshComponentIn)
{
  if (!IsValid(ProfileIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::UnregisterHandPoseDetection - ProfileIn was invalid"));
    return false;
  }
  if (!IsValid(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::UnregisterHandPoseDetection - MeshComponentIn was invalid"));
    return false;
  }

  const uint32 ProfileIdx = RegisteredDetectionProfiles.IndexOfByKey(ProfileIn);

  if (ProfileIdx == INDEX_NONE)
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::UnregisterHandPoseDetection - Profile index not found"));
    return false;
  }

  if (!RegisteredMeshComponentProfileMap.Contains(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::UnregisterHandPoseDetection - Mesh Component not found in profile map"));
    return false;
  }
  if (!RegisteredMeshComponentProfileMap[MeshComponentIn].ProfileIndices.Contains(ProfileIdx))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::UnregisterHandPoseDetection - Given profile is not registered for that hand mesh"));
    return false;
  }

  // Remove this profile from those registered with this mesh component
  RegisteredMeshComponentProfileMap[MeshComponentIn].ProfileIndices.Remove(ProfileIdx);

  // Null out the profile object, but don't remove it so we maintain the other indices
  RegisteredDetectionProfiles[ProfileIdx] = nullptr;

  // Empty the results as well
  DetectionProfileDeltaResults[ProfileIdx].FingersResults.Empty();
  DetectionProfileDeltaResults[ProfileIdx].ThumbResults.Empty();

  return true;
}

bool UIsdkHandPoseSubsystem::RegisterHandMeshForDetection(UIsdkHandMeshComponent* MeshComponentIn)
{
  if (!IsValid(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT("UIsdkHandPoseSubsystem::RegisterHandMeshForDetection - MeshComponentIn was invalid"));
    return false;
  }

  // Map Hand Mesh with Profile if not already done
  if (RegisteredMeshComponentProfileMap.Contains(MeshComponentIn) &&
      HandMeshComponentResults.Contains(MeshComponentIn))
  {
    return true;
  }
  else
  {
    // Empty Profile
    FIsdkHandPoseDetectionMeshToProfile NewMeshProfile;
    RegisteredMeshComponentProfileMap.Add(MeshComponentIn, {NewMeshProfile});
    FIsdkHandPoseRecognizerResults NewResultsStruct;

    HandMeshComponentResults.Add(MeshComponentIn, NewResultsStruct);
  }

  MeshComponentIn->TryGetApiIHandPositionFrame();
  // Brand new mesh, create all recognizers needed by the profile

  for (int i = 0; i < 4; i++)
  {
    const EIsdkFingerType ThisFingerType = (EIsdkFingerType)i;
    CreateFingerRecognizers(MeshComponentIn, ThisFingerType);
  }
  CreateThumbRecognizers(MeshComponentIn);

  bHandPoseDetectionEnabled = true;
  return true;
}

bool UIsdkHandPoseSubsystem::UnregisterHandMeshForDetection(UIsdkHandMeshComponent* MeshComponentIn)
{
  if (!IsValid(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::UnregisterHandMeshForDetection - MeshComponentIn was invalid"));
    return false;
  }

  if (!RegisteredMeshComponentProfileMap.Contains(MeshComponentIn) ||
      !RegisteredMeshComponentMap.Contains(MeshComponentIn) ||
      !HandMeshComponentResults.Contains(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Error,
        TEXT(
            "UIsdkHandPoseSubsystem::UnregisterHandMeshForDetection - MeshComponentIn not found in one/all of caches"));
    return false;
  }

  // Nullify all of its mesh component recognizers
  // Fingers
  FIsdkHandPoseDetectionMeshGroup& MeshGroup = RegisteredMeshComponentMap[MeshComponentIn];
  for (auto& FingerElem : MeshGroup.FingerRecognizerGroups)
  {
    FIsdkHandPoseFingerRecognizerGroup& RecognizerGroup = FingerElem.Value;
    for (auto& FingerGroupElem : RecognizerGroup.FingerCalcToRecognizerIndex)
    {
      const uint32 RecognizerIdx = FingerGroupElem.Value;
      if (!FingerRecognizers.IsValidIndex(RecognizerIdx))
      {
        UE_LOG(
            LogOculusInteraction,
            Error,
            TEXT(
                "UIsdkHandPoseSubsystem::UnregisterHandMeshForDetection - Registered finger recognizer index not found in array"));
        return false;
      }

      // Null out the recognizer, maintaining order for others
      FingerRecognizers[RecognizerIdx] = nullptr;
    }
  }
  // Thumb
  for (auto& ThumbElem : MeshGroup.ThumbRecognizerGroup.ThumbCalcToRecognizerIndex)
  {
    const uint32 RecognizerIdx = ThumbElem.Value;
    if (!ThumbRecognizers.IsValidIndex(RecognizerIdx))
    {
      UE_LOG(
          LogOculusInteraction,
          Error,
          TEXT(
              "UIsdkHandPoseSubsystem::UnregisterHandMeshForDetection - Registered thumb recognizer index not found in array"));
      return false;
    }
    // Null out the recognizer, maintaining order for others
    ThumbRecognizers[RecognizerIdx] = nullptr;
  }

  FIsdkHandPoseDetectionMeshToProfile& MeshProfiles =
      RegisteredMeshComponentProfileMap[MeshComponentIn];

  // Find if the profiles we would remove are still referenced by other mesh components
  for (const uint32 ThisIdx : MeshProfiles.ProfileIndices)
  {
    if (!RegisteredDetectionProfiles.IsValidIndex(ThisIdx))
    {
      UE_LOG(
          LogOculusInteraction,
          Error,
          TEXT(
              "UIsdkHandPoseSubsystem::UnregisterHandMeshForDetection - Registered profile index not actually found in cache"));
      return false;
    }
    bool bProfileInUseElsewhere = false;
    for (auto& ProfileElem : RegisteredMeshComponentProfileMap)
    {
      if (ProfileElem.Key == MeshComponentIn)
      {
        continue;
      }
      FIsdkHandPoseDetectionMeshToProfile& ThisProfile = ProfileElem.Value;
      if (ThisProfile.ProfileIndices.Contains(ThisIdx))
      {
        bProfileInUseElsewhere = true;
        break;
      }
    }
    // Any profiles that were only referenced by this mesh component are good to null out
    // (maintaining indices for others)
    if (!bProfileInUseElsewhere)
    {
      RegisteredDetectionProfiles[ThisIdx] = nullptr;
    }
  }

  // Now that we've made it this far, remove all mesh component references

  // Remove from component map
  RegisteredMeshComponentMap.Remove(MeshComponentIn);

  // Remove from Profile Map
  RegisteredMeshComponentProfileMap.Remove(MeshComponentIn);

  // Remove from Results Cache
  HandMeshComponentResults.Remove(MeshComponentIn);

  return true;
}

void UIsdkHandPoseSubsystem::DetectRegisteredPoses(float& DeltaTime)
{
  if (bHandPoseDetectionEnabled)
  {
    for (auto& Elem : RegisteredMeshComponentProfileMap)
    {
      UIsdkHandMeshComponent* ThisHandMesh = Elem.Key;
      FIsdkHandPoseDetectionMeshToProfile& MeshProfile = Elem.Value;
      if (!IsValid(ThisHandMesh))
      {
        UE_LOG(
            LogOculusInteraction,
            Warning,
            TEXT("UIsdkHandPoseSubsystem::DetectRegisteredPoses - MeshComponent was invalid"));
        continue;
      }
      if (!HandMeshComponentResults.Contains(ThisHandMesh))
      {
        continue;
      }
      if (!GetHandMeshDetectionResults(
              ThisHandMesh, HandMeshComponentResults[ThisHandMesh], DeltaTime))
      {
        continue;
      }
      for (uint32 ProfileIdx : MeshProfile.ProfileIndices)
      {
        if (RegisteredDetectionProfiles.IsValidIndex(ProfileIdx) &&
            IsValid(RegisteredDetectionProfiles[ProfileIdx]))
        {
          UIsdkHandPoseDetectionProfile* ThisProfile = RegisteredDetectionProfiles[ProfileIdx];
          if (!IsValid(ThisProfile) || !DetectionProfileDeltaResults.IsValidIndex(ProfileIdx))
          {
            continue;
          }
          bool bPoseDetected = false;
          if (!GetProfileDeltasFromResults(
                  ThisProfile,
                  HandMeshComponentResults[ThisHandMesh],
                  bDebugHandPoseDetection,
                  bPoseDetected,
                  DetectionProfileDeltaResults[ProfileIdx]))
          {
            continue;
          }
          if (bPoseDetected)
          {
            HandPoseDetectionDelegate.Broadcast(
                ThisProfile->PoseDetectionName, ThisProfile, ThisHandMesh);
          }
        }
      }
    }
  }
}

bool UIsdkHandPoseSubsystem::GetHandMeshDetectionResults(
    UIsdkHandMeshComponent* MeshComponentIn,
    FIsdkHandPoseRecognizerResults& ResultsOut,
    const float DeltaTime)
{
  if (!IsValid(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT("UIsdkHandPoseSubsystem::GetHandMeshDetectionResults - MeshComponent was invalid"));
    return false;
  }
  if (!RegisteredMeshComponentMap.Contains(MeshComponentIn))
  {
    UE_LOG(
        LogOculusInteraction,
        Warning,
        TEXT(
            "UIsdkHandPoseSubsystem::GetHandMeshDetectionResults - MeshComponent found but not registered"));
    return false;
  }

  FIsdkHandPoseDetectionMeshGroup& MeshGroup = RegisteredMeshComponentMap[MeshComponentIn];
  ResultsOut.FingersResults.Empty();
  ResultsOut.ThumbResults.Empty();
  // Fingers
  for (auto& OuterElem : MeshGroup.FingerRecognizerGroups)
  {
    const EIsdkFingerType FingerType = OuterElem.Key;
    const FIsdkHandPoseFingerRecognizerGroup& RecognizerGroup = OuterElem.Value;
    FIsdkHandPoseRecognizerFingerResult NewFingerResult;
    for (auto& InnerElem : RecognizerGroup.FingerCalcToRecognizerIndex)
    {
      const EIsdkDetection_FingerCalcType& FingerCalcType = InnerElem.Key;
      const uint32& RecognizerIndex = InnerElem.Value;
      UIsdkHandFingerRecognizer* ThisFingerRecognizer = nullptr;
      if (FingerRecognizers.IsValidIndex(RecognizerIndex))
      {
        ThisFingerRecognizer = FingerRecognizers[RecognizerIndex];
      }
      if (!IsValid(ThisFingerRecognizer))
      {
        continue;
      }
      ThisFingerRecognizer->UpdateState(DeltaTime);
      const float RawValue = ThisFingerRecognizer->GetRawValue();
      NewFingerResult.FingerResults.Add(FingerCalcType, RawValue);
    }
    ResultsOut.FingersResults.Add(FingerType, NewFingerResult);
  }

  // Thumb
  FIsdkHandPoseThumbRecognizerGroup& ThumbGroup = MeshGroup.ThumbRecognizerGroup;

  for (auto& OuterElem : ThumbGroup.ThumbCalcToRecognizerIndex)
  {
    const EIsdkDetection_ThumbCalcType& ThumbCalcType = OuterElem.Key;
    const uint32& RecognizerIndex = OuterElem.Value;
    UIsdkHandThumbRecognizer* ThisThumbRecognizer = nullptr;
    if (ThumbRecognizers.IsValidIndex(RecognizerIndex))
    {
      ThisThumbRecognizer = ThumbRecognizers[RecognizerIndex];
    }
    if (!IsValid(ThisThumbRecognizer))
    {
      continue;
    }
    ThisThumbRecognizer->UpdateState(DeltaTime);
    const float RawValue = ThisThumbRecognizer->GetRawValue();
    ResultsOut.ThumbResults.Add(ThumbCalcType, RawValue);
  }
  return (ResultsOut.FingersResults.Num() > 0 || ResultsOut.ThumbResults.Num() > 0);
}

bool UIsdkHandPoseSubsystem::CreateFingerRecognizers(
    UIsdkHandMeshComponent* MeshIn,
    EIsdkFingerType FingerType)
{
  if (!IsValid(MeshIn))
  {
    return false;
  }

  if (!RegisteredMeshComponentMap.Contains(MeshIn))
  {
    const FIsdkHandPoseDetectionMeshGroup NewMeshGroup;
    RegisteredMeshComponentMap.Add(MeshIn, NewMeshGroup);
  }
  FIsdkHandPoseDetectionMeshGroup& MeshGroup = RegisteredMeshComponentMap[MeshIn];
  // Only create the recognizers if they don't already exist
  if (!MeshGroup.FingerRecognizerGroups.Contains(FingerType))
  {
    FIsdkHandPoseFingerRecognizerGroup NewFingerRecognizerGroup;
    MeshGroup.FingerRecognizerGroups.Add(FingerType, NewFingerRecognizerGroup);

    for (const EIsdkDetection_FingerCalcType FingerCalcType : FingerCalcTypes)
    {
      UIsdkHandFingerRecognizer* NewFingerRecognizer = NewObject<UIsdkHandFingerRecognizer>(this);
      NewFingerRecognizer->CalcType = FingerCalcType;
      NewFingerRecognizer->FingerType = FingerType;
      NewFingerRecognizer->HandMesh = MeshIn;
      FingerRecognizers.Add(NewFingerRecognizer);
      MeshGroup.FingerRecognizerGroups[FingerType].FingerCalcToRecognizerIndex.Add(
          FingerCalcType, FingerRecognizers.Num() - 1);
    }
  }
  return true;
}

bool UIsdkHandPoseSubsystem::CreateThumbRecognizers(UIsdkHandMeshComponent* MeshIn)
{
  if (!IsValid(MeshIn))
  {
    return false;
  }
  if (!RegisteredMeshComponentMap.Contains(MeshIn))
  {
    const FIsdkHandPoseDetectionMeshGroup NewMeshGroup;
    RegisteredMeshComponentMap.Add(MeshIn, NewMeshGroup);
  }
  FIsdkHandPoseDetectionMeshGroup& MeshGroup = RegisteredMeshComponentMap[MeshIn];
  // Only create the recognizers if they don't already exist
  if (MeshGroup.ThumbRecognizerGroup.ThumbCalcToRecognizerIndex.Num() == 0)
  {
    for (const EIsdkDetection_ThumbCalcType ThumbCalcType : ThumbCalcTypes)
    {
      UIsdkHandThumbRecognizer* NewThumbRecognizer = NewObject<UIsdkHandThumbRecognizer>(this);
      NewThumbRecognizer->CalcType = ThumbCalcType;
      NewThumbRecognizer->HandMesh = MeshIn;
      ThumbRecognizers.Add(NewThumbRecognizer);
      MeshGroup.ThumbRecognizerGroup.ThumbCalcToRecognizerIndex.Add(
          ThumbCalcType, ThumbRecognizers.Num() - 1);
    }
  }

  return true;
}

bool UIsdkHandPoseSubsystem::GetProfileDeltasFromResults(
    UIsdkHandPoseDetectionProfile* ProfileIn,
    FIsdkHandPoseRecognizerResults& ResultsIn,
    bool bGenerateFullDeltas,
    bool& bResultsWithinTolerances,
    FIsdkHandPoseRecognizerResults& DeltaResultsOut)
{
  if (!IsValid(ProfileIn))
  {
    return false;
  }
  bResultsWithinTolerances = true;

  DeltaResultsOut.FingersResults.Empty();
  DeltaResultsOut.ThumbResults.Empty();
  // FINGERS
  for (auto& OuterElem : ProfileIn->ProfileFingerTargets)
  {
    const EIsdkFingerType& FingerType = OuterElem.Key;
    const FIsdkHandPoseDetectionFingerTarget& FingerTargets = OuterElem.Value;

    if (!ResultsIn.FingersResults.Contains(FingerType))
    {
      UE_LOG(
          LogOculusInteraction,
          Warning,
          TEXT(
              "UIsdkHandPoseSubsystem::GetProfileDeltasFromResults - Results don't contain expected fingertype %d"),
          (uint8)FingerType);
      bResultsWithinTolerances = false;
      return false;
    }
    for (auto& InnerElem : FingerTargets.FingerCalcTargets)
    {
      const EIsdkDetection_FingerCalcType& CalcType = InnerElem.Key;
      if (!FingerTargets.FingerCalcTolerances.Contains(CalcType))
      {
        bResultsWithinTolerances = false;
        return false;
      }
      const float& FingerTarget = InnerElem.Value;
      const float& FingerTolerance = FingerTargets.FingerCalcTolerances[CalcType];
      const FIsdkHandPoseRecognizerFingerResult& ThisFingerResult =
          ResultsIn.FingersResults[FingerType];
      if (!ResultsIn.FingersResults[FingerType].FingerResults.Contains(CalcType))
      {
        UE_LOG(
            LogOculusInteraction,
            Warning,
            TEXT(
                "UIsdkHandPoseSubsystem::GetProfileDeltasFromResults - Results don't contain expected finger calculation type %d"),
            (uint8)CalcType);
        bResultsWithinTolerances = false;
        return false;
      }
      const float& FingerResult = ResultsIn.FingersResults[FingerType].FingerResults[CalcType];
      const float ProfileResultDelta = FingerTarget - FingerResult;

      if (bGenerateFullDeltas)
      {
        if (!DeltaResultsOut.FingersResults.Contains(FingerType))
        {
          FIsdkHandPoseRecognizerFingerResult NewFingerResultOut;
          DeltaResultsOut.FingersResults.Add(FingerType, NewFingerResultOut);
        }
        DeltaResultsOut.FingersResults[FingerType].FingerResults.Add(CalcType, ProfileResultDelta);
      }

      if (FMath::Abs(ProfileResultDelta) > FingerTolerance)
      {
        bResultsWithinTolerances = false;
        if (!bGenerateFullDeltas)
        {
          break;
        }
      }
    }
    // No need to proceed, we're already outside of tolerance, so we're bailing on the rest
    if (!bResultsWithinTolerances && !bGenerateFullDeltas)
    {
      break;
    }
  }
  // THUMB

  // No need to proceed, we're already outside of tolerance, so we're bailing on the rest
  if (!bResultsWithinTolerances && !bGenerateFullDeltas)
  {
    return true;
  }

  FIsdkHandPoseDetectionThumbTarget& ProfileThumbTarget = ProfileIn->ProfileThumbTarget;
  for (auto& Elem : ProfileThumbTarget.ThumbCalcTargets)
  {
    const EIsdkDetection_ThumbCalcType& CalcType = Elem.Key;
    const float& ThumbTarget = Elem.Value;

    if (!ProfileThumbTarget.ThumbCalcTolerances.Contains(CalcType))
    {
      UE_LOG(
          LogOculusInteraction,
          Warning,
          TEXT(
              "UIsdkHandPoseSubsystem::GetProfileDeltasFromResults - Results don't contain expected thumb tolerances for calculation type %d"),
          (uint8)CalcType);
      bResultsWithinTolerances = false;
      return false;
    }
    if (!ResultsIn.ThumbResults.Contains(CalcType))
    {
      UE_LOG(
          LogOculusInteraction,
          Warning,
          TEXT(
              "UIsdkHandPoseSubsystem::GetProfileDeltasFromResults - Results don't contain expected thumb calculation type %d"),
          (uint8)CalcType);
      bResultsWithinTolerances = false;
      return false;
    }
    const float& ThumbResult = ResultsIn.ThumbResults[CalcType];
    const float& ThumbTolerance = ProfileThumbTarget.ThumbCalcTolerances[CalcType];
    const float ProfileResultDelta = ThumbTarget - ThumbResult;

    if (bGenerateFullDeltas)
    {
      DeltaResultsOut.ThumbResults.Add(CalcType, ProfileResultDelta);
    }

    if (FMath::Abs(ProfileResultDelta) > ThumbTolerance)
    {
      bResultsWithinTolerances = false;
      if (!bGenerateFullDeltas)
      {
        break;
      }
    }
  }

  return true;
}
