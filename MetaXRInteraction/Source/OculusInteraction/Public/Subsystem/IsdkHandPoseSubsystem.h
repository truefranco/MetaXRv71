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
#include "Engine/SkinnedAsset.h"
#include "Engine/SkeletalMesh.h"
#include "GameplayTagContainer.h"
#include "IsdkHandData.h"
#include "IsdkHandPoseData.h"
#include "Interaction/IsdkIInteractorState.h"
#include "HandPoseDetection/IsdkHandPoseDetectionProfile.h"
#include "IsdkHandPoseSubsystem.generated.h"

class UIsdkHandMeshComponent;
class UIsdkHandGrabPose;
class UIsdkHandFingerRecognizer;
class UIsdkHandThumbRecognizer;

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
struct FIsdkHandPoseDetectionMeshToProfile
{
  GENERATED_BODY()

  //
  UPROPERTY()
  TArray<uint32> ProfileIndices;
};

USTRUCT()
struct FIsdkHandPoseFingerRecognizerGroup
{
  GENERATED_BODY()

  //
  UPROPERTY()
  TMap<EIsdkDetection_FingerCalcType, uint32> FingerCalcToRecognizerIndex;
};

USTRUCT()
struct FIsdkHandPoseThumbRecognizerGroup
{
  GENERATED_BODY()

  //
  UPROPERTY()
  TMap<EIsdkDetection_ThumbCalcType, uint32> ThumbCalcToRecognizerIndex;
};

USTRUCT()
struct FIsdkHandPoseDetectionMeshGroup
{
  GENERATED_BODY()

  UPROPERTY()
  TMap<EIsdkFingerType, FIsdkHandPoseFingerRecognizerGroup> FingerRecognizerGroups;
  //
  UPROPERTY()
  FIsdkHandPoseThumbRecognizerGroup ThumbRecognizerGroup;
};

/**/
USTRUCT(BlueprintType)
struct FIsdkHandPoseRecognizerFingerResult
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = InteractionSDK)
  TMap<EIsdkDetection_FingerCalcType, float> FingerResults;
};

/**/
USTRUCT(BlueprintType)
struct FIsdkHandPoseRecognizerResults
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = InteractionSDK)
  TMap<EIsdkFingerType, FIsdkHandPoseRecognizerFingerResult> FingersResults;

  UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = InteractionSDK)
  TMap<EIsdkDetection_ThumbCalcType, float> ThumbResults;
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
 * Signifies that a registered pose detection has been triggered
 * 0 - The event
 * 1 - Name of pose
 * 2 - The profile used to determine the pose
 * 3 - The hand mesh component registered that detected the pose
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FIsdkHandPoseDetectionEvent,
    FName,
    PoseName,
    UIsdkHandPoseDetectionProfile*,
    DetectionProfile,
    UIsdkHandMeshComponent*,
    DetectedMeshComponent);

