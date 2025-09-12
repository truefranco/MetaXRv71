// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRSimulator.h"

#include <sstream>

#include "JsonObjectConverter.h"
//#include "MaterialHLSLGenerator.h"
#include "Algo/MaxElement.h"

#if PLATFORM_WINDOWS
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "libzip/zip.h"

#include "HAL/FileManager.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "OculusXRTelemetryEvents.h"
#include "Misc/MessageDialog.h"
#include "OpenXR/OculusXROpenXRUtilities.h"
#include "Internationalization/Regex.h"

#include "Windows/WindowsPlatformMisc.h"
#include "Interfaces/IPluginManager.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#if WITH_EDITOR
#include "UnrealEdMisc.h"
#endif // WITH_EDITOR

const FString OpenXrRuntimeEnvKey = "XR_RUNTIME_JSON";
const FString PreviousOpenXrRuntimeEnvKey = "XR_RUNTIME_JSON_PREV";

namespace
{
	class FZipArchiveReader
	{
	public:
		FZipArchiveReader(IFileHandle* InFileHandle);
		~FZipArchiveReader();

		bool IsValid() const;
		TArray<FString> GetFileNames() const;
		bool TryReadFile(FStringView FileName, TArray<uint8>& OutData) const;

	private:
		TMap<FString, zip_int64_t> EmbeddedFileToIndex;
		IFileHandle* FileHandle = nullptr;
		zip_source_t* ZipFileSource = nullptr;
		zip_t* ZipFile = nullptr;
		uint64 FilePos = 0;
		uint64 FileSize = 0;

		void Destruct();
		zip_int64_t ZipSourceFunctionReader(void* OutData, zip_uint64_t DataLen, zip_source_cmd_t Command);

		static zip_int64_t ZipSourceFunctionReaderStatic(void* InUserData, void* OutData, zip_uint64_t DataLen,
			zip_source_cmd_t Command);
	};

	FZipArchiveReader::FZipArchiveReader(IFileHandle* InFileHandle)
		: FileHandle(InFileHandle)
	{
		if (!FileHandle)
		{
			Destruct();
			return;
		}

		if (FileHandle->Tell() != 0)
		{
			FileHandle->Seek(0);
		}
		FilePos = 0;
		FileSize = FileHandle->Size();
		zip_error_t ZipError;
		zip_error_init(&ZipError);
		ZipFileSource = zip_source_function_create(ZipSourceFunctionReaderStatic, this, &ZipError);
		if (!ZipFileSource)
		{
			zip_error_fini(&ZipError);
			Destruct();
			return;
		}

		zip_error_init(&ZipError);
		ZipFile = zip_open_from_source(ZipFileSource, ZIP_RDONLY, &ZipError);
		if (!ZipFile)
		{
			zip_error_fini(&ZipError);
			Destruct();
			return;
		}

		zip_int64_t NumberOfFiles = zip_get_num_entries(ZipFile, 0);
		if (NumberOfFiles < 0 || MAX_int32 < NumberOfFiles)
		{
			Destruct();
			return;
		}
		EmbeddedFileToIndex.Reserve(NumberOfFiles);

		// produce the manifest file first in case the operation gets canceled while unzipping
		for (zip_int64_t i = 0; i < NumberOfFiles; i++)
		{
			zip_stat_t ZipFileStat;
			if (zip_stat_index(ZipFile, i, 0, &ZipFileStat) != 0)
			{
				Destruct();
				return;
			}
			zip_uint64_t ValidStat = ZipFileStat.valid;
			if (!(ValidStat & ZIP_STAT_NAME))
			{
				Destruct();
				return;
			}
			EmbeddedFileToIndex.Add(FString(ANSI_TO_TCHAR(ZipFileStat.name)), i);
		}
	}

	FZipArchiveReader::~FZipArchiveReader()
	{
		Destruct();
	}

	void FZipArchiveReader::Destruct()
	{
		EmbeddedFileToIndex.Empty();
		if (ZipFile)
		{
			zip_close(ZipFile);
			ZipFile = nullptr;
		}
		if (ZipFileSource)
		{
			zip_source_close(ZipFileSource);
			ZipFileSource = nullptr;
		}
		delete FileHandle;
		FileHandle = nullptr;
	}

	bool FZipArchiveReader::IsValid() const
	{
		return ZipFile != nullptr;
	}

	TArray<FString> FZipArchiveReader::GetFileNames() const
	{
		TArray<FString> Result;
		EmbeddedFileToIndex.GenerateKeyArray(Result);
		return Result;
	}

