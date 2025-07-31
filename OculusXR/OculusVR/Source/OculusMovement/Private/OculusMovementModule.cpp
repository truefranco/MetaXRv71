/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusMovementModule.h"
#include "OculusHMDModule.h"
#include "OculusMovementLog.h"

#define LOCTEXT_NAMESPACE "OculusMovement"

DEFINE_LOG_CATEGORY(LogOculusMovement);

//-------------------------------------------------------------------------------------------------
// FOculusMovementModule
//-------------------------------------------------------------------------------------------------

FOculusMovementModule::FOculusMovementModule()
{
}

void FOculusMovementModule::StartupModule()
{
}

void FOculusMovementModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FOculusMovementModule, OculusMovement)

#undef LOCTEXT_NAMESPACE
