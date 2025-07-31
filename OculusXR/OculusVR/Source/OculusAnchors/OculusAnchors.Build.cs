// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusAnchors : ModuleRules
	{
		public OculusAnchors(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"OculusHMD",
					"OVRPluginXR",
					"ProceduralMeshComponent",
				});

			PrivateIncludePaths.AddRange(
				new string[] {
					// Relative to Engine\Plugins\Runtime\Oculus\OculusVR\Source
					"OculusHMD/Private",
					"../../../../../Source/Runtime/Engine/Classes/Components",
				});
		}
	}
}