	bool FZipArchiveReader::TryReadFile(FStringView FileName, TArray<uint8>& OutData) const
	{
		OutData.Reset();

		const zip_int64_t* Index = EmbeddedFileToIndex.FindByHash(GetTypeHash(FileName), FileName);
		if (!Index)
		{
			return false;
		}

		zip_stat_t ZipFileStat;
		if (zip_stat_index(ZipFile, *Index, 0, &ZipFileStat) != 0)
		{
			return false;
		}

		if (!(ZipFileStat.valid & ZIP_STAT_SIZE))
		{
			return false;
		}

		if (ZipFileStat.size == 0)
		{
			return true;
		}
		if (ZipFileStat.size > MAX_int32)
		{
			return false;
		}

		OutData.SetNumUninitialized(ZipFileStat.size, EAllowShrinking::No);

		zip_file* EmbeddedFile = zip_fopen_index(ZipFile, *Index, 0 /* flags */);
		if (!EmbeddedFile)
		{
			OutData.Reset();
			return false;
		}
		bool bReadSuccess = zip_fread(EmbeddedFile, OutData.GetData(), ZipFileStat.size) == ZipFileStat.size;
		zip_fclose(EmbeddedFile);
		if (!bReadSuccess)
		{
			OutData.Reset();
			return false;
		}
		return true;
	}

	zip_int64_t FZipArchiveReader::ZipSourceFunctionReaderStatic(
		void* InUserData, void* OutData, zip_uint64_t DataLen, zip_source_cmd_t Command)
	{
		return reinterpret_cast<FZipArchiveReader*>(InUserData)->ZipSourceFunctionReader(OutData, DataLen, Command);
	}

	zip_int64_t FZipArchiveReader::ZipSourceFunctionReader(
		void* OutData, zip_uint64_t DataLen, zip_source_cmd_t Command)
	{
		switch (Command)
		{
			case ZIP_SOURCE_OPEN:
				return 0;
			case ZIP_SOURCE_READ:
				if (FilePos == FileSize)
				{
					return 0;
				}
				DataLen = FMath::Min(static_cast<zip_uint64_t>(FileSize - FilePos), DataLen);
				if (!FileHandle->Read(reinterpret_cast<uint8*>(OutData), DataLen))
				{
					return 0;
				}
				FilePos += DataLen;
				return DataLen;
			case ZIP_SOURCE_CLOSE:
				return 0;
			case ZIP_SOURCE_STAT:
			{
				zip_stat_t* OutStat = reinterpret_cast<zip_stat_t*>(OutData);
				zip_stat_init(OutStat);
				OutStat->size = FileSize;
				OutStat->comp_size = FileSize;
				OutStat->comp_method = ZIP_CM_STORE;
				OutStat->encryption_method = ZIP_EM_NONE;
				OutStat->valid = ZIP_STAT_SIZE | ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD | ZIP_STAT_ENCRYPTION_METHOD;
				return sizeof(*OutStat);
			}
			case ZIP_SOURCE_ERROR:
			{
				zip_uint32_t* OutLibZipError = reinterpret_cast<zip_uint32_t*>(OutData);
				zip_uint32_t* OutSystemError = OutLibZipError + 1;
				*OutLibZipError = ZIP_ER_INTERNAL;
				*OutSystemError = 0;
				return 2 * sizeof(*OutLibZipError);
			}
			case ZIP_SOURCE_FREE:
				return 0;
			case ZIP_SOURCE_SEEK:
			{
				zip_int64_t NewOffset = zip_source_seek_compute_offset(FilePos, FileSize, OutData, DataLen, nullptr);
				if (NewOffset < 0 || FileSize < static_cast<uint64>(NewOffset))
				{
					return -1;
				}

				if (!FileHandle->Seek(NewOffset))
				{
					return -1;
				}
				FilePos = NewOffset;
				return 0;
			}
			case ZIP_SOURCE_TELL:
				return static_cast<zip_int64_t>(FilePos);
			case ZIP_SOURCE_SUPPORTS:
				return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT,
					ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, ZIP_SOURCE_SEEK, ZIP_SOURCE_TELL, ZIP_SOURCE_SUPPORTS, -1);
			default:
				return 0;
		}
	}

	bool Unzip(const FString& Path, const FString& TargetPath, const TSharedPtr<SNotificationItem>& Notification)
	{
		IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

		IFileHandle* ArchiveFileHandle = FileManager.OpenRead(*Path);
		const FZipArchiveReader ZipArchiveReader(ArchiveFileHandle);
		if (!ZipArchiveReader.IsValid())
		{
			return false;
		}

		const TArray<FString> ArchiveFiles = ZipArchiveReader.GetFileNames();
		uint64 Size = ArchiveFiles.Num();
		uint64 Index = 0;
		for (const FString& FileName : ArchiveFiles)
		{
			Index++;
			if (Notification.IsValid())
			{
				Notification->SetText(FText::FromString(FString::Format(TEXT("Unzipping {0} / {1}"), { Index, Size })));
			}

			if (FileName.EndsWith("/") || FileName.EndsWith("\\"))
				continue;
			if (TArray<uint8> FileBuffer; ZipArchiveReader.TryReadFile(FileName, FileBuffer))
			{
				if (!FFileHelper::SaveArrayToFile(FileBuffer, *(TargetPath / FileName)))
				{
					return false;
				}
			}
		}
		return true;
	}

	int CompareVersionStrings(const FString& Version1, const FString& Version2)
	{
		// Discard everything after '-'
		int32 Inx1 = Version1.Find(TEXT("-"));
		FString V1 = Inx1 == INDEX_NONE ? Version1 : Version1.Left(Inx1);
		int32 Inx2 = Version2.Find(TEXT("-"));
		FString V2 = Inx2 == INDEX_NONE ? Version2 : Version2.Left(Inx2);
		// Split the version strings into parts
		TArray<FString> V1Parts;
		V1.ParseIntoArray(V1Parts, TEXT("."));
		TArray<FString> V2Parts;
		V2.ParseIntoArray(V2Parts, TEXT("."));
		// Determine the maximum length of the version parts
		int32 MaxLength = FMath::Max(V1Parts.Num(), V2Parts.Num());
		// Compare each part of the version strings
		for (int32 i = 0; i < MaxLength; i++)
		{
			// Get the current part of each version, defaulting to 0 if not present
			int32 V1Part = 0;
			if (i < V1Parts.Num())
			{
				V1Part = FCString::Atoi(*V1Parts[i]);
			}
			int32 V2Part = 0;
			if (i < V2Parts.Num())
			{
				V2Part = FCString::Atoi(*V2Parts[i]);
			}
			if (V1Part == V2Part)
			{
				continue;
			}
			return V1Part < V2Part ? -1 : 1;
		}
		// If all parts are equal, the versions are the same
		return 0;
	}

} // namespace

