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

#include "OculusInteractionEditor.h"

#include "IsdkRuntimeSettings.h"
#include "ISettingsModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "IsdkHandPoseVisualizer.h"
#include "Interaction/Grabbable/IsdkHandGrabPose.h"
#include "HandPoseDetection/IsdkHandPoseDetectionProfile.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

class FOculusInteractionEditor : public IOculusInteractionEditor
{
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
  void HandleIsdkDownloadClicked() const;
  void HandleMetaXrDownloadClicked() const;
  void HandleEditorInitialized(double) const;
};

#define LOCTEXT_NAMESPACE "FOculusInteractionEditorModule"

DEFINE_LOG_CATEGORY(LogOculusInteractionEditor);

IMPLEMENT_MODULE(FOculusInteractionEditor, OculusInteractionEditor)

void FOculusInteractionEditor::StartupModule()
{
  // This code will execute after your module is loaded into memory (but after global variables are
  // initialized, of course.)

  // register settings
  ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

  if (SettingsModule != nullptr)
  {
    SettingsModule->RegisterSettings(
        "Project",
        "Plugins",
        "Isdk",
        LOCTEXT("IsdkSettingsName", "Meta XR - InteractionSDK"),
        LOCTEXT("IsdkSettingsDescription", "Project settings for Meta InteractionSdk plugin"),
        GetMutableDefault<UIsdkRuntimeSettings>());
  }
  // Register our Custom Asset Actions
  IsdkHandPoseDataAssetTypeActions = MakeShared<FIsdkHandPoseDataAssetTypeActions>();
  FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(
      IsdkHandPoseDataAssetTypeActions.ToSharedRef());

  IsdkHandPoseDetectionProfileAssetTypeActions =
      MakeShared<FIsdkHandPoseDetectionProfileAssetTypeActions>();
  FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(
      IsdkHandPoseDetectionProfileAssetTypeActions.ToSharedRef());

  // Wait for editor to be initialized before performing version check logic to avoid any potential
  // module load timing issues
  FEditorDelegates::OnEditorInitialized.AddRaw(
      this, &FOculusInteractionEditor::HandleEditorInitialized);

  if (GUnrealEd)
  {
    TSharedPtr<FIsdkHandPoseVisualizer> Visualizer = MakeShareable(new FIsdkHandPoseVisualizer());
    GUnrealEd->RegisterComponentVisualizer(
        UIsdkHandGrabPose::StaticClass()->GetFName(), Visualizer);
    Visualizer->OnRegister();
  }
}

void FOculusInteractionEditor::ShutdownModule()
{
  // unregister settings
  ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

  if (SettingsModule != nullptr)
  {
    SettingsModule->UnregisterSettings("Project", "Plugins", "Isdk");
  }

  if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
    return;
  FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(
      IsdkHandPoseDataAssetTypeActions.ToSharedRef());

  if (GUnrealEd)
  {
    GUnrealEd->UnregisterComponentVisualizer(UIsdkHandGrabPose::StaticClass()->GetFName());
  }
}

