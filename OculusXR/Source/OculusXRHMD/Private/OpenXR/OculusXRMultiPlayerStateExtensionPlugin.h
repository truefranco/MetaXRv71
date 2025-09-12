// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include "CoreMinimal.h"
#include "OpenXR/IOculusXRExtensionPlugin.h"

#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
class ULevelEditorPlaySettings;
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogOculusMultiPlayerStateExtensionPlugin, Log, All);

namespace OculusXR
{
	class FMultiPlayerStateExtensionPlugin : public IOculusXRExtensionPlugin
	{
	public:
		FMultiPlayerStateExtensionPlugin();
		~FMultiPlayerStateExtensionPlugin();

		void SwitchPrimaryPIE(int PrimaryPIEIndex);
#ifdef WITH_OCULUS_BRANCH
		virtual void ResetPose() override;
		virtual void ReCalcPose(FTransform& CurHMDHeadPose) override;
#endif // WITH_OCULUS_BRANCH
	private:
		void InitMultiPlayerPoses(const FTransform& CurPose);

		int CurPlayerIndex;
		FTransform LastFrameHMDHeadPose;
		TArray<FTransform> MultiPlayerPoses;
	};

} // namespace OculusXR