FMetaXRSimulator::FMetaXRSimulator()
	: InstallationPath(FPaths::Combine(FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA")), TEXT("MetaXR"), TEXT("MetaXRSimulator")))
{
}

bool FMetaXRSimulator::IsSimulatorActivated()
{
	FString MetaXRSimPath = GetSimulatorJsonPath();
	FString CurRuntimePath = FWindowsPlatformMisc::GetEnvironmentVariable(*OpenXrRuntimeEnvKey);
	return !MetaXRSimPath.IsEmpty() && MetaXRSimPath == CurRuntimePath;
}

void FMetaXRSimulator::ToggleOpenXRRuntime()
{
	OculusXRTelemetry::TScopedMarker<OculusXRTelemetry::Events::FSimulator> Event;
	FString MetaXRSimPath = GetSimulatorJsonPath();
	if (!IFileManager::Get().FileExists(*MetaXRSimPath))
	{
		if (!MaxAvailableVersion.IsSet())
		{
			UE_LOG(LogMetaXRSim, Error, TEXT("Could not find available version. Try following installation steps from https://developers.meta.com/horizon/documentation/unreal/xrsim-intro manually."));
			return;
		}

		InstallSimulator(MaxAvailableVersion->DownloadUrl, MaxAvailableVersion->Version, [&]() { ToggleOpenXRRuntime(); });
		UE_LOG(LogMetaXRSim, Log, TEXT("Meta XR Simulator Not Installed.\nInstalling Meta XR Simulator %s."), *(MaxAvailableVersion->Version));
		return;
	}

#if WITH_EDITOR
	if (OculusXR::IsOpenXRSystem())
	{
		FString ActivationText = IsSimulatorActivated() ? "deactivate" : "activate";
		FString Message = FString::Format(TEXT("A restart is required in order to {0} XR simulator. The restart must be performed from this dialog, opening and closing the editor manually will not work. Restart now?"), { ActivationText });
		if (FMessageDialog::Open(EAppMsgType::OkCancel, FText::FromString(Message)) == EAppReturnType::Cancel)
		{
			UE_LOG(LogMetaXRSim, Log, TEXT("Meta XR Simulator %s action canceled."), *ActivationText);
			const auto& NotEnd = Event.SetResult(OculusXRTelemetry::EAction::Fail).AddAnnotation("reason", "restart canceled");
			return;
		}
	}
#endif // WITH_EDITOR

	if (IsSimulatorActivated())
	{
		// Deactivate MetaXR Simulator
		FString PrevOpenXrRuntimeEnvKey = FWindowsPlatformMisc::GetEnvironmentVariable(*PreviousOpenXrRuntimeEnvKey);

		FWindowsPlatformMisc::SetEnvironmentVar(*PreviousOpenXrRuntimeEnvKey,
			TEXT(""));
		FWindowsPlatformMisc::SetEnvironmentVar(*OpenXrRuntimeEnvKey, *PrevOpenXrRuntimeEnvKey);

		UE_LOG(LogMetaXRSim, Log, TEXT("Meta XR Simulator is deactivated. (%s : %s)"), *OpenXrRuntimeEnvKey, *PrevOpenXrRuntimeEnvKey);
		const auto& NotEnd = Event.AddAnnotation("action", "deactivated");
	}
	else
	{
		// Activate MetaXR Simulator
		FString CurOpenXrRuntimeEnvKey = FWindowsPlatformMisc::GetEnvironmentVariable(*OpenXrRuntimeEnvKey);

		FWindowsPlatformMisc::SetEnvironmentVar(*PreviousOpenXrRuntimeEnvKey,
			*CurOpenXrRuntimeEnvKey);
		FWindowsPlatformMisc::SetEnvironmentVar(*OpenXrRuntimeEnvKey, *MetaXRSimPath);

		UE_LOG(LogMetaXRSim, Log, TEXT("Meta XR Simulator is activated. (%s : %s)"), *OpenXrRuntimeEnvKey, *MetaXRSimPath);
		const auto& NotEnd = Event.AddAnnotation("action", "activated");
	}

#if WITH_EDITOR
	if (OculusXR::IsOpenXRSystem())
	{
		FUnrealEdMisc::Get().RestartEditor(false);
	}
#endif // WITH_EDITOR
}

FString FMetaXRSimulator::GetSimulatorJsonPath()
{
	return FPaths::Combine(GetPackagePath(), TEXT("meta_openxr_simulator.json"));
}

bool FMetaXRSimulator::IsSimulatorInstalled()
{
	return FPaths::FileExists(GetSimulatorJsonPath());
}

TArray<FString> FMetaXRSimulator::GetInstalledVersions() const
{
	if (!FPaths::DirectoryExists(InstallationPath))
	{
		return {};
	}

	TArray<FString> Versions;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.IterateDirectory(*InstallationPath, [&Versions](const TCHAR* InFilenameOrDirectory, const bool InIsDirectory) {
			if (InIsDirectory)
			{
				FStringView BaseName = FPathViews::GetCleanFilename(InFilenameOrDirectory);
				Versions.AddUnique(FString(BaseName));
			}
			return true;
		}))
	{
		return {};
	}

	return Versions;
}

FString FMetaXRSimulator::GetPackagePath() const
{
	auto Settings = GetMutableDefault<UOculusXRHMDRuntimeSettings>();
	auto PreferredVersion = Settings->OculusXRSimulatorPreferredVersion;
	if (PreferredVersion.IsEmpty())
	{
		auto InstalledVersions = GetInstalledVersions();
		auto MaxVersion = Algo::MaxElement(
			InstalledVersions,
			[](const FString& InVersion, const FString& InBaseVersion) {
				return CompareVersionStrings(InVersion, InBaseVersion);
			});
		PreferredVersion = MaxVersion ? *MaxVersion : GetPluginVersion();

		Settings->OculusXRSimulatorPreferredVersion = PreferredVersion;
		Settings->TryUpdateDefaultConfigFile();
	}

	return FPaths::Combine(InstallationPath, PreferredVersion);
}

void FMetaXRSimulator::InstallSimulator(const FString& URL, const FString& Version, const TFunction<void()>& OnSuccess)
{
	FNotificationInfo Progress(FText::FromString("Installing Meta XR Simulator..."));
	Progress.bFireAndForget = false;
	Progress.FadeInDuration = 0.5f;
	Progress.FadeOutDuration = 0.5f;
	Progress.ExpireDuration = 5.0f;
	Progress.bUseThrobber = true;
	Progress.bUseSuccessFailIcons = true;

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Progress);
	if (NotificationItem.IsValid())
	{
		NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
	}

	auto DestinationFolder = FPaths::Combine(InstallationPath, Version);
	auto DownloadPath = FPaths::Combine(FPaths::EngineSavedDir(), TEXT("Downloads"), TEXT("MetaXRSimulator"), Version, TEXT("MetaXRSimulator.zip"));

	auto OnSuccessCallback = [OnSuccess, Version]() {
		auto Settings = GetMutableDefault<UOculusXRHMDRuntimeSettings>();
		Settings->OculusXRSimulatorPreferredVersion = Version;
		Settings->TryUpdateDefaultConfigFile();

		if (OnSuccess)
		{
			OnSuccess();
		}
	};

	if (FPaths::FileExists(DownloadPath))
	{
		UnzipSimulator(DownloadPath, DestinationFolder, NotificationItem, OnSuccessCallback);
		return;
	}

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->OnProcessRequestComplete().BindLambda([DownloadPath, NotificationItem, DestinationFolder, OnSuccessCallback, Version, this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
		Request->OnRequestProgress64().Unbind();
		if (Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			// Save the downloaded zip file
			FFileHelper::SaveArrayToFile(Response->GetContent(), *DownloadPath);
			if (NotificationItem.IsValid())
			{
				NotificationItem->SetText(FText::FromString("Unzipping ... "));
			}

			UnzipSimulator(DownloadPath, DestinationFolder, NotificationItem, OnSuccessCallback);

			auto Settings = GetMutableDefault<UOculusXRHMDRuntimeSettings>();
			Settings->OculusXRSimulatorPreferredVersion = Version;
			Settings->TryUpdateDefaultConfigFile();
			return;
		}

		UE_LOG(LogMetaXRSim, Error, TEXT("Failed to install Meta XR Simulator."));
		if (NotificationItem.IsValid())
		{
			NotificationItem->SetText(FText::FromString("Installation failed!"));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
			NotificationItem->ExpireAndFadeout();
		}
	});

	Request->OnRequestProgress64().BindLambda([NotificationItem](const FHttpRequestPtr& Request, uint64 /* BytesSent */, uint64 BytesReceived) {
		uint64 ContentLength = Request->GetResponse()->GetContentLength();
		if (NotificationItem.IsValid())
		{
			NotificationItem->SetText(FText::FromString(FString::Format(TEXT("Downloading {0} / {1}"), { BytesReceived, ContentLength })));
		}
	});

	Request->SetURL(URL);
	Request->SetVerb(TEXT("GET"));
	Request->ProcessRequest();
}

