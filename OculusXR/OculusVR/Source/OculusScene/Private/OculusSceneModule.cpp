// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusSceneModule.h"

#if OCULUS_SCENE_SUPPORTED_PLATFORMS
#include "OculusHMDModule.h"

DEFINE_LOG_CATEGORY(LogOculusScene);

#define LOCTEXT_NAMESPACE "OculusScene"


//-------------------------------------------------------------------------------------------------
// FOculusSceneModule
//-------------------------------------------------------------------------------------------------
void FOculusSceneModule::StartupModule()
{
	
}

void FOculusSceneModule::ShutdownModule()
{

}

#endif	// OCULUS_SCENE_SUPPORTED_PLATFORMS

IMPLEMENT_MODULE( FOculusSceneModule, OculusScene )

#undef LOCTEXT_NAMESPACE
