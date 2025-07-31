// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusEditorModule.h"
#include "OculusToolStyle.h"
#include "OculusToolCommands.h"
#include "OculusToolWidget.h"
#include "OculusPlatformToolWidget.h"
#include "OculusAssetDirectory.h"
#include "OculusHMDRuntimeSettings.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "PropertyEditorModule.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISettingsModule.h"
#include "OculusEditorSettings.h"

#define LOCTEXT_NAMESPACE "OculusEditor"

const FName FOculusEditorModule::OculusPerfTabName = FName("OculusPerfCheck");
const FName FOculusEditorModule::OculusPlatToolTabName = FName("OculusPlatormTool");

void FOculusEditorModule::PostLoadCallback()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
}

void FOculusEditorModule::StartupModule()
{
	bModuleValid = true;
	RegisterSettings();
	FOculusAssetDirectory::LoadForCook();

	if (!IsRunningCommandlet())
	{
		FOculusToolStyle::Initialize();
		FOculusToolStyle::ReloadTextures();

		FOculusToolCommands::Register();

		PluginCommands = MakeShareable(new FUICommandList);

		PluginCommands->MapAction(
			FOculusToolCommands::Get().OpenPluginWindow,
			FExecuteAction::CreateRaw(this, &FOculusEditorModule::PluginButtonClicked),
			FCanExecuteAction());
		PluginCommands->MapAction(
			FOculusToolCommands::Get().OpenPlatWindow,
			FExecuteAction::CreateRaw(this, &FOculusEditorModule::PluginOpenPlatWindow),
			FCanExecuteAction());
		PluginCommands->MapAction(
			FOculusToolCommands::Get().ToggleDeploySo,
			FExecuteAction::CreateLambda([=]() {
				UOculusHMDRuntimeSettings* settings = GetMutableDefault<UOculusHMDRuntimeSettings>();
				settings->bDeploySoToDevice = !settings->bDeploySoToDevice;
				}),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([=]() {
				return GetMutableDefault<UOculusHMDRuntimeSettings>()->bDeploySoToDevice;
				}));
		PluginCommands->MapAction(
			FOculusToolCommands::Get().ToggleMetaXRSim,
			FExecuteAction::CreateRaw(this, &FOculusEditorModule::ToggleOpenXRRuntime),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([=]() {
				return false;//FMetaXRSimulator::IsSimulatorActivated();
				}));
		PluginCommands->MapAction(
			FOculusToolCommands::Get().LaunchGameRoom,
			FExecuteAction::CreateRaw(this, &FOculusEditorModule::LaunchSESGameRoom),
			FCanExecuteAction());
		PluginCommands->MapAction(
			FOculusToolCommands::Get().LaunchLivingRoom,
			FExecuteAction::CreateRaw(this, &FOculusEditorModule::LaunchSESLivingRoom),
			FCanExecuteAction());
		PluginCommands->MapAction(
			FOculusToolCommands::Get().LaunchBedroom,
			FExecuteAction::CreateRaw(this, &FOculusEditorModule::LaunchSESBedroom),
			FCanExecuteAction());
		PluginCommands->MapAction(
			FOculusToolCommands::Get().StopServer,
			FExecuteAction::CreateRaw(this, &FOculusEditorModule::StopSESServer),
			FCanExecuteAction());

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

		// Adds an option to launch the tool to Window->Developer Tools.
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("Miscellaneous", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FOculusEditorModule::AddMenuExtension));
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

		// We add the Oculus menu on the toolbar
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Play", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FOculusEditorModule::AddToolbarExtension));
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

		/*
		// If you want to make the tool even easier to launch, and add a toolbar button.
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Launch", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FOculusEditorModule::AddToolbarExtension));
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		*/

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(OculusPerfTabName, FOnSpawnTab::CreateRaw(this, &FOculusEditorModule::OnSpawnPluginTab))
			.SetDisplayName(LOCTEXT("FOculusEditorTabTitle", "Oculus Performance Check"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(OculusPlatToolTabName, FOnSpawnTab::CreateRaw(this, &FOculusEditorModule::OnSpawnPlatToolTab))
			.SetDisplayName(LOCTEXT("FOculusPlatfToolTabTitle", "Oculus Platform Tool"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);
	}
}

void FOculusEditorModule::ShutdownModule()
{
	if (!bModuleValid)
	{
		return;
	}

	if (!IsRunningCommandlet())
	{
		FOculusToolStyle::Shutdown();
		FOculusToolCommands::Unregister();
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(OculusPerfTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(OculusPlatToolTabName);
	}

	FOculusAssetDirectory::ReleaseAll();
	if (UObjectInitialized())
	{
		UnregisterSettings();
	}
}

TSharedRef<SDockTab> FOculusEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	auto myTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SOculusToolWidget)
		];


	return myTab;
}

