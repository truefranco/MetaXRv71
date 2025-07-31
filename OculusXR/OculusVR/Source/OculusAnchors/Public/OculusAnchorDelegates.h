/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreTypes.h"
#include "OculusAnchorTypes.h"
#include "Delegates/Delegate.h"

class FOculusAnchorEventDelegates
{
public:
	/* ovrpEventType_SpatialAnchorCreateComplete
	 *
	 *        SpatialAnchorCreateComplete
	 * Prefix:
	 * FOculusSpatialAnchorCreateComplete
	 * Suffix:
	 * FOculusSpatialAnchorCreateCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_FourParams(FOculusSpatialAnchorCreateCompleteDelegate, FUInt64 /*requestId*/, int /*result*/, FUInt64 /*space*/, FUUID /*uuid*/);
	static OCULUSANCHORS_API FOculusSpatialAnchorCreateCompleteDelegate OculusSpatialAnchorCreateComplete;

	/* ovrpEventType_SpaceSetComponentStatusComplete
	 *
	 *        SpaceSetComponentStatusComplete
	 * Prefix:
	 * FOculusSpaceSetComponentStatusComplete
	 * Suffix:
	 * FOculusSpaceSetComponentStatusCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_SixParams(FOculusSpaceSetComponentStatusCompleteDelegate, FUInt64 /*requestId*/, int /*result*/, FUInt64 /*space*/, FUUID /*uuid*/, EOculusSpaceComponentType /*componenttype */, bool /*enabled*/);
	static OCULUSANCHORS_API FOculusSpaceSetComponentStatusCompleteDelegate OculusSpaceSetComponentStatusComplete;

	/* ovrpEventType_SpaceQueryResults
	 *
	 *        SpaceQueryResults
	 * Prefix:
	 * FOculusSpaceQueryResults
	 * Suffix:
	 * FOculusSpaceQueryResultsDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOculusSpaceQueryResultsDelegate, FUInt64 /*requestId*/);
	static OCULUSANCHORS_API FOculusSpaceQueryResultsDelegate OculusSpaceQueryResults;

	/* SpaceQueryResult (no ovrp event type)
	 *
	 *        SpaceQueryResult
	 * Prefix:
	 * FOculusSpaceQueryResult
	 * Suffix:
	 * FOculusSpaceQueryResultDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOculusSpaceQueryResultDelegate, FUInt64  /*requestId*/, FUInt64 /* space*/, FUUID /*uuid*/);
	static OCULUSANCHORS_API FOculusSpaceQueryResultDelegate OculusSpaceQueryResult;

	/* ovrpEventType_SpaceQueryComplete
	 *
	 *        SpaceQueryComplete
	 * Prefix:
	 * FOculusSpaceQueryComplete
	 * Suffix:
	 * FOculusSpaceQueryCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOculusSpaceQueryCompleteDelegate, FUInt64  /*requestId*/, int /*result*/);
	static OCULUSANCHORS_API FOculusSpaceQueryCompleteDelegate OculusSpaceQueryComplete;

	/* ovrpEventType_SpaceSaveComplete
	 *
	 *        SpaceSaveComplete
	 * Prefix:
	 * FOculusSpaceSaveComplete
	 * Suffix:
	 * FOculusSpaceSaveCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_FiveParams(FOculusSpaceSaveCompleteDelegate, FUInt64  /*requestId*/, FUInt64 /* space*/, bool /* sucess*/, int /*result*/, FUUID /*uuid*/);
	static OCULUSANCHORS_API FOculusSpaceSaveCompleteDelegate OculusSpaceSaveComplete;

	/* ovrpEventType_SpaceListSaveResult
	 *
	 *        SpaceListSaveComplete
	 * Prefix:
	 * FOculusSpaceListSaveComplete
	 * Suffix:
	 * FOculusSpaceListSaveCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOculusSpaceListSaveCompleteDelegate, FUInt64 /*requestId*/, int /*result*/);
	static OCULUSANCHORS_API FOculusSpaceListSaveCompleteDelegate OculusSpaceListSaveComplete;

	/* ovrpEventType_SpaceEraseComplete
	 *
	 *        SpaceEraseComplete
	 * Prefix:
	 * FOculusSpaceEraseComplete
	 * Suffix:
	 * FOculusSpaceEraseCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_FourParams(FOculusSpaceEraseCompleteDelegate, FUInt64  /*requestId*/, int /* result*/, FUUID /*uuid*/, EOculusSpaceStorageLocation /*location*/);
	static OCULUSANCHORS_API FOculusSpaceEraseCompleteDelegate OculusSpaceEraseComplete;

	/* ovrpEventType_SpaceShareSpaceResult
	 *
	 *        SpaceShareComplete
	 * Prefix:
	 * FOculusSpaceShareSpacesComplete
	 * Suffix:
	 * FOculusSpaceShareSpacesCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOculusSpaceShareCompleteDelegate, FUInt64 /*requestId*/, int /*result*/);
	static OCULUSANCHORS_API FOculusSpaceShareCompleteDelegate OculusSpaceShareComplete;

	/* ovrpEventType_SceneCaptureComplete
	 *
	 *        SceneCaptureComplete
	 * Prefix:
	 * FOculusSceneCaptureComplete
	 * Suffix:
	 * FOculusSceneCaptureCompleteDelegate
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOculusSceneCaptureCompleteDelegate, FUInt64 /*requestId*/, bool /*success*/);
	static OCULUSANCHORS_API FOculusSceneCaptureCompleteDelegate OculusSceneCaptureComplete;

};
