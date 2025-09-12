// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRLayerExtensionPlugin.h"
#include "Async/Async.h"
#include "DynamicResolutionState.h"
#include "IHeadMountedDisplay.h"
#include "IOpenXRHMD.h"
#include "IOpenXRHMDModule.h"
#include "OculusXROpenXRDynamicResolutionState.h"
#include "OculusXRHMDRuntimeSettings.h"
#include "OculusXROpenXRUtilities.h"
#include "OculusXRXRFunctions.h"
#include "OpenXRCore.h"
#include "XRThreadUtils.h"

namespace
{
	XrCompositionLayerSettingsFlagsFB ToSharpenLayerFlag(EOculusXREyeBufferSharpenType EyeBufferSharpenType)
	{
		XrCompositionLayerSettingsFlagsFB Flag = 0;
		switch (EyeBufferSharpenType)
		{
			case EOculusXREyeBufferSharpenType::SLST_None:
				Flag = 0;
				break;
			case EOculusXREyeBufferSharpenType::SLST_Normal:
				Flag = XR_COMPOSITION_LAYER_SETTINGS_NORMAL_SHARPENING_BIT_FB;
				break;
			case EOculusXREyeBufferSharpenType::SLST_Quality:
				Flag = XR_COMPOSITION_LAYER_SETTINGS_QUALITY_SHARPENING_BIT_FB;
				break;
			case EOculusXREyeBufferSharpenType::SLST_Auto:
				Flag = XR_COMPOSITION_LAYER_SETTINGS_AUTO_LAYER_FILTER_BIT_META;
				break;
			default:
				break;
		}
		return Flag;
	}

	XrColor4f ToXrColor4f(FLinearColor Color)
	{
		return XrColor4f{ Color.R, Color.G, Color.B, Color.A };
	}

} // namespace

namespace OculusXR
{
	FLayerExtensionPlugin::FLayerExtensionPlugin()
		: Session(XR_NULL_HANDLE)
		, bExtLocalDimmingAvailable(false)
		, bExtCompositionLayerSettingsAvailable(false)
		, bRecommendedResolutionExtensionAvailable(false)
		, LocalDimmingMode_RHIThread(XR_LOCAL_DIMMING_MODE_ON_META)
		, LocalDimmingExt_RHIThread{}
		, EyeSharpenLayerFlags_RHIThread(0)
		, ColorScaleInfo_RHIThread{}
		, _HeadersStorage{}
		, bPixelDensityAdaptive(false)
		, RecommendedImageHeight_GameThread(0)
		, Settings_GameThread{}
		, MaxPixelDensity_RenderThread(0)
	{
	}

