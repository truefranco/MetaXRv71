/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once
#include "IOculusMovementModule.h"
#include "OculusMovement.h"

#define LOCTEXT_NAMESPACE "OculusMovement"

//-------------------------------------------------------------------------------------------------
// FOculusMovementModule
//-------------------------------------------------------------------------------------------------

class FOculusMovementModule : public IOculusMovementModule
{
public:
	FOculusMovementModule();

	static inline FOculusMovementModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FOculusMovementModule>("OculusMovement");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

#undef LOCTEXT_NAMESPACE
