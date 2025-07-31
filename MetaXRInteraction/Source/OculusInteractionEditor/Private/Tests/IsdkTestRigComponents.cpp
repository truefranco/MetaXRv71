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

#include "IsdkTestRigComponents.h"

#include "Tests/IsdkCommonTestCommands.h"
#include "IsdkRuntimeSettings.h"
#include "Interaction/IsdkPokeInteractor.h"
#include "Interaction/IsdkRayInteractor.h"
#include "Rig/IsdkRayInteractionRigComponent.h"
#include "Rig/IsdkGrabInteractionRigComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include <functional>

#include "IsdkControllerMeshComponent.h"
#include "IsdkHandMeshComponent.h"
#include "Rig/IsdkControllerVisualsRigComponent.h"
#include "Rig/IsdkPokeInteractionRigComponent.h"
#include "Runtime/Launch/Resources/Version.h"

// A generic test step that finds the AIsdkTestRigActor test object, then passes it into a lambda
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FIsdkTestRigLambda,
    FAutomationTestBase*,
    Test,
    std::function<void(FAutomationTestBase* Test, AIsdkTestRigActor& Actor)>,
    TestCallback);

bool FIsdkTestRigLambda::Update()
{
  AIsdkTestRigActor* TestActor{};
  if(!AIsdkTestRigActor::TryGetChecked(TestActor))
  {
	  Test->AddErrorIfFalse(false, TEXT("AIsdkTestRigActor was not in the test scene."));
      TestCallback(Test, *TestActor);
  }
  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestSpawnComponents,
    FAutomationTestBase*,
    Test);

// Test Step: Create an instance of AIsdkTestRigActor, which contains the object under test & its
// dependencies.
bool FIsdkTestSpawnComponents::Update()
{
  UWorld* TestWorld = GEditor->GetPIEWorldContext()->World();

#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5)
  auto Subsystems = TestWorld->GetSubsystemArrayCopy<UWorldSubsystem>();
#else
  auto Subsystems = TestWorld->GetSubsystemArray<UWorldSubsystem>();
#endif

  // Disable any known tracking subsystems, in case they are loaded (i.e. when running on local
  // machine) This ensures we test the path that those subsystems are unavailable.
  auto& DisabledSubsystems = UIsdkRuntimeSettings::Get().DisabledTrackingDataSubsystems;
  DisabledSubsystems.Empty();
  for (const auto Subsystem : Subsystems)
  {
    // Keep the fake tracking data subsystem, we need that for testing.
    if (Subsystem->Implements<UIsdkITrackingDataSubsystem>() &&
        !Subsystem->IsA<UIsdkFakeTrackingDataSubsystem>())
    {
      DisabledSubsystems.Add(Subsystem->GetClass());
    }
  }

  // Spawn the test actor
  const auto TestActor =
      TestWorld->SpawnActor<AIsdkTestRigActor>(FVector::ZeroVector, FRotator::ZeroRotator);

  if (!TestActor)
  {
    Test->AddError("Failed to find TestActor in the scene.");
  }

  auto FakeTrackingDataSubsystem =
      TestActor->GetWorld()->GetSubsystem<UIsdkFakeTrackingDataSubsystem>();
  TestActor->ControllerVisuals->CustomTrackingData = FakeTrackingDataSubsystem;
  TestActor->HandVisuals->CustomTrackingData = FakeTrackingDataSubsystem;
  TestActor->ControllerRigLeft->GetControllerVisuals()->CustomTrackingData =
      FakeTrackingDataSubsystem;

  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestControllerVisualsInit,
    FAutomationTestBase*,
    Test);