	bool FLayerExtensionPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_META_LOCAL_DIMMING_EXTENSION_NAME);
		OutExtensions.Add(XR_FB_COMPOSITION_LAYER_SETTINGS_EXTENSION_NAME);
		OutExtensions.Add(XR_META_RECOMMENDED_LAYER_RESOLUTION_EXTENSION_NAME);
		return true;
	}

	const void* FLayerExtensionPlugin::OnCreateInstance(class IOpenXRHMDModule* InModule, const void* InNext)
	{
		if (InModule != nullptr)
		{
			bExtLocalDimmingAvailable = InModule->IsExtensionEnabled(XR_META_LOCAL_DIMMING_EXTENSION_NAME);
			bExtCompositionLayerSettingsAvailable = InModule->IsExtensionEnabled(XR_FB_COMPOSITION_LAYER_SETTINGS_EXTENSION_NAME);
			bRecommendedResolutionExtensionAvailable = InModule->IsExtensionEnabled(XR_META_RECOMMENDED_LAYER_RESOLUTION_EXTENSION_NAME);
		}
		return IOculusXRExtensionPlugin::OnCreateInstance(InModule, InNext);
	}

	void FLayerExtensionPlugin::PostCreateSession(XrSession InSession)
	{
		Session = InSession;
		const UOculusXRHMDRuntimeSettings* HMDSettings = GetDefault<UOculusXRHMDRuntimeSettings>();
		if (HMDSettings != nullptr)
		{
#if defined(WITH_OCULUS_BRANCH) || defined(WITH_OPENXR_BRANCH)
			bPixelDensityAdaptive = HMDSettings->bDynamicResolution && bRecommendedResolutionExtensionAvailable;
#endif

			if (IConsoleVariable* MobileDynamicResCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.MobileLDRDynamicResolution")))
			{
				MobileDynamicResCVar->Set(bPixelDensityAdaptive);
			}

			if (bPixelDensityAdaptive)
			{
				Settings_GameThread = MakeShareable(new OculusXRHMD::FSettings());
				Settings_GameThread->Flags.bPixelDensityAdaptive = bPixelDensityAdaptive;

				if (IConsoleVariable* DynamicResOperationCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicRes.OperationMode")))
				{
					// Operation mode for dynamic resolution
					// Enable regardless of the game user settings
					DynamicResOperationCVar->Set(2);
				}

				GEngine->ChangeDynamicResolutionStateAtNextFrame(MakeShareable(new OculusXR::FOpenXRDynamicResolutionState(Settings_GameThread)));

				const float MaxPixelDensity = Settings_GameThread->GetPixelDensityMax();

				ENQUEUE_RENDER_COMMAND(OculusXR_SetEnableLocalDimming)
				([this, MaxPixelDensity](FRHICommandListImmediate& RHICmdList) {
					MaxPixelDensity_RenderThread = MaxPixelDensity;
				});
			}
		}
	}

	void FLayerExtensionPlugin::OnBeginRendering_GameThread(XrSession InSession, FSceneViewFamily& InViewFamily, TArrayView<const uint32> VisibleLayers)
	{
		check(IsInGameThread());

		if (bPixelDensityAdaptive)
		{
			IXRTrackingSystem* TrackingSystem = OculusXR::GetOpenXRTrackingSystem();
			if (TrackingSystem != nullptr)
			{
				IOpenXRHMD* OpenXRHMD = TrackingSystem->GetIOpenXRHMD();
				check(OpenXRHMD != nullptr);

				const XrTime PredictedDisplayTime = OpenXRHMD->GetDisplayTime();

				ENQUEUE_RENDER_COMMAND(OculusXR_UpdatePredictedTime)
				([this, PredictedDisplayTime](FRHICommandListImmediate& RHICmdList) {
					RHICmdList.EnqueueLambda([this, PredictedDisplayTime](FRHICommandListImmediate& RHICmdList) {
						PredictedDisplayTime_RHIThread = PredictedDisplayTime;
					});
				});

				IHeadMountedDisplay* Hmd = TrackingSystem->GetHMDDevice();
				IHeadMountedDisplay::MonitorInfo MonitorInfo = {};
				check(Hmd != nullptr);

				if (Hmd->GetHMDMonitorInfo(MonitorInfo))
				{
					static auto PixelDensityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.SecondaryScreenPercentage.HMDRenderTarget"));
					if (PixelDensityCVar != nullptr)
					{
						// Set pixel density to dynamic resolutions's max so default target is sized to this
						// FOpenXRDynamicResolutionState driver will only scale down to the current recommmended resolution
						PixelDensityCVar->Set(Settings_GameThread->GetPixelDensityMax() * 100.0f);
					}

					float PixelDensity = RecommendedImageHeight_GameThread == 0 ? 1.0f : static_cast<float>(RecommendedImageHeight_GameThread) / (MonitorInfo.ResolutionY);

					static const auto CVarOculusDynamicPixelDensity = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Oculus.DynamicResolution.PixelDensity"));
					const float PixelDensityCVarOverride = CVarOculusDynamicPixelDensity != nullptr ? CVarOculusDynamicPixelDensity->GetValueOnAnyThread() : 0.0f;
					if (PixelDensityCVarOverride > 0.0f)
					{
						PixelDensity = PixelDensityCVarOverride;
					}

					check(Settings_GameThread != nullptr)
						Settings_GameThread->SetPixelDensitySmooth(PixelDensity);
				}
			}
		}
		else
		{
			if (Settings_GameThread != nullptr)
			{
#if !UE_VERSION_OLDER_THAN(5, 5, 0)
				static const auto PixelDensityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("xr.SecondaryScreenPercentage.HMDRenderTarget"));
#else
				static const auto PixelDensityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("vr.PixelDensity"));
#endif

				Settings_GameThread->SetPixelDensity(PixelDensityCVar ? PixelDensityCVar->GetFloat() : 1.0f);
			}
		}
	}

	const void* FLayerExtensionPlugin::OnEndFrame(XrSession InSession, XrTime DisplayTime, const void* InNext)
	{
		check(IsInRenderingThread() || IsInRHIThread());
		const void* Next = InNext;
		if (bExtLocalDimmingAvailable)
		{
			LocalDimmingExt_RHIThread.type = XR_TYPE_LOCAL_DIMMING_FRAME_END_INFO_META;
			LocalDimmingExt_RHIThread.localDimmingMode = LocalDimmingMode_RHIThread;
			LocalDimmingExt_RHIThread.next = Next;
			Next = &LocalDimmingExt_RHIThread;
		}
		return Next;
	}

	const void* FLayerExtensionPlugin::OnEndProjectionLayer_RHIThread(XrSession InSession, int32 InLayerIndex, const void* InNext, XrCompositionLayerFlags& OutFlags)
	{
		check(IsInRenderingThread() || IsInRHIThread());
		const void* Next = InNext;
		if (bExtCompositionLayerSettingsAvailable)
		{
			XrCompositionLayerSettingsExt.type = XR_TYPE_COMPOSITION_LAYER_SETTINGS_FB;
			XrCompositionLayerSettingsExt.next = Next;
			XrCompositionLayerSettingsExt.layerFlags = EyeSharpenLayerFlags_RHIThread;
			Next = &XrCompositionLayerSettingsExt;
		}
		return Next;
	}

	void FLayerExtensionPlugin::SetEnableLocalDimming(bool Enable)
	{
		ENQUEUE_RENDER_COMMAND(OculusXR_SetEnableLocalDimming)
		([this, Enable](FRHICommandListImmediate& RHICmdList) {
			RHICmdList.EnqueueLambda([this, Enable](FRHICommandListImmediate& RHICmdList) {
				LocalDimmingMode_RHIThread = Enable ? XR_LOCAL_DIMMING_MODE_ON_META : XR_LOCAL_DIMMING_MODE_OFF_META;
			});
		});
	}

	void FLayerExtensionPlugin::SetEyeBufferSharpenType(EOculusXREyeBufferSharpenType EyeBufferSharpenType)
	{
		ENQUEUE_RENDER_COMMAND(OculusXR_SetEyeBufferSharpenType)
		([this, EyeBufferSharpenType](FRHICommandListImmediate& RHICmdList) {
			RHICmdList.EnqueueLambda([this, EyeBufferSharpenType](FRHICommandListImmediate& RHICmdList) {
				EyeSharpenLayerFlags_RHIThread = ToSharpenLayerFlag(EyeBufferSharpenType);
			});
		});
	}

	void FLayerExtensionPlugin::SetColorScaleAndOffset(FLinearColor ColorScale, FLinearColor ColorOffset, bool bApplyToAllLayers)
	{
		IXRTrackingSystem* TrackingSystem = OculusXR::GetOpenXRTrackingSystem();
		if (TrackingSystem != nullptr)
		{
			IHeadMountedDisplay* Hmd = TrackingSystem->GetHMDDevice();
			Hmd->SetColorScaleAndBias(ColorScale, ColorOffset);
		}
#if defined(WITH_OCULUS_BRANCH) || defined(WITH_OPENXR_BRANCH)
		ENQUEUE_RENDER_COMMAND(OculusXR_SetColorScaleAndOffset)
		([this, ColorScale, ColorOffset, bApplyToAllLayers](FRHICommandListImmediate& RHICmdList) {
			RHICmdList.EnqueueLambda([this, ColorScale, ColorOffset, bApplyToAllLayers](FRHICommandListImmediate& RHICmdList) {
				ColorScaleInfo_RHIThread.ColorScale = ColorScale;
				ColorScaleInfo_RHIThread.ColorOffset = ColorOffset;
				ColorScaleInfo_RHIThread.bApplyColorScaleAndOffsetToAllLayers = bApplyToAllLayers;
			});
		});
#endif
	}

