// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRSpaceWarp.h"
#include "IOpenXRHMD.h"
#include "IOpenXRHMDModule.h"
#include "IXRTrackingSystem.h"
#include "OculusXROpenXRUtilities.h"
#include "OpenXRCore.h"
#include "OpenXRHMD_Swapchain.h"
#include "StereoRenderUtils.h"

#if defined(WITH_OCULUS_BRANCH) || defined(WITH_OPENXR_BRANCH)

DEFINE_LOG_CATEGORY(LogOculusSpaceWarpExtensionPlugin);

static const int64 VELOCITY_SWAPCHAIN_WAIT_TIMEOUT = 100000000ll; // 100ms in nanoseconds.

namespace OculusXR
{
	FSpaceWarpExtensionPlugin::FSpaceWarpExtensionPlugin()
	{
	}

	bool FSpaceWarpExtensionPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		UE_LOG(LogOculusSpaceWarpExtensionPlugin, Warning, TEXT("FSpaceWarpExtensionPlugin::GetRequiredExtensions"));

		static const auto CVarSupportMobileSpaceWarp = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.SupportMobileSpaceWarp"));
		if (CVarSupportMobileSpaceWarp && (CVarSupportMobileSpaceWarp->GetValueOnAnyThread() != 0))
		{
			OutExtensions.Add(XR_FB_SPACE_WARP_EXTENSION_NAME);
		}

