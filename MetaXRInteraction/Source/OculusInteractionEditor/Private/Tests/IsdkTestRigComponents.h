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
#include "IsdkTestFakes.h"
#include "XRMotionControllerBase.h"
#include "Rig/IsdkHandVisualsRigComponent.h"
#include "Rig/IsdkInputActionsRigComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "Misc/AutomationTest.h"
#include "Rig/IsdkControllerVisualsRigComponent.h"
#include "Rig/IsdkInteractionGroupRigComponent.h"

#include "IsdkTestRigComponents.generated.h"

UCLASS()
class AIsdkTestRigActor : public AActor
{
  GENERATED_BODY()

 public:
  UPROPERTY()
  UIsdkHandVisualsRigComponent* HandVisuals{};

  UPROPERTY()
  UIsdkControllerVisualsRigComponent* ControllerVisuals{};

  UPROPERTY()
  UIsdkInputActionsRigComponent* InputActions{};

  UPROPERTY()
  UIsdkInteractionGroupRigComponent* InteractionGroup{};

  UPROPERTY()
  UIsdkTestHandRigComponent* RigComponentBase;

  // Fakes
  UPROPERTY()
  UIsdkFakeHmdDataSource* FakeHmdDataSource{};
  UPROPERTY()
  UIsdkFakeSceneInteractor* FakeInteractor1{};
  UPROPERTY()
  UIsdkFakeSceneInteractor* FakeInteractor2{};
  UPROPERTY()
  UIsdkFakeSceneInteractor* FakeInteractor3{};
  UPROPERTY()
  UIsdkConditional* InteractorGroupConditional1{};
  UPROPERTY()
  UIsdkConditional* InteractorGroupConditional2{};
  UPROPERTY()
  UIsdkConditional* InteractorGroupConditional3{};

  // Supporting Dependencies
  UPROPERTY()
  UMotionControllerComponent* LeftHandMotionController{};
  UPROPERTY()
  UMotionControllerComponent* RightHandMotionController{};
  UPROPERTY()
  UIsdkFakeControllerRigComponentLeft* ControllerRigLeft{};

  AIsdkTestRigActor()
  {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // Root
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

    // Objects to be tested
    HandVisuals = CreateDefaultSubobject<UIsdkHandVisualsRigComponentLeft>(TEXT("HandVisuals"));
    ControllerVisuals =
        CreateDefaultSubobject<UIsdkControllerVisualsRigComponentLeft>(TEXT("ControllerVisuals"));
    InputActions = CreateDefaultSubobject<UIsdkInputActionsRigComponent>(TEXT("InputActions"));
    InteractionGroup =
        CreateDefaultSubobject<UIsdkInteractionGroupRigComponent>(TEXT("InteractionGroup"));
    RigComponentBase = CreateDefaultSubobject<UIsdkTestHandRigComponent>(TEXT("RigComponentBase"));

    // Important: Set Construction Defaults
    InputActions->SetSubobjectPropertyDefaults(EIsdkHandedness::Left);

    // Fakes
    FakeHmdDataSource = CreateDefaultSubobject<UIsdkFakeHmdDataSource>(TEXT("FakeHmdDataSource"));
    FakeInteractor1 = CreateDefaultSubobject<UIsdkFakeSceneInteractor>(TEXT("FakeInteractor1"));
    FakeInteractor2 = CreateDefaultSubobject<UIsdkFakeSceneInteractor>(TEXT("FakeInteractor2"));
    FakeInteractor3 = CreateDefaultSubobject<UIsdkFakeSceneInteractor>(TEXT("FakeInteractor3"));

    // Supporting dependencies
    LeftHandMotionController =
        CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHandMotionController"));
    LeftHandMotionController->SetupAttachment(RootComponent);
    ControllerRigLeft =
        CreateDefaultSubobject<UIsdkFakeControllerRigComponentLeft>(TEXT("FakeControllerRigLeft"));
    ControllerRigLeft->SetupAttachment(LeftHandMotionController);
    RigComponentBase->SetupAttachment(LeftHandMotionController);
  }

  virtual void OnConstruction(const FTransform& Transform) override
  {
    Super::OnConstruction(Transform);
    LeftHandMotionController->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
  }

