// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <functional>
#include <mutex>

#include "khronos/openxr/openxr.h"
#include "IOpenXRExtensionPlugin.h"
#include "OculusXRHMDTypes.h"
#include "Shader.h"
#include "Misc/EngineVersionComparison.h"

namespace OculusXR
{
	/// Texture handle which can be cast to GLuint, VkImage, ID3D11Texture2D*, or ID3D12Resource*
	using TextureHandle = unsigned long long;

	constexpr uint32 EYE_COUNT = 2;

	void RenderHardOcclusions_RenderThread(IRendererModule* RendererModule, const FVector2f& DepthFactors,
		const FMatrix44f ScreenToDepthMatrices[EYE_COUNT], FRHITexture* DepthTexture, FRHICommandList& RHICmdList, const FSceneView& InView);
	void RenderEnvironmentDepthMinMaxTexture_RenderThread(IRendererModule* RendererModule, FTextureRHIRef EnvironmentDepthMinMaxTexture,
		FTextureRHIRef EnvironmentDepthSwapchain, FRHICommandListImmediate& RHICmdList);

#ifdef WITH_OCULUS_BRANCH

	extern PFN_xrCreateEnvironmentDepthProviderMETA xrCreateEnvironmentDepthProviderMETA;
	extern PFN_xrDestroyEnvironmentDepthProviderMETA xrDestroyEnvironmentDepthProviderMETA;
	extern PFN_xrStartEnvironmentDepthProviderMETA xrStartEnvironmentDepthProviderMETA;
	extern PFN_xrStopEnvironmentDepthProviderMETA xrStopEnvironmentDepthProviderMETA;
	extern PFN_xrCreateEnvironmentDepthSwapchainMETA xrCreateEnvironmentDepthSwapchainMETA;
	extern PFN_xrDestroyEnvironmentDepthSwapchainMETA xrDestroyEnvironmentDepthSwapchainMETA;
	extern PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA xrEnumerateEnvironmentDepthSwapchainImagesMETA;
	extern PFN_xrGetEnvironmentDepthSwapchainStateMETA xrGetEnvironmentDepthSwapchainStateMETA;
	extern PFN_xrAcquireEnvironmentDepthImageMETA xrAcquireEnvironmentDepthImageMETA;
	extern PFN_xrSetEnvironmentDepthHandRemovalMETA xrSetEnvironmentDepthHandRemovalMETA;

	class FEnvironmentDepthExtensionPlugin : public IOpenXRExtensionPlugin
	{
	public:
		void RegisterOpenXRExtensionPlugin()
		{
#if defined(WITH_OCULUS_BRANCH)
			RegisterOpenXRExtensionModularFeature();
#endif
		}

		virtual void PostCreateSession(XrSession InSession) override;
		virtual void BindExtensionPluginDelegates(class IOpenXRExtensionPluginDelegates& OpenXRHMD) override;
		virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		virtual const void* OnCreateInstance(class IOpenXRHMDModule* InModule, const void* InNext) override;
		virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		virtual void OnBeginRendering_GameThread(XrSession InSession) override;
		virtual void OnBeginRenderingLate_RenderThread(XrSession InSession, FRHICommandListImmediate& RHICmdList) override;
		virtual void PostBeginFrame_RHIThread(XrTime PredictedDisplayTime) override;
		virtual void PostRenderBasePassMobile_RenderThread(FRHICommandList& RHICmdList, FSceneView& InView) override;
		virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
			const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures) override;
		virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;
		virtual bool OnEndGameFrame(FWorldContext& WorldContext) override;
		virtual void OnDestroySession(XrSession InSession) override;
		virtual const void* OnLocateViews(XrSession InSession, XrTime InDisplayTime, XrViewConfigurationType ViewConfigurationType, const void* InNext) override;
#ifdef WITH_OCULUS_BRANCH
		virtual bool FindEnvironmentDepthTexture_RenderThread(FTextureRHIRef& OutTexture, FTextureRHIRef& OutMinMaxTexture,
			FVector2f& OutDepthFactors, FMatrix44f OutScreenToDepthMatrices[2], FMatrix44f OutDepthViewProjMatrices[2]) override;
#endif // WITH_OCULUS_BRANCH
		bool StartEnvironmentDepth();
		bool StopEnvironmentDepth();
		void SetXROcclusionsMode(UObject* WorldContextObject, EOculusXROcclusionsMode Mode);
		bool IsEnvironmentDepthStarted() const;

