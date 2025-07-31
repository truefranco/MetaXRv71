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

#include "Tests/IsdkTestGrabInteraction.h"
#include "Tests/IsdkCommonTestCommands.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

// Command: Spawn Test Actors
DEFINE_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnGrabActors);
bool FIsdkTestSpawnGrabActors::Update()
{
  AIsdkTestGrabInteractorActor::Setup();
  AIsdkTestGrabInteractableActor::Setup();
  return true;
}

// Command: Set Grab Interactor Position
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FIsdkTestSetGrabInteractorPosition,
    FAutomationTestBase*,
    Test,
    FVector,
    ActorPosition);
bool FIsdkTestSetGrabInteractorPosition::Update()
{
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  Test->TestTrue("Grab Interactor Actor was not created", IsValid(GrabInteractorActor));
  GrabInteractorActor->SetActorLocation(ActorPosition);
  return true;
}

// Command: Set Grab Interactable Position
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FIsdkTestSetGrabInteractablePosition,
    FAutomationTestBase*,
    Test,
    FVector,
    ActorPosition);
bool FIsdkTestSetGrabInteractablePosition::Update()
{
  AIsdkTestGrabInteractableActor* GrabInteractableActor = &AIsdkTestGrabInteractableActor::Get();
  Test->TestTrue("Grab Interactable Actor was not created", IsValid(GrabInteractableActor));
  GrabInteractableActor->SetActorLocation(ActorPosition);
  return true;
}

// Command: Setup Interactable Collision
DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(
    FIsdkTestSetGrabInteractableCollision,
    FAutomationTestBase*,
    Test,
    EIsdkGrabbableColliderMode,
    CollisionMode,
    float,
    CollisionSize);
bool FIsdkTestSetGrabInteractableCollision::Update()
{
  AIsdkTestGrabInteractableActor* GrabInteractableActor = &AIsdkTestGrabInteractableActor::Get();
  Test->TestTrue("Grab Interactable Actor was not created", IsValid(GrabInteractableActor));
  GrabInteractableActor->SetColliderMode(CollisionMode, CollisionSize);

  return true;
}

// Command: Perform Pinch Grab
DEFINE_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPinchGrab);
bool FIsdkTestPerformPinchGrab::Update()
{
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  GrabInteractorActor->PinchGrab();
  return true;
}

// Command: Perform Pinch Release
DEFINE_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPinchRelease);
bool FIsdkTestPerformPinchRelease::Update()
{
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  GrabInteractorActor->PinchRelease();
  return true;
}

// Command: Perform Palm Grab
DEFINE_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPalmGrab);
bool FIsdkTestPerformPalmGrab::Update()
{
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  GrabInteractorActor->PalmGrab();
  return true;
}

// Command: Perform Palm Release
DEFINE_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPalmRelease);
bool FIsdkTestPerformPalmRelease::Update()
{
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  GrabInteractorActor->PalmRelease();
  return true;
}

// Command: Destroy Grabbable while being grabbed
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestDestroyGrabbable,
    FAutomationTestBase*,
    Test);
bool FIsdkTestDestroyGrabbable::Update()
{
  AIsdkTestGrabInteractableActor* GrabInteractableActor = &AIsdkTestGrabInteractableActor::Get();
  Test->TestTrue("Grab Interactable Actor was not created", IsValid(GrabInteractableActor));
  GrabInteractableActor->Destroy();
  return true;
}

// Command: Disable Palm Grab on Grabbable
DEFINE_LATENT_AUTOMATION_COMMAND(FIsdkTestDisablePalmGrab);
bool FIsdkTestDisablePalmGrab::Update()
{
  AIsdkTestGrabInteractableActor* GrabInteractableActor = &AIsdkTestGrabInteractableActor::Get();
  GrabInteractableActor->SetGrabInput(EIsdkGrabInputMethod::Palm, false);
  return true;
}

// Command (Test): Validate Hover State for Grabbable
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestValidateHoverState,
    FAutomationTestBase*,
    Test);
bool FIsdkTestValidateHoverState::Update()
{
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  Test->TestTrue("Grab Interactor Actor was not created", IsValid(GrabInteractorActor));

  AIsdkTestGrabInteractableActor* GrabInteractableActor = &AIsdkTestGrabInteractableActor::Get();
  Test->TestTrue("Grab Interactable Actor was not created", IsValid(GrabInteractableActor));

  UIsdkHandGrabDetector* TestGrabDetector = GrabInteractorActor->GetHandGrabDetector();
  const TSet<UIsdkGrabbableComponent*> HoveredGrabbables =
      TestGrabDetector->GetHoveredGrabbables();

  UIsdkGrabbableComponent* HoveredComponent = nullptr;
  if (HoveredGrabbables.Num() == 1)
  {
    const bool bHoveredContainsGrabbable =
        HoveredGrabbables.Contains(GrabInteractableActor->GetGrabbableComponent());
    FString TestDescription = FString::Printf(
        TEXT(
            "Validate Hover: Grabber (via HandGrabDetector) is Hovering Grabbable? (actual: %s, expected: true)"),
        bHoveredContainsGrabbable ? TEXT("true") : TEXT("false"),
        true);
    Test->TestEqual(*TestDescription, bHoveredContainsGrabbable, true);
  }
  else
  {
    Test->AddError(TEXT("HoveredGrabbables does not have only one element"));
  }

  return true;
}

// Command (Test): Validate Grab State
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FIsdkTestValidateGrabState,
    FAutomationTestBase*,
    Test,
    bool,
    bTestState);
bool FIsdkTestValidateGrabState::Update()
{
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  Test->TestTrue("Grab Interactor Actor was not created", IsValid(GrabInteractorActor));

  UIsdkGrabberComponent* TestGrabberComponent = GrabInteractorActor->GetGrabberComponent();
  const bool bIsGrabberGrabbing = TestGrabberComponent->IsGrabbing();

  FString TestDescription = FString::Printf(
      TEXT("Validate Grab States: Grabber is Grabbing? (actual: %s, expected: %s)"),
      bIsGrabberGrabbing ? TEXT("true") : TEXT("false"),
      bTestState ? TEXT("true") : TEXT("false"));
  Test->TestEqual(*TestDescription, bIsGrabberGrabbing, bTestState);
  return true;
}

// Command (Test): Validate Grabber Grabbing Test Grabbable
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FIsdkTestValidateGrabbedObject,
    FAutomationTestBase*,
    Test);
bool FIsdkTestValidateGrabbedObject::Update()
{
  AIsdkTestGrabInteractableActor* GrabInteractableActor = &AIsdkTestGrabInteractableActor::Get();
  Test->TestTrue("Grab Interactable Actor was not created", IsValid(GrabInteractableActor));
  AIsdkTestGrabInteractorActor* GrabInteractorActor = &AIsdkTestGrabInteractorActor::Get();
  Test->TestTrue("Grab Interactor Actor was not created", IsValid(GrabInteractorActor));

  UIsdkGrabberComponent* TestGrabberComponent = GrabInteractorActor->GetGrabberComponent();
  const UIsdkGrabbableComponent* TestGrabbableComponent =
      GrabInteractableActor->GetGrabbableComponent();
  const UIsdkGrabbableComponent* GrabbedComponent = TestGrabberComponent->GetGrabbedComponent();

  FString TestDescription =
      FString::Printf(TEXT("Validate Grab States: Test Grabber is Grabbing Test Grabbable?"));
  Test->TestEqual(*TestDescription, TestGrabbableComponent, GrabbedComponent);
  return true;
}

// Command (Test): Validate Interactable Location
DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(
    FIsdkTestValidateInteractableLocation,
    FAutomationTestBase*,
    Test,
    FVector,
    LocationTestIn,
    bool,
    bTestCondition);
bool FIsdkTestValidateInteractableLocation::Update()
{
  AIsdkTestGrabInteractableActor* GrabInteractableActor = &AIsdkTestGrabInteractableActor::Get();
  Test->TestTrue("Grab Interactable Actor was not created", IsValid(GrabInteractableActor));

  const FVector InteractableLocation = GrabInteractableActor->GetActorLocation();

  const bool bActorAtLocation = InteractableLocation.Equals(LocationTestIn, 0.01f);
  FString TestDescription = FString::Printf(
      TEXT(
          "Validate Interactable Location, Validating (actual: %s tested: %s) Result: (actual: %s, expected: %s)"),
      *InteractableLocation.ToString(),
      *LocationTestIn.ToString(),
      bActorAtLocation ? TEXT("true") : TEXT("false"),
      bTestCondition ? TEXT("true") : TEXT("false"));
  Test->TestEqual(*TestDescription, bActorAtLocation, bTestCondition);

  return true;
}

// Automation: Grab Battery
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkGrabInteractionHoverGrabTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.IsdkGrabInteractionHoverGrabTests",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool IsdkGrabInteractionHoverGrabTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  // Initial spawn & setup
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnGrabActors());
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetGrabInteractableCollision(this, EIsdkGrabbableColliderMode::Sphere, 10.f));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetGrabInteractorPosition(this, FVector(-1000.0, -1000.0, -1000.0)));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetGrabInteractablePosition(this, FVector(1.0, 1.0, 1.0)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  // Move interactor to the interactable, test that we're hovering
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetGrabInteractorPosition(this, FVector(1.0, 1.0, 1.0)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateHoverState(this));

  // Pinch grab the interactable, confirm state, release and confirm it is no longer being grabbed
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPinchGrab());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, true));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabbedObject(this));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPinchRelease());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, false));

  // Palm grab the interactable, confirm state, release and confirm state
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPalmGrab());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, true));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabbedObject(this));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPalmRelease());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, false));

  // Disable the palm grab input mode and confirm we cannot grab it
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestDisablePalmGrab());
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPalmGrab());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, false));

  // Pinch grab again, move the interactor, and validate the interactable has moved (via
  // transformer), release
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPinchGrab());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, true));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetGrabInteractorPosition(this, FVector(5.0, 5.0, 5.0)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestValidateInteractableLocation(this, FVector(1.0, 1.0, 1.0), false));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPinchRelease());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  // Pinch grab again, destroy held grabbable, confirm it is no longer grabbing
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestPerformPinchGrab());
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, true));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestDestroyGrabbable(this));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestValidateGrabState(this, false));

  return true;
}
