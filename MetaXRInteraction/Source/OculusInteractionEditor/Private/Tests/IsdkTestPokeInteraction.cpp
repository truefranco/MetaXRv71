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

#include "IsdkTestPokeInteraction.h"
#include "Misc/AutomationTest.h"
#include "Tests/IsdkCommonTestCommands.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

static std::function<
    void(TScriptInterface<IIsdkIInteractorState>&, TScriptInterface<IIsdkIInteractableState>&)>
IsdkTestPokeInteractionCreateGetActorsFn()
{
  return [](TScriptInterface<IIsdkIInteractorState>& OutInteractor,
            TScriptInterface<IIsdkIInteractableState>& OutInteractable)
  {
    OutInteractor = AIsdkTestPokeInteractorActor::Get().TestPokeInteractor;
    OutInteractable = AIsdkTestPokeInteractableActor::Get().TestPokeInteractable;
  };
}

DEFINE_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnActors);
bool FIsdkTestSpawnActors::Update()
{
  AIsdkTestPokeInteractorActor::Setup();
  AIsdkTestPokeInteractableActor::Setup();
  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(
    FIsdkTestSetPokeInteractorPosition,
    FAutomationTestBase*,
    Test,
    FVector,
    ActorPosition);
bool FIsdkTestSetPokeInteractorPosition::Update()
{
  AIsdkTestPokeInteractorActor* PokeInteractorActor = &AIsdkTestPokeInteractorActor::Get();
  Test->TestTrue("Poke Interactor Actor was not created", IsValid(PokeInteractorActor));
  PokeInteractorActor->SetActorLocation(ActorPosition);
  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(
    FIsdkTestCheckPokeInteractableCurrentState,
    FAutomationTestBase*,
    Test,
    EIsdkInteractableState,
    ExpectedState,
    FString,
    TestStepName);
bool FIsdkTestCheckPokeInteractableCurrentState::Update()
{
  AIsdkTestPokeInteractableActor* PokeInteractableActor = &AIsdkTestPokeInteractableActor::Get();
  Test->TestTrue("Poke Interactable Actor was not created", IsValid(PokeInteractableActor));
  EIsdkInteractableState CurrentState =
      PokeInteractableActor->TestPokeInteractable->GetCurrentState();
  const FString TestDescription = FString::Printf(
      TEXT("%s: Poke Interactable State (actual: %d, expected: %d)"),
      *TestStepName,
      CurrentState,
      ExpectedState);
  Test->TestEqual(*TestDescription, CurrentState, ExpectedState);
  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(
    FIsdkTestSetPokeInteractableSurfaceClippers,
    float,
    ActorScale,
    FVector,
    ClipperPosition,
    float,
    ClipperSize);
bool FIsdkTestSetPokeInteractableSurfaceClippers::Update()
{
  AIsdkTestPokeInteractableActor* PokeInteractableActor = &AIsdkTestPokeInteractableActor::Get();
  PokeInteractableActor->SetSingleClipper(ClipperPosition, ClipperSize);
  PokeInteractableActor->SetActorScale3D(FVector(ActorScale, ActorScale, ActorScale));
  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(
    FIsdkTestSetPokeInteractableSurfaceClippersOverlap,
    float,
    ActorScale,
    FVector,
    ClipperPosition,
    float,
    ClipperSize,
    float,
    ClipperOffset);
bool FIsdkTestSetPokeInteractableSurfaceClippersOverlap::Update()
{
  AIsdkTestPokeInteractableActor* PokeInteractableActor = &AIsdkTestPokeInteractableActor::Get();
  PokeInteractableActor->SetTwoClippers(ClipperPosition, ClipperSize, ClipperOffset);
  PokeInteractableActor->SetActorScale3D(FVector(ActorScale, ActorScale, ActorScale));
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkPokeInteractionClippersUpdateTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.IsdkPokeInteractionClippersUpdateTests",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool IsdkPokeInteractionClippersUpdateTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnActors());

  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractorPosition(this, FVector(-1.0, -2.0, -2.0)));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractableSurfaceClippers(1.0, FVector(0.0, 0.0, 0.0), 1.0));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this, EIsdkInteractableState::Normal, TEXT("Initial Poke interactable state check")));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetPokeInteractorPosition(this, FVector(-1.0, 0.0, 0.0)));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this, EIsdkInteractableState::Hover, TEXT("Poke interactable check after adding clippers.")));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractableSurfaceClippers(1.0, FVector(0.0, 5.0, 5.0), 3.0));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this,
      EIsdkInteractableState::Normal,
      TEXT("Poke interactable check after moving clippers out of the way.")));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetPokeInteractorPosition(this, FVector(-1.0, 5.0, 5.0)));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this,
      EIsdkInteractableState::Hover,
      TEXT("Poke interactable check after moving interactor into the clipper.")));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractableSurfaceClippers(2.0, FVector(0.0, 5.0, 5.0), 3.0));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this,
      EIsdkInteractableState::Normal,
      TEXT("Poke interactable check after scaling clipper causing them to move out of the way.")));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetPokeInteractorPosition(this, FVector(-1.0, 8.0, 8.0)));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this,
      EIsdkInteractableState::Hover,
      TEXT(
          "Poke interactable check after scaling clipper then moving the interactor near the clipped surface again.")));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetPokeInteractorPosition(this, FVector(-1.0, 0.0, 0.0)));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractableSurfaceClippersOverlap(1.0, FVector(0.0, 0.0, 0.0), 3.0, 5.0));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this,
      EIsdkInteractableState::Normal,
      TEXT("Poke interactable check after setting 2 non overlapping clippers.")));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractableSurfaceClippersOverlap(1.0, FVector(0.0, 0.0, 0.0), 3.0, 1.0));

  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this,
      EIsdkInteractableState::Hover,
      TEXT("Poke interactable check after setting 2 overlapping clippers.")));

  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);

  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    IsdkPokeInteractionStateTests,
    "InteractionSDK.OculusInteraction.Source.OculusInteractionEditor.Private.Tests.IsdkPokeInteractionStateTests",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool IsdkPokeInteractionStateTests::RunTest(const FString& Parameters)
{
  isdk::test::AddInitPieTestSteps(this);

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSpawnActors());

  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractorPosition(this, FVector(-1.0, -2.0, -2.0)));
  ADD_LATENT_AUTOMATION_COMMAND(
      FIsdkTestSetPokeInteractableSurfaceClippers(1.0, FVector(0.0, 0.0, 0.0), 1.0));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this, EIsdkInteractableState::Normal, TEXT("State when normal.")));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckInteractionRelationships(
      this, IsdkTestPokeInteractionCreateGetActorsFn(), {0, 0}, TEXT("Relationships when normal")));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetPokeInteractorPosition(this, FVector(-1.0, 0.0, 0.0)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this, EIsdkInteractableState::Hover, TEXT("State when hovered.")));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckInteractionRelationships(
      this,
      IsdkTestPokeInteractionCreateGetActorsFn(),
      {1, 0},
      TEXT("Relationships when hovered")));

  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestSetPokeInteractorPosition(this, FVector(0.0, 0.0, 0.0)));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(isdk::test::OneFrameDelay));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckPokeInteractableCurrentState(
      this, EIsdkInteractableState::Select, TEXT("State when selected.")));
  ADD_LATENT_AUTOMATION_COMMAND(FIsdkTestCheckInteractionRelationships(
      this,
      IsdkTestPokeInteractionCreateGetActorsFn(),
      {0, 1},
      TEXT("Relationships when selected")));

  return true;
}