bool FIsdkTestControllerVisualsInit::Update()
{
  AIsdkTestRigActor* TestActor;
  if (!AIsdkTestRigActor::TryGetChecked(TestActor))
  {
    Test->AddError("Failed to find TestActor in the scene.");
    return true;
  }

  TestActor->ControllerRigLeft->GetControllerVisuals()->TryAttachToParentMotionController(
      TestActor->LeftHandMotionController);

  TestActor->ControllerVisuals->TryAttachToParentMotionController(
      TestActor->LeftHandMotionController);

  Test->TestEqual(
      TEXT(
          "TryAttachToParentMotionController should be able to attach a provided motion controller component"),
      TestActor->ControllerVisuals->AttachedToMotionController,
      TestActor->LeftHandMotionController);

  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestControllerVisualsCreateDataSources,
    FAutomationTestBase*,
    Test);

bool FIsdkTestControllerVisualsCreateDataSources::Update()
{
  AIsdkTestRigActor* TestActor;
  if (!AIsdkTestRigActor::TryGetChecked(TestActor))
  {
    Test->AddError("Failed to find TestActor in the scene.");
    return true;
  }

  {
    const UIsdkHandDataSource* Actual =
        TestActor->ControllerVisuals->GetDataSources().DataSourceComponent;
    Test->TestNull(
        TEXT(
            "ControllerVisuals should not have a data source set prior to calling CreateDataSourcesAsTrackedController."),
        Actual);
  }

  TestActor->ControllerVisuals->CreateDataSourcesAsTrackedController();
  TestActor->ControllerRigLeft->GetControllerVisuals()->CreateDataSourcesAsTrackedController();

  {
    const auto DataSources = TestActor->ControllerVisuals->GetDataSources();
    const UIsdkHandDataSource* Actual = DataSources.DataSourceComponent;
    const UIsdkHandDataSource* Expected = TestActor->GetFakeControllerDataSource();
    Test->TestEqual(
        TEXT("ControllerVisuals should use the `CustomTrackingData`, when it is provided."),
        Actual,
        Expected);

    TScriptInterface<IIsdkIHmdDataSource> FakeHmdDataSource = TestActor->FakeHmdDataSource;
    TestActor->ControllerRigLeft->PokeInteraction->BindDataSources(
        DataSources, TestActor->GetRootComponent(), {});
    TestActor->ControllerRigLeft->RayInteraction->BindDataSources(
        DataSources, FakeHmdDataSource, TestActor->GetRootComponent(), {});
    TestActor->ControllerRigLeft->GrabInteraction->BindDataSources(
        DataSources, FakeHmdDataSource, TestActor->GetRootComponent(), {});
  }

  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestRigControllerSetDataSourceConnectedValue,
    bool,
    bIsConnected);

bool FIsdkTestRigControllerSetDataSourceConnectedValue::Update()
{
  AIsdkTestRigActor::Get().GetFakeControllerDataSource()->IsConnected->SetValue(bIsConnected);

  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestHandVisualsInit,
    FAutomationTestBase*,
    Test);

// Test Step: Initialize the HandVisuals component with a fake TrackingDataSybsystem.
bool FIsdkTestHandVisualsInit::Update()
{
  AIsdkTestRigActor* TestActor;
  if (!AIsdkTestRigActor::TryGetChecked(TestActor))
  {
    Test->AddError("Failed to find TestActor in the scene.");
    return true;
  }

  TestActor->HandVisuals->TryAttachToParentMotionController(TestActor->LeftHandMotionController);

  Test->TestEqual(
      TEXT(
          "TryAttachToParentMotionController should be able to attach a provided motion controller component"),
      TestActor->HandVisuals->AttachedToMotionController,
      TestActor->LeftHandMotionController);

  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestHandVisualsCreateDataSources,
    FAutomationTestBase*,
    Test);