FString FMetaXRSimulator::GetPluginVersion()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("OculusXR"));
	if (Plugin.IsValid())
	{
		FString VersionName = Plugin->GetDescriptor().VersionName;
		TArray<FString> ParsedParts;
		VersionName.ParseIntoArray(ParsedParts, TEXT("."), true);
		return FString::FromInt(FCString::Atoi(*ParsedParts[1]) - 32);
	}
	return "0";
}

void FMetaXRSimulator::UnzipSimulator(const FString& Path, const FString& TargetPath, const TSharedPtr<SNotificationItem>& Notification,
	const TFunction<void()>& OnSuccess)
{

	if (!Unzip(Path, TargetPath, Notification))
	{
		UE_LOG(LogMetaXRSim, Error, TEXT("Failed to unzip the file."));
		if (Notification.IsValid())
		{
			Notification->SetText(FText::FromString("Installation failed!"));
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
			Notification->ExpireAndFadeout();
		}
		return;
	}
	if (Notification.IsValid())
	{
		Notification->SetText(FText::FromString("Installation succeeded!"));
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->ExpireAndFadeout();
	}

	if (OnSuccess)
	{
		OnSuccess();
	}
}

void FMetaXRSimulator::FetchAvailableVersions()
{
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->OnProcessRequestComplete().BindLambda([&](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
		Request->OnRequestProgress64().Unbind();
		if (Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			AvailableVersions.Empty();
			MaxAvailableVersion.Reset();
			auto ResponseStr = Response->GetContentAsString();
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseStr);
			if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
			{
				return;
			}
			const TArray<TSharedPtr<FJsonValue>> Binaries = JsonObject->GetArrayField(TEXT("binaries"));
			for (auto& Binary : Binaries)
			{
				auto& BinaryObject = Binary->AsObject();
				if (!BinaryObject.IsValid())
				{
					continue;
				}
				FMetaXRSimulatorVersion Version;
				if (!BinaryObject->TryGetStringField(TEXT("version"), Version.Version))
				{
					continue;
				}
				if (!BinaryObject->TryGetStringField(TEXT("download_url"), Version.DownloadUrl))
				{
					continue;
				}
				if (!BinaryObject->TryGetNumberField(TEXT("url_validity"), Version.UrlValidity))
				{
					continue;
				}

				AvailableVersions.Add(Version);
			}

			auto Maximum = Algo::MaxElement(
				AvailableVersions,
				[](const FMetaXRSimulatorVersion& InVersion, const FMetaXRSimulatorVersion& InBaseVersion) {
					return CompareVersionStrings(InVersion.Version, InBaseVersion.Version) < 0;
				});

			if (Maximum != nullptr)
			{
				MaxAvailableVersion.Emplace(*Maximum);
			}

			SpawnNotificationToUpdateIfAvailable();
		}
	});


	Request->SetURL("https://www.facebook.com/horizon_devcenter_download?app_id=28549923061320041&sdk_version=" + GetPluginVersion());
	Request->SetVerb(TEXT("GET"));
	Request->ProcessRequest();
}

