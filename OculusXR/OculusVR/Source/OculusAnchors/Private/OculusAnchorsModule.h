// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "IOculusAnchorsModule.h"
#include "OculusAnchors.h"

#define LOCTEXT_NAMESPACE "OculusAnchors"


//-------------------------------------------------------------------------------------------------
// FOculusAnchorsModule
//-------------------------------------------------------------------------------------------------

#if OCULUS_ANCHORS_SUPPORTED_PLATFORMS

DECLARE_LOG_CATEGORY_EXTERN(LogOculusAnchors, Log, All);

class FOculusAnchorsModule : public IOculusAnchorsModule
{
public:
	virtual ~FOculusAnchorsModule() = default;

	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static OculusAnchors::FOculusAnchors* GetOculusAnchors();

private:
	OculusAnchors::FOculusAnchors Anchors;
};

#else	// OCULUS_ANCHORS_SUPPORTED_PLATFORMS

class FOculusAnchorsModule : public FDefaultModuleImpl
{

};

#endif	// OCULUS_ANCHORS_SUPPORTED_PLATFORMS


#undef LOCTEXT_NAMESPACE