/**
 * @class UIsdkHandPoseSubsystem
 * @brief Holds asset references for Hand Pose visualization, systems and helpers for monitoring and
 * enabling hand grab poses. Also used to register and monitor hand pose detections via hand mesh
 * components, and broadcast delegates when hand shape targets (within tolerances) have been met.
 * @see UIsdkHandMeshComponent
 * @addtogroup InteractionSDK
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

  /* @brief Checks if a given interactable and mesh component need to trigger a registered hand grab
   * pose */
  bool CheckForHandPose(
      USceneComponent* InteractableIn,
      UIsdkHandMeshComponent* InteractingHandIn,
      const FIsdkInteractorStateEvent& StateEventIn,
      UIsdkHandPoseData*& HandPoseOut,
      FIsdkHandGrabPoseProperties& GrabPosePropertiesOut,
      FTransform& RootOffsetOut);

  /* Registers a hand grab pose with the subsystem, enabling it to be checked by future interactions
   * to enable hand grab poses */
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

  /* @brief For a given hand pose data, create the mirrored version of it (left->right or
   * right->left) */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool GenerateMirroredHandPoseData(
      UIsdkHandPoseData* PreviousHandData,
      UIsdkHandPoseData*& MirroredHandData);

  /* @brief Returns a delta profile containing the joint data from the HandData,
   *    applying a given base
   * tolerance. If no tolerances are defined, will apply set default tolerances instead*/
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool SetHandPoseDetectionProfileFromMesh(
      UIsdkHandPoseDetectionProfile* ProfileIn,
      UIsdkHandMeshComponent* MeshComponentIn,
      float BaseTolerances,
      TArray<EIsdkFingerType> FingersToIgnore,
      bool bIgnoreThumb,
      FName PoseName);

  /*@brief Wrapper for SetHandPoseDetectionProfileFromMesh that creates a new profile object instead
  of using an existing one*/
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool CreateHandPoseDetectionProfileFromMesh(
      UIsdkHandMeshComponent* MeshComponentIn,
      float BaseTolerances,
      TArray<EIsdkFingerType> FingersToIgnore,
      bool bIgnoreThumb,
      FName PoseName,
      UIsdkHandPoseDetectionProfile*& ProfileOut);

  /* @brief Sets the default tolerances for all hand pose detections */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetDefaultHandPoseDetectionTolerance(float NewDigitCalcTolerance)
  {
    HandPoseDetectionDigitCalcTolerance = NewDigitCalcTolerance;
  }

  /* @brief Registers a hand pose detection profile, will also create recognizers for the hand mesh
   * component if they're not already in place, using the profile to optimally only create needed
   * recognizers */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool RegisterHandPoseDetection(
      UIsdkHandPoseDetectionProfile* ProfileIn,
      UIsdkHandMeshComponent* MeshComponentIn);

  /* @brief Unregisters a hand pose detection profile for a given hand mesh, will not remove
   * recognizers or registered hand mesh components */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool UnregisterHandPoseDetection(
      UIsdkHandPoseDetectionProfile* ProfileIn,
      UIsdkHandMeshComponent* MeshComponentIn);

  /* @brief Registers a Hand Mesh Component, and creates a full suite of recognizers for it */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool RegisterHandMeshForDetection(UIsdkHandMeshComponent* MeshComponentIn);

  /* @brief Unregisters a Hand Mesh Component, and also removes ALL recognizers for it and ALL
   * profiles that were solely linked to it*/
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool UnregisterHandMeshForDetection(UIsdkHandMeshComponent* MeshComponentIn);

  /* @brief For a given hand mesh component, polls all of its created finger and thumb recognizers
   * and writes the results out to HandMeshComponentResults   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool GetHandMeshDetectionResults(
      UIsdkHandMeshComponent* MeshComponentIn,
      FIsdkHandPoseRecognizerResults& ResultsOut,
      const float DeltaTime);

  /* @brief For a given profile and hand mesh components results, compares the digits and
   * calculations defined in the profile and produces results output, as well as a boolean to
   * signify if all results fell within tolerances. If full deltas are enabled, all delta results
   * will be written to DetectionProfileDeltaResults   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool GetProfileDeltasFromResults(
      UIsdkHandPoseDetectionProfile* ProfileIn,
      UPARAM(ref) FIsdkHandPoseRecognizerResults& ResultsIn,
      bool bRunFullDeltas,
      bool& bResultsWithinTolerances,
      FIsdkHandPoseRecognizerResults& DeltaResultsOut);

  /* @brief When enabled, debugging hand pose detection will produce a full results struct for each
   * profile, written to DetectionProfileDeltaResults */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetDebugHandPoseDetection(bool bNewDebugHandPoseDetection)
  {
    bDebugHandPoseDetection = bNewDebugHandPoseDetection;
  }

  /* @brief Delegate broadcast when a hand pose has been detected as per the profile registered with
   * it, broadcasts the profile, the hand mesh component and the pose name */
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkHandPoseDetectionEvent HandPoseDetectionDelegate;

  // TODO: Set Hand Mesh/Material (as overrides)
  USkinnedAsset* GetHandMesh(EIsdkHandedness& Handedness);
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

  /* Creates the battery Finger Recognizers for the given finger of a hand mesh component, which
   * will produce results on tick, enabling hand pose/shape detection */
  bool CreateFingerRecognizers(UIsdkHandMeshComponent* MeshIn, EIsdkFingerType FingerType);

  /* Creates the battery Finger Recognizers for the thumb of a given hand mesh component, which will
   * produce results on tick, enabling hand pose/shape detection */
  bool CreateThumbRecognizers(UIsdkHandMeshComponent* MeshIn);

  /* Primary method for detecting hand poses, iterates through all registered hand mesh components,
   * taking results and comparing them against all registered (and related) detection profiles. If
   * within the tolerances of the profile, broadcasts a delegate with profile information as a
   * sidecar. */
  void DetectRegisteredPoses(float& DeltaTime);

  UPROPERTY()
  TObjectPtr<UMaterial> HandMeshMaterial;
  UPROPERTY()
  TObjectPtr<USkinnedAsset> HandMeshRight;
  UPROPERTY()
  TObjectPtr<USkinnedAsset> HandMeshLeft;
  UPROPERTY()
  TMap<FName, FIsdkHandPoseDataActorCache> ActorPoseCacheMap;
  UPROPERTY()
  TArray<TObjectPtr<USceneComponent>> HandGrabPoseDestroyQueue;

  /* Registered Mesh Components Map to Detection Profiles Registered with that Mesh */
  UPROPERTY()
  TMap<UIsdkHandMeshComponent*, FIsdkHandPoseDetectionMeshToProfile>
      RegisteredMeshComponentProfileMap;

  /* All registered detection profiles, typically accessed by array index via stored within
   * RegisteredMeshComponentProfileMap */
  UPROPERTY()
  TArray<UIsdkHandPoseDetectionProfile*> RegisteredDetectionProfiles;

  /* Registered mesh components mapped to the finger and thumb recognizers that were created to poll
   * data from them */
  UPROPERTY()
  TMap<UIsdkHandMeshComponent*, FIsdkHandPoseDetectionMeshGroup> RegisteredMeshComponentMap;

  /* All created finger recognizers for pose detection, typically accessed by array index via
   * RegisteredMeshComponentMap*/
  UPROPERTY()
  TArray<UIsdkHandFingerRecognizer*> FingerRecognizers;

  /* All created thumb recognizers for pose detection, typically accessed by array index via
   * RegisteredMeshComponentMap*/
  UPROPERTY()
  TArray<UIsdkHandThumbRecognizer*> ThumbRecognizers;

  /* Cached structs of results for hand mesh components, enabling quicker seek times and reducing
   * new object instantiation */
  UPROPERTY()
  TMap<UIsdkHandMeshComponent*, FIsdkHandPoseRecognizerResults> HandMeshComponentResults;

  /* Cached structs of results for profile delta calculations, enabling quicker seek times and
   * reducing new object instantiation */
  UPROPERTY()
  TArray<FIsdkHandPoseRecognizerResults> DetectionProfileDeltaResults;

  int LastHandPoseMirroredSuffix = 1;

  /* Finger Calculation types to create when generating finger recognizers for hand pose detection*/
  const TArray<EIsdkDetection_FingerCalcType> FingerCalcTypes = {
      EIsdkDetection_FingerCalcType::Abduction,
      EIsdkDetection_FingerCalcType::Curl,
      EIsdkDetection_FingerCalcType::Flexion,
      EIsdkDetection_FingerCalcType::Opposition};

  /* Thumb Calculation types to create when generating finger recognizers for hand pose detection*/
  const TArray<EIsdkDetection_ThumbCalcType> ThumbCalcTypes = {
      EIsdkDetection_ThumbCalcType::Curl,
      EIsdkDetection_ThumbCalcType::Flexion};

  bool bDebugHandPoseDetection = false;
  bool bHandPoseDetectionEnabled = false;
  bool bDestroyDummyMeshComponent = false;

  // Defaults
  float HandPoseDetectionDigitCalcTolerance = 15.f;
};
