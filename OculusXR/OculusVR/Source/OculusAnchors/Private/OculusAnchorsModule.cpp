// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusAnchorsModule.h"

#if OCULUS_ANCHORS_SUPPORTED_PLATFORMS
#include "OculusHMDModule.h"
#include "OculusHMD.h"
#include "OculusAnchors.h"
#include "OculusAnchorManager.h"
#include "OculusRoomLayoutManager.h"

DEFINE_LOG_CATEGORY(LogOculusAnchors);

#define LOCTEXT_NAMESPACE "OculusAnchors"


//-------------------------------------------------------------------------------------------------
// FOculusAnchorsModule
//-------------------------------------------------------------------------------------------------
void FOculusAnchorsModule::StartupModule()
{
	OculusHMD::FOculusHMD* HMD = GEngine->XRSystem.IsValid() ? (OculusHMD::FOculusHMD*)(GEngine->XRSystem->GetHMDDevice()) : nullptr;
	if (!HMD || (GEngine->XRSystem->GetSystemName() != OculusHMD::FOculusHMD::OculusSystemName))
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to retrieve OculusHMD, cannot add event polling delegates."));
		return;
	}

	HMD->AddEventPollingDelegate(OculusHMD::FOculusHMDEventPollingDelegate::CreateStatic(&OculusAnchors::FOculusAnchorManager::OnPollEvent));
	HMD->AddEventPollingDelegate(OculusHMD::FOculusHMDEventPollingDelegate::CreateStatic(&OculusAnchors::FOculusRoomLayoutManager::OnPollEvent));

	Anchors.Initialize();
}

void FOculusAnchorsModule::ShutdownModule()
{
	Anchors.Teardown();
}

OculusAnchors::FOculusAnchors* FOculusAnchorsModule::GetOculusAnchors()
{
	FOculusAnchorsModule& Module = FModuleManager::LoadModuleChecked<FOculusAnchorsModule>(TEXT("OculusAnchors"));
	return &Module.Anchors;
}

#endif	// OCULUS_ANCHORS_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE( FOculusAnchorsModule, OculusAnchors )

#undef LOCTEXT_NAMESPACE