  UIsdkFakeHandDataSource* GetFakeHandDataSource()
  {
    return GetWorld()->GetSubsystem<UIsdkFakeTrackingDataSubsystem>()->HandDataSource;
  }
  UIsdkFakeHandDataSource* GetFakeControllerDataSource()
  {
    return GetWorld()->GetSubsystem<UIsdkFakeTrackingDataSubsystem>()->ControllerDataSource;
  }

  static AIsdkTestRigActor& Get()
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    AIsdkTestRigActor* Instance = Cast<AIsdkTestRigActor, AActor>(
        UGameplayStatics::GetActorOfClass(TestWorld, AIsdkTestRigActor::StaticClass()));
    check(Instance);
    return *Instance;
  }

  static [[nodiscard]] bool TryGetChecked(AIsdkTestRigActor*& Instance)
  {
    const UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();
    check(TestWorld);
    Instance = Cast<AIsdkTestRigActor, AActor>(
        UGameplayStatics::GetActorOfClass(TestWorld, AIsdkTestRigActor::StaticClass()));
    const bool bIsValid = IsValid(Instance);
    check(bIsValid);
    return bIsValid;
  }

  static FIsdkInteractorGroupMember::CalculateStateFn FakeCalculateStateFn()
  {
    return [](const FIsdkInteractorStateEvent& Event)
    {
		FIsdkInteractionGroupMemberState state{};
          state.bIsSelectStateBlocking = Event.Args.NewState == EIsdkInteractorState::Select;
		  return state;
    };
  }

  static auto CheckGroupMemberStatesFn(
      FString TestStepName,
      TOptional<bool> Expected1,
      TOptional<bool> Expected2)
  {
    return [TestStepName, Expected1, Expected2](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
    {
      const auto Group = Actor.InteractionGroup;

      const auto* Conditional1 = Group->FindConditionalForInteractor(Actor.FakeInteractor1);
      const auto* Conditional2 = Group->FindConditionalForInteractor(Actor.FakeInteractor2);

      const TOptional<bool> Actual1 =
          IsValid(Conditional1) ? TOptional<bool>(Conditional1->GetResolvedValue()) : TOptional<bool>();
      const TOptional<bool> Actual2 =
          IsValid(Conditional2) ? TOptional<bool>(Conditional2->GetResolvedValue()) : TOptional<bool>();

      Test->TestEqual(*(TestStepName + TEXT(": Conditional1")), Expected1, Actual1);
      Test->TestEqual(*(TestStepName + TEXT(": Conditional2")), Expected2, Actual2);
    };
  }

  static auto SetGroupMemberStatesFn(EIsdkInteractorState State1, EIsdkInteractorState State2)
  {
    return [State1, State2](FAutomationTestBase*, AIsdkTestRigActor& Actor)
    {
      Actor.FakeInteractor1->SetStateImpl(State1);
      Actor.FakeInteractor2->SetStateImpl(State2);
    };
  }

  static auto
  CheckGroupMemberStatesFn(FString TestStepName, bool bExpected1, bool bExpected2, bool bExpected3)
  {
    return [TestStepName, bExpected1, bExpected2, bExpected3](
               FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
    {
      const auto Group = Actor.InteractionGroup;

      const auto* Conditional1 = Group->FindConditionalForInteractor(Actor.FakeInteractor1);
      const auto* Conditional2 = Group->FindConditionalForInteractor(Actor.FakeInteractor2);
      const auto* Conditional3 = Group->FindConditionalForInteractor(Actor.FakeInteractor3);

      Test->TestEqual(
          *(TestStepName + TEXT(": Conditional1")), bExpected1, Conditional1->GetResolvedValue());
      Test->TestEqual(
          *(TestStepName + TEXT(": Conditional2")), bExpected2, Conditional2->GetResolvedValue());
      Test->TestEqual(
          *(TestStepName + TEXT(": Conditional3")), bExpected3, Conditional3->GetResolvedValue());
    };
  }

  static auto SetGroupMemberStatesFn(
      EIsdkInteractorState State1,
      EIsdkInteractorState State2,
      EIsdkInteractorState State3)
  {
    return [State1, State2, State3](FAutomationTestBase*, AIsdkTestRigActor& Actor)
    {
      Actor.FakeInteractor1->SetStateImpl(State1);
      Actor.FakeInteractor2->SetStateImpl(State2);
      Actor.FakeInteractor3->SetStateImpl(State3);
    };
  }

  static auto InitGroupMembersFn(
      FIsdkInteractionGroupMemberBehavior Behavior1,
      FIsdkInteractionGroupMemberBehavior Behavior2,
      FIsdkInteractorGroupMember::CalculateStateFn CalculateState)
  {
    return
        [Behavior1, Behavior2, CalculateState](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
    {
      UIsdkInteractionGroupRigComponent* Group = Actor.InteractionGroup;

      const auto Conditional1 = Group->AddInteractor(
          Actor.FakeInteractor1,
          *Actor.FakeInteractor1->GetInteractorStateChangedDelegate(),
          CalculateState,
          Behavior1);
      const auto Conditional2 = Group->AddInteractor(
          Actor.FakeInteractor2,
          *Actor.FakeInteractor2->GetInteractorStateChangedDelegate(),
          CalculateState,
          Behavior2);

      Actor.InteractorGroupConditional1 = Conditional1;
      Actor.InteractorGroupConditional2 = Conditional2;

      // Initial Conditions
      Test->TestTrue(TEXT("Conditional1 Initial Condition"), Conditional1->GetResolvedValue());
      Test->TestTrue(TEXT("Conditional2 Initial Condition"), Conditional2->GetResolvedValue());
    };
  }

  static auto InitGroupMembersFn(
      FIsdkInteractionGroupMemberBehavior Behavior1,
      FIsdkInteractionGroupMemberBehavior Behavior2,
      FIsdkInteractionGroupMemberBehavior Behavior3,
      FIsdkInteractorGroupMember::CalculateStateFn CalculateState)
  {
    return [Behavior1, Behavior2, Behavior3, CalculateState](
               FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
    {
      UIsdkInteractionGroupRigComponent* Group = Actor.InteractionGroup;

      const auto Conditional1 = Group->AddInteractor(
          Actor.FakeInteractor1,
          *Actor.FakeInteractor1->GetInteractorStateChangedDelegate(),
          CalculateState,
          Behavior1);
      const auto Conditional2 = Group->AddInteractor(
          Actor.FakeInteractor2,
          *Actor.FakeInteractor2->GetInteractorStateChangedDelegate(),
          CalculateState,
          Behavior2);
      const auto Conditional3 = Group->AddInteractor(
          Actor.FakeInteractor3,
          *Actor.FakeInteractor3->GetInteractorStateChangedDelegate(),
          CalculateState,
          Behavior3);

      Actor.InteractorGroupConditional1 = Conditional1;
      Actor.InteractorGroupConditional2 = Conditional2;
      Actor.InteractorGroupConditional3 = Conditional3;

      // Initial Conditions
      Test->TestTrue(TEXT("Conditional1 Initial Condition"), Conditional1->GetResolvedValue());
      Test->TestTrue(TEXT("Conditional2 Initial Condition"), Conditional2->GetResolvedValue());
      Test->TestTrue(TEXT("Conditional3 Initial Condition"), Conditional2->GetResolvedValue());
    };
  }

  static auto CheckPointerPoseEqualFn(FTransform T)
  {
    return [T](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
    {
      const auto SocketName = Actor.RigComponentBase->GetPointerPoseSocketName();
      const auto ActualTransform = Actor.RigComponentBase->GetSocketTransform(SocketName);
	  const float Tolerance = 0.001f;
	  Test->TestEqual(TEXT("Pointer Pose Translation Equal"), ActualTransform.GetTranslation(), T.GetTranslation(), Tolerance);
	  Test->TestTrue(TEXT("Pointer Pose Rotation Equal"), ActualTransform.GetRotation().Equals(T.GetRotation(), Tolerance));
	  Test->TestEqual(TEXT("Pointer Pose Scale Equal"), ActualTransform.GetScale3D(), T.GetScale3D(), Tolerance);
    };
  }
};
