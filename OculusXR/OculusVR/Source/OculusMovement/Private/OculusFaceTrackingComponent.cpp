/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "OculusFaceTrackingComponent.h"
#include "OculusHMD.h"
#include "OculusPluginWrapper.h"
#include "OculusMovementFunctionLibrary.h"
#include "OculusMovementHelpers.h"
#include "OculusMovementLog.h"

int UOculusFaceTrackingComponent::TrackingInstanceCount = 0;

UOculusFaceTrackingComponent::UOculusFaceTrackingComponent()
	: TargetMeshComponentName(NAME_None)
	, InvalidFaceDataResetTime(2.0f)
	, bUpdateFace(true)
	, TargetMeshComponent(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Some defaults
	ExpressionNames.Add(EOculusFaceExpression::BrowLowererL, "browLowerer_L");
	ExpressionNames.Add(EOculusFaceExpression::BrowLowererR, "browLowerer_R");
	ExpressionNames.Add(EOculusFaceExpression::CheekPuffL, "cheekPuff_L");
	ExpressionNames.Add(EOculusFaceExpression::CheekPuffR, "cheekPuff_R");
	ExpressionNames.Add(EOculusFaceExpression::CheekRaiserL, "cheekRaiser_L");
	ExpressionNames.Add(EOculusFaceExpression::CheekRaiserR, "cheekRaiser_R");
	ExpressionNames.Add(EOculusFaceExpression::CheekSuckL, "cheekSuck_L");
	ExpressionNames.Add(EOculusFaceExpression::CheekSuckR, "cheekSuck_R");
	ExpressionNames.Add(EOculusFaceExpression::ChinRaiserB, "chinRaiserB");
	ExpressionNames.Add(EOculusFaceExpression::ChinRaiserT, "chinRaiserT");
	ExpressionNames.Add(EOculusFaceExpression::DimplerL, "dimpler_L");
	ExpressionNames.Add(EOculusFaceExpression::DimplerR, "dimpler_R");
	ExpressionNames.Add(EOculusFaceExpression::EyesClosedL, "eyesClosed_L");
	ExpressionNames.Add(EOculusFaceExpression::EyesClosedR, "eyesClosed_R");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookDownL, "eyesLookDown_L");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookDownR, "eyesLookDown_R");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookLeftL, "eyesLookLeft_L");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookLeftR, "eyesLookLeft_R");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookRightL, "eyesLookRight_L");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookRightR, "eyesLookRight_R");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookUpL, "eyesLookUp_L");
	ExpressionNames.Add(EOculusFaceExpression::EyesLookUpR, "eyesLookUp_R");
	ExpressionNames.Add(EOculusFaceExpression::InnerBrowRaiserL, "innerBrowRaiser_L");
	ExpressionNames.Add(EOculusFaceExpression::InnerBrowRaiserR, "innerBrowRaiser_R");
	ExpressionNames.Add(EOculusFaceExpression::JawDrop, "jawDrop");
	ExpressionNames.Add(EOculusFaceExpression::JawSidewaysLeft, "jawSidewaysLeft");
	ExpressionNames.Add(EOculusFaceExpression::JawSidewaysRight, "jawSidewaysRight");
	ExpressionNames.Add(EOculusFaceExpression::JawThrust, "jawThrust");
	ExpressionNames.Add(EOculusFaceExpression::LidTightenerL, "lidTightener_L");
	ExpressionNames.Add(EOculusFaceExpression::LidTightenerR, "lidTightener_R");
	ExpressionNames.Add(EOculusFaceExpression::LipCornerDepressorL, "lipCornerDepressor_L");
	ExpressionNames.Add(EOculusFaceExpression::LipCornerDepressorR, "lipCornerDepressor_R");
	ExpressionNames.Add(EOculusFaceExpression::LipCornerPullerL, "lipCornerPuller_L");
	ExpressionNames.Add(EOculusFaceExpression::LipCornerPullerR, "lipCornerPuller_R");
	ExpressionNames.Add(EOculusFaceExpression::LipFunnelerLB, "lipFunnelerLB");
	ExpressionNames.Add(EOculusFaceExpression::LipFunnelerLT, "lipFunnelerLT");
	ExpressionNames.Add(EOculusFaceExpression::LipFunnelerRB, "lipFunnelerRB");
	ExpressionNames.Add(EOculusFaceExpression::LipFunnelerRT, "lipFunnelerRT");
	ExpressionNames.Add(EOculusFaceExpression::LipPressorL, "lipPressor_L");
	ExpressionNames.Add(EOculusFaceExpression::LipPressorR, "lipPressor_R");
	ExpressionNames.Add(EOculusFaceExpression::LipPuckerL, "lipPucker_L");
	ExpressionNames.Add(EOculusFaceExpression::LipPuckerR, "lipPucker_R");
	ExpressionNames.Add(EOculusFaceExpression::LipStretcherL, "lipStretcher_L");
	ExpressionNames.Add(EOculusFaceExpression::LipStretcherR, "lipStretcher_R");
	ExpressionNames.Add(EOculusFaceExpression::LipSuckLB, "lipSuckLB");
	ExpressionNames.Add(EOculusFaceExpression::LipSuckLT, "lipSuckLT");
	ExpressionNames.Add(EOculusFaceExpression::LipSuckRB, "lipSuckRB");
	ExpressionNames.Add(EOculusFaceExpression::LipSuckRT, "lipSuckRT");
	ExpressionNames.Add(EOculusFaceExpression::LipTightenerL, "lipTightener_L");
	ExpressionNames.Add(EOculusFaceExpression::LipTightenerR, "lipTightener_R");
	ExpressionNames.Add(EOculusFaceExpression::LipsToward, "lipsToward");
	ExpressionNames.Add(EOculusFaceExpression::LowerLipDepressorL, "lowerLipDepressor_L");
	ExpressionNames.Add(EOculusFaceExpression::LowerLipDepressorR, "lowerLipDepressor_R");
	ExpressionNames.Add(EOculusFaceExpression::MouthLeft, "mouthLeft");
	ExpressionNames.Add(EOculusFaceExpression::MouthRight, "mouthRight");
	ExpressionNames.Add(EOculusFaceExpression::NoseWrinklerL, "noseWrinkler_L");
	ExpressionNames.Add(EOculusFaceExpression::NoseWrinklerR, "noseWrinkler_R");
	ExpressionNames.Add(EOculusFaceExpression::OuterBrowRaiserL, "outerBrowRaiser_L");
	ExpressionNames.Add(EOculusFaceExpression::OuterBrowRaiserR, "outerBrowRaiser_R");
	ExpressionNames.Add(EOculusFaceExpression::UpperLidRaiserL, "upperLidRaiser_L");
	ExpressionNames.Add(EOculusFaceExpression::UpperLidRaiserR, "upperLidRaiser_R");
	ExpressionNames.Add(EOculusFaceExpression::UpperLipRaiserL, "upperLipRaiser_L");
	ExpressionNames.Add(EOculusFaceExpression::UpperLipRaiserR, "upperLipRaiser_R");
	ExpressionNames.Add(EOculusFaceExpression::TongueTipInterdental, "tongueTipInterdental");
	ExpressionNames.Add(EOculusFaceExpression::TongueTipAlveolar, "tongueTipAlveolar");
	ExpressionNames.Add(EOculusFaceExpression::TongueFrontDorsalPalate, "tongueFrontDorsalPalate");
	ExpressionNames.Add(EOculusFaceExpression::TongueMidDorsalPalate, "tongueMidDorsalPalate");
	ExpressionNames.Add(EOculusFaceExpression::TongueBackDorsalVelar, "tongueBackDorsalVelar");
	ExpressionNames.Add(EOculusFaceExpression::TongueOut, "tongueOut");
}

void UOculusFaceTrackingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!UOculusMovementFunctionLibrary::IsFaceTrackingSupported())
	{
		// Early exit if face tracking isn't supported
		UE_LOG(LogOculusMovement, Warning, TEXT("Face tracking is not supported. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	if (TargetMeshComponentName == NAME_None)
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Invalid mesh component name. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	if (!InitializeFaceTracking())
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Failed to initialize face tracking. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	if (!UOculusMovementFunctionLibrary::StartFaceTracking())
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Failed to start face tracking. (%s: %s)"), *GetOwner()->GetName(), *GetName());
		SetComponentTickEnabled(false);
		return;
	}
	++TrackingInstanceCount;
}

void UOculusFaceTrackingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsComponentTickEnabled())
	{
		if (--TrackingInstanceCount == 0)
		{
			if (!UOculusMovementFunctionLibrary::StopFaceTracking())
			{
				UE_LOG(LogOculusMovement, Warning, TEXT("Failed to stop face tracking. (%s: %s)"), *GetOwner()->GetName(), *GetName());
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UOculusFaceTrackingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(TargetMeshComponent))
	{
		UE_LOG(LogOculusMovement, VeryVerbose, TEXT("No target mesh specified. (%s:%s)"), *GetOwner()->GetName(), *GetName());
		return;
	}

	if (UOculusMovementFunctionLibrary::TryGetFaceState(FaceState) && bUpdateFace)
	{
		InvalidFaceStateTimer = 0.0f;

		MorphTargets.ResetMorphTargetCurves(TargetMeshComponent);

		for (int32 FaceExpressionIndex = 0; FaceExpressionIndex < static_cast<int32>(EOculusFaceExpression::COUNT); ++FaceExpressionIndex)
		{
			if (ExpressionValid[FaceExpressionIndex])
			{
				FName ExpressionName = ExpressionNames[static_cast<EOculusFaceExpression>(FaceExpressionIndex)];
				MorphTargets.SetMorphTarget(ExpressionName, FaceState.ExpressionWeights[FaceExpressionIndex]);
			}
		}
	}
	else
	{
		InvalidFaceStateTimer += DeltaTime;
		if (InvalidFaceStateTimer >= InvalidFaceDataResetTime)
		{
			MorphTargets.ResetMorphTargetCurves(TargetMeshComponent);
		}
	}

	MorphTargets.ApplyMorphTargets(TargetMeshComponent);
}

void UOculusFaceTrackingComponent::SetExpressionValue(EOculusFaceExpression Expression, float Value)
{
	if (Expression >= EOculusFaceExpression::COUNT)
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Cannot set expression value with invalid expression index."));
		return;
	}

	if (!ExpressionValid[static_cast<int32>(Expression)])
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Cannot set expression value for an expression with an invalid associated morph target name. Expression name: %s"), *StaticEnum<EOculusFaceExpression>()->GetValueAsString(Expression));
		return;
	}

	FName ExpressionName = ExpressionNames[Expression];
	MorphTargets.SetMorphTarget(ExpressionName, Value);
}

float UOculusFaceTrackingComponent::GetExpressionValue(EOculusFaceExpression Expression) const
{
	if (Expression >= EOculusFaceExpression::COUNT)
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Cannot request expression value using an invalid expression index."));
		return 0.0f;
	}

	FName ExpressionName = ExpressionNames[Expression];
	if (ExpressionName == NAME_None)
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Cannot request expression value for an expression with an invalid associated morph target name. Expression name: %s"), *StaticEnum<EOculusFaceExpression>()->GetValueAsString(Expression));
		return 0.0f;
	}

	return MorphTargets.GetMorphTarget(ExpressionName);
}

void UOculusFaceTrackingComponent::ClearExpressionValues()
{
	MorphTargets.ClearMorphTargets();
}

bool UOculusFaceTrackingComponent::InitializeFaceTracking()
{
	TargetMeshComponent = OculusUtility::FindComponentByName<USkinnedMeshComponent>(GetOwner(), TargetMeshComponentName);

	if (!IsValid(TargetMeshComponent))
	{
		UE_LOG(LogOculusMovement, Warning, TEXT("Could not find skeletal mesh component with name: (%s). (%s:%s)"), *TargetMeshComponentName.ToString(), *GetOwner()->GetName(), *GetName());
		return false;
	}

	if (TargetMeshComponent && TargetMeshComponent->SkeletalMesh)
	{
		const TMap<FName, int32>& MorphTargetIndexMap = TargetMeshComponent->SkeletalMesh->GetMorphTargetIndexMap();

		for (const auto& it : ExpressionNames)
		{
			ExpressionValid[static_cast<int32>(it.Key)] = MorphTargetIndexMap.Contains(it.Value);
		}

		return true;
	}

	return false;
}
