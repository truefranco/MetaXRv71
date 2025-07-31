// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusScene : ModuleRules
	{
		public OculusScene(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"OculusHMD",
					"OculusAnchors",
					"OVRPluginXR",
					"ProceduralMeshComponent",
				});

			PrivateIncludePaths.AddRange(
				new string[] {
					// Relative to Engine\Plugins\Runtime\Oculus\OculusVR\Source
					"OculusHMD/Private",
					"OculusAnchors/Private",
					"../../../../../Source/Runtime/Engine/Classes/Components",
				});
		}
	}
}
