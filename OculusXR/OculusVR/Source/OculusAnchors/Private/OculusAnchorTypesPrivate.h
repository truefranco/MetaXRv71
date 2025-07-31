/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once
#include "OculusAnchorTypes.h"
#include "OVR_Plugin_Types.h"

ovrpSpaceComponentType ConvertToOvrpComponentType(const EOculusSpaceComponentType ComponentType);

EOculusSpaceComponentType ConvertToUe4ComponentType(const ovrpSpaceComponentType ComponentType);