#if defined(WITH_OCULUS_BRANCH) || defined(WITH_OPENXR_BRANCH)
	static bool ShouldApplyColorScale(const XrCompositionLayerBaseHeader* Header)
	{
		switch (Header->type)
		{
			case XR_TYPE_COMPOSITION_LAYER_QUAD:
			case XR_TYPE_COMPOSITION_LAYER_EQUIRECT_KHR:
			case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR:
			case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR:
				return true;
				break;
			default:
				break;
		}
		return false;
	}

	void FLayerExtensionPlugin::UpdatePixelDensity(const XrCompositionLayerBaseHeader* LayerHeader)
	{
		check(LayerHeader != nullptr);

		if (LayerHeader->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION && bPixelDensityAdaptive && bRecommendedResolutionExtensionAvailable)
		{
			IXRTrackingSystem* TrackingSystem = OculusXR::GetOpenXRTrackingSystem();
			if (TrackingSystem != nullptr)
			{
				XrRecommendedLayerResolutionMETA ResolutionRecommendation = {};
				ResolutionRecommendation.type = XR_TYPE_RECOMMENDED_LAYER_RESOLUTION_META;
				ResolutionRecommendation.next = nullptr;
				ResolutionRecommendation.isValid = false;

				XrRecommendedLayerResolutionGetInfoMETA ResolutionRecommendationGetInfo = {};
				ResolutionRecommendationGetInfo.type = XR_TYPE_RECOMMENDED_LAYER_RESOLUTION_GET_INFO_META;
				ResolutionRecommendationGetInfo.next = nullptr;
				ResolutionRecommendationGetInfo.layer = LayerHeader;
				ResolutionRecommendationGetInfo.predictedDisplayTime = PredictedDisplayTime_RHIThread;

				ENSURE_XRCMD(xrGetRecommendedLayerResolutionMETA.GetValue()(Session, &ResolutionRecommendationGetInfo, &ResolutionRecommendation));

				if (ResolutionRecommendation.isValid == XR_TRUE)
				{
					AsyncTask(ENamedThreads::GameThread, [this, ResolutionRecommendation] {
						RecommendedImageHeight_GameThread = ResolutionRecommendation.recommendedImageDimensions.height;
					});
				}
			}
		}
	}

	void FLayerExtensionPlugin::UpdateCompositionLayers_RHIThread(XrSession InSession, TArray<XrCompositionLayerBaseHeader*>& Headers)
	{
		check(IsInRenderingThread() || IsInRHIThread());

		if (ColorScaleInfo_RHIThread.bApplyColorScaleAndOffsetToAllLayers)
		{
			ColorScale_RHIThread.Reset(Headers.Num());
		}

		for (const XrCompositionLayerBaseHeader* Header : Headers)
		{
			if (Header->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION)
			{
				UpdatePixelDensity(Header);
			}

			if (ColorScaleInfo_RHIThread.bApplyColorScaleAndOffsetToAllLayers && ShouldApplyColorScale(Header))
			{
				ColorScale_RHIThread.AddUninitialized();
				XrCompositionLayerColorScaleBiasKHR& ColorScaleBias = ColorScale_RHIThread.Last();
				ColorScaleBias = { XR_TYPE_COMPOSITION_LAYER_COLOR_SCALE_BIAS_KHR };
				ColorScaleBias.next = const_cast<void*>(Header->next);
				ColorScaleBias.colorScale = ToXrColor4f(ColorScaleInfo_RHIThread.ColorScale);
				ColorScaleBias.colorBias = ToXrColor4f(ColorScaleInfo_RHIThread.ColorOffset);
				const_cast<XrCompositionLayerBaseHeader*>(Header)->next = &ColorScaleBias;
			}
		}
	}
#endif

} // namespace OculusXR