		bool InitializeEnvironmentDepth_RenderThread();
		bool DestroyEnvironmentDepth_RenderThread();
		bool GetEnvironmentDepthTextureStageCount_RenderThread(int& OutStageCount);
		bool GetEnvironmentDepthTexture_RenderThread(int Stage, TextureHandle& OutHandle);
		bool SetEnvironmentDepthHandRemoval_RenderThread(bool Enabled);
		bool StartEnvironmentDepth_RenderThread();
		bool StopEnvironmentDepth_RenderThread();
		bool AcquireEnvironmentDepthTexture_RHIThread(XrTime predictedDisplayTime);
		bool GetEnvironmentDepthFrameDesc_RenderThread(TOptional<XrEnvironmentDepthImageMETA>& OutEnvironmentDepthFrameDesc);

	private:
		XrSession Session = XR_NULL_HANDLE;
		XrSpace StageSpace = XR_NULL_HANDLE;
		XrSpace StageSpace_RHIThread = XR_NULL_HANDLE;

		XrEnvironmentDepthProviderMETA EnvironmentDepthProviderMeta = XR_NULL_HANDLE;
		XrEnvironmentDepthSwapchainMETA EnvironmentDepthSwapchainMeta = XR_NULL_HANDLE;
		XrEnvironmentDepthSwapchainStateMETA EnvironmentDepthSwapchainStateMeta{ XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_STATE_META };

		std::mutex EnvironmentDepthTextureMutex;

		bool bExtEnvironmentDepthAvailable = false;
		bool bHandsRemovalSupported = false;
		bool bEnvironmentDepthRunning = false;

		std::function<FTextureRHIRef(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, const FClearValueBinding& InBinding, uint32 InNumMips,
			uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType, TextureHandle InTexture,
			ETextureCreateFlags InTexCreateFlags)>
			CreateTexture_RenderThread_Fn;

		TArray<FTextureRHIRef> EnvironmentDepthSwapchain;
		FTextureRHIRef EnvironmentDepthMinMaxTexture;
		int PrevEnvironmentDepthMinMaxSwapchainIndex = -1;

		TArray<TextureHandle> EnvironmentDepthTextures;
		TOptional<XrEnvironmentDepthImageMETA> EnvironmentDepthFrameDesc;

		std::atomic<bool> bHardOcclusionsEnabled = false;
		std::atomic<bool> bSoftOcclusionsEnabled = false;
		std::atomic<bool> bEnvironmentDepthHandRemovalEnabled = false;

		FString RHIString;
		IRendererModule* RendererModule = nullptr;
		float WorldToMeters;
		float WorldToMeters_RenderThread;
		FTransform TrackingToWorld;
		FTransform TrackingToWorld_RenderThread;
		FQuat HeadOrientation;
		FQuat HeadOrientation_RenderThread;
		FQuat BaseOrientation;
		FQuat BaseOrientation_RenderThread;

		TArray<XrView> EyeViews;
		TArray<XrView> EyeViews_RenderThread;

		XrStructureType GetEnvironmentDepthSwapchainImageType() const;
		bool ComputeEnvironmentDepthParameters_RenderThread(FVector2f& DepthFactors, FMatrix44f ScreenToDepth[EYE_COUNT],
			FMatrix44f DepthViewProj[EYE_COUNT], int& SwapchainIndex);
		TArray<FTextureRHIRef> CreateSwapChainTextures_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat,
			const FClearValueBinding& InBinding, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType,
			const TArray<TextureHandle>& InTextures, ETextureCreateFlags InTexCreateFlags, const TCHAR* DebugName) const;
		void PrepareAndRenderHardOcclusions_RenderThread(FRHICommandList& RHICmdList, FSceneView& InView);
	};
#endif
} // namespace OculusXR