TSharedRef<SDockTab> FOculusEditorModule::OnSpawnPlatToolTab(const FSpawnTabArgs& SpawnTabArgs)
{
	auto myTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SOculusPlatformToolWidget)
		];

	return myTab;
}

void FOculusEditorModule::RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "OculusVR",
			LOCTEXT("RuntimeSettingsName", "OculusVR"),
			LOCTEXT("RuntimeSettingsDescription", "Configure the OculusVR plugin"),
			GetMutableDefault<UOculusHMDRuntimeSettings>()
		);

		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(UOculusHMDRuntimeSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FOculusHMDSettingsDetailsCustomization::MakeInstance));
	}
}

void FOculusEditorModule::UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "OculusVR");
	}
}

FReply FOculusEditorModule::PluginClickFn(bool text)
{
	PluginButtonClicked();
	return FReply::Handled();
}

void FOculusEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(OculusPerfTabName);
}

void FOculusEditorModule::PluginOpenPlatWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(OculusPlatToolTabName);
}

void FOculusEditorModule::ToggleOpenXRRuntime()
{
	//FMetaXRSimulator::ToggleOpenXRRuntime();
}

void FOculusEditorModule::LaunchSESGameRoom()
{
	//FMetaXRSES::LaunchGameRoom();
}

void FOculusEditorModule::LaunchSESLivingRoom()
{
	//FMetaXRSES::LaunchLivingRoom();
}

void FOculusEditorModule::LaunchSESBedroom()
{
	//FMetaXRSES::LaunchBedroom();
}

void FOculusEditorModule::StopSESServer()
{
	//FMetaXRSES::StopServer();
}

void FOculusEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	bool v = false;
	GConfig->GetBool(TEXT("/Script/OculusEditor.OculusEditorSettings"), TEXT("bAddMenuOption"), v, GEditorIni);
	if (v)
	{
		Builder.AddMenuEntry(FOculusToolCommands::Get().OpenPluginWindow);
	}
}

void FOculusEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FOculusToolCommands::Get().OpenPluginWindow);
}

// Add the entries to the OculusXR Tools toolbar menu button
TSharedRef<SWidget> FOculusEditorModule::CreateToolbarEntryMenu(TSharedPtr<class FUICommandList> Commands)
{
	FMenuBuilder MenuBuilder(true, Commands);
	MenuBuilder.BeginSection("OculusXRBuilds", LOCTEXT("OculusXRBuilds", "Builds"));
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().ToggleDeploySo);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("OculusXRTools", LOCTEXT("OculusXRTools", "Tools"));
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().OpenPluginWindow);
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().OpenPlatWindow);
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FOculusEditorModule::CreateXrSimToolbarEntryMenu(TSharedPtr<class FUICommandList> Commands)
{
	FMenuBuilder MenuBuilder(true, Commands);

	MenuBuilder.BeginSection("MetaXRSimulator", LOCTEXT("MetaXRSimulator", "Toggle"));
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().ToggleMetaXRSim);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("SES", LOCTEXT("SES", "SES"));
	MenuBuilder.AddSubMenu(
		LOCTEXT("Synthetic Environment Server", "Synthetic Environment Server"),
		LOCTEXT("Synthetic Environment Server", "Synthetic Environment Server"),
		FNewMenuDelegate::CreateRaw(this, &FOculusEditorModule::CreateSESSubMenus));
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FOculusEditorModule::CreateSESSubMenus(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("Synthetic Environment Server", LOCTEXT("Synthetic Environment Server", "Synthetic Environment Server"));
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().LaunchGameRoom);
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().LaunchLivingRoom);
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().LaunchBedroom);
	MenuBuilder.AddMenuEntry(FOculusToolCommands::Get().StopServer);
	MenuBuilder.EndSection();
}