		return true;
	}

	void* FSpaceWarpExtensionPlugin::OnEnumerateViewConfigurationViews(XrInstance InInstance, XrSystemId InSystem, XrViewConfigurationType InViewConfigurationType, uint32_t InViewIndex, void* InNext)
	{
		SelectedViewConfigurationType = InViewConfigurationType;
		return InNext;
	}

	const void* FSpaceWarpExtensionPlugin::OnCreateInstance(IOpenXRHMDModule* InModule, const void* InNext)
	{
		static const auto CVarSupportMobileSpaceWarp = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("vr.SupportMobileSpaceWarp"));
		if (InModule && CVarSupportMobileSpaceWarp && (CVarSupportMobileSpaceWarp->GetValueOnAnyThread() != 0))
		{
			bSpaceWarpExtensionEnabled = InModule->IsExtensionEnabled(XR_FB_SPACE_WARP_EXTENSION_NAME);
		}

		return InNext;
	}

	void FSpaceWarpExtensionPlugin::PostCreateInstance(XrInstance InInstance)
	{
		if (!bSpaceWarpExtensionEnabled)
		{
			return;
		}

		XrInstanceProperties InstanceProperties = { XR_TYPE_INSTANCE_PROPERTIES, nullptr };
		XR_ENSURE(xrGetInstanceProperties(InInstance, &InstanceProperties));
		InstanceProperties.runtimeName[XR_MAX_RUNTIME_NAME_SIZE - 1] = 0; // Ensure the name is null terminated.
	}

	void FSpaceWarpExtensionPlugin::PostCreateSession(XrSession InSession)
	{
		if (!bSpaceWarpExtensionEnabled)
		{
			return;
		}

		XrSystemProperties SystemProperties;
		SystemProperties = XrSystemProperties{ XR_TYPE_SYSTEM_PROPERTIES, &SpaceWarpSystemProperties };
		XR_ENSURE(xrGetSystemProperties(IOpenXRHMDModule::Get().GetInstance(), IOpenXRHMDModule::Get().GetSystemId(), &SystemProperties));

		SpaceWarpViewExtension = FSceneViewExtensions::NewExtension<FSpaceWarpViewExtension>(this);

		UE::StereoRenderUtils::FStereoShaderAspects Aspects(GMaxRHIShaderPlatform);
		bIsMobileMultiViewEnabled = Aspects.IsMobileMultiViewEnabled();
	}

	void FSpaceWarpExtensionPlugin::OnBeginRendering_RenderThread(XrSession InSession)
	{
		check(IsInRenderingThread());

		if (!PipelinedVelocityState_RenderThread.bEnabled)
		{
			return;
		}

		SCOPED_NAMED_EVENT(FSpaceWarpExtensionPlugin_OnBeginRendering_RenderThread, FColor::Red);

		const FXRSwapChainPtr& VelocitySwapchain = PipelinedVelocityState_RenderThread.VelocitySwapchain;
		const FXRSwapChainPtr& VelocityDepthSwapchain = PipelinedVelocityState_RenderThread.VelocityDepthSwapchain;
		if (VelocitySwapchain)
		{
			VelocitySwapchain->IncrementSwapChainIndex_RHIThread();
			if (VelocityDepthSwapchain)
			{
				VelocityDepthSwapchain->IncrementSwapChainIndex_RHIThread();
			}
		}
	}

	const void* FSpaceWarpExtensionPlugin::OnBeginProjectionView(XrSession InSession, int32 InLayerIndex, int32 InViewIndex, const void* InNext)
	{
		check(IsInRenderingThread());

		IXRTrackingSystem* XRTrackingSystem = GetOpenXRTrackingSystem();
		if (!XRTrackingSystem || !PipelinedVelocityState_RenderThread.bEnabled)
		{
			return InNext;
		}

		SCOPED_NAMED_EVENT(FSpaceWarpExtensionPlugin_OnBeginProjectionView, FColor::Red);

		TSharedPtr<XrCompositionLayerSpaceWarpInfoFB> SpaceWarpLayerInfo = MakeShared<XrCompositionLayerSpaceWarpInfoFB>();

		FTransform TrackingToWorld = XRTrackingSystem->GetTrackingToWorldTransform();
		FTransform TrackingSpaceDeltaPose = TrackingToWorld * LastTrackingToWorld.Inverse();
		LastTrackingToWorld = TrackingToWorld;
		FTransform BaseTransform = FTransform(XRTrackingSystem->GetBaseOrientation(), XRTrackingSystem->GetBasePosition());
		TrackingSpaceDeltaPose = BaseTransform.Inverse() * TrackingSpaceDeltaPose * BaseTransform;

		SpaceWarpLayerInfo->type = XR_TYPE_COMPOSITION_LAYER_SPACE_WARP_INFO_FB;
		SpaceWarpLayerInfo->next = InNext;
		SpaceWarpLayerInfo->layerFlags = 0;
		SpaceWarpLayerInfo->appSpaceDeltaPose = ToXrPose(TrackingSpaceDeltaPose);
		SpaceWarpLayerInfo->farZ = GNearClippingPlane / 100.f;
		SpaceWarpLayerInfo->nearZ = INFINITY;
		SpaceWarpLayerInfo->minDepth = 0.0f;
		SpaceWarpLayerInfo->maxDepth = 1.0f;

		FIntPoint TextureSize;
		if (GetRecommendedVelocityTextureSize_RenderThread(TextureSize) && PipelinedVelocityState_RenderThread.VelocitySwapchain.IsValid())
		{
			FIntPoint TextureOffset(0, 0);
			FIntRect ImageRect(TextureOffset, TextureOffset + TextureSize);

			TSharedPtr<XrSwapchainSubImage> VelocityImage = MakeShared<XrSwapchainSubImage>();
			VelocityImage->swapchain = static_cast<FOpenXRSwapchain*>(PipelinedVelocityState_RenderThread.VelocitySwapchain.Get())->GetHandle();
			VelocityImage->imageArrayIndex = (bIsMobileMultiViewEnabled && InViewIndex < 2) ? InViewIndex : 0;
			VelocityImage->imageRect = ToXrRect(ImageRect);
			SpaceWarpLayerInfo->motionVectorSubImage = *VelocityImage;

			TSharedPtr<XrSwapchainSubImage> VelocityDepthImage = nullptr;
			if (PipelinedVelocityState_RenderThread.VelocityDepthSwapchain.IsValid())
			{
				VelocityDepthImage = MakeShared<XrSwapchainSubImage>();
				VelocityDepthImage->swapchain = static_cast<FOpenXRSwapchain*>(PipelinedVelocityState_RenderThread.VelocityDepthSwapchain.Get())->GetHandle();
				VelocityDepthImage->imageArrayIndex = (bIsMobileMultiViewEnabled && InViewIndex < 2) ? InViewIndex : 0;
				VelocityDepthImage->imageRect = ToXrRect(ImageRect);
				SpaceWarpLayerInfo->depthSubImage = *VelocityDepthImage;
			}

			// The SpaceWarpInfo that we return from here gets used when the layer is submitted on xrEndFrame on the RHI Thread,
			// so we need to keep the XrSwapchainSubImage objects on the RHI Thread until PostEndFrame_RHIThread()
			GetImmediateCommandList_ForRenderCommand().EnqueueLambda([this, SpaceWarpLayerInfo, VelocityImage, VelocityDepthImage, InViewIndex](FRHICommandList& RHICmdList) {
				if (SpaceWarpLayerInfo_RHIThread.IsValidIndex(InViewIndex))
				{
					SpaceWarpLayerInfo_RHIThread[InViewIndex] = SpaceWarpLayerInfo;
				}

				if (VelocityImages_RHIThread.IsValidIndex(InViewIndex) && VelocityImage.IsValid())
				{
					VelocityImages_RHIThread[InViewIndex] = VelocityImage;
				}

				if (VelocityDepthImages_RHIThread.IsValidIndex(InViewIndex) && VelocityDepthImage.IsValid())
				{
					VelocityDepthImages_RHIThread[InViewIndex] = VelocityDepthImage;
				}
			});

			return SpaceWarpLayerInfo.Get();
		}
		return InNext;
	}

	void FSpaceWarpExtensionPlugin::PostBeginFrame_RHIThread(XrTime PredictedDisplayTime)
	{
		check(IsRunningRHIInSeparateThread() ? IsInRHIThread() : IsInRenderingThread());

		if (!PipelinedVelocityState_RHIThread.bEnabled)
		{
			return;
		}

		SCOPED_NAMED_EVENT(FSpaceWarpExtensionPlugin_PostBeginFrame_RHIThread, FColor::Red);

		// We need a new swapchain image unless we've already acquired one for rendering
		if (PipelinedVelocityState_RHIThread.VelocitySwapchain.IsValid())
		{
			PipelinedVelocityState_RHIThread.VelocitySwapchain->WaitCurrentImage_RHIThread(VELOCITY_SWAPCHAIN_WAIT_TIMEOUT);
			if (PipelinedVelocityState_RHIThread.VelocityDepthSwapchain.IsValid())
			{
				PipelinedVelocityState_RHIThread.VelocityDepthSwapchain->WaitCurrentImage_RHIThread(VELOCITY_SWAPCHAIN_WAIT_TIMEOUT);
			}
		}
	}

	const void* FSpaceWarpExtensionPlugin::OnEndFrame(XrSession InSession, XrTime DisplayTime, const void* InNext)
	{
		check(IsRunningRHIInSeparateThread() ? IsInRHIThread() : IsInRenderingThread());

		if (!PipelinedVelocityState_RHIThread.bEnabled)
		{
			return InNext;
		}

		SCOPED_NAMED_EVENT(FSpaceWarpExtensionPlugin_OnEndFrame, FColor::Red);

		if (PipelinedVelocityState_RHIThread.VelocitySwapchain.IsValid())
		{
			PipelinedVelocityState_RHIThread.VelocitySwapchain->ReleaseCurrentImage_RHIThread(nullptr);
			if (PipelinedVelocityState_RHIThread.VelocityDepthSwapchain.IsValid())
			{
				PipelinedVelocityState_RHIThread.VelocityDepthSwapchain->ReleaseCurrentImage_RHIThread(nullptr);
			}
		}

		return InNext;
	}

	void FSpaceWarpExtensionPlugin::PostEndFrame_RHIThread()
	{
		SpaceWarpLayerInfo_RHIThread.Reset();
		VelocityImages_RHIThread.Reset();
		VelocityDepthImages_RHIThread.Reset();
	}

	void FSpaceWarpExtensionPlugin::AllocateRenderTargetTextures_RenderThread()
	{
		check(IsInRenderingThread());

		if (!bSpaceWarpExtensionEnabled)
		{
			return;
		}

		SCOPED_NAMED_EVENT(FSpaceWarpExtensionPlugin_AllocateRenderTargetTextures_RenderThread, FColor::Red);

		FIntPoint VelocitySize;
		if (GetRecommendedVelocityTextureSize_RenderThread(VelocitySize))
		{
			IOpenXRHMD* OpenXRHMD = nullptr;
			if (GEngine && GEngine->XRSystem.IsValid())
			{
				OpenXRHMD = GEngine->XRSystem.Get()->GetIOpenXRHMD();
				if (OpenXRHMD)
				{
					uint8 UnusedActualFormat = 0;
					const FOpenXRSwapchainProperties VelocitySwapchainProperties = {
						TEXT("VelocitySwapchain"),
						PF_FloatRGBA,
						static_cast<uint32>(VelocitySize.X),
						static_cast<uint32>(VelocitySize.Y),
						static_cast<uint32>(bIsMobileMultiViewEnabled ? 2 : 1),
						1,
						1,
#if defined(WITH_OCULUS_BRANCH) || defined(WITH_OPENXR_BRANCH)
						(bIsMobileMultiViewEnabled) ? ETextureDimension::Texture2DArray : ETextureDimension::Texture2D,
#endif // defined(WITH_OCULUS_BRANCH)
						TexCreate_RenderTargetable | TexCreate_ResolveTargetable | TexCreate_ShaderResource | TexCreate_InputAttachmentRead | TexCreate_Dynamic,
						FClearValueBinding::Transparent,
						TexCreate_None
					};

					OpenXRHMD->AllocateSwapchainTextures_RenderThread(
						VelocitySwapchainProperties,
						PipelinedVelocityState_RenderThread.VelocitySwapchain,
						UnusedActualFormat);

					const FOpenXRSwapchainProperties VelocityDepthSwapchainProperties = {
						TEXT("VelocityDepthSwapchain"),
						PF_DepthStencil,
						static_cast<uint32>(VelocitySize.X),
						static_cast<uint32>(VelocitySize.Y),
						static_cast<uint32>(bIsMobileMultiViewEnabled ? 2 : 1),
						1,
						1,
#if defined(WITH_OCULUS_BRANCH) || defined(WITH_OPENXR_BRANCH)
						(bIsMobileMultiViewEnabled) ? ETextureDimension::Texture2DArray : ETextureDimension::Texture2D,
#endif // defined(WITH_OCULUS_BRANCH)
						TexCreate_DepthStencilTargetable | TexCreate_ShaderResource | TexCreate_InputAttachmentRead | TexCreate_Dynamic,
						FClearValueBinding::DepthZero,
						TexCreate_None
					};
					OpenXRHMD->AllocateSwapchainTextures_RenderThread(
						VelocityDepthSwapchainProperties,
						PipelinedVelocityState_RenderThread.VelocityDepthSwapchain,
						UnusedActualFormat);
				}
			}
		}
	}

	bool FSpaceWarpExtensionPlugin::GetRecommendedVelocityTextureSize_RenderThread(FIntPoint& OutTextureSize)
	{
		check(IsInRenderingThread());

		if (!bSpaceWarpExtensionEnabled)
		{
			return false;
		}

		OutTextureSize = FIntPoint(SpaceWarpSystemProperties.recommendedMotionVectorImageRectWidth, SpaceWarpSystemProperties.recommendedMotionVectorImageRectHeight);

		return true;
	}

	FXRSwapChainPtr FSpaceWarpExtensionPlugin::GetVelocitySwapchain_RenderThread()
	{
		check(IsInRenderingThread());

		if (!PipelinedVelocityState_RenderThread.bEnabled)
		{
			return nullptr;
		}

		return PipelinedVelocityState_RenderThread.VelocitySwapchain;
	}

	FXRSwapChainPtr FSpaceWarpExtensionPlugin::GetVelocityDepthSwapchain_RenderThread()
	{
		check(IsInRenderingThread());

		if (!PipelinedVelocityState_RenderThread.bEnabled)
		{
			return nullptr;
		}

		return PipelinedVelocityState_RenderThread.VelocityDepthSwapchain;
	}

	bool FSpaceWarpExtensionPlugin::IsSpaceWarpEnabled() const
	{
		if (bSpaceWarpExtensionEnabled)
		{
			static const auto CVarOculusEnableSpaceWarp = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Mobile.Oculus.SpaceWarp.Enable"));
			return CVarOculusEnableSpaceWarp && (CVarOculusEnableSpaceWarp->GetValueOnAnyThread() != 0);
		}
		return false;
	}

	FSpaceWarpExtensionPlugin::FSpaceWarpViewExtension::FSpaceWarpViewExtension(const FAutoRegister& AutoRegister, FSpaceWarpExtensionPlugin* InPlugin)
		: FHMDSceneViewExtension(AutoRegister)
		, SpaceWarpExtensionPlugin(InPlugin)
	{
		check(SpaceWarpExtensionPlugin);
	}

	void FSpaceWarpExtensionPlugin::FSpaceWarpViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
	{
		check(IsInGameThread())
			InViewFamily.bRenderStereoVelocity = SpaceWarpExtensionPlugin->IsSpaceWarpEnabled();
	}

	void FSpaceWarpExtensionPlugin::FSpaceWarpViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
	{
	}

	void FSpaceWarpExtensionPlugin::FSpaceWarpViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
	{
		bool bEnabled = SpaceWarpExtensionPlugin->IsSpaceWarpEnabled();

		ENQUEUE_RENDER_COMMAND(FSpaceWarpViewExtension_BeginRenderViewFamily)
		([this, bEnabled](FRHICommandList& RHICmdList) {
			SpaceWarpExtensionPlugin->PipelinedVelocityState_RenderThread.bEnabled = bEnabled;

			FPipelinedVelocityState VelocityState = SpaceWarpExtensionPlugin->PipelinedVelocityState_RenderThread;
			RHICmdList.EnqueueLambda([this, VelocityState, bEnabled](FRHICommandList& RHICmdList) {
				SpaceWarpExtensionPlugin->PipelinedVelocityState_RHIThread = VelocityState;

				uint32_t ViewConfigCount = 0;
				XR_ENSURE(xrEnumerateViewConfigurationViews(IOpenXRHMDModule::Get().GetInstance(), IOpenXRHMDModule::Get().GetSystemId(), SpaceWarpExtensionPlugin->SelectedViewConfigurationType, 0, &ViewConfigCount, nullptr));
				SpaceWarpExtensionPlugin->SpaceWarpLayerInfo_RHIThread.SetNum(ViewConfigCount);
				SpaceWarpExtensionPlugin->VelocityImages_RHIThread.SetNum(ViewConfigCount);
				SpaceWarpExtensionPlugin->VelocityDepthImages_RHIThread.SetNum(ViewConfigCount);
			});
		});
	}
} // namespace OculusXR

#endif // defined(WITH_OCULUS_BRANCH)
