// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "IOculusSceneModule.h"

#define LOCTEXT_NAMESPACE "OculusScene"


//-------------------------------------------------------------------------------------------------
// FOculusSceneModule
//-------------------------------------------------------------------------------------------------

#if OCULUS_SCENE_SUPPORTED_PLATFORMS

DECLARE_LOG_CATEGORY_EXTERN(LogOculusScene, Log, All);

class FOculusSceneModule : public IOculusSceneModule
{
public:
	virtual ~FOculusSceneModule() = default;

	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};

#else	// OCULUS_SCENE_SUPPORTED_PLATFORMS

class FOculusSceneModule : public FDefaultModuleImpl
{

};

#endif	// OCULUS_SCENE_SUPPORTED_PLATFORMS


#undef LOCTEXT_NAMESPACE