void FOculusInteractionEditor::HandleEditorInitialized(double) const
{
  // Validate whether oculus xr plugin is enabled
  IPluginManager& PluginManager = IPluginManager::Get();

  // OculusInteraction plugin should be enabled if this module is starting up
  const auto OculusInteractionPlugin = PluginManager.FindEnabledPlugin("OculusInteraction");
  if (!ensure(OculusInteractionPlugin))
  {
    return;
  }

  // If OculusXR is not enabled, this is totally fine, no checks necessary
  const auto OculusXrPlugin = PluginManager.FindEnabledPlugin("OculusXR");
  if (!OculusXrPlugin)
  {
    return;
  }

  // Get the ISDK version
  const auto IsdkVersionName = OculusInteractionPlugin->GetDescriptor().VersionName;

  // Parse it into major, minor, and patch versions
  TArray<FString> VersionParts;
  IsdkVersionName.ParseIntoArrayWS(VersionParts, TEXT("."));
  if (VersionParts.Num() != 3)
  {
    UE_LOG(
        LogOculusInteractionEditor,
        Warning,
        TEXT("Found plugin \"OculusXR\" but could not determine version from VersionName"));
    return;
  }

  const int32 IsdkMajor = FCString::Atoi(*VersionParts[0]);
  const int32 IsdkMinor = FCString::Atoi(*VersionParts[1]);
  const int32 IsdkPatch = FCString::Atoi(*VersionParts[2]);

  // Get the OculusXR version
  const auto OculusXrVersionName = OculusXrPlugin->GetDescriptor().VersionName;

  // Parse it into major, minor, and patch versions
  OculusXrVersionName.ParseIntoArrayWS(VersionParts, TEXT("."));
  if (VersionParts.Num() != 3)
  {
    UE_LOG(
        LogOculusInteractionEditor,
        Warning,
        TEXT("Found plugin \"OculusXR\" but could not determine version from VersionName"));
    return;
  }

  const int32 OculusXrMajor = FCString::Atoi(*VersionParts[0]);
  const int32 OculusXrMinor = FCString::Atoi(*VersionParts[1]);
  const int32 OculusXrPatch = FCString::Atoi(*VersionParts[2]);

  // OculusXR's SDK version is the minor version minus 32.  Eg, v71 is 1.103.0.
  const int32 OculusXrSdkVersion = OculusXrMinor - 32;

  // If both versions match, we're good to go
  if (IsdkMinor == OculusXrSdkVersion)
  {
    UE_LOG(
        LogOculusInteractionEditor,
        Log,
        TEXT("OculusInteraction Version \"%s\" matches OculusXR Version \"%s\""),
        *IsdkVersionName,
        *OculusXrVersionName);
    return;
  }

  const auto DesiredOculusXrVersion =
      FString::Printf(TEXT("%d.%d.%d"), OculusXrMajor, IsdkMinor + 32, OculusXrPatch);
  const auto WarningString = FString::Printf(
      TEXT(
          "Meta Interaction SDK version \"%s\" (v%d) requires MetaXR Version \"%s\" (v%d) but found \"%s\" (v%d).\nMismatched versions may result in unexpected behavior."),
      *IsdkVersionName,
      IsdkMinor,
      *DesiredOculusXrVersion,
      IsdkMinor,
      *OculusXrVersionName,
      OculusXrSdkVersion);
  const auto WarningText = FText::FromString(WarningString);

  FNotificationInfo* Info = new FNotificationInfo(WarningText);
  Info->ExpireDuration = 30.f;
  Info->WidthOverride = 900.f;

  // Add the buttons with text, tooltip and callback
  Info->ButtonDetails.Add(FNotificationButtonInfo(
      LOCTEXT("IsdkDownload", "Download Latest Meta Interaction Plugin"),
      FText(),
      FSimpleDelegate::CreateRaw(this, &FOculusInteractionEditor::HandleIsdkDownloadClicked),
      SNotificationItem::CS_None));
  Info->ButtonDetails.Add(FNotificationButtonInfo(
      LOCTEXT("MetaXrDownload", "Download Latest Meta XR Plugin"),
      FText(),
      FSimpleDelegate::CreateRaw(this, &FOculusInteractionEditor::HandleMetaXrDownloadClicked),
      SNotificationItem::CS_None));

  // Some additional styling information
  Info->bUseLargeFont = false;
  Info->bUseThrobber = false;
  Info->bUseSuccessFailIcons = false;

  // Display a different info icon than default warning
  Info->Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Warning"));

  FSlateNotificationManager::Get().QueueNotification(Info);
}

void FOculusInteractionEditor::HandleIsdkDownloadClicked() const
{
  const FString DocsURL =
      TEXT("https://developers.meta.com/horizon/downloads/package/meta-xr-interaction-sdk-unreal/");
  FPlatformProcess::LaunchURL(*DocsURL, nullptr, nullptr);
}

void FOculusInteractionEditor::HandleMetaXrDownloadClicked() const
{
  const FString DocsURL =
      TEXT("https://developers.meta.com/horizon/downloads/package/unreal-engine-5-integration/");
  FPlatformProcess::LaunchURL(*DocsURL, nullptr, nullptr);
}

#undef LOCTEXT_NAMESPACE
