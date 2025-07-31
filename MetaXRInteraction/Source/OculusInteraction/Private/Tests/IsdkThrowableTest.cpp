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

#include "Interaction/IsdkThrowable.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIsdkThrowableTest,
    "InteractionSDK.OculusInteraction.Source.OculusInteraction.Private.Tests.FIsdkThrowableTest.All",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIsdkThrowableTest::RunTest(const FString& Parameters)
{
  UIsdkThrowable* MockThrowable = NewObject<UIsdkThrowable>();
  MockThrowable->Settings.SampleSize = 3;
  // Set up sample data for positions and rotations
  TArray<TPair<FVector, float>> SamplePositions;
  SamplePositions.Add(TPair<FVector, float>(FVector(0, 0, 0), 0.0f));
  SamplePositions.Add(TPair<FVector, float>(FVector(2, 0, 0), 1.0f));
  SamplePositions.Add(TPair<FVector, float>(FVector(4, 0, 0), 2.0f));

  TArray<TPair<FQuat, float>> SampleRotations;
  SampleRotations.Add(TPair<FQuat, float>(FQuat::Identity, 0.0f));
  SampleRotations.Add(TPair<FQuat, float>(FQuat(1, 0, 0, 0), 1.0f));
  SampleRotations.Add(TPair<FQuat, float>(FQuat(1, 1, 0, 0), 2.0f));

  MockThrowable->SetSamplePositions(SamplePositions);
  MockThrowable->SetSampleRotations(SampleRotations);

  const FVector ReleaseVelocity = MockThrowable->GetVelocity();
  // Check if the release velocity is close to the expected value
  TestEqual("Release Velocity X", ReleaseVelocity.X, 2.0f, 0.001f);
  TestEqual("Release Velocity Y", ReleaseVelocity.Y, 0.0f, 0.001f);
  TestEqual("Release Velocity Z", ReleaseVelocity.Z, 0.0f, 0.001f);

  const FQuat ReleaseAngularVelocity = MockThrowable->GetAngularVelocity();
  // Check if the release angular velocity is close to the expected value
  TestEqual("Release Angular Velocity Z", ReleaseAngularVelocity.W, 1.0f, 0.001f);

  return true;
}
