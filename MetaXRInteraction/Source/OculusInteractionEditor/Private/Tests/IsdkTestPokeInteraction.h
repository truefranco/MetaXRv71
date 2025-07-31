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

#include "Interaction/IsdkPokeInteractor.h"
#include "Interaction/IsdkPokeInteractable.h"
#include "Interaction/IsdkClippedPlaneSurface.h"
#include "Interaction/Surfaces/IsdkPointablePlane.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "IsdkTestFakes.h"

#include "IsdkTestPokeInteraction.generated.h"

UCLASS()
class OCULUSINTERACTIONEDITOR_API AIsdkTestPokeInteractorActor : public AActor
{
  GENERATED_BODY()
 public:
  AIsdkTestPokeInteractorActor()
  {
    const auto Root = CreateDefaultSubobject<USceneComponent>(FName("Root"));
    SetRootComponent(Root);

    TestPokeInteractor = CreateDefaultSubobject<UIsdkPokeInteractor>(TEXT("TestPokeInteractor"));
    TestPokeInteractor->SetupAttachment(RootComponent);
  }

  static bool Setup()
  {
    // No getting the level dirty
    FActorSpawnParameters ActorParameters{};
    ActorParameters.bNoFail = true;
    UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();

    const auto TestActor = TestWorld->SpawnActor<AIsdkTestPokeInteractorActor>(
        AIsdkTestPokeInteractorActor::StaticClass(), ActorParameters);
    return ensure(TestActor);
  }

  static AIsdkTestPokeInteractorActor& Get()
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    AIsdkTestPokeInteractorActor* Instance = Cast<AIsdkTestPokeInteractorActor, AActor>(
        UGameplayStatics::GetActorOfClass(TestWorld, AIsdkTestPokeInteractorActor::StaticClass()));
    check(Instance);
    return *Instance;
  }

  UPROPERTY()
  UIsdkPokeInteractor* TestPokeInteractor{};
};

UCLASS()
class OCULUSINTERACTIONEDITOR_API AIsdkTestPokeInteractableActor : public AActor
{
  GENERATED_BODY()
 public:
  AIsdkTestPokeInteractableActor()
  {
    const auto Root = CreateDefaultSubobject<USceneComponent>(FName("Root"));
    SetRootComponent(Root);

    TestPointablePlane = CreateDefaultSubobject<UIsdkPointablePlane>(TEXT("TestPointablePlane"));
    TestPointablePlane->SetupAttachment(Root);

    TestPokeInteractable =
        CreateDefaultSubobject<UIsdkPokeInteractable>(TEXT("TestPokeInteractablePlane"));
    TestPokeInteractable->SetupAttachment(Root);

    TestClippedPlaneSurface =
        CreateDefaultSubobject<UIsdkClippedPlaneSurface>(TEXT("TestClippedPlaneSurface"));

    TestClippedPlaneSurface->SetPointablePlane(TestPointablePlane);
    TestPokeInteractable->SetSurfacePatch(TestClippedPlaneSurface);
  }

  static bool Setup()
  {
    // No getting the level dirty
    FActorSpawnParameters ActorParameters{};
    ActorParameters.bNoFail = true;
    UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();

    const auto TestActor = TestWorld->SpawnActor<AIsdkTestPokeInteractableActor>(
        AIsdkTestPokeInteractableActor::StaticClass(), ActorParameters);

    return ensure(TestActor);
  }

  static AIsdkTestPokeInteractableActor& Get()
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    AIsdkTestPokeInteractableActor* Instance =
        Cast<AIsdkTestPokeInteractableActor, AActor>(UGameplayStatics::GetActorOfClass(
            TestWorld, AIsdkTestPokeInteractableActor::StaticClass()));
    check(Instance);
    return *Instance;
  }

  void SetTwoClippers(FVector Center, float Size, float Offset)
  {
    TArray<FIsdkBoundsClipper> Clippers = TArray<FIsdkBoundsClipper>();

    auto ClipperA = FIsdkBoundsClipper();
    ClipperA.PoseProvider = TestPointablePlane;
    ClipperA.Position = Center + FVector(0.0f, Offset, 0.0f);
    ClipperA.Size = FVector(0.1, Size, Size);
    Clippers.Add(ClipperA);

    auto ClipperB = FIsdkBoundsClipper();
    ClipperB.PoseProvider = TestPointablePlane;
    ClipperB.Position = Center + FVector(0.0, -Offset, 0.0);
    ClipperB.Size = FVector(0.1, Size, Size);
    Clippers.Add(ClipperB);

    TestClippedPlaneSurface->SetBoundsClippers(Clippers);
  }

  void SetSingleClipper(FVector Center, float Size)
  {
    TArray<FIsdkBoundsClipper> Clippers = TArray<FIsdkBoundsClipper>();

    auto ClipperA = FIsdkBoundsClipper();
    ClipperA.PoseProvider = TestPointablePlane;
    ClipperA.Position = Center;
    ClipperA.Size = FVector(0.1, Size, Size);
    Clippers.Add(ClipperA);

    TestClippedPlaneSurface->SetBoundsClippers(Clippers);
  }

  UPROPERTY()
  UIsdkPokeInteractable* TestPokeInteractable{};
  UPROPERTY()
  UIsdkPointablePlane* TestPointablePlane{};
  UPROPERTY()
  UIsdkClippedPlaneSurface* TestClippedPlaneSurface{};
};
