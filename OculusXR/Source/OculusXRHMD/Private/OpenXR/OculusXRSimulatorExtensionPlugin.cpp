// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRSimulatorExtensionPlugin.h"

#include "OculusXRSimulator.h"

DEFINE_LOG_CATEGORY(LogOculusXRSimulatorPlugin);

namespace OculusXR
{
	bool FXRSimulatorExtensionPlugin::GetCustomLoader(PFN_xrGetInstanceProcAddr* OutGetProcAddr)
	{
		return false;
	}

} // namespace OculusXR
