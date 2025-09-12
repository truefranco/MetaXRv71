// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRSyntheticEnvironmentServer.h"

#include "OculusXRSimulator.h"
#if PLATFORM_WINDOWS
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "OculusXRTelemetryEvents.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Windows/WindowsPlatformMisc.h"

const FString SynthEnvServer = "Synthetic Environment Server";
const FString LocalSharingServer = "Local Sharing Server";

FProcHandle FMetaXRSES::EnvProcHandle;
FProcHandle FMetaXRSES::LSSProcHandle;

void FMetaXRSES::StopServer()
{
	StopProcess(EnvProcHandle, SynthEnvServer);
	StopProcess(LSSProcHandle, LocalSharingServer);
}

void FMetaXRSES::LaunchEnvironment(int32 EnvironmentIndex)
{
	const FString EnvironmentName = SynthEnvRooms[EnvironmentIndex].SynthName;
	const FString SESPath = SynthEnvRooms[EnvironmentIndex].Executable;

	if (FMetaXRSimulator::Get().GetPackagePath().IsEmpty() || SESPath.IsEmpty() || EnvironmentName.IsEmpty())
	{
		return;
	}
	StopServer();

	OculusXRTelemetry::TScopedMarker<OculusXRTelemetry::Events::FSimulator> Event;
	const bool bLaunched = LaunchProcess(SESPath, EnvironmentName, LocalSharingServer, EnvProcHandle);
	const auto& _ = Event.SetResult(bLaunched ? OculusXRTelemetry::EAction::Success : OculusXRTelemetry::EAction::Fail).AddAnnotation("launch", StringCast<ANSICHAR>(*EnvironmentName).Get());

	LaunchLocalSharingServer();
}

void FMetaXRSES::LaunchLocalSharingServer()
{
	OculusXRTelemetry::TScopedMarker<OculusXRTelemetry::Events::FSimulator> Event;
	FString LSSPath = GetLocalSharingServerPath();
	const bool bLaunched = LaunchProcess(LSSPath, "", LocalSharingServer, LSSProcHandle);
	const auto& _ = Event.SetResult(bLaunched ? OculusXRTelemetry::EAction::Success : OculusXRTelemetry::EAction::Fail).AddAnnotation("launch", "localsharingserver");
}

bool FMetaXRSES::LaunchProcess(FString BinaryPath, FString Arguments, FString LogContext, FProcHandle& OutProcHandle)
{
	if (!IFileManager::Get().FileExists(*BinaryPath))
	{
		UE_LOG(LogMetaXRSES, Error, TEXT("Failed to find %s."), *BinaryPath);
		return false;
	}
	UE_LOG(LogMetaXRSES, Log, TEXT("Launching %s."), *BinaryPath);

	uint32 OutProcessId = 0;
	OutProcHandle = FPlatformProcess::CreateProc(*BinaryPath, *Arguments, false, false, false, &OutProcessId, 0, NULL, NULL);
	if (!OutProcHandle.IsValid())
	{
		UE_LOG(LogMetaXRSES, Error, TEXT("Failed to launch %s."), *BinaryPath);
		FPlatformProcess::CloseProc(OutProcHandle);
		return false;
	}

	UE_LOG(LogMetaXRSES, Log, TEXT("Launched %s."), *BinaryPath);
	return true;
}

void FMetaXRSES::StopProcess(FProcHandle& ProcHandle, FString LogContext)
{
	if (ProcHandle.IsValid())
	{
		if (FPlatformProcess::IsProcRunning(ProcHandle))
		{
			UE_LOG(LogMetaXRSES, Log, TEXT("Stopping %s."), *LogContext);
			FPlatformProcess::TerminateProc(ProcHandle);
		}
		FPlatformProcess::CloseProc(ProcHandle);
	}
	else
	{
		UE_LOG(LogMetaXRSES, Warning, TEXT("Failed to stop process %s because it is not active anymore."), *LogContext);
	}
}

TArray<FMetaXRSES::ServerInfo> FMetaXRSES::SynthEnvRooms = {};
bool FMetaXRSES::SynthEnvParsed = false;

TArray<FMetaXRSES::ServerInfo> GatherServers(const FString& path)
{
	TArray<FMetaXRSES::ServerInfo> servers{};
	TArray<FString> batFiles{};
	const FString ext("bat");
	IFileManager::Get().FindFiles(batFiles, *path, *ext);
	const static FRegexPattern pattern(TEXT("start (\\S+) (\\S+)"));

	for (const FString& stem : batFiles)
	{
		const FString file = path + "/" + stem;
		FString result;
		if (!FFileHelper::LoadFileToString(result, *file))
		{
			continue;
		}

		FRegexMatcher matcher(pattern, result);

		if (!matcher.FindNext())
		{
			continue;
		}
		UE_LOG(LogMetaXRSES, Warning, TEXT("In %s, found %s && %s"), *file, *matcher.GetCaptureGroup(1), *matcher.GetCaptureGroup(2));
		FString left;
		stem.Split(TEXT("."), &left, NULL);
		auto absPath = path + "/" + matcher.GetCaptureGroup(1);
		servers.Push({ left, matcher.GetCaptureGroup(2), IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(absPath.GetCharArray().GetData()) });
	}
	return servers;
}

TArray<FMetaXRSES::ServerInfo>& FMetaXRSES::GetSynthEnvRooms()
{
	if (SynthEnvParsed)
	{
		return SynthEnvRooms;
	}

	SynthEnvRooms = {};

	FString dirPath = FMetaXRSimulator::Get().GetPackagePath();
	if (dirPath.IsEmpty())
	{
		return SynthEnvRooms;
	}

	TArray<FString> dirNames;
	IFileManager::Get().IterateDirectory(dirPath.GetCharArray().GetData(), [&dirNames](const TCHAR* name, bool) -> bool {
		dirNames.Add(name);
		return true;
	});

	for (auto dir : dirNames)
	{
		// find all of the servers in the subdirectories
		SynthEnvRooms.Append(GatherServers(dir));
	}
	SynthEnvParsed = true;
	return SynthEnvRooms;
}

FString FMetaXRSES::GetLocalSharingServerPath()
{
	FString Path = FMetaXRSimulator::Get().GetPackagePath() + "/local_sharing_server~/local_sharing_server.exe";

	if (!IFileManager::Get().FileExists(*Path))
	{
		UE_LOG(LogMetaXRSES, Warning, TEXT("Failed to find %s, trying the previous version"), *Path);
		Path = FMetaXRSimulator::Get().GetPackagePath() + "/.local_sharing_server/local_sharing_server.exe";

		if (!IFileManager::Get().FileExists(*Path))
		{
			UE_LOG(LogMetaXRSES, Error, TEXT("Failed to find LocalSharingServer, giving up"));
			Path = "";
		}
	}

	return Path;
}
#endif
