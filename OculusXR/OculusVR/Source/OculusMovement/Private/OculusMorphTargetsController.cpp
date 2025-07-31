/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusMorphTargetsController.h"

#include "AnimationRuntime.h"

void FOculusMorphTargetsController::ResetMorphTargetCurves(USkinnedMeshComponent* TargetMeshComponent)
{
	if (TargetMeshComponent)
	{
		TargetMeshComponent->ActiveMorphTargets.Reset();

		if (TargetMeshComponent->SkeletalMesh)
		{
			TargetMeshComponent->MorphTargetWeights.SetNum(TargetMeshComponent->SkeletalMesh->GetMorphTargets().Num());

			// we need this code to ensure the buffer gets cleared whether or not you have morphtarget curve set
			// the case, where you had morphtargets weight on, and when you clear the weight, you want to make sure
			// the buffer gets cleared and resized
			if (TargetMeshComponent->MorphTargetWeights.Num() > 0)
			{
				FMemory::Memzero(TargetMeshComponent->MorphTargetWeights.GetData(), TargetMeshComponent->MorphTargetWeights.GetAllocatedSize());
			}
		}
		else
		{
			TargetMeshComponent->MorphTargetWeights.Reset();
		}
	}
}

void FOculusMorphTargetsController::ApplyMorphTargets(USkinnedMeshComponent* TargetMeshComponent)
{
	if (TargetMeshComponent && TargetMeshComponent->SkeletalMesh)
	{
		if (MorphTargetCurves.Num() > 0)
		{
			FAnimationRuntime::AppendActiveMorphTargets(TargetMeshComponent->SkeletalMesh, MorphTargetCurves, TargetMeshComponent->ActiveMorphTargets, TargetMeshComponent->MorphTargetWeights);
		}
	}
}

void FOculusMorphTargetsController::SetMorphTarget(FName MorphTargetName, float Value)
{
	float* CurveValPtr = MorphTargetCurves.Find(MorphTargetName);
	bool bShouldAddToList = FPlatformMath::Abs(Value) > ZERO_ANIMWEIGHT_THRESH;
	if (bShouldAddToList)
	{
		if (CurveValPtr)
		{
			// sum up, in the future we might normalize, but for now this just sums up
			// this won't work well if all of them have full weight - i.e. additive
			*CurveValPtr = Value;
		}
		else
		{
			MorphTargetCurves.Add(MorphTargetName, Value);
		}
	}
	// if less than ZERO_ANIMWEIGHT_THRESH
	// no reason to keep them on the list
	else
	{
		// remove if found
		MorphTargetCurves.Remove(MorphTargetName);
	}
}

float FOculusMorphTargetsController::GetMorphTarget(FName MorphTargetName) const
{
	const float* CurveValPtr = MorphTargetCurves.Find(MorphTargetName);

	if (CurveValPtr)
	{
		return *CurveValPtr;
	}
	else
	{
		return 0.0f;
	}
}

void FOculusMorphTargetsController::ClearMorphTargets()
{
	MorphTargetCurves.Empty();
}
