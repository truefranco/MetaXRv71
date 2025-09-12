// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_WINDOWS

#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY_STATIC(LogMetaXRSim, Log, All);

/**  */
class FMetaXRSimulator
{
public:
	static FMetaXRSimulator& Get()
	{
		static FMetaXRSimulator instance;
		return instance;
	}
	FMetaXRSimulator(const FMetaXRSimulator&) = delete;
	FMetaXRSimulator& operator=(const FMetaXRSimulator&) = delete;

	bool IsSimulatorActivated();
	void ToggleOpenXRRuntime();
	FString GetPackagePath() const;
	bool IsSimulatorInstalled();
	TArray<FString> GetInstalledVersions() const;
	void FetchAvailableVersions();
	void InstallLatestVersion();
	bool IsLatestVersionInstalled();

private:
	struct FMetaXRSimulatorVersion
	{
		FString DownloadUrl;
		FString Version;
		double UrlValidity;
	};

	void SpawnNotificationToUpdateIfAvailable();
	FMetaXRSimulator();
	~FMetaXRSimulator() = default;
	FString GetSimulatorJsonPath();
	void InstallSimulator(const FString& URL, const FString& Version, const TFunction<void()>& OnSuccess);
	static FString GetPluginVersion();
	void UnzipSimulator(const FString& Path, const FString& TargetPath, const TSharedPtr<SNotificationItem>& Notification, const TFunction<void()>& OnSuccess);

	const FString InstallationPath;

	TArray<FMetaXRSimulatorVersion> AvailableVersions;
	TOptional<FMetaXRSimulatorVersion> MaxAvailableVersion;
};
#endif
