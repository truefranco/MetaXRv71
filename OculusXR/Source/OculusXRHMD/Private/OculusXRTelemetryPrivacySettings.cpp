// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRTelemetryPrivacySettings.h"

#include "OculusXRHMDModule.h"
#include "OculusXRTelemetry.h"

#include <regex>

#define LOCTEXT_NAMESPACE "OculusXRTelemetryPrivacySettings"

constexpr int CONSENT_NOTIFICATION_MAX_LENGTH = 1024;
TMap<FString, FString> GetLinks(const std::string& markdown, std::string& textWithoutLink)
{
	TMap<FString, FString> links;
	const std::regex linkRegex(R"(\[(.*?)\]\((.*?)\))");

	std::smatch matches;
	std::string::const_iterator searchStart(markdown.cbegin());
	while (std::regex_search(searchStart, markdown.cend(), matches, linkRegex))
	{
		links.Add(matches[1].str().c_str(), matches[2].str().c_str());
		searchStart = matches.suffix().first;
	}

	const std::string linkReplacement = "";
	textWithoutLink = std::regex_replace(markdown, linkRegex, linkReplacement);
	return links;
}

UOculusXRTelemetryPrivacySettings::UOculusXRTelemetryPrivacySettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!FOculusXRHMDModule::Get().IsOVRPluginAvailable() || !FOculusXRHMDModule::GetPluginWrapper().IsInitialized())
	{
		return;
	}

	bIsEnabled = FOculusXRHMDModule::GetPluginWrapper().GetUnifiedConsent(OculusXRTelemetry::UNREAL_TOOL_ID) == ovrpBool_True;

	char SettingsText[CONSENT_NOTIFICATION_MAX_LENGTH];
	if (FOculusXRHMDModule::GetPluginWrapper().GetConsentSettingsChangeText(SettingsText) == ovrpFailure)
	{
		return;
	}

	std::string settingsDesc = "";
	Links = GetLinks(SettingsText, settingsDesc);
	Description = FText::FromString(settingsDesc.c_str());
}

void UOculusXRTelemetryPrivacySettings::GetToggleCategoryAndPropertyNames(FName& OutCategory, FName& OutProperty) const
{
	OutCategory = FName("Options");
	OutProperty = FName("bIsEnabled");
}

FText UOculusXRTelemetryPrivacySettings::GetFalseStateLabel() const
{
	return LOCTEXT("FalseStateLabel", "Only share essential data");
}

FText UOculusXRTelemetryPrivacySettings::GetFalseStateTooltip() const
{
	return LOCTEXT("FalseStateTooltip", "Only share essential data");
}

FText UOculusXRTelemetryPrivacySettings::GetFalseStateDescription() const
{
	return Description;
}

FText UOculusXRTelemetryPrivacySettings::GetTrueStateLabel() const
{
	return LOCTEXT("TrueStateLabel", "Share additional data");
}

FText UOculusXRTelemetryPrivacySettings::GetTrueStateTooltip() const
{
	return LOCTEXT("TrueStateTooltip", "Share additional data");
}

FText UOculusXRTelemetryPrivacySettings::GetTrueStateDescription() const
{
	return Description;
}

FString UOculusXRTelemetryPrivacySettings::GetAdditionalInfoUrl() const
{
	if (Links.Num() > 0)
	{
		return Links.begin()->Value;
	}
	return FString();
}

FText UOculusXRTelemetryPrivacySettings::GetAdditionalInfoUrlLabel() const
{
	if (Links.Num() > 0)
	{
		return FText::FromString(Links.begin()->Key);
	}
	return FText();
}

#if WITH_EDITOR
void UOculusXRTelemetryPrivacySettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UOculusXRTelemetryPrivacySettings, bIsEnabled))
	{
		using namespace OculusXRTelemetry;
		if (FOculusXRHMDModule::Get().IsOVRPluginAvailable() && FOculusXRHMDModule::GetPluginWrapper().IsInitialized())
		{
			FOculusXRHMDModule::GetPluginWrapper().SaveUnifiedConsent(UNREAL_TOOL_ID, bIsEnabled ? ovrpBool_True : ovrpBool_False);
			PropagateTelemetryConsent();
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE
