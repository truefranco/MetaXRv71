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

#include "Interaction/IsdkRayInteractor.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "IsdkTestFakes.h"
#include "Interaction/IsdkRayInteractable.h"
#include "Interaction/Surfaces/IsdkPointablePlane.h"
#include "Interaction/Surfaces/IsdkPointableBox.h"

#include "IsdkTestRayInteraction.generated.h"

UCLASS()
class OCULUSINTERACTIONEDITOR_API AIsdkTestRayInteractorActor : public AActor
{
  GENERATED_BODY()
 public:
  AIsdkTestRayInteractorActor()
  {
    const auto Root = CreateDefaultSubobject<USceneComponent>(FName("Root"));
    SetRootComponent(Root);

    TestHandPointerPose =
        CreateDefaultSubobject<UIsdkFakeHandPointerPose>(TEXT("TestHandPointerPose"));
    TestRayInteractor = CreateDefaultSubobject<UIsdkRayInteractor>(TEXT("TestRayInteractor"));
    TestRayInteractor->SetupAttachment(RootComponent);

    TestRayInteractorStateChangedHandler =
        CreateDefaultSubobject<UIsdkFakeInteractorStateChangedHandler>(
            TEXT("TestRayInteractorStateChangedHandler"));
    TestRayInteractor->GetInteractorStateChangedDelegate()->AddDynamic(
        TestRayInteractorStateChangedHandler,
        &UIsdkFakeInteractorStateChangedHandler::HandleStateChanged);
  }

  static bool SetUp()
  {
    // No getting the level dirty
    FActorSpawnParameters ActorParameters{};
    ActorParameters.bNoFail = true;
    UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();

    const auto TestActor = TestWorld->SpawnActor<AIsdkTestRayInteractorActor>(
        AIsdkTestRayInteractorActor::StaticClass(), ActorParameters);
    return ensureMsgf(TestActor, TEXT("Failed to spawn test actor: AIsdkTestRayInteractorActor"));
  }

  static AIsdkTestRayInteractorActor& Get()
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    AIsdkTestRayInteractorActor* Instance = Cast<AIsdkTestRayInteractorActor, AActor>(
        UGameplayStatics::GetActorOfClass(TestWorld, AIsdkTestRayInteractorActor::StaticClass()));
    checkf(Instance, TEXT("Failed to cast actor to test object"));
    return *Instance;
  }

  static bool TryGetChecked(AIsdkTestRayInteractorActor*& Instance)
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    Instance = Cast<AIsdkTestRayInteractorActor, AActor>(
        UGameplayStatics::GetActorOfClass(TestWorld, AIsdkTestRayInteractorActor::StaticClass()));
    const bool bIsValid = IsValid(Instance);
    check(bIsValid);
    return bIsValid;
  }

  UIsdkRayInteractor* GetTestRayInteractor() const
  {
    return TestRayInteractor;
  }

  UIsdkFakeHandPointerPose* GetTestHandPointerPose() const
  {
    return TestHandPointerPose;
  }

  UIsdkFakeInteractorStateChangedHandler* GetInteractorStateChangedHandler() const
  {
    return TestRayInteractorStateChangedHandler;
  }

 private:
  UPROPERTY()
  UIsdkRayInteractor* TestRayInteractor{};
  UPROPERTY()
  UIsdkFakeHandPointerPose* TestHandPointerPose{};
  UPROPERTY()
  UIsdkFakeInteractorStateChangedHandler* TestRayInteractorStateChangedHandler;
};

UCLASS()
class OCULUSINTERACTIONEDITOR_API UIsdkTestRayInteractableComponent : public USceneComponent
{
  GENERATED_BODY()
 public:
  UIsdkTestRayInteractableComponent()
  {
    TestRayInteractable = CreateDefaultSubobject<UIsdkRayInteractable>("TestRayInteractable");
    TestRayInteractable->SetupAttachment(this);

    TestRayInteractableStateChangedHandler =
        CreateDefaultSubobject<UIsdkFakeInteractableStateChangedHandler>(
            TEXT("TestRayInteractableStateChangedHandler"));
    TestRayInteractable->GetInteractableStateChangedDelegate()->AddDynamic(
        TestRayInteractableStateChangedHandler,
        &UIsdkFakeInteractableStateChangedHandler::HandleStateChanged);
    TestRayInteractable->GetInteractionPointerEventDelegate().AddDynamic(
        TestRayInteractableStateChangedHandler,
        &UIsdkFakeInteractableStateChangedHandler::HandleInteractablePointerEvent);
  }

