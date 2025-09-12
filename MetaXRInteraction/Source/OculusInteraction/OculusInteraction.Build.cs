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

// @generated - signal to linters that this file should be excluded

using System.IO;
using UnrealBuildTool;

public class OculusInteraction : ModuleRules
{
    public OculusInteraction(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] { }
        );

        PrivateIncludePaths.AddRange(
            new[]
            {
                "ThirdParty/OculusInteractionLibrary/Include"
            }
        );

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "OculusInteractionLibrary",
                "Projects",
                "HeadMountedDisplay",
                "XRBase",
                "InputCore",
                "Slate",
                "SlateCore",
                "UMG",
                "EnhancedInput",
                "GameplayTags",
                "Niagara"
            }
        );

        // InteractionSDK DLL
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/OculusInteractionLibrary/Lib/" + Target.Platform.ToString() + "/InteractionSDK.dll");
        }

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
            });
        }

        if (Target.Platform == UnrealTargetPlatform.Android)
		{
			var PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "OculusInteraction_APL.xml"));
		}
	}
}
