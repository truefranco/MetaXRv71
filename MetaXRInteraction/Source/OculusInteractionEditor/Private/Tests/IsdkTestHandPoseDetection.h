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
#include "IsdkHandMeshComponent.h"
#include "IsdkContentAssetPaths.h"
#include "IsdkTestFakes.h"
#include "DataSources/IsdkExternalHandDataSource.h"
#include "DataSources/IsdkOneEuroFilterDataModifier.h"
#include "HandPoseDetection/IsdkHandFingerPinchGrabRecognizer.h"

#include "UObject/ConstructorHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "IsdkTestHandPoseDetection.generated.h"

UCLASS()
class OCULUSINTERACTIONEDITOR_API AIsdkTestHandPoseDetectionActor : public AActor
{
  GENERATED_BODY()
 public:
  UPROPERTY()
  UIsdkFakeHandDataSource* TestDataSourceExternal{};

  UPROPERTY()
  UIsdkHandMeshComponent* TestTrackedHandVisual{};

  UPROPERTY()
  UIsdkHandFingerPinchGrabRecognizer* TestFingerPinchGrabRecognizer{};

  AIsdkTestHandPoseDetectionActor()
  {
    const auto Root = CreateDefaultSubobject<USceneComponent>(FName("Root"));
    SetRootComponent(Root);

    TestDataSourceExternal =
        CreateDefaultSubobject<UIsdkFakeHandDataSource>(FName("TestDataSourceExternal"));

    // Set up HandVisual for Left Hand
    TestTrackedHandVisual =
        CreateDefaultSubobject<UIsdkHandMeshComponent>(FName("TestTrackedHandVisual"));
    struct FStaticObjectFinders
    {
      ConstructorHelpers::FObjectFinderOptional<USkeletalMesh> LeftHandMeshFinder;
      ConstructorHelpers::FObjectFinderOptional<UMaterial> HandMaterialFinder;

      FStaticObjectFinders()
          : LeftHandMeshFinder(IsdkContentAssetPaths::Models::Hand::OpenXRLeftHand),
            HandMaterialFinder(IsdkContentAssetPaths::Models::Hand::OculusHandMaterial)
      {
      }
    };
    static FStaticObjectFinders StaticObjectFinders;
    TestTrackedHandVisual->SetMaterial(0, StaticObjectFinders.HandMaterialFinder.Get());
    TestTrackedHandVisual->SetSkeletalMesh(StaticObjectFinders.LeftHandMeshFinder.Get());
    TestTrackedHandVisual->SetMappedBoneNamesAsDefault();

    TestFingerPinchGrabRecognizer = CreateDefaultSubobject<UIsdkHandFingerPinchGrabRecognizer>(
        TEXT("TestFingerPinchGrabRecognizer"));
  }

  virtual void BeginPlay() override
  {
    Super::BeginPlay();

    // Wire up data sources
    TestTrackedHandVisual->SetJointsDataSource(TestDataSourceExternal);
    TestTrackedHandVisual->SetRootPoseDataSource(TestDataSourceExternal);

    // Wire hand visual into the pinch grab recognizer
    TestFingerPinchGrabRecognizer->HandVisual = TestTrackedHandVisual;

    // Set the data to neutral pose by default
    TestDataSourceExternal->SetHandJointsToNeutralPose();
  }

  static bool SetUp()
  {
    // No getting the level dirty
    FActorSpawnParameters ActorParameters{};
    ActorParameters.bNoFail = true;
    UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();

    const auto TestActor = TestWorld->SpawnActor<AIsdkTestHandPoseDetectionActor>(
        AIsdkTestHandPoseDetectionActor::StaticClass(), ActorParameters);
    return ensure(TestActor);
  }

  static AIsdkTestHandPoseDetectionActor& Get()
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    AIsdkTestHandPoseDetectionActor* Instance =
        Cast<AIsdkTestHandPoseDetectionActor, AActor>(UGameplayStatics::GetActorOfClass(
            TestWorld, AIsdkTestHandPoseDetectionActor::StaticClass()));
    check(Instance);
    return *Instance;
  }

  static bool TryGetChecked(AIsdkTestHandPoseDetectionActor*& Instance)
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    Instance = Cast<AIsdkTestHandPoseDetectionActor, AActor>(UGameplayStatics::GetActorOfClass(
        TestWorld, AIsdkTestHandPoseDetectionActor::StaticClass()));
    const bool bIsValid = IsValid(Instance);
    check(bIsValid);
    return bIsValid;
  }

  const TArray<FVector> JointLocationsLeftPinch = {
      {0, 0, 0},
      {0, 0, 0},
      {-1.049, -2.006, -1.155},
      {-3.034, -3.438, -1.585},
      {-5.128, -5.764, -2.469},
      {-6.454, -8.730, -3.396},
      {-2.355, -9.599, -0.7316},
      {-2.770, -13.30, -1.399},
      {-2.973, -15.46, -2.504},
      {-0.1725, -9.56, -0.2543},
      {-0.05565, -13.81, -0.875},
      {0.02110, -16.42, -1.749},
      {1.74, -8.869, -0.6529},
      {2.324, -12.71, -0.9638},
      {2.630, -15.24, -1.695},
      {2.299, -3.407, -0.9419},
      {3.5, -7.789, -1.369},
      {4.544, -10.67, -1.465},
      {5.127, -12.46, -2.248},
      {-7.25, -10.86, -4.332},
      {-3.011, -17.36, -3.682},
      {0.13, -18.80, -2.487},
      {2.781, -17.57, -2.418},
      {5.569, -14.4, -2.971},
  };
};
