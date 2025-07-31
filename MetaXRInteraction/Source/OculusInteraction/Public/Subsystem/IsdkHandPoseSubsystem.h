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

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Subsystems/WorldSubsystem.h"
#include "IsdkContentAssetPaths.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Engine/SkeletalMesh.h"
#include "IsdkHandData.h"
#include "IsdkHandPoseData.h"
#include "Interaction/IsdkIInteractorState.h"
#include "IsdkHandPoseSubsystem.generated.h"

class UIsdkHandMeshComponent;
class UIsdkHandGrabPose;

USTRUCT()
struct FIsdkHandPoseDataGroup
{
  GENERATED_BODY()

  UPROPERTY()
  TMap<EIsdkHandedness, UIsdkHandPoseData*> CachedHandPoses;
};

USTRUCT()
struct FIsdkHandPoseDataCache
{
  GENERATED_BODY()

  // Cached relative transforms for each hand grab pose
  UPROPERTY()
  TArray<FTransform> CachedTransforms;

  // Core behavior properties for each hand grab pose
  UPROPERTY()
  TArray<FIsdkHandGrabPoseProperties> CachedProperties;

  // Pose groups, defining for each handedness type, which handposedata to use
  UPROPERTY()
  TArray<FIsdkHandPoseDataGroup> CachedHandPoseGroups;

  // FNames of each grab pose (as specified in editor for each component)
  UPROPERTY()
  TArray<FName> CachedPoseNames;
};

USTRUCT()
struct FIsdkHandPoseDataActorVariations
{
  GENERATED_BODY()

  // Each entry is one index in the FIsdkHandPoseDataCache that is associated with this Actor
  UPROPERTY()
  TSet<uint32> Variations;
};

USTRUCT()
struct FIsdkHandPoseDataActorCache
{
  GENERATED_BODY()

  // Map linking each actor instance to an array of variations associated with that specific
  // instance (multiple actors of the same blueprint class will likely share many variations unless
  // instance-specific changes are made
  UPROPERTY()
  TMap<AActor*, FIsdkHandPoseDataActorVariations> InstanceVariations;

  // The primary cache for all pose data
  UPROPERTY()
  FIsdkHandPoseDataCache PoseDataCache;

  // Map linking an original pose with its mirrored one, as they will need to be added as a pair
  // when caching for other instances
  UPROPERTY()
  TMap<uint32, uint32> OriginalMirrorVariationMap;
};

// TODO Make this a configuration instead of always hardcoded
struct FStaticObjectFinders
{
  ConstructorHelpers::FObjectFinderOptional<USkeletalMesh> LeftHandMeshFinder;
  ConstructorHelpers::FObjectFinderOptional<USkeletalMesh> RightHandMeshFinder;
  ConstructorHelpers::FObjectFinderOptional<UMaterial> HandMaterialFinder;

  FStaticObjectFinders()
      : LeftHandMeshFinder(IsdkContentAssetPaths::Models::Hand::OpenXRLeftHand),
        RightHandMeshFinder(IsdkContentAssetPaths::Models::Hand::OpenXRRightHand),
        HandMaterialFinder(IsdkContentAssetPaths::Models::Hand::OculusHandTestMaterial)
  {
  }
};

/**
 * Holds asset references for Hand Pose visualization, active information about hand pose
 * comparisons
 */
UCLASS()
class OCULUSINTERACTION_API UIsdkHandPoseSubsystem : public UTickableWorldSubsystem
{
  GENERATED_BODY()

 public:
  UIsdkHandPoseSubsystem();
  virtual void BeginDestroy() override;

  // USubsystem implementation Begin
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  // USubsystem implementation End

  // FTickableGameObject implementation Begin
  virtual void Tick(float DeltaSeconds) override;
  virtual bool IsTickableInEditor() const override
  {
    return true;
  }
  virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
  virtual ETickableTickType GetTickableTickType() const override;
  virtual TStatId GetStatId() const override;
  // FTickableGameObject implementation End

  bool CheckForHandPose(
      USceneComponent* InteractableIn,
      UIsdkHandMeshComponent* InteractingHandIn,
      const FIsdkInteractorStateEvent& StateEventIn,
      UIsdkHandPoseData*& HandPoseOut,
      FIsdkHandGrabPoseProperties& GrabPosePropertiesOut,
      FTransform& RootOffsetOut);
  void RegisterHandPoseData(UIsdkHandGrabPose* HandGrabPoseIn, bool bDestroyAfterRegistration);

  /**
   * Static helper function to get an Interaction SDK subsystem from a world.
   */
  static UIsdkHandPoseSubsystem* Get(const UWorld* InWorld)
  {
    if (!IsValid(InWorld))
    {
      UE_LOG(
          LogOculusInteraction,
          Warning,
          TEXT("World passed to UIsdkHandPoseSubsystem::Get was nullptr"));
      return nullptr;
    }

    UIsdkHandPoseSubsystem* Instance = InWorld->GetSubsystem<UIsdkHandPoseSubsystem>();
    if (!IsValid(Instance))
    {
      UE_LOG(
          LogOculusInteraction,
          Warning,
          TEXT("Failed to find a UIsdkHandPoseSubsystem for the world \"%s\""),
          *InWorld->GetDebugDisplayName());
    }

    return Instance;
  }

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool GenerateMirroredHandPoseData(
      UIsdkHandPoseData* PreviousHandData,
      UIsdkHandPoseData*& MirroredHandData);

  // TODO: Set Hand Mesh/Material (as overrides)
  USkeletalMesh* GetHandMesh(EIsdkHandedness& Handedness);
  UMaterial* GetHandMeshMaterial();

 private:
  FName GetActorClassNameFromComponent(USceneComponent* ComponentIn);

  /* Adds a new variation with the given properties to the ActorCacheMap. Returns the new variation
   * index or -1 if there was an issue */
  uint32 AddNewVariationToActorCacheMap(
      FName OuterActorName,
      AActor* ActorIn,
      FTransform TransformIn,
      FIsdkHandPoseDataGroup PoseDataGroupIn,
      FName PoseNameIn,
      FIsdkHandGrabPoseProperties PosePropertiesIn);

  /* Adds an existing variation for a given actor to the cache map
   */
  bool AddExistingVariationToActorCacheMap(
      FName OuterActorName,
      AActor* ActorIn,
      int32 VariationIndexIn);

  /* Checks if the HandPoseIn represents any new data against what has been cached for that actor.
   * Returns the index of a variation if found */
  bool ShouldCreateNewActorPoseVariation(UIsdkHandGrabPose* HandPoseIn, int32& VariationMatched);

  UPROPERTY()
  UMaterial* HandMeshMaterial;
  UPROPERTY()
  USkeletalMesh* HandMeshRight;
  UPROPERTY()
  USkeletalMesh* HandMeshLeft;
  UPROPERTY()
  TMap<FName, FIsdkHandPoseDataActorCache> ActorPoseCacheMap;
  UPROPERTY()
  TArray<USceneComponent*> HandGrabPoseDestroyQueue;

  int LastHandPoseMirroredSuffix = 1;
};
