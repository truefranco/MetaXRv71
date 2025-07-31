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

#include "IsdkTestRayFixtures.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/IsdkEventQueueImpl.h"
#include "Subsystem/IsdkWorldSubsystem.h"

bool VerifyEventsReceived(UIsdkTestRayFixture* RayTest, FAutomationTestBase* TestBase)
{
  bool bPassed =
      TestBase->TestEqual(
          TEXT("Interactor Queue Event Count"), RayTest->HandledInteractorStateEvents.Num(), 1) &&
      TestBase->TestEqual(
          TEXT("Interactable Queue Event Count"),
          RayTest->HandledInteractableStateEvents.Num(),
          1) &&
      TestBase->TestEqual(
          TEXT("Pointer Queue Event Count"), RayTest->HandledPointerEvents.Num(), 2);
  if (bPassed)
  {
    bPassed = TestBase->TestEqual(
                  TEXT("Interactor Event Value"),
                  RayTest->HandledInteractorStateEvents[0],
                  EIsdkInteractorState::Hover) &&
        TestBase->TestEqual(
            TEXT("Interactable Event Value"),
            RayTest->HandledInteractableStateEvents[0],
            EIsdkInteractableState::Hover) &&
        TestBase->TestEqual(
            TEXT("Pointer Event [0] Value"),
            RayTest->HandledPointerEvents[0].Type,
            EIsdkPointerEventType::Hover) &&
        TestBase->TestEqual(
            TEXT("Pointer Event [1] Value"),
            RayTest->HandledPointerEvents[1].Type,
            EIsdkPointerEventType::Move) &&
        TestBase->TestEqual(
            TEXT("Pointer Event Source [0] Interactable"),
            RayTest->HandledPointerEvents[0].Interactable.GetObject(),
            static_cast<UObject*>(RayTest->MockInteractable)) &&
        TestBase->TestEqual(
            TEXT("Pointer Event Source [1] Interactable"),
            RayTest->HandledPointerEvents[1].Interactable.GetObject(),
            static_cast<UObject*>(RayTest->MockInteractable));
  }
  return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIsdkSubsystemEventQueueImplTest,
    "InteractionSDK.OculusInteraction.Source.OculusInteraction.Private.Tests.Subsystem.EventQueueImpl",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIsdkSubsystemEventQueueImplTest::RunTest(const FString& Parameters)
{
  UIsdkTestRayFixture* RayTest = NewObject<UIsdkTestRayFixture>();

  isdk::api::helper::FInteractorStateEventQueueImpl InteractorStateEventQueue(
      [&RayTest]() -> isdk::api::IInteractor* { return &RayTest->RayInteractor.Get(); },
      &isdk::api::IInteractor::getIInteractorHandle,
      isdk::api::helper::CreateInteractorStateEventConverter(RayTest),
      TEXT("test"),
      RayTest->InteractorStateChanged);
  isdk::api::helper::FInteractableStateEventQueueImpl InteractableStateEventQueue(
      [&RayTest]() -> isdk::api::IInteractable* { return &RayTest->RayInteractable.Get(); },
      &isdk::api::IInteractable::getIInteractableHandle,
      isdk::api::helper::CreateInteractableStateEventConverter(RayTest),
      RayTest->InteractableStateChanged);
  auto PayloadLookup = [](const isdk_IPayload*) -> UIsdkInteractorComponent* { return nullptr; };
  isdk::api::helper::FPointerEventQueueImpl PointerEventQueue(
      [&RayTest]() -> isdk::api::IPointable* { return &RayTest->RayInteractable.Get(); },
      &isdk::api::IPointable::getIPointableHandle,
      isdk::api::helper::CreatePointerEventConverter(RayTest->MockInteractable, PayloadLookup),
      RayTest->InteractablePointed);

  // SetUp after we create the queues to test that adding delegate listeners after queue creation
  // works
  RayTest->SetUp();

  // The native queue instance will be lazy-created by this call. It won't watch for events until
  // it is created!
  InteractorStateEventQueue.GetOrCreateInstance();
  InteractableStateEventQueue.GetOrCreateInstance();
  PointerEventQueue.GetOrCreateInstance();

  // Invoke the interactor and drain event queue
  RayTest->RayInteractor->drive();
  InteractorStateEventQueue.TryHandleEvents();
  InteractableStateEventQueue.TryHandleEvents();
  PointerEventQueue.TryHandleEvents();

  // Verify Assertions
  const bool bPassed = VerifyEventsReceived(RayTest, this);

  // Cleanup
  RayTest->TearDown();
  delete RayTest;

  return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIsdkSubsystemTickTest,
    "InteractionSDK.OculusInteraction.Source.OculusInteraction.Private.Tests.Subsystem.Tick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIsdkSubsystemTickTest::RunTest(const FString& Parameters)
{
  UIsdkTestRayFixture* RayTest = NewObject<UIsdkTestRayFixture>();
  RayTest->SetUp();

  // Create a test instance of the subsystem
  FSubsystemCollection<UWorldSubsystem> TestSubsystemCollection{};
  UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
  TestSubsystemCollection.Initialize(World);
  const auto Subsystem =
      TestSubsystemCollection.GetSubsystem<UIsdkWorldSubsystem>(UIsdkWorldSubsystem::StaticClass());
  if (!TestTrue("Failed to create UIsdkWorldSubsystem", IsValid(Subsystem)))
  {
    return false;
  }

  // Listen to interactor/able events and forward to our text fixture
  const auto UpdatedToken = Subsystem->RegisterUpdateEventHandler(
      [RayTest]() -> isdk::api::IUpdate* { return &RayTest->RayInteractor.Get(); },
      RayTest->Updated);
  const auto InteractorStateChangedToken = Subsystem->RegisterInteractorStateEventHandler(
      RayTest,
      TEXT("test"),
      [RayTest]() -> isdk::api::IInteractor* { return &RayTest->RayInteractor.Get(); },
      RayTest->InteractorStateChanged);
  const auto InteractableStateChangedToken = Subsystem->RegisterInteractableStateEventHandler(
      RayTest,
      [RayTest]() -> isdk::api::IInteractable* { return &RayTest->RayInteractable.Get(); },
      RayTest->InteractableStateChanged);
  const auto InteractablePointedToken = Subsystem->RegisterPointerEventHandler(
      [RayTest]() -> isdk::api::IPointable* { return &RayTest->RayInteractable.Get(); },
      RayTest->InteractablePointed,
      RayTest->MockInteractable);

  // Invoke the interactor and drain event queue
  Subsystem->Tick(0.01f);

  // Verify Assertions
  const bool bPassed = VerifyEventsReceived(RayTest, this);

  // Cleanup
  Subsystem->UnregisterUpdateEventHandler(UpdatedToken);
  Subsystem->UnregisterInteractorStateEventHandler(InteractorStateChangedToken);
  Subsystem->UnregisterInteractableStateEventHandler(InteractableStateChangedToken);
  Subsystem->UnregisterPointerEventHandler(InteractablePointedToken);

  TestSubsystemCollection.Deinitialize();
  RayTest->TearDown();
  delete RayTest;

  World->DestroyWorld(false);
  delete RayTest;

  return bPassed;
}