TSharedRef<IDetailCustomization> FOculusHMDSettingsDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FOculusHMDSettingsDetailsCustomization);
}

FReply FOculusHMDSettingsDetailsCustomization::PluginClickPerfFn(bool text)
{
	FGlobalTabmanager::Get()->TryInvokeTab(FOculusEditorModule::OculusPerfTabName);
	return FReply::Handled();
}

FReply FOculusHMDSettingsDetailsCustomization::PluginClickPlatFn(bool text)
{
	FGlobalTabmanager::Get()->TryInvokeTab(FOculusEditorModule::OculusPlatToolTabName);
	return FReply::Handled();
}

FReply FOculusHMDSettingsDetailsCustomization::DisableEngineSplash(bool text)
{
	UOculusHMDRuntimeSettings* Settings = GetMutableDefault<UOculusHMDRuntimeSettings>();
	Settings->bAutoEnabled = false;
	Settings->SplashDescs.Empty();
	Settings->UpdateDefaultConfigFile();
	return FReply::Handled();
}

void FOculusHMDSettingsDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Labeled "General Oculus" instead of "General" to enable searchability. The button "Launch Oculus Utilities Window" doesn't show up if you search for "Oculus"
	IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory("General Oculus", FText::GetEmpty(), ECategoryPriority::Important);
	CategoryBuilder.AddCustomRow(LOCTEXT("General Oculus", "General"))
		.WholeRowContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("LaunchTool", "Launch Oculus Performance Window"))
							.OnClicked(this, &FOculusHMDSettingsDetailsCustomization::PluginClickPerfFn, true)
						]
					+ SHorizontalBox::Slot().FillWidth(8)
				]
			+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("LaunchPlatTool", "Launch Oculus Platform Window"))
							.OnClicked(this, &FOculusHMDSettingsDetailsCustomization::PluginClickPlatFn, true)
						]
					+ SHorizontalBox::Slot().FillWidth(8)
				]
		];

	IDetailCategoryBuilder& CTXPTCategoryBuilder = DetailLayout.EditCategory("System SplashScreen", FText::GetEmpty(), ECategoryPriority::Important);

	static const FName WarningColorStyle("Colors.AccentYellow");

	CTXPTCategoryBuilder.AddCustomRow(LOCTEXT("CTXPTWarning", "Contextual Passthrough Warning"))
		.Visibility(TAttribute<EVisibility>(this, &FOculusHMDSettingsDetailsCustomization::GetContextualPassthroughWarningVisibility))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(EVerticalAlignment::VAlign_Center)
						[
							SNew(STextBlock)
								.Font(IDetailLayoutBuilder::GetDetailFont())
								.AutoWrapText(true)
								.Justification(ETextJustify::Center)
								.Text(LOCTEXT("CTXPT_EngineSplashWarning", "Engine Splash Screen is enabled, this will result in an inconsistent experience."))
								.ColorAndOpacity(FEditorStyle::Get().GetSlateColor(WarningColorStyle))
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(EHorizontalAlignment::HAlign_Left)
						[
							SNew(SButton)
								.VAlign(EVerticalAlignment::VAlign_Center)
								.Text(LOCTEXT("DisableEngineSplashScreen", "Disable Engine Splash Screen"))
								.OnClicked(this, &FOculusHMDSettingsDetailsCustomization::DisableEngineSplash, true)
						]
				]
		];
}

EVisibility FOculusHMDSettingsDetailsCustomization::GetContextualPassthroughWarningVisibility() const
{
	UOculusHMDRuntimeSettings* OculusSettings = GetMutableDefault<UOculusHMDRuntimeSettings>();
	return OculusSettings->SystemSplashBackground == ESystemSplashBackgroundType::Contextual && (OculusSettings->bAutoEnabled || !OculusSettings->SplashDescs.IsEmpty()) ? EVisibility::Visible : EVisibility::Collapsed;
}
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FOculusEditorModule, OculusEditor);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE