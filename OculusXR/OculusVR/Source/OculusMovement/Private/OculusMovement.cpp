/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusMovement.h"
#include "OculusMovementModule.h"
#include "OculusHMDPrivate.h"
#include "OculusHMD.h"
#include "OculusPluginWrapper.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "OculusMovement"

namespace XRSpaceFlags
{
	const uint64 XR_SPACE_LOCATION_ORIENTATION_VALID_BIT = 0x00000001;
	const uint64 XR_SPACE_LOCATION_POSITION_VALID_BIT = 0x00000002;
} // namespace XRSpaceFlags

bool OculusMovement::IsFullBodyTrackingEnabled()
{
	bool bResult = false;

	ovrpBool IsEnabled = ovrpBool_False;
	ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetFullBodyTrackingEnabled(&IsEnabled);

	if (OVRP_SUCCESS(TrackingEnabledResult))
	{
		bResult = (IsEnabled == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::GetBodyState(FOculusBodyState& outOculusBodyState, float WorldToMeters)
{
	static_assert(ovrpBoneId_FullBody_End == static_cast<int>(EOculusBoneID::COUNT), "The size of the OVRPlugin Bone ID enum should be the same as the EOculusXRBoneID enum.");

	const auto AvailableJoints = IsFullBodyTrackingEnabled() ? ovrpBoneId_FullBody_End : ovrpBoneId_Body_End;
	checkf(outOculusBodyState.Joints.Num() >= AvailableJoints, TEXT("Not enough joints in FOculusXRBodyState::Joints array. You must have at least %d joints"), AvailableJoints);

	ovrpBodyState4 OVRBodyState;
	ovrpResult OVRBodyStateResult = FOculusHMDModule::GetPluginWrapper().GetBodyState4(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, &OVRBodyState);
	ensureMsgf(OVRBodyStateResult != ovrpFailure_NotYetImplemented, TEXT("Body tracking is not implemented on this platform."));

	if (OVRP_SUCCESS(OVRBodyStateResult))
	{
		outOculusBodyState.IsActive = (OVRBodyState.IsActive == ovrpBool_True);
		outOculusBodyState.Confidence = OVRBodyState.Confidence;
		outOculusBodyState.SkeletonChangedCount = OVRBodyState.SkeletonChangedCount;
		outOculusBodyState.Time = static_cast<float>(OVRBodyState.Time);

		for (int i = 0; i < ovrpBoneId_Body_End; ++i)
		{
			ovrpBodyJointLocation OVRJointLocation = OVRBodyState.JointLocations[i];
			ovrpPosef OVRJointPose = OVRJointLocation.Pose;

			FOculusBodyJoint& OculusBodyJoint = outOculusBodyState.Joints[i];
			OculusBodyJoint.LocationFlags = OVRJointLocation.LocationFlags;
			OculusBodyJoint.bIsValid = OVRJointLocation.LocationFlags & (XRSpaceFlags::XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XRSpaceFlags::XR_SPACE_LOCATION_POSITION_VALID_BIT);
			OculusBodyJoint.Orientation = FRotator(OculusHMD::ToFQuat(OVRJointPose.Orientation));
			OculusBodyJoint.Position = OculusHMD::ToFVector(OVRJointPose.Position) * WorldToMeters;
		}
		if (AvailableJoints < outOculusBodyState.Joints.Num())
		{
			for (int i = AvailableJoints; i < outOculusBodyState.Joints.Num(); ++i)
			{
				outOculusBodyState.Joints[i].bIsValid = false;
			}
		}
		return true;
	}

	return false;
}

bool OculusMovement::IsBodyTrackingEnabled()
{
	bool bResult = false;

	ovrpBool IsEnabled = ovrpBool_False;
	ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetBodyTrackingEnabled(&IsEnabled);

	if (OVRP_SUCCESS(TrackingEnabledResult))
	{
		bResult = (IsEnabled == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::IsBodyTrackingSupported()
{
	bool bResult = false;

	ovrpBool IsSupported = ovrpBool_False;
	ovrpResult TrackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetBodyTrackingSupported(&IsSupported);

	if (OVRP_SUCCESS(TrackingSupportedResult))
	{
		bResult = (IsSupported == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::RequestBodyTrackingFidelity(EOculusHMDBodyTrackingFidelity fidelity)
{
	static_assert(static_cast<int>(EOculusHMDBodyTrackingFidelity::Low) == static_cast<int>(ovrpBodyTrackingFidelity2::ovrpBodyTrackingFidelity2_Low), "EOculusXRBodyTrackingFidelity and ovrpBodyTrackingFidelity2 should be sync");
	static_assert(static_cast<int>(EOculusHMDBodyTrackingFidelity::High) == static_cast<int>(ovrpBodyTrackingFidelity2::ovrpBodyTrackingFidelity2_High), "EOculusXRBodyTrackingFidelity and ovrpBodyTrackingFidelity2 should be sync");

	auto* OculusHMD = OculusHMD::FOculusHMD::GetOculusHMD();
	if (OculusHMD)
	{
		OculusHMD->GetSettings()->BodyTrackingFidelity = static_cast<EOculusHMDBodyTrackingFidelity>(fidelity);
	}

	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().RequestBodyTrackingFidelity(static_cast<ovrpBodyTrackingFidelity2>(fidelity)));
}

bool OculusMovement::ResetBodyTrackingCalibration()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().ResetBodyTrackingCalibration());
}

bool OculusMovement::SuggestBodyTrackingCalibrationOverride(float height)
{
	ovrpBodyTrackingCalibrationInfo calibrationInfo{ height };
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().SuggestBodyTrackingCalibrationOverride(calibrationInfo));
}

bool OculusMovement::StartBodyTracking()
{
	static_assert(static_cast<int>(EOculusHMDBodyJointSet::UpperBody) == static_cast<int>(ovrpBodyJointSet::ovrpBodyJointSet_UpperBody), "EOculusXRBodyJointSet and ovrpBodyJointSet should be sync");
	static_assert(static_cast<int>(EOculusHMDBodyJointSet::FullBody) == static_cast<int>(ovrpBodyJointSet::ovrpBodyJointSet_FullBody), "EOculusXRBodyJointSet and ovrpBodyJointSet should be sync");

	bool result = false;

	const auto* OculusXRHMD = OculusHMD::FOculusHMD::GetOculusHMD();
	if (OculusXRHMD)
	{
		const auto JointSet = OculusXRHMD->GetSettings()->BodyTrackingJointSet;
		if (!OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartBodyTracking2(static_cast<ovrpBodyJointSet>(JointSet))))
		{
			return false;
		}

		const auto Fidelity = OculusXRHMD->GetSettings()->BodyTrackingFidelity;
		FOculusHMDModule::GetPluginWrapper().RequestBodyTrackingFidelity(static_cast<ovrpBodyTrackingFidelity2>(Fidelity));
		return true;
	}
	else
	{
		return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartBodyTracking2(ovrpBodyJointSet::ovrpBodyJointSet_UpperBody));
	}
}

bool OculusMovement::StartBodyTrackingByJointSet(EOculusHMDBodyJointSet jointSet)
{
	bool result = false;

	auto* OculusXRHMD = OculusHMD::FOculusHMD::GetOculusHMD();
	if (OculusXRHMD)
	{
		OculusXRHMD->GetSettings()->BodyTrackingJointSet = static_cast<EOculusHMDBodyJointSet>(jointSet);
		result = StartBodyTracking();
	}
	else
	{
		result = OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartBodyTracking2(static_cast<ovrpBodyJointSet>(jointSet)));
	}

	return result;
}

bool OculusMovement::StopBodyTracking()
{
	FOculusHMDModule::GetPluginWrapper().RequestBodyTrackingFidelity(ovrpBodyTrackingFidelity2::ovrpBodyTrackingFidelity2_Low);
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StopBodyTracking());
}

bool OculusMovement::GetFaceState(FOculusFaceState& outOculusFaceState)
{
	const auto blendShapeCount = ovrpFaceExpression2_Max;

	static_assert(blendShapeCount == static_cast<int>(EOculusFaceExpression::COUNT), "The size of the OVRPlugin Face Expression enum should be the same as the EOculusXRFaceExpression enum.");

	checkf(outOculusFaceState.ExpressionWeightConfidences.Num() >= ovrpFaceConfidence_Max, TEXT("Not enough expression weight confidences in FOculusXRFaceState::ExpressionWeightConfidences. Requires %d available elements in the array."), ovrpFaceConfidence_Max);
	checkf(outOculusFaceState.ExpressionWeights.Num() >= blendShapeCount, TEXT("Not enough expression weights in FOculusXRFaceState::ExpressionWeights. Requires %d available elements in the array."), blendShapeCount);

	ovrpFaceState2 OVRFaceState;
	ovrpResult OVRFaceStateResult = FOculusHMDModule::GetPluginWrapper().GetFaceState2(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, &OVRFaceState);

	ensureMsgf(OVRFaceStateResult != ovrpFailure_NotYetImplemented, TEXT("Face tracking is not implemented on this platform."));

	if (OVRP_SUCCESS(OVRFaceStateResult))
	{
		outOculusFaceState.bIsValid = (OVRFaceState.Status.IsValid == ovrpBool_True);
		outOculusFaceState.bIsEyeFollowingBlendshapesValid = (OVRFaceState.Status.IsEyeFollowingBlendshapesValid == ovrpBool_True);
		outOculusFaceState.Time = static_cast<float>(OVRFaceState.Time);

		for (int i = 0; i < blendShapeCount; ++i)
		{
			outOculusFaceState.ExpressionWeights[i] = OVRFaceState.ExpressionWeights[i];
		}

		for (int i = 0; i < ovrpFaceConfidence_Max; ++i)
		{
			outOculusFaceState.ExpressionWeightConfidences[i] = OVRFaceState.ExpressionWeightConfidences[i];
		}

		outOculusFaceState.DataSource = static_cast<EFaceTrackingDataSourceConfig>(OVRFaceState.DataSource);

		return true;
	}

	return false;
}

bool OculusMovement::IsFaceTrackingEnabled()
{
	bool bResult = false;

	ovrpBool IsEnabled = ovrpBool_False;
	ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetFaceTracking2Enabled(&IsEnabled);

	if (OVRP_SUCCESS(TrackingEnabledResult))
	{
		bResult = (IsEnabled == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::IsFaceTrackingSupported()
{
	bool bResult = false;

	ovrpBool IsSupported = ovrpBool_False;
	ovrpResult TrackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetFaceTracking2Supported(&IsSupported);

	if (OVRP_SUCCESS(TrackingSupportedResult))
	{
		bResult = (IsSupported == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::StartFaceTracking()
{
	const auto* OculusXRHMD = OculusHMD::FOculusHMD::GetOculusHMD();
	if (OculusXRHMD)
	{
		ovrpFaceTrackingDataSource2 dataSources[ovrpFaceConstants_FaceTrackingDataSourcesCount];
		int count = 0;
		for (auto Iterator = OculusXRHMD->GetSettings()->FaceTrackingDataSource.CreateConstIterator(); Iterator; ++Iterator)
		{
			dataSources[count++] = static_cast<ovrpFaceTrackingDataSource2>(*Iterator);
		}
		return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartFaceTracking2(dataSources, count));
	}
	return false;
}

bool OculusMovement::StopFaceTracking()
{

	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StopFaceTracking2());
}

bool OculusMovement::GetEyeGazesState(FOculusEyeGazesState& outOculusEyeGazesState, float WorldToMeters)
{
	static_assert(ovrpEye_Count == (int)EOculusEye::COUNT, "The size of the OVRPlugin Eye enum should be the same as the EOculusEye enum.");

	checkf(outOculusEyeGazesState.EyeGazes.Num() >= ovrpEye_Count, TEXT("Not enough eye gaze states in FOculusEyeGazesState::EyeGazes. Requires %d available elements in the array."), ovrpEye_Count);

	ovrpEyeGazesState OVREyeGazesState;
	ovrpResult OVREyeGazesStateResult = FOculusHMDModule::GetPluginWrapper().GetEyeGazesState(ovrpStep_Render, OVRP_CURRENT_FRAMEINDEX, &OVREyeGazesState);
	ensureMsgf(OVREyeGazesStateResult != ovrpFailure_NotYetImplemented, TEXT("Eye tracking is not implemented on this platform."));

	if (OVRP_SUCCESS(OVREyeGazesStateResult))
	{
		outOculusEyeGazesState.Time = static_cast<float>(OVREyeGazesState.Time);
		for (int i = 0; i < ovrpEye_Count; ++i)
		{
			const auto& EyeGazePose = OVREyeGazesState.EyeGazes[i].Pose;
			outOculusEyeGazesState.EyeGazes[i].Orientation = FRotator(OculusHMD::ToFQuat(EyeGazePose.Orientation));
			outOculusEyeGazesState.EyeGazes[i].Position = OculusHMD::ToFVector(EyeGazePose.Position) * WorldToMeters;
			outOculusEyeGazesState.EyeGazes[i].bIsValid = (OVREyeGazesState.EyeGazes[i].IsValid == ovrpBool_True);
			outOculusEyeGazesState.EyeGazes[i].Confidence = OVREyeGazesState.EyeGazes[i].Confidence;
		}

		return true;
	}

	return false;
}

bool OculusMovement::IsEyeTrackingEnabled()
{
	bool bResult = false;

	ovrpBool IsEnabled = ovrpBool_False;
	ovrpResult TrackingEnabledResult = FOculusHMDModule::GetPluginWrapper().GetEyeTrackingEnabled(&IsEnabled);

	if (OVRP_SUCCESS(TrackingEnabledResult))
	{
		bResult = (IsEnabled == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::IsEyeTrackingSupported()
{
	bool bResult = false;

	ovrpBool IsSupported = ovrpBool_False;
	ovrpResult TrackingSupportedResult = FOculusHMDModule::GetPluginWrapper().GetEyeTrackingSupported(&IsSupported);

	if (OVRP_SUCCESS(TrackingSupportedResult))
	{
		bResult = (IsSupported == ovrpBool_True);
	}

	return bResult;
}

bool OculusMovement::StartEyeTracking()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StartEyeTracking());
}

bool OculusMovement::StopEyeTracking()
{
	return OVRP_SUCCESS(FOculusHMDModule::GetPluginWrapper().StopEyeTracking());
}
