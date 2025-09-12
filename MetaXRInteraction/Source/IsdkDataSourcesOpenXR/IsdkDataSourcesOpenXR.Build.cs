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

using UnrealBuildTool;
using System.IO;

public class IsdkDataSourcesOpenXR : ModuleRules
{
    public IsdkDataSourcesOpenXR(ReadOnlyTargetRules Target) : base(Target)
    {
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Core",
                "Engine",
                "OculusInteraction",
                "HeadMountedDisplay",
                "InputCore",
                "XRBase",
                "OpenXRHandTracking"
            }
        );
        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            // AndroidPlugin
            {
                string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
                AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "OpenXRAndroidPermissions_UPL.xml"));
            }
        }
    }
}
