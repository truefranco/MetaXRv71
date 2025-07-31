/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchorTypesPrivate.h"

ovrpSpaceComponentType ConvertToOvrpComponentType(const EOculusSpaceComponentType ComponentType)
{
	ovrpSpaceComponentType ovrpType = ovrpSpaceComponentType_Max;
	switch (ComponentType)
	{
		case EOculusSpaceComponentType::Locatable:
			ovrpType = ovrpSpaceComponentType_Locatable;
			break;
		case EOculusSpaceComponentType::Sharable:
			ovrpType = ovrpSpaceComponentType_Sharable;
			break;
		case EOculusSpaceComponentType::Storable:
			ovrpType = ovrpSpaceComponentType_Storable;
			break;
		case EOculusSpaceComponentType::ScenePlane:
			ovrpType = ovrpSpaceComponentType_Bounded2D;
			break;
		case EOculusSpaceComponentType::SceneVolume:
			ovrpType = ovrpSpaceComponentType_Bounded3D;
			break;
		case EOculusSpaceComponentType::SemanticClassification:
			ovrpType = ovrpSpaceComponentType_SemanticLabels;
			break;
		case EOculusSpaceComponentType::RoomLayout:
			ovrpType = ovrpSpaceComponentType_RoomLayout;
			break;
		case EOculusSpaceComponentType::SpaceContainer:
			ovrpType = ovrpSpaceComponentType_SpaceContainer;
			break;
		case EOculusSpaceComponentType::TriangleMesh:
			ovrpType = ovrpSpaceComponentType_TriangleMesh;
			break;
		default:;
	}

	return ovrpType;
}

EOculusSpaceComponentType ConvertToUe4ComponentType(const ovrpSpaceComponentType ComponentType)
{
	EOculusSpaceComponentType ue4ComponentType = EOculusSpaceComponentType::Undefined;
	switch (ComponentType)
	{
		case ovrpSpaceComponentType_Locatable:
			ue4ComponentType = EOculusSpaceComponentType::Locatable;
			break;
		case ovrpSpaceComponentType_Sharable:
			ue4ComponentType = EOculusSpaceComponentType::Sharable;
			break;
		case ovrpSpaceComponentType_Storable:
			ue4ComponentType = EOculusSpaceComponentType::Storable;
			break;
		case ovrpSpaceComponentType_Bounded2D:
			ue4ComponentType = EOculusSpaceComponentType::ScenePlane;
			break;
		case ovrpSpaceComponentType_Bounded3D:
			ue4ComponentType = EOculusSpaceComponentType::SceneVolume;
			break;
		case ovrpSpaceComponentType_SemanticLabels:
			ue4ComponentType = EOculusSpaceComponentType::SemanticClassification;
			break;
		case ovrpSpaceComponentType_RoomLayout:
			ue4ComponentType = EOculusSpaceComponentType::RoomLayout;
			break;
		case ovrpSpaceComponentType_SpaceContainer:
			ue4ComponentType = EOculusSpaceComponentType::SpaceContainer;
			break;
		case ovrpSpaceComponentType_TriangleMesh:
			ue4ComponentType = EOculusSpaceComponentType::TriangleMesh;
			break;
		default:;
	}

	return ue4ComponentType;
}
