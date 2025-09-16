// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneSoftwareOcclusion.cpp
=============================================================================*/

#ifdef WITH_OCULUS_BRANCH

#include "SceneSoftwareOcclusionProvider.h"
#include "SceneSoftwareOcclusion.h"

#include "ScenePrivate.h"

TSharedPtr<FSceneSoftwareOcclusionProvider> FSceneSoftwareOcclusionProvider::Instance = NULL;

TSharedPtr<FSceneSoftwareOcclusionProvider> FSceneSoftwareOcclusionProvider::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeShared<FSceneSoftwareOcclusionProvider>();
	}

	return Instance;
}

FSceneSoftwareOcclusionProvider::FSceneSoftwareOcclusionProvider()
{
}

void FSceneSoftwareOcclusionProvider::SetupCustomOcclusion(FViewInfo& View)
{
	if (!View.ViewState)
	{
		return;
	}
	View.ViewState->CustomOcclusion = MakeUnique<FSceneSoftwareOcclusion>();
}
#endif // WITH_OCULUS_BRANCH
