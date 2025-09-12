// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRToolCommands.h"

#include "OculusXRProjectSetupToolModule.h"
#include "OculusXRSyntheticEnvironmentServer.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/EngineVersionComparison.h"

#define LOCTEXT_NAMESPACE "FOculusXREditorModule"

#if UE_VERSION_OLDER_THAN(5, 5, 0)
#define AS_LOCALIZABLE_ADVANCED(Namespace, Key, TextLiteral) \
	FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(TextLiteral, Namespace, Key)
#else
#define AS_LOCALIZABLE_ADVANCED(Namespace, Key, TextLiteral) \
	FText::AsLocalizable_Advanced(Namespace, Key, TextLiteral)
#endif

void FOculusToolCommands::RegisterCommands()
{
	UI_COMMAND(OpenProjectSetupTool, "Meta XR Project Setup Tool", "Show Meta XR Project Setup Tool", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToggleDeploySo, "Deploy compiled .so directly to device", "Faster deploy when we only have code changes by deploying compiled .so directly to device", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(ToggleIterativeCookOnTheFly, "Enable Iterative Cook on the Fly", "Faster deploy for asset changes by keeping previously cooked contents on the device (Uses Cook on the Fly)", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(OpenPlatWindow, "Meta XR Platform Window", "Show Meta XR Platform Window", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(ToggleMetaXRSim, "Meta XR Simulator", "Activate/Deactivate Meta XR Simulator", EUserInterfaceActionType::ToggleButton, FInputChord());

	UI_COMMAND(CheckForUpdateXRSim, "Check For Updates", "Check If Meta XR Simulator Update Is Available.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(UpdateXRSim, "Update To Latest Version", "Update Meta XR Simulator To Latest Version.", EUserInterfaceActionType::Button, FInputChord());

#if PLATFORM_WINDOWS
	static const FString launch("Launch ");
	static const FString dot(".");
	const auto& rooms = FMetaXRSES::GetSynthEnvRooms();
	for (const auto& room : rooms)
	{
		TSharedPtr<FUICommandInfo> command;
		const FString DotCommandName = dot + room.GuiName;
		const FString InDescription = launch + room.GuiName;
		const FString InCommandNameUnderscoreTooltip = room.GuiName + TEXT("_ToolTip");

		const FString InSubNamespace = TEXT(LOCTEXT_NAMESPACE);
		const FString UICommandsStr = TEXT("UICommands");
		const FString Namespace = UICommandsStr + dot + InSubNamespace;

		FUICommandInfo::MakeCommandInfo(
			this->AsShared(),
			command,
			FName(InCommandNameUnderscoreTooltip),
			AS_LOCALIZABLE_ADVANCED(*Namespace, *InDescription, *InDescription),
			AS_LOCALIZABLE_ADVANCED(*Namespace, InCommandNameUnderscoreTooltip.GetCharArray().GetData(), *InDescription),
			FSlateIcon(),
			EUserInterfaceActionType::Button,
			FInputChord(),
			FInputChord());
		RoomCommands.Add(command);
	}
#endif

	UI_COMMAND(StopServer, "Stop Server", "Stop Server", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(OpenSettings, "Open Settings", "Open Meta XR Simulator Settings", EUserInterfaceActionType::Button, FInputChord());
}

void FOculusToolCommands::ShowOculusTool()
{
	IOculusXRProjectSetupToolModule::Get().ShowProjectSetupTool("Console");
}

#undef LOCTEXT_NAMESPACE
