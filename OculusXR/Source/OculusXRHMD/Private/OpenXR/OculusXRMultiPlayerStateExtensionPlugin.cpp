// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRMultiPlayerStateExtensionPlugin.h"
#include "OculusXRHMDRuntimeSettings.h"

DEFINE_LOG_CATEGORY(LogOculusMultiPlayerStateExtensionPlugin);

namespace OculusXR
{

	FMultiPlayerStateExtensionPlugin::FMultiPlayerStateExtensionPlugin()
	{
#ifdef WITH_OCULUS_BRANCH
		ResetPose();
#endif // WITH_OCULUS_BRANCH
	}

	FMultiPlayerStateExtensionPlugin::~FMultiPlayerStateExtensionPlugin()
	{
	}

	void FMultiPlayerStateExtensionPlugin::SwitchPrimaryPIE(int PrimaryPIEIndex)
	{
		CurPlayerIndex = PrimaryPIEIndex;
	}

	void FMultiPlayerStateExtensionPlugin::InitMultiPlayerPoses(const FTransform& CurPose)
	{
#if WITH_EDITOR && PLATFORM_WINDOWS
		if (!GIsEditor || MultiPlayerPoses.Num())
		{
			return;
		}

		if (!FApp::HasVRFocus())
		{
			return;
		}

		ULevelEditorPlaySettings* PlayInSettings = GetMutableDefault<ULevelEditorPlaySettings>();
		check(PlayInSettings);
		int PlayNumberOfClients;
		PlayInSettings->GetPlayNumberOfClients(PlayNumberOfClients);
		if (PlayNumberOfClients <= 1)
		{
			return;
		}

		EPlayNetMode OutPlayNetMode;
		PlayInSettings->GetPlayNetMode(OutPlayNetMode);
		if (OutPlayNetMode != EPlayNetMode::PIE_Standalone)
		{
			// In case of non-standalone mode, server is the first player, client idx should start from 1
			PlayNumberOfClients++;
		}

		LastFrameHMDHeadPose = CurPose;
		MultiPlayerPoses.Empty();
		MultiPlayerPoses.InsertDefaulted(0, PlayNumberOfClients);
		for (auto& PlayerPose : MultiPlayerPoses)
		{
			PlayerPose = CurPose;
		}
		UE_LOG(LogHMD, Log, TEXT("MultiPlayer poses are initialized."));
#endif
	}

#ifdef WITH_OCULUS_BRANCH
	void FMultiPlayerStateExtensionPlugin::ResetPose()
	{
#if WITH_EDITOR && PLATFORM_WINDOWS
		CurPlayerIndex = 0;
		LastFrameHMDHeadPose = FTransform::Identity;
		MultiPlayerPoses.Empty();
#endif
	}

	void FMultiPlayerStateExtensionPlugin::ReCalcPose(FTransform& CurHMDHeadPose)
	{
#if WITH_EDITOR && PLATFORM_WINDOWS
		if (!GIsEditor || GetMutableDefault<UOculusXRHMDRuntimeSettings>()->MPPoseRestoreType == EOculusXRMPPoseRestoreType::Disabled)
		{
			return;
		}

		if (!MultiPlayerPoses.Num())
		{
			InitMultiPlayerPoses(CurHMDHeadPose);
		}

		if (MultiPlayerPoses.Num() <= 1)
		{
			return;
		}

		if (CurPlayerIndex >= MultiPlayerPoses.Num())
		{
			UE_LOG(LogHMD, Error, TEXT("CurPlayerIndex %i is larger than MultiPlayerPoses.Num() !"), CurPlayerIndex, MultiPlayerPoses.Num());
		}

		FTransform& PlayerPose = MultiPlayerPoses[CurPlayerIndex];
		if (GetMutableDefault<UOculusXRHMDRuntimeSettings>()->MPPoseRestoreType == EOculusXRMPPoseRestoreType::PositionOnly)
		{
			FVector DeltaPosition = CurHMDHeadPose.GetTranslation() - LastFrameHMDHeadPose.GetTranslation();
			PlayerPose.SetTranslation(PlayerPose.GetTranslation() + DeltaPosition);
			LastFrameHMDHeadPose.SetTranslation(CurHMDHeadPose.GetTranslation());
			CurHMDHeadPose.SetTranslation(PlayerPose.GetTranslation());
		}
		else
		{
			FTransform DeltaPose = LastFrameHMDHeadPose.Inverse() * CurHMDHeadPose;
			PlayerPose = PlayerPose * DeltaPose;
			LastFrameHMDHeadPose = CurHMDHeadPose;
			CurHMDHeadPose = PlayerPose;
		}
#endif
	}
#endif // WITH_OCULUS_BRANCH

} // namespace OculusXR
