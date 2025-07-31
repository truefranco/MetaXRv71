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

#include "OculusInteractionLog.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectHash.h"
#include "UObject/Class.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIsdkPrefixTest,
    "InteractionSDK.OculusInteraction.Source.OculusInteraction.Private.Tests.FIsdkPrefixTest.All",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIsdkPrefixTest::RunTest(const FString& Parameters)
{
  FString LeftStr = "", RightStr = "", FullObjectName = "";
  auto CompareObjNameSubstring =
      [&LeftStr, &RightStr, this](
          const FString& FullName, const FString& ObjectType, const FString& CompareString)
  {
    // This will hit both OculusInteraction and OculusInteractionEditor
    if (FullName.Contains(TEXT("/Script/OculusInteraction")))
    {
      FullName.Split(
          TEXT("."), &LeftStr, &RightStr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
      FString PrefixString = RightStr.LeftChop(RightStr.Len() - CompareString.Len());
      if (!TestEqual(TEXT("ISDK " + ObjectType + " Prefix"), PrefixString, CompareString))
      {
        UE_LOG(
            LogOculusInteraction,
            Warning,
            TEXT("FIsdkPrefixTest - Failed %s Name: %s"),
            *ObjectType,
            *RightStr);
      }
    }
  };

  TArray<UObject*> ObjectBuffer;

  // Iterate through every UClass
  GetObjectsOfClass(
      UClass::StaticClass(),
      ObjectBuffer,
      true,
      RF_ClassDefaultObject,
	  EInternalObjectFlags::AllFlags);
  for (const UObject* ThisObject : ObjectBuffer)
  {
    FullObjectName = ThisObject->GetFullName();
    CompareObjNameSubstring(FullObjectName, "Class", "Isdk");
  }

  // Iterate through every UScriptStruct (every UStruct also hits every class & function as well)
  ObjectBuffer.Empty();
  GetObjectsOfClass(
      UScriptStruct::StaticClass(),
      ObjectBuffer,
      true,
      RF_ClassDefaultObject,
	  EInternalObjectFlags::AllFlags);
  for (const UObject* ThisObject : ObjectBuffer)
  {
    FullObjectName = ThisObject->GetFullName();
    CompareObjNameSubstring(FullObjectName, "Struct", "Isdk");
  }

  // Iterate through every UEnum
  ObjectBuffer.Empty();
  GetObjectsOfClass(
      UEnum::StaticClass(),
      ObjectBuffer,
      true,
      RF_ClassDefaultObject,
	  EInternalObjectFlags::AllFlags);
  for (const UObject* ThisObject : ObjectBuffer)
  {
    FullObjectName = ThisObject->GetFullName();
    CompareObjNameSubstring(FullObjectName, "Enum", "EIsdk");
  }

  return true;
}
