/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusMovementTypes.h"
#include "OculusHMDPrivate.h"
#include "OculusHMD.h"

FOculusBodyJoint::FOculusBodyJoint()
	: LocationFlags(0)
	, bIsValid(false)
	, Orientation(FRotator::ZeroRotator)
	, Position(FVector::ZeroVector)
{
}

FOculusBodyState::FOculusBodyState()
	: IsActive(false)
	, Confidence(0)
	, SkeletonChangedCount(0)
	, Time(0.f)
{
	Joints.SetNum(static_cast<int32>(EOculusBoneID::COUNT));
}

FOculusFaceState::FOculusFaceState()
	: bIsValid(false)
	, bIsEyeFollowingBlendshapesValid(false)
	, Time(0.f)
	, DataSource(EFaceTrackingDataSourceConfig::MAX)
{
	ExpressionWeights.SetNum(static_cast<int32>(EOculusFaceExpression::COUNT));
	ExpressionWeightConfidences.SetNum(static_cast<int32>(EOculusFaceConfidence::COUNT));
}

FOculusEyeGazeState::FOculusEyeGazeState()
	: Orientation(FRotator::ZeroRotator)
	, Position(FVector::ZeroVector)
	, Confidence(0.f)
	, bIsValid(false)
{
}

FOculusEyeGazesState::FOculusEyeGazesState()
	: Time(0.f)
{
	EyeGazes.SetNum(static_cast<int32>(EOculusEye::COUNT));
}