void FMetaXRSimulator::InstallLatestVersion()
{
	if (!MaxAvailableVersion.IsSet())
	{
		return;
	}

	if (IsLatestVersionInstalled())
	{
		return;
	}

	InstallSimulator(MaxAvailableVersion->DownloadUrl, MaxAvailableVersion->Version, nullptr);
}

bool FMetaXRSimulator::IsLatestVersionInstalled()
{
	if (!MaxAvailableVersion.IsSet())
	{
		return true;
	}

	auto InstalledVersions = GetInstalledVersions();
	if (InstalledVersions.Find(MaxAvailableVersion->Version) != INDEX_NONE)
	{
		// Already installed
		return true;
	}

	return false;
}

void FMetaXRSimulator::SpawnNotificationToUpdateIfAvailable()
{
	if (!FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		return;
	}

	if (!GetMutableDefault<UOculusXRHMDRuntimeSettings>()->bNotifyWhenNewVersionIsAvailable)
	{
		return;
	}

	if (!MaxAvailableVersion.IsSet())
	{
		return;
	}

	auto InstalledVersions = GetInstalledVersions();
	const auto& MaxInstalledVersion = Algo::MaxElement(
		InstalledVersions,
		[](const FString& InVersion, const FString& InBaseVersion) {
			return CompareVersionStrings(InVersion, InBaseVersion) <= 0;
		});

	if (MaxInstalledVersion == nullptr || CompareVersionStrings(*MaxInstalledVersion, MaxAvailableVersion->Version) < 0)
	{
		FNotificationInfo UpdateSim(FText::FromString("Meta XR Simulator Update Available"));
		UpdateSim.bFireAndForget = false;
		UpdateSim.FadeInDuration = 0.5f;
		UpdateSim.FadeOutDuration = 0.5f;
		UpdateSim.ExpireDuration = 5.0f;
		UpdateSim.bUseThrobber = false;
		UpdateSim.bUseSuccessFailIcons = true;
		UpdateSim.SubText = FText::FromString("New version includes improvements and bug fixes.");
		TPromise<TSharedPtr<SNotificationItem>> BtnNotificationPromise;
		const auto ButtonClicked = [&, NotificationFuture = BtnNotificationPromise.GetFuture().Share()](bool bUpdate) {
			NotificationFuture.Get()->Fadeout();
			if (!bUpdate)
			{
				return;
			}
			InstallSimulator(MaxAvailableVersion->DownloadUrl, MaxAvailableVersion->Version, nullptr);
		};

		UpdateSim.ButtonDetails.Add(FNotificationButtonInfo(
			FText::FromString("Skip"),
			FText::FromString("Skip update"),
			FSimpleDelegate::CreateLambda(ButtonClicked, false),
			SNotificationItem::CS_Pending));

		UpdateSim.ButtonDetails.Add(FNotificationButtonInfo(
			FText::FromString("Update"),
			FText::FromString("Update Meta XR Simulator to" + MaxAvailableVersion->Version),
			FSimpleDelegate::CreateLambda(ButtonClicked, true),
			SNotificationItem::CS_Pending));
		TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(UpdateSim);
		if (NotificationItem.IsValid())
		{
			NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
			BtnNotificationPromise.SetValue(NotificationItem);
		}
	}
}

#endif // PLATFORM_WINDOWS
