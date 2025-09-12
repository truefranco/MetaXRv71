// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once
#include "khronos/openxr/openxr.h"
#include "CoreMinimal.h"
#include "OculusXRHMDTypes.h"
#include "OculusXRHMD_Settings.h"
#include "OpenXR/IOculusXRExtensionPlugin.h"

namespace OculusXR
{

	class FLayerExtensionPlugin : public IOculusXRExtensionPlugin
	{
	public:
		FLayerExtensionPlugin();
		// IOpenXRExtensionPlugin
		virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		const void* OnCreateInstance(class IOpenXRHMDModule* InModule, const void* InNext);
		virtual const void* OnEndFrame(XrSession InSession, XrTime DisplayTime, const void* InNext) override;
		virtual const void* OnEndProjectionLayer_RHIThread(XrSession InSession, int32 InLayerIndex, const void* InNext, XrCompositionLayerFlags& OutFlags);
		virtual void PostCreateSession(XrSession InSession) override;
		virtual void OnBeginRendering_GameThread(XrSession InSession, FSceneViewFamily& InViewFamily, TArrayView<const uint32> VisibleLayers) override;

#if defined(WITH_OCULUS_BRANCH) || defined(WITH_OPENXR_BRANCH)
		virtual void UpdateCompositionLayers_RHIThread(XrSession InSession, TArray<XrCompositionLayerBaseHeader*>& Headers) override;
#endif
		void SetEnableLocalDimming(bool Enable);
		void SetEyeBufferSharpenType(EOculusXREyeBufferSharpenType EyeBufferSharpenType);
		void SetColorScaleAndOffset(FLinearColor ColorScale, FLinearColor ColorOffset, bool bApplyToAllLayers);

	private:
		void UpdatePixelDensity(const XrCompositionLayerBaseHeader* LayerHeader);

		XrSession Session;
		bool bExtLocalDimmingAvailable;
		bool bExtCompositionLayerSettingsAvailable;
		bool bRecommendedResolutionExtensionAvailable;

		XrLocalDimmingModeMETA LocalDimmingMode_RHIThread;
		XrLocalDimmingFrameEndInfoMETA LocalDimmingExt_RHIThread;
		XrCompositionLayerSettingsFlagsFB EyeSharpenLayerFlags_RHIThread;
		XrCompositionLayerSettingsFB XrCompositionLayerSettingsExt;

		struct FColorScaleInfo
		{
			FColorScaleInfo()
				: ColorScale{ 1.0f, 1.0f, 1.0f, 1.0f }
				, ColorOffset{ 0.0f, 0.0f, 0.0f, 0.0f }
				, bApplyColorScaleAndOffsetToAllLayers(false){};
			FLinearColor ColorScale;
			FLinearColor ColorOffset;
			bool bApplyColorScaleAndOffsetToAllLayers;
		};

		FColorScaleInfo ColorScaleInfo_RHIThread;
		TArray<XrCompositionLayerColorScaleBiasKHR> ColorScale_RHIThread;

		TArray<XrCompositionLayerBaseHeader> _HeadersStorage;
		bool bPixelDensityAdaptive;
		uint32_t RecommendedImageHeight_GameThread;
		OculusXRHMD::FSettingsPtr Settings_GameThread;
		XrTime PredictedDisplayTime_RHIThread;
		float MaxPixelDensity_RenderThread;
	};

} // namespace OculusXR
