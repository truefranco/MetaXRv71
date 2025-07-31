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

#include "Interaction/IsdkIInteractableState.h"
#include "Interaction/IsdkIInteractorState.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Misc/Paths.h"

#include <functional>

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(
    FIsdkTestCheckInteractionRelationships,
    FAutomationTestBase*,
    Test,
    std::function<
        void(TScriptInterface<IIsdkIInteractorState>&, TScriptInterface<IIsdkIInteractableState>&)>,
    GetTestInstances,
    FIsdkInteractionRelationshipCounts,
    ExpectedCounts,
    FString,
    TestStepName);

namespace isdk {
	namespace test
	{
		constexpr float OneFrameDelay = 0.05f;

		inline void AddInitPieTestSteps(FAutomationTestBase* TestBase)
		{
			const auto TestMapPath = FPaths::Combine(
				FPaths::ProjectPluginsDir(),
				TEXT("OculusInteraction"),
				TEXT("Content"),
				TEXT("AutomationTests"),
				TEXT("Maps"),
				TEXT("IsdkTestEmptyMap"));
			ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TestMapPath));
			ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(OneFrameDelay));
			ADD_LATENT_AUTOMATION_COMMAND(
				FEditorAutomationLogCommand(FString::Printf(TEXT("LoadMap-End: %s"), *TestMapPath)));
			ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
			ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(OneFrameDelay));
			ADD_LATENT_AUTOMATION_COMMAND(FEditorAutomationLogCommand(TEXT("StartPIE-End")));
		}
	}
}; // namespace isdk::test