// Test Step: Verify that data sources created by the HandVisuals match the fake data source.
bool FIsdkTestHandVisualsCreateDataSources::Update()
{
  AIsdkTestRigActor* TestActor;
  if (!AIsdkTestRigActor::TryGetChecked(TestActor))
  {
    Test->AddError("Failed to find TestActor in the scene.");
    return true;
  }

  {
    const UIsdkHandDataSource* Actual =
        TestActor->HandVisuals->GetDataSources().DataSourceComponent;
    Test->TestNull(
        TEXT(
            "HandVisuals not have a data source set prior to calling CreateDataSourcesAsTrackedHand."),
        Actual);
  }

  TestActor->HandVisuals->CreateDataSourcesAsTrackedHand();
  const auto DataSources = TestActor->HandVisuals->GetDataSources();

  {
    const UIsdkHandDataSource* Actual = DataSources.DataSourceComponent;
    const UIsdkHandDataSource* Expected = TestActor->GetFakeHandDataSource();
    Test->TestEqual(
        TEXT("HandVisuals should use the `CustomTrackingData`, when it is provided."),
        Actual,
        Expected);
  }

  // Empty HMD data that isn't used during testing
  TScriptInterface<IIsdkIHmdDataSource> FakeHmdDataSource{};

  TestActor->ControllerRigLeft->PokeInteraction->BindDataSources(
      DataSources, TestActor->GetRootComponent(), {});
  TestActor->ControllerRigLeft->RayInteraction->BindDataSources(
      DataSources, FakeHmdDataSource, TestActor->GetRootComponent(), {});
  TestActor->ControllerRigLeft->GrabInteraction->BindDataSources(
      DataSources, FakeHmdDataSource, TestActor->GetRootComponent(), {});

  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestRigHandSetDataSourceConnectedValue,
    bool,
    bIsConnected);

bool FIsdkTestRigHandSetDataSourceConnectedValue::Update()
{
  AIsdkTestRigActor::Get().GetFakeHandDataSource()->IsConnected->SetValue(bIsConnected);
  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(
    FIsdkTestRigHandCheckInteractorEnabledValue,
    FAutomationTestBase*,
    Test,
    bool,
    bExpectedPokeEnabled,
    bool,
    bExpectedRayEnabled,
    FString,
    TestName);

bool FIsdkTestRigHandCheckInteractorEnabledValue::Update()
{
  const AIsdkTestRigActor& Actor = AIsdkTestRigActor::Get();
  const UIsdkPokeInteractor* PokeInteractor =
      Actor.ControllerRigLeft->PokeInteraction->PokeInteractor;
  const UIsdkRayInteractor* RayInteractor = Actor.ControllerRigLeft->RayInteraction->RayInteractor;

  const bool bPokeIsEnabled = PokeInteractor->GetCurrentState() != EIsdkInteractorState::Disabled;
  const bool bRayIsEnabled = RayInteractor->GetCurrentState() != EIsdkInteractorState::Disabled;

  Test->TestEqual(*TestName, bPokeIsEnabled, bExpectedPokeEnabled);
  Test->TestEqual(*TestName, bRayIsEnabled, bExpectedRayEnabled);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkRigHandIsConnectedTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.HandIsConnected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool IsdkRigHandIsConnectedTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestHandVisualsInit(this));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestHandVisualsCreateDataSources(this));

  // Check that toggling the connected state on the tracked hand will enable/disable interactors.
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigHandSetDataSourceConnectedValue(true));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigHandCheckInteractorEnabledValue(
      this, true, true, TEXT("When DataSource is connected")));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigHandSetDataSourceConnectedValue(false));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigHandCheckInteractorEnabledValue(
      this, false, false, TEXT("When DataSource not connected")));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkRigHandPointerPoseSocketTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.HandPointerPoseSocket",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool IsdkRigHandPointerPoseSocketTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestHandVisualsInit(this));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestHandVisualsCreateDataSources(this));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigHandSetDataSourceConnectedValue(true));

  // Set the pointer pose to invalid; the socket should use the actor transform (identity).
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [](FAutomationTestBase*, AIsdkTestRigActor& Actor)
      { Actor.GetFakeHandDataSource()->bIsFakePointerPoseValid = false; }));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestRigLambda(this, AIsdkTestRigActor::CheckPointerPoseEqualFn(FTransform::Identity)));

  // Set the pointer pose to a valid test value; socket should return pointer pose.
  const FTransform TestRootPose = FTransform(FQuat::Identity, {100, 200, 300}, FVector::One());
  const FTransform TestRelativeTransform = FTransform(FQuat::Identity, {1, 2, 3}, FVector::One());
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [TestRelativeTransform, TestRootPose](FAutomationTestBase*, AIsdkTestRigActor& Actor)
      {
        Actor.GetFakeHandDataSource()->FakeRootPose = TestRootPose;
        Actor.GetFakeHandDataSource()->bIsFakePointerPoseValid = true;
        Actor.GetFakeHandDataSource()->FakePointerPoseRelative = TestRelativeTransform;
      }));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckPointerPoseEqualFn(TestRootPose * TestRelativeTransform)));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkRigInteractionGroupSelectedTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.InteractionGroupSelected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Check that selecting interactors disables others accordingly
bool IsdkRigInteractionGroupSelectedTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));

  FIsdkInteractionGroupMemberBehavior Behavior1{};
  Behavior1.bDisableOnOtherSelect = true;
  Behavior1.bDisableOnOtherNearFieldHover = false;
  Behavior1.bIsNearField = false;
  
  FIsdkInteractionGroupMemberBehavior Behavior2{};
  Behavior2.bDisableOnOtherSelect = false;
  Behavior2.bDisableOnOtherNearFieldHover = false;
  Behavior2.bIsNearField = false;
  
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      AIsdkTestRigActor::InitGroupMembersFn(
          Behavior1, Behavior2, AIsdkTestRigActor::FakeCalculateStateFn())));

  // Select #1 ; #2 should remain enabled since bDisableOnOtherSelect = false.
  using EState = EIsdkInteractorState;
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Select, EState::Normal)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Select #1"), true, true)));

  // Select #2 ; #1 should disable since bDisableOnOtherSelect = true.
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Normal, EState::Select)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Select #2"), false, true)));

  // Test deselect re-enables the conditional
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Normal, EState::Normal)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Select None"), true, true)));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkRigInteractionGroupNearFieldHoverTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.InteractionGroupNearFieldHover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Check that selecting interactors disables others accordingly
bool IsdkRigInteractionGroupNearFieldHoverTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));

  // Poke-like behavior
  const FIsdkInteractionGroupMemberBehavior Behavior1 = { true, false, true };

  // Ray-like behavior
  const FIsdkInteractionGroupMemberBehavior Behavior2 = { true, true, false };

  // Grab-like behavior
  const FIsdkInteractionGroupMemberBehavior Behavior3 = { true, true, true };

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      AIsdkTestRigActor::InitGroupMembersFn(
          Behavior1, Behavior2, Behavior3, AIsdkTestRigActor::FakeCalculateStateFn())));

  // Poke Hovers; Ray & Grab should disable
  using EState = EIsdkInteractorState;
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Hover, EState::Normal, EState::Normal)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Hover #1"), true, false, false)));

  // Ray Hovers; nothing should disable
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Normal, EState::Hover, EState::Normal)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Hover #2"), true, true, true)));

  // Grab Hovers; Ray should disable but poke remain enabled as it has
  // bDisableOnOtherNearFieldHover=false
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Normal, EState::Normal, EState::Hover)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Hover #3"), true, false, true)));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkRigInteractionGroupNonBlockingSelectTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.InteractionGroupNonBlockingSelect",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Tests blocking versus non-blocking select. A non blocking select happens when an interactor
// is in the Select state, but doesn't have a selected interactor.
bool IsdkRigInteractionGroupNonBlockingSelectTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));

  const FIsdkInteractionGroupMemberBehavior Behavior{true, false, false};
   

  const FIsdkInteractorGroupMember::CalculateStateFn CalculateStateFn =
	  [](const FIsdkInteractorStateEvent& Event)
	  {
		  FIsdkInteractionGroupMemberState State{};
		  // FakeInteractor1 is "Blocking", FakeInteractor2 is not.
		  State.bIsSelectStateBlocking = (Event.Interactor.GetObject()->GetName() == TEXT("FakeInteractor1"));
		  return State;
	  };

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::InitGroupMembersFn(Behavior, Behavior, CalculateStateFn)));

  // When blocking selector Selects, other one should disable.
  using EState = EIsdkInteractorState;
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Select, EState::Normal)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Select #1"), true, false)));

  // When non-blocking selector Selects, nothing should disable
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Normal, EState::Select)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Select #2"), true, true)));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkRigInteractionGroupDeletedInteractorTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.InteractionGroupDeletedInteractor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Tests that interactors can be removed from the interaction group, and that
// the state of the conditionals are updated correctly.
bool IsdkRigInteractionGroupDeletedInteractorTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));

  const FIsdkInteractionGroupMemberBehavior Behavior{ true, false, false };
      
  const FIsdkInteractorGroupMember::CalculateStateFn CalculateStateFn =
	  [](const FIsdkInteractorStateEvent& Event)
	  {
		  FIsdkInteractionGroupMemberState State{};
		  // FakeInteractor1 is "Blocking", FakeInteractor2 is not.
		  State.bIsSelectStateBlocking = (Event.Interactor.GetObject()->GetName() == TEXT("FakeInteractor1"));
		  return State;
	  };

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::InitGroupMembersFn(Behavior, Behavior, CalculateStateFn)));

  using EState = EIsdkInteractorState;
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::SetGroupMemberStatesFn(EState::Select, EState::Hover)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("Select #1"), true, false)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  // Now delete the first interactor. Second one should now be able to hover
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [](FAutomationTestBase*, AIsdkTestRigActor& Actor)
      { Actor.InteractionGroup->RemoveInteractor(Actor.FakeInteractor1); }));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  // After the component was removed, its conditional should be set to false and the other
  // conditional will become true as it is no longer blocked by the select.
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this, AIsdkTestRigActor::CheckGroupMemberStatesFn(TEXT("After Delete"), {}, true)));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      {
        Test->TestEqual(
            TEXT("After Delete conditional value"),
            Actor.InteractorGroupConditional1->GetResolvedValue(),
            false);
      }));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

  return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkRigControllerIsConnectedTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.ControllerIsConnected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool IsdkRigControllerIsConnectedTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestControllerVisualsInit(this));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestControllerVisualsCreateDataSources(this));

  // Check that toggling the connected state on the tracked controller will enable/disable
  // interactors.
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigControllerSetDataSourceConnectedValue(true));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigHandCheckInteractorEnabledValue(
      this, true, true, TEXT("When DataSource is connected")));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigControllerSetDataSourceConnectedValue(false));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigHandCheckInteractorEnabledValue(
      this, false, false, TEXT("When DataSource not connected")));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);
  return true;
}