  UIsdkFakeInteractableStateChangedHandler* GetInteractableStateChangedHandler() const
  {
    return TestRayInteractableStateChangedHandler;
  }

  UIsdkRayInteractable* GetInteractable() const
  {
    return TestRayInteractable;
  }

  void SetSurface(TScriptInterface<IIsdkISurface> Surface)
  {
    const auto SurfaceComponent = Cast<USceneComponent>(Surface.GetObject());
    SurfaceComponent->SetupAttachment(this);
    TestRayInteractable->SetSurface(Surface);
  }

 private:
  UPROPERTY()
  UIsdkRayInteractable* TestRayInteractable{};
  UPROPERTY()
  UIsdkFakeInteractableStateChangedHandler* TestRayInteractableStateChangedHandler{};
};

UCLASS()
class OCULUSINTERACTIONEDITOR_API AIsdkTestRayInteractableActor : public AActor
{
  GENERATED_BODY()
 public:
  AIsdkTestRayInteractableActor()
  {
    const auto Root = CreateDefaultSubobject<USceneComponent>(FName("Root"));
    SetRootComponent(Root);

    auto TestPointablePlane =
        CreateDefaultSubobject<UIsdkPointablePlane>(TEXT("TestPointablePlane"));
    TestPointablePlane->SetSize(PlaneSize);
    TestRayInteractablePlane =
        CreateDefaultSubobject<UIsdkTestRayInteractableComponent>(TEXT("TestRayInteractablePlane"));
    TestRayInteractablePlane->SetupAttachment(RootComponent);
    TestRayInteractablePlane->SetSurface(TestPointablePlane);
    TestRayInteractablePlane->SetWorldLocationAndRotation(
        FVector::ForwardVector * 100.0f, FQuat::Identity);

    auto TestPointableBox = CreateDefaultSubobject<UIsdkPointableBox>(TEXT("TestPointableBox"));
    TestPointableBox->SetSize(BoxSize);
    TestRayInteractableBox =
        CreateDefaultSubobject<UIsdkTestRayInteractableComponent>(TEXT("TestRayInteractableBox"));
    TestRayInteractableBox->SetupAttachment(RootComponent);
    TestRayInteractableBox->SetSurface(TestPointableBox);
    TestRayInteractableBox->SetWorldLocationAndRotation(
        FVector::RightVector * 100.0f, FQuat::Identity);
  }

  static bool SetUp()
  {
    // No getting the level dirty
    FActorSpawnParameters ActorParameters{};
    ActorParameters.bNoFail = true;
    UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();

    const auto TestActor = TestWorld->SpawnActor<AIsdkTestRayInteractableActor>(
        AIsdkTestRayInteractableActor::StaticClass(), ActorParameters);

    return ensureMsgf(TestActor, TEXT("Failed to spawn test actor: AIsdkTestRayInteractableActor"));
  }

  static AIsdkTestRayInteractableActor& Get()
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    AIsdkTestRayInteractableActor* Instance = Cast<AIsdkTestRayInteractableActor, AActor>(
        UGameplayStatics::GetActorOfClass(TestWorld, AIsdkTestRayInteractableActor::StaticClass()));
    checkf(Instance, TEXT("Failed to cast actor to test object"));
    return *Instance;
  }

  static bool TryGetChecked(AIsdkTestRayInteractableActor*& Instance)
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    Instance = Cast<AIsdkTestRayInteractableActor, AActor>(
        UGameplayStatics::GetActorOfClass(TestWorld, AIsdkTestRayInteractableActor::StaticClass()));
    const bool bIsValid = IsValid(Instance);
    check(bIsValid);
    return bIsValid;
  }

  static inline const FVector2D PlaneSize{100, 100};
  static inline const FVector BoxSize{100, 100, 100};

  UPROPERTY()
  UIsdkTestRayInteractableComponent* TestRayInteractablePlane{};
  UPROPERTY()
  UIsdkTestRayInteractableComponent* TestRayInteractableBox{};
};
