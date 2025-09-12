// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class EngineTelemetry : ModuleRules
{
	public EngineTelemetry(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;


		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Include"));

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// EngineTelemetry_APL.xml is reponsible for copying over the .so library
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "EngineTelemetry_APL.xml"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "Lib/Win64/EngineTelemetry.dll"));
		}
	}
}