class FIsdkControllerRigComponentTestBase : public FAutomationTestBase
{
 public:
  FIsdkControllerRigComponentTestBase(const FString& InName, bool bInComplexTask)
      : FAutomationTestBase(InName, bInComplexTask)
  {
  }

 protected:
  void CheckControllerHandBehavior(FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
  {
    const auto Mode = UIsdkFunctionLibrary::GetControllerHandBehavior(Actor.GetWorld());

    const auto AnimatedHandMesh = Actor.ControllerVisuals->GetAnimatedHandMeshComponent();
    const auto PoseableHandMesh = Actor.ControllerVisuals->GetPoseableHandMeshComponent();
    const auto ControllerMeshComponent = Actor.ControllerVisuals->GetControllerMeshComponent();

    switch (Mode)
    {
      case EControllerHandBehavior::BothProcedural:
        Test->TestFalse(
            "Controller's Animated Hand Mesh should not be visible in Both (Procedural) mode.",
            AnimatedHandMesh->IsVisible());
        Test->TestTrue(
            "Controller's Poseable Hand Mesh should be visible in Both (Procedural) mode.",
            PoseableHandMesh->IsVisible());
        Test->TestTrue(
            "Controller's Controller Mesh should be visible in Both (Procedural) mode.",
            ControllerMeshComponent->IsVisible());
        break;
      case EControllerHandBehavior::BothAnimated:
        Test->TestTrue(
            "Controller's Animated Hand Mesh should be visible in Both (Animated) mode.",
            AnimatedHandMesh->IsVisible());
        Test->TestFalse(
            "Controller's Poseable Hand Mesh should not be visible in Both (Animated) mode.",
            PoseableHandMesh->IsVisible());
        Test->TestTrue(
            "Controller's Controller Mesh should be visible in Both (Animated) mode.",
            ControllerMeshComponent->IsVisible());
        break;
      case EControllerHandBehavior::ControllerOnly:
        Test->TestFalse(
            "Controller's Animated Hand Mesh should not be visible in ControllerOnly mode.",
            AnimatedHandMesh->IsVisible());
        Test->TestFalse(
            "Controller's Poseable Hand Mesh should not be visible in ControllerOnly mode.",
            PoseableHandMesh->IsVisible());
        Test->TestTrue(
            "Controller's Controller Mesh should be visible in ControllerOnly mode.",
            ControllerMeshComponent->IsVisible());
        break;
      case EControllerHandBehavior::HandsOnlyProcedural:
        Test->TestFalse(
            "Controller's Animated Hand Mesh should not be visible in HandsOnly (Procedural) mode.",
            AnimatedHandMesh->IsVisible());
        Test->TestTrue(
            "Controller's Poseable Hand Mesh should be visible in HandsOnly (Procedural) mode.",
            PoseableHandMesh->IsVisible());
        Test->TestFalse(
            "Controller's Controller Mesh should not be visible in HandsOnly (Procedural) mode.",
            ControllerMeshComponent->IsVisible());
        break;
      case EControllerHandBehavior::HandsOnlyAnimated:
        Test->TestTrue(
            "Controller's Animated Hand Mesh should be visible in HandsOnly (Animated) mode.",
            AnimatedHandMesh->IsVisible());
        Test->TestFalse(
            "Controller's Poseable Hand Mesh should not be visible in HandsOnly (Animated) mode.",
            PoseableHandMesh->IsVisible());
        Test->TestFalse(
            "Controller's Controller Mesh should not be visible in HandsOnly (Animated) mode.",
            ControllerMeshComponent->IsVisible());
        break;
    }
  };

  void SetTestControllerHandBehavior(
      FAutomationTestBase* Test,
      AIsdkTestRigActor& Actor,
      EControllerHandBehavior ControllerHandBehavior)
  {
    UIsdkFunctionLibrary::SetControllerHandBehavior(Actor.GetWorld(), ControllerHandBehavior);
    Actor.ControllerVisuals->OnVisibilityUpdated(true, false);
  }
};

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(
    IsdkControllerRigComponentTest,
    FIsdkControllerRigComponentTestBase,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.Rig.ControllerHands",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool IsdkControllerRigComponentTest::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnComponents(this));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestControllerVisualsInit(this));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestControllerVisualsCreateDataSources(this));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      { SetTestControllerHandBehavior(Test, Actor, EControllerHandBehavior::BothAnimated); }))
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      { CheckControllerHandBehavior(Test, Actor); }))
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      { SetTestControllerHandBehavior(Test, Actor, EControllerHandBehavior::ControllerOnly); }))
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      { CheckControllerHandBehavior(Test, Actor); }))

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor) {
        SetTestControllerHandBehavior(Test, Actor, EControllerHandBehavior::HandsOnlyProcedural);
      }))
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      { CheckControllerHandBehavior(Test, Actor); }))

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      { SetTestControllerHandBehavior(Test, Actor, EControllerHandBehavior::HandsOnlyAnimated); }))
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestRigLambda(
      this,
      [this](FAutomationTestBase* Test, AIsdkTestRigActor& Actor)
      { CheckControllerHandBehavior(Test, Actor); }))

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);
  return true;
}
