// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusEyeTracker : ModuleRules
	{
		public OculusEyeTracker(ReadOnlyTargetRules Target) : base(Target)
		{
			if (Target.Platform == UnrealTargetPlatform.Win64 ||
				Target.Platform == UnrealTargetPlatform.Android)
			{
				PrivateIncludePaths.AddRange(
					new string[] {
						// Relative to Engine\Plugins\Runtime\Oculus\OculusVR\Source
						"OculusHMD/Private",
					});

				PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"InputDevice",
						"EyeTracker",
						"OVRPluginXR",
						"OculusHMD",
					}
				);

				PrivateDependencyModuleNames.AddRange(
					new string[]
					{
						"Core",
						"CoreUObject",
						"Engine",
						"InputCore",
					}
				);
            }
        }
	}
}
