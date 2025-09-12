// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXREnvironmentDepthExtensionPlugin.h"

#include "IOpenXRHMDModule.h"
#include "OculusXRHMD_DynamicResolutionState.h"
#include "OpenXR/OculusXROpenXRUtilities.h"
#include "OpenXRPlatformRHI.h"

#include <vulkan/vulkan.h>
#include "khronos/openxr/openxr_platform.h"

#if PLATFORM_ANDROID
#include "AndroidPermissionCallbackProxy.h"
#include "AndroidPermissionFunctionLibrary.h"
#endif
#include "DataDrivenShaderPlatformInfo.h"
#include "OculusXRHMD.h"
#include "OculusXRHMDModule.h"
#include "OpenXRHMD.h"
#include "HardwareInfo.h"
#include "ScreenRendering.h"
#include "XRThreadUtils.h"
#include "ScreenPass.h"
#include "RenderResource.h"
#include "Shader.h"

namespace
{
#ifdef WITH_OCULUS_BRANCH
#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
	FTextureRHIRef CreateTextureVulkan_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, const FClearValueBinding& InBinding,
		uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType, OculusXR::TextureHandle InTexture,
		ETextureCreateFlags InTexCreateFlags)
	{
		OculusXRHMD::CheckInRenderThread();

		IVulkanDynamicRHI* VulkanRHI = GetIVulkanDynamicRHI();
		constexpr VkImageSubresourceRange SubresourceRangeAll = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS };

		if (EnumHasAnyFlags(InTexCreateFlags, TexCreate_RenderTargetable))
		{
			VulkanRHI->RHISetImageLayout(reinterpret_cast<VkImage>(InTexture), VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, SubresourceRangeAll);
		}
		else if (EnumHasAnyFlags(InTexCreateFlags, TexCreate_Foveation))
		{
			VulkanRHI->RHISetImageLayout(reinterpret_cast<VkImage>(InTexture), VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT, SubresourceRangeAll);
		}

		switch (InResourceType)
		{
			case RRT_Texture2D:
				return VulkanRHI->RHICreateTexture2DFromResource(InFormat, InSizeX, InSizeY, InNumMips, InNumSamples,
									reinterpret_cast<VkImage>(InTexture), InTexCreateFlags, InBinding)
					.GetReference();

			case RRT_Texture2DArray:
				return VulkanRHI->RHICreateTexture2DArrayFromResource(InFormat, InSizeX, InSizeY, 2, InNumMips, InNumSamples,
									reinterpret_cast<VkImage>(InTexture), InTexCreateFlags, InBinding)
					.GetReference();

			case RRT_TextureCube:
				return VulkanRHI->RHICreateTextureCubeFromResource(InFormat, InSizeX, false, 1, InNumMips,
									reinterpret_cast<VkImage>(InTexture), InTexCreateFlags, InBinding)
					.GetReference();

			default:
				return nullptr;
		}
	}
#endif

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
	FTextureRHIRef CreateTextureD3D11_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, const FClearValueBinding& InBinding,
		uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType, OculusXR::TextureHandle InTexture,
		ETextureCreateFlags InTexCreateFlags)
	{
		OculusXRHMD::CheckInRenderThread();

		switch (InResourceType)
		{
			case RRT_Texture2D:
				return GetID3D11DynamicRHI()->RHICreateTexture2DFromResource(InFormat, InTexCreateFlags, InBinding,
												reinterpret_cast<ID3D11Texture2D*>(InTexture))
					.GetReference();

			case RRT_Texture2DArray:
				return GetID3D11DynamicRHI()->RHICreateTexture2DArrayFromResource(InFormat, InTexCreateFlags, InBinding,
												reinterpret_cast<ID3D11Texture2D*>(InTexture))
					.GetReference();

			case RRT_TextureCube:
				return GetID3D11DynamicRHI()->RHICreateTextureCubeFromResource(InFormat, InTexCreateFlags | TexCreate_TargetArraySlicesIndependently,
												InBinding, reinterpret_cast<ID3D11Texture2D*>(InTexture))
					.GetReference();

			default:
				return nullptr;
		}
	}

#endif

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
	FTextureRHIRef CreateTextureD3D12_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, const FClearValueBinding& InBinding,
		uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType, OculusXR::TextureHandle InTexture,
		ETextureCreateFlags InTexCreateFlags)
	{
		OculusXRHMD::CheckInRenderThread();

		ID3D12DynamicRHI* DynamicRHI = GetID3D12DynamicRHI();

		switch (InResourceType)
		{
			case RRT_Texture2D:
				return DynamicRHI->RHICreateTexture2DFromResource(InFormat, InTexCreateFlags, InBinding,
									 reinterpret_cast<ID3D12Resource*>(InTexture))
					.GetReference();

			case RRT_Texture2DArray:
				return DynamicRHI->RHICreateTexture2DArrayFromResource(InFormat, InTexCreateFlags, InBinding,
									 reinterpret_cast<ID3D12Resource*>(InTexture))
					.GetReference();

			case RRT_TextureCube:
				return DynamicRHI->RHICreateTextureCubeFromResource(InFormat, InTexCreateFlags, InBinding,
									 reinterpret_cast<ID3D12Resource*>(InTexture))
					.GetReference();

			default:
				return nullptr;
		}
	}
#endif

	void InitOpenXRFunctions(XrInstance InInstance)
	{
		using namespace OculusXR;

		XRGetInstanceProcAddr(InInstance, "xrCreateEnvironmentDepthProviderMETA", &xrCreateEnvironmentDepthProviderMETA);
		XRGetInstanceProcAddr(InInstance, "xrDestroyEnvironmentDepthProviderMETA", &xrDestroyEnvironmentDepthProviderMETA);
		XRGetInstanceProcAddr(InInstance, "xrStartEnvironmentDepthProviderMETA", &xrStartEnvironmentDepthProviderMETA);
		XRGetInstanceProcAddr(InInstance, "xrStopEnvironmentDepthProviderMETA", &xrStopEnvironmentDepthProviderMETA);
		XRGetInstanceProcAddr(InInstance, "xrCreateEnvironmentDepthSwapchainMETA", &xrCreateEnvironmentDepthSwapchainMETA);
		XRGetInstanceProcAddr(InInstance, "xrDestroyEnvironmentDepthSwapchainMETA", &xrDestroyEnvironmentDepthSwapchainMETA);
		XRGetInstanceProcAddr(InInstance, "xrEnumerateEnvironmentDepthSwapchainImagesMETA", &xrEnumerateEnvironmentDepthSwapchainImagesMETA);
		XRGetInstanceProcAddr(InInstance, "xrGetEnvironmentDepthSwapchainStateMETA", &xrGetEnvironmentDepthSwapchainStateMETA);
		XRGetInstanceProcAddr(InInstance, "xrAcquireEnvironmentDepthImageMETA", &xrAcquireEnvironmentDepthImageMETA);
		XRGetInstanceProcAddr(InInstance, "xrSetEnvironmentDepthHandRemovalMETA", &xrSetEnvironmentDepthHandRemovalMETA);
	}

	FMatrix MakeProjection(float ZNear, float ZFar, float UpTan, float DownTan, float LeftTan, float RightTan, bool leftHanded)
	{
		float handednessScale = leftHanded ? 1.0f : -1.0f;

		// A projection matrix is very like a scaling from NDC, so we can start with that.
		float projXScale = 2.0f / (LeftTan + RightTan);
		float projXOffset = (LeftTan - RightTan) * projXScale * 0.5f;
		float projYScale = 2.0f / (UpTan + DownTan);
		float projYOffset = (UpTan - DownTan) * projYScale * 0.5f;

		FMatrix projection;

		// Produces X result, mapping clip edges to [-w,+w]
		projection.M[0][0] = projXScale;
		projection.M[0][1] = 0.0f;
		projection.M[0][2] = handednessScale * projXOffset;
		projection.M[0][3] = 0.0f;

		// Produces Y result, mapping clip edges to [-w,+w]
		// Hey - why is that YOffset negated?
		// It's because a projection matrix transforms from world coords with Y=up,
		// whereas this is derived from an NDC scaling, which is Y=down.
		projection.M[1][0] = 0.0f;
		projection.M[1][1] = projYScale;
		projection.M[1][2] = handednessScale * -projYOffset;
		projection.M[1][3] = 0.0f;

		// Produces Z-buffer result
		projection.M[2][0] = 0.0f;
		projection.M[2][1] = 0.0f;
		if (FGenericPlatformMath::IsFinite(ZFar))
		{
			projection.M[2][2] = -handednessScale * ZFar / (ZNear - ZFar);
			projection.M[2][3] = (ZFar * ZNear) / (ZNear - ZFar);
		}
		else
		{
			projection.M[2][2] = handednessScale;
			projection.M[2][3] = -ZNear;
		}

		// Produces W result (= Z in)
		projection.M[3][0] = 0.0f;
		projection.M[3][1] = 0.0f;
		projection.M[3][2] = handednessScale;
		projection.M[3][3] = 0.0f;

		return projection.GetTransposed();
	}

	FMatrix44f MakeUnprojectionMatrix(float UpTan, float DownTan, float LeftTan, float RightTan)
	{
		FMatrix44f Matrix = FMatrix44f::Identity;

		// Scale
		Matrix.M[0][0] = RightTan + LeftTan;
		Matrix.M[1][1] = UpTan + DownTan;

		// Offset
		Matrix.M[0][3] = -LeftTan;
		Matrix.M[1][3] = -DownTan;
		Matrix.M[2][3] = 1.0;

		return Matrix;
	}

	FMatrix44f MakeProjectionMatrix(float UpTan, float DownTan, float LeftTan, float RightTan)
	{
		const float tanAngleWidth = RightTan + LeftTan;
		const float tanAngleHeight = UpTan + DownTan;

		FMatrix44f Matrix = FMatrix44f::Identity;

		// Scale
		Matrix.M[0][0] = 1.0f / tanAngleWidth;
		Matrix.M[1][1] = 1.0f / tanAngleHeight;

		// Offset
		Matrix.M[0][3] = LeftTan / tanAngleWidth;
		Matrix.M[1][3] = DownTan / tanAngleHeight;
		Matrix.M[2][3] = -1.0f;

		return Matrix;
	}

#endif
#if !UE_VERSION_OLDER_THAN(5, 3, 0)
	BEGIN_SHADER_PARAMETER_STRUCT(FDrawRectangleParameters, )
	SHADER_PARAMETER(FVector4f, PosScaleBias)
	SHADER_PARAMETER(FVector4f, UVScaleBias)
	SHADER_PARAMETER(FVector4f, InvTargetSizeAndTextureSize)
	END_SHADER_PARAMETER_STRUCT()
#endif

	void DrawHmdViewMesh(IRendererModule* RendererModule, FRHICommandList& RHICmdList, float X, float Y, float SizeX, float SizeY, float U,
		float V, float SizeU, float SizeV, FIntPoint TargetSize, FIntPoint TextureSize, int32 StereoView, const TShaderRef<FShader>& VertexShader)
	{
		FDrawRectangleParameters Parameters;
		Parameters.PosScaleBias = FVector4f(SizeX, SizeY, X, Y);
		Parameters.UVScaleBias = FVector4f(SizeU, SizeV, U, V);

		Parameters.InvTargetSizeAndTextureSize = FVector4f(
			1.0f / TargetSize.X, 1.0f / TargetSize.Y,
			1.0f / TextureSize.X, 1.0f / TextureSize.Y);

#if UE_VERSION_OLDER_THAN(5, 3, 0)
		SetUniformBufferParameterImmediate(RHICmdList, VertexShader.GetVertexShader(),
			VertexShader->GetUniformBufferParameter<FDrawRectangleParameters>(), Parameters);
#else
		FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
		SetUniformBufferParameterImmediate(BatchedParameters, VertexShader->GetUniformBufferParameter<FDrawRectangleParameters>(), Parameters);
		RHICmdList.SetBatchedShaderParameters(VertexShader.GetVertexShader(), BatchedParameters);
#endif
		RendererModule->DrawRectangle(
			RHICmdList,
			X, Y,
			SizeX, SizeY,
			0, 0,
			TextureSize.X, TextureSize.Y,
			TargetSize,
			TextureSize,
			VertexShader);
	}

} // namespace

namespace OculusXR
{
	void RenderHardOcclusions_RenderThread(IRendererModule* RendererModule, const FVector2f& DepthFactors, const FMatrix44f ScreenToDepthMatrices[EYE_COUNT],
		FRHITexture* DepthTexture, FRHICommandList& RHICmdList, const FSceneView& InView)
	{
		checkSlow(RHICmdList.IsInsideRenderPass());

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<>::GetRHI();

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.FeatureLevel);
		TShaderMapRef<FScreenPassVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<OculusXRHMD::FHardOcclusionsPS> PixelShader(GlobalShaderMap);

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

		FRHISamplerState* DepthSampler = TStaticSamplerState<>::GetRHI();

		FIntPoint TextureSize = DepthTexture->GetDesc().Extent;
		FIntRect ScreenRect = InView.UnscaledViewRect;

#if UE_VERSION_OLDER_THAN(5, 3, 0)
		PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), DepthSampler, DepthTexture, DepthFactors,
			ScreenToDepthMatrices, InView.StereoViewIndex);
#else
		FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
		PixelShader->SetParameters(BatchedParameters, DepthSampler, DepthTexture, DepthFactors, ScreenToDepthMatrices, InView.StereoViewIndex);
		RHICmdList.SetBatchedShaderParameters(PixelShader.GetPixelShader(), BatchedParameters);
#endif

		const auto FeatureLevel = GEngine ? GEngine->GetDefaultWorldFeatureLevel() : GMaxRHIFeatureLevel;
		EShaderPlatform CurrentShaderPlatform = GShaderPlatformForFeatureLevel[FeatureLevel];
		check(CurrentShaderPlatform == InView.Family->Scene->GetShaderPlatform());
		if (!IsMobilePlatform(CurrentShaderPlatform) && InView.StereoViewIndex != INDEX_NONE)
		{
			SCOPED_DRAW_EVENTF(RHICmdList, RenderHardOcclusions_RenderThread, TEXT("View %d"), InView.StereoViewIndex);

			int32 width = ScreenRect.Width() / 2;
			int32 height = ScreenRect.Height();
			int32 x = InView.StereoViewIndex == EStereoscopicEye::eSSE_LEFT_EYE ? 0 : width;
			int32 y = 0;

			DrawHmdViewMesh(
				RendererModule,
				RHICmdList,
				x, y,
				width, height,
				0, 0,
				TextureSize.X, TextureSize.Y,
				FIntPoint(ScreenRect.Width(), ScreenRect.Height()),
				TextureSize,
				InView.StereoViewIndex,
				VertexShader);
		}
		else
		{
			SCOPED_DRAW_EVENT(RHICmdList, RenderHardOcclusions_RenderThread);

			RendererModule->DrawRectangle(
				RHICmdList,
				0, 0,
				ScreenRect.Width(), ScreenRect.Height(),
				0, 0,
				TextureSize.X, TextureSize.Y,
				FIntPoint(ScreenRect.Width(), ScreenRect.Height()),
				TextureSize,
				VertexShader);
		}
	}

	void RenderEnvironmentDepthMinMaxTexture_RenderThread(IRendererModule* RendererModule, FTextureRHIRef EnvironmentDepthMinMaxTexture,
		FTextureRHIRef EnvironmentDepthSwapchain, FRHICommandListImmediate& RHICmdList)
	{
		FRHIRenderPassInfo RPInfo(EnvironmentDepthMinMaxTexture, ERenderTargetActions::DontLoad_Store);
		int32 SliceCount = EnvironmentDepthMinMaxTexture->GetDesc().ArraySize;
#if PLATFORM_ANDROID
		bool bEnableMultiView = GSupportsMobileMultiView;
#else
		bool bEnableMultiView = false;
#endif
		if (bEnableMultiView)
		{
			RPInfo.MultiViewCount = 2;
			SliceCount = 1;
		}
		for (int32 SliceIndex = 0; SliceIndex < SliceCount; ++SliceIndex)
		{
			if (!bEnableMultiView)
			{
				RPInfo.ColorRenderTargets[0].ArraySlice = SliceIndex;
			}
			RHICmdList.BeginRenderPass(RPInfo, TEXT("EnvironmentDepthMinMaxPrePass"));
			{
				auto SrcTexture = EnvironmentDepthSwapchain;
				auto Extent = SrcTexture->GetDesc().Extent;
				const uint32 ViewportWidth = Extent.X;
				const uint32 ViewportHeight = Extent.Y;
				const FIntPoint TargetSize(ViewportWidth, ViewportHeight);

				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
				GraphicsPSOInit.PrimitiveType = PT_TriangleList;

				const auto FeatureLevel = GEngine ? GEngine->GetDefaultWorldFeatureLevel() : GMaxRHIFeatureLevel;
				auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
				TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();

				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				FRHISamplerState* SamplerState = TStaticSamplerState<SF_Point>::GetRHI();

				FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
				if (bEnableMultiView)
				{
					GraphicsPSOInit.MultiViewCount = 2;
					TShaderMapRef<OculusXRHMD::FScreenPSEnvironmentDepthMinMax<true>> PixelShader(ShaderMap);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
					PixelShader->SetParameters(BatchedParameters, SamplerState, SrcTexture, SliceIndex);
				}
				else
				{
					TShaderMapRef<OculusXRHMD::FScreenPSEnvironmentDepthMinMax<false>> PixelShader(ShaderMap);
					GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
					PixelShader->SetParameters(BatchedParameters, SamplerState, SrcTexture, SliceIndex);
				}
				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
				RHICmdList.SetBatchedShaderParameters(RHICmdList.GetBoundPixelShader(), BatchedParameters);

#ifdef WITH_OCULUS_BRANCH
				// If GSupportsMultiViewPerViewViewports is true then we must specify a stereo viewport otherwise
				// it will lead to undefined behaviour in the right eye.
				if (GSupportsMultiViewPerViewViewports)
				{
					RHICmdList.SetStereoViewport(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, ViewportWidth,
						ViewportWidth, ViewportHeight, ViewportHeight, 1.0f);
					RHICmdList.SetStereoScissor(0.0f, 0.0f, 0.0f, 0.0f, ViewportWidth,
						ViewportWidth, ViewportHeight, ViewportHeight);
				}
				else
#endif
				{
					RHICmdList.SetViewport(0.0f, 0.0f, 0.0f, ViewportWidth, ViewportHeight, 1.0f);
				}

				RendererModule->DrawRectangle(
					RHICmdList,
					0.0f, 0.0f, ViewportWidth, ViewportHeight,
					0.0f, 0.0f, 1.0f, 1.0f,
					TargetSize,
					FIntPoint(1, 1),
					VertexShader,
					EDRF_Default);
			}
			RHICmdList.EndRenderPass();
		}
	}

#ifdef WITH_OCULUS_BRANCH
	PFN_xrCreateEnvironmentDepthProviderMETA xrCreateEnvironmentDepthProviderMETA = nullptr;
	PFN_xrDestroyEnvironmentDepthProviderMETA xrDestroyEnvironmentDepthProviderMETA = nullptr;
	PFN_xrStartEnvironmentDepthProviderMETA xrStartEnvironmentDepthProviderMETA = nullptr;
	PFN_xrStopEnvironmentDepthProviderMETA xrStopEnvironmentDepthProviderMETA = nullptr;
	PFN_xrCreateEnvironmentDepthSwapchainMETA xrCreateEnvironmentDepthSwapchainMETA = nullptr;
	PFN_xrDestroyEnvironmentDepthSwapchainMETA xrDestroyEnvironmentDepthSwapchainMETA = nullptr;
	PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA xrEnumerateEnvironmentDepthSwapchainImagesMETA = nullptr;
	PFN_xrGetEnvironmentDepthSwapchainStateMETA xrGetEnvironmentDepthSwapchainStateMETA = nullptr;
	PFN_xrAcquireEnvironmentDepthImageMETA xrAcquireEnvironmentDepthImageMETA = nullptr;
	PFN_xrSetEnvironmentDepthHandRemovalMETA xrSetEnvironmentDepthHandRemovalMETA = nullptr;

	void FEnvironmentDepthExtensionPlugin::PostCreateSession(XrSession InSession)
	{
		Session = InSession;
	}

	void FEnvironmentDepthExtensionPlugin::BindExtensionPluginDelegates(class IOpenXRExtensionPluginDelegates& OpenXRHMD)
	{
		// grab a pointer to the renderer module for displaying our mirror window
		static const FName RendererModuleName("Renderer");
		RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

		FString HardwareDetails = FHardwareInfo::GetHardwareDetailsString();
		FString RHILookup = NAME_RHI.ToString() + TEXT("=");

		if (!FParse::Value(*HardwareDetails, *RHILookup, RHIString))
		{
			UE_LOG(LogHMD, Warning, TEXT("Failed to parse RHI string. A wrong swapchain will probably be created"));
		}

#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
		if (RHIString == TEXT("Vulkan"))
		{
			CreateTexture_RenderThread_Fn = CreateTextureVulkan_RenderThread;
			return;
		}
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
		if (RHIString == TEXT("D3D11"))
		{
			CreateTexture_RenderThread_Fn = CreateTextureD3D11_RenderThread;
			return;
		}
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
		if (RHIString == TEXT("D3D12"))
		{
			CreateTexture_RenderThread_Fn = CreateTextureD3D12_RenderThread;
			return;
		}
#endif

		UE_LOG(LogHMD, Error, TEXT("Can not create textures for RHIString '%s'. This will result in a crash"), *RHIString);
		CreateTexture_RenderThread_Fn = nullptr;
	}

	bool FEnvironmentDepthExtensionPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME);
		return true;
	}

	const void* FEnvironmentDepthExtensionPlugin::OnCreateInstance(class IOpenXRHMDModule* InModule, const void* InNext)
	{
		if (InModule)
		{
			bExtEnvironmentDepthAvailable = InModule->IsExtensionEnabled(XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME);
			bHandsRemovalSupported = true;
		}
		return InNext;
	}

	const void* FEnvironmentDepthExtensionPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		::InitOpenXRFunctions(InInstance);
		return InNext;
	}

	void FEnvironmentDepthExtensionPlugin::OnBeginRendering_GameThread(XrSession InSession)
	{
		StageSpace = OculusXR::GetOpenXRTrackingSystem()->GetIOpenXRHMD()->GetTrackingSpace();
		XrSpace CurrentStageSpace = StageSpace;
		ENQUEUE_RENDER_COMMAND(TransferFrameStateToRenderingThread)
		([this, CurrentStageSpace](FRHICommandListImmediate& RHICmdList) mutable {
			EyeViews_RenderThread = EyeViews;
			WorldToMeters_RenderThread = WorldToMeters;
			TrackingToWorld_RenderThread = TrackingToWorld;
			HeadOrientation_RenderThread = HeadOrientation;
			BaseOrientation_RenderThread = BaseOrientation;

			RHICmdList.EnqueueLambda([this, CurrentStageSpace](FRHICommandListImmediate& InRHICmdList) {
				StageSpace_RHIThread = CurrentStageSpace;
			});
		});
	}

	void FEnvironmentDepthExtensionPlugin::PostBeginFrame_RHIThread(XrTime PredictedDisplayTime)
	{
		AcquireEnvironmentDepthTexture_RHIThread(PredictedDisplayTime);
	}

	void FEnvironmentDepthExtensionPlugin::OnBeginRenderingLate_RenderThread(XrSession InSession, FRHICommandListImmediate& RHICmdList)
	{
		if (bSoftOcclusionsEnabled && EnvironmentDepthMinMaxTexture != nullptr && !EnvironmentDepthSwapchain.IsEmpty())
		{
			TOptional<XrEnvironmentDepthImageMETA> DepthFrameDesc{};
			if (!GetEnvironmentDepthFrameDesc_RenderThread(DepthFrameDesc) || !DepthFrameDesc.IsSet())
			{
				return;
			}
			if (static_cast<uint32_t>(EnvironmentDepthSwapchain.Num()) <= DepthFrameDesc->swapchainIndex
				|| DepthFrameDesc->swapchainIndex == PrevEnvironmentDepthMinMaxSwapchainIndex)
			{
				return;
			}

			RenderEnvironmentDepthMinMaxTexture_RenderThread(RendererModule, EnvironmentDepthMinMaxTexture,
				EnvironmentDepthSwapchain[DepthFrameDesc->swapchainIndex], RHICmdList);

			PrevEnvironmentDepthMinMaxSwapchainIndex = DepthFrameDesc->swapchainIndex;
		}
	}

	bool FEnvironmentDepthExtensionPlugin::ComputeEnvironmentDepthParameters_RenderThread(FVector2f& DepthFactors,
		FMatrix44f ScreenToDepth[EYE_COUNT], FMatrix44f DepthViewProj[EYE_COUNT], int& SwapchainIndex)
	{
		float ScreenNearZ = GNearClippingPlane / WorldToMeters_RenderThread;

		TOptional<XrEnvironmentDepthImageMETA> DepthFrameDesc;
		if (!GetEnvironmentDepthFrameDesc_RenderThread(DepthFrameDesc) || !DepthFrameDesc.IsSet())
		{
			return false;
		}

		SwapchainIndex = DepthFrameDesc->swapchainIndex;
		const auto InverseBaseOrientation = BaseOrientation_RenderThread.Inverse();

		// Assume NearZ and FarZ are the same for left and right eyes
		const float DepthNearZ = DepthFrameDesc->nearZ;
		const float DepthFarZ = DepthFrameDesc->farZ;

		float Scale;
		float Offset;

		if (DepthFarZ < DepthNearZ || (!FGenericPlatformMath::IsFinite(DepthFarZ)))
		{
			// Inf far plane:
			Scale = DepthNearZ;
			Offset = 0.0f;
		}
		else
		{
			// Finite far plane:
			Scale = (DepthFarZ * DepthNearZ) / (DepthFarZ - DepthNearZ);
			Offset = DepthNearZ / (DepthFarZ - DepthNearZ);
		}

		DepthFactors.X = -ScreenNearZ / Scale;
		DepthFactors.Y = (Offset + 1.0f) * ScreenNearZ / Scale;

		// The pose extrapolated to the predicted display time of the current frame
		const FQuat ScreenOrientation = HeadOrientation_RenderThread;

		for (uint32 i = 0; i < EYE_COUNT; ++i)
		{
			const float DepthUpTan = tanf(DepthFrameDesc->views[i].fov.angleUp);
			const float DepthDownTan = tanf(-DepthFrameDesc->views[i].fov.angleDown);
			const float DepthLeftTan = tanf(-DepthFrameDesc->views[i].fov.angleLeft);
			const float DepthRightTan = tanf(DepthFrameDesc->views[i].fov.angleRight);

			if (DepthViewProj != nullptr)
			{
				const float ZNear = DepthFrameDesc->nearZ * WorldToMeters_RenderThread;
				const float ZFar = DepthFrameDesc->farZ * WorldToMeters_RenderThread;
				FMatrix DepthProjectionMatrix = MakeProjection(ZNear, ZFar, DepthUpTan, DepthDownTan, DepthLeftTan, DepthRightTan, true);

				auto DepthOrientation = TrackingToWorld_RenderThread.GetRotation() * InverseBaseOrientation * ToFQuat(DepthFrameDesc->views[i].pose.orientation);

				// NOTE: This matrix is the same as applied in SetupViewFrustum in SceneView.cpp
				auto ViewMatrix = DepthOrientation.Inverse().ToMatrix()
					* FMatrix(FPlane(0, 0, 1, 0), FPlane(1, 0, 0, 0),
						FPlane(0, 1, 0, 0), FPlane(0, 0, 0, 1));

				FVector EyePos{};
				if (EyeViews_RenderThread.IsValidIndex(i))
				{
					EyePos = ToFVector(EyeViews_RenderThread[i].pose.position, WorldToMeters_RenderThread);
				}

				auto DepthTranslation = ToFVector(DepthFrameDesc->views[i].pose.position, WorldToMeters_RenderThread);
				auto Delta = EyePos - DepthTranslation;

				// NOTE: The view matrix here is relative to the VR camera, this is necessary to support
				// Large Worlds and avoid rounding errors when getting very far away from the origin
				ViewMatrix = ViewMatrix.ConcatTranslation(ViewMatrix.TransformPosition(Delta));

				DepthViewProj[i] = static_cast<FMatrix44f>(ViewMatrix * DepthProjectionMatrix);
			}

			// Screen To Depth represents the transformation matrix used to map normalised screen UV coordinates to
			// normalised environment depth texture UV coordinates. This needs to account for 2 things:
			// 1. The field of view of the two textures may be different, Unreal typically renders using a symmetric fov.
			//    That is to say the FOV of the left and right eyes is the same. The environment depth on the other hand
			//    has a different FOV for the left and right eyes. So we need to scale and offset accordingly to account
			//    for this difference.

			float UpAngle, DownAngle, LeftAngle, RightAngle;
			const IHeadMountedDisplay* Hmd = GEngine->XRSystem.Get()->GetHMDDevice();
			Hmd->GetStereoFieldOfView(i, LeftAngle, RightAngle, UpAngle, DownAngle);

			const float ScreenUpTan = tanf(UpAngle);
			const float ScreenDownTan = tanf(-DownAngle);
			const float ScreenLeftTan = tanf(-LeftAngle);
			const float ScreenRightTan = tanf(RightAngle);

			auto T_ScreenCamera_ScreenNormCoord = MakeUnprojectionMatrix(ScreenUpTan, ScreenDownTan, ScreenLeftTan, ScreenRightTan);
			auto T_DepthNormCoord_DepthCamera = MakeProjectionMatrix(DepthUpTan, DepthDownTan, DepthLeftTan, DepthRightTan);

			// 2. The headset may have moved in between capturing the environment depth and rendering the frame. We
			//    can only account for rotation of the headset, not translation.
			auto DepthOrientation = InverseBaseOrientation * ToFQuat(DepthFrameDesc->views[i].pose.orientation);
			if (!DepthOrientation.IsNormalized())
			{
				UE_LOG(LogHMD, Error, TEXT("DepthOrientation is not normalized %f %f %f %f"),
					DepthOrientation.X, DepthOrientation.Y, DepthOrientation.Z, DepthOrientation.W);
				DepthOrientation.Normalize();
			}
			const auto ScreenToDepthQuat = ScreenOrientation.Inverse() * DepthOrientation;

			FMatrix44f R_DepthCamera_ScreenCamera = FQuat4f(ScreenToDepthQuat.Y, ScreenToDepthQuat.Z,
				ScreenToDepthQuat.X, ScreenToDepthQuat.W)
														.GetNormalized()
														.ToMatrix();

			ScreenToDepth[i] = T_DepthNormCoord_DepthCamera * R_DepthCamera_ScreenCamera * T_ScreenCamera_ScreenNormCoord;
		}

		return true;
	}

	void FEnvironmentDepthExtensionPlugin::PostRenderBasePassMobile_RenderThread(FRHICommandList& RHICmdList, FSceneView& InView)
	{
		if (bHardOcclusionsEnabled)
		{
			PrepareAndRenderHardOcclusions_RenderThread(RHICmdList, InView);
		}
	}

	void FEnvironmentDepthExtensionPlugin::PrepareAndRenderHardOcclusions_RenderThread(FRHICommandList& RHICmdList, FSceneView& InView)
	{
		FVector2f DepthFactors;
		FMatrix44f ScreenToDepthMatrices[EYE_COUNT];
		int SwapchainIndex;

		if (InView.bIsSceneCapture || InView.bIsReflectionCapture || InView.bIsPlanarReflection
			|| !ComputeEnvironmentDepthParameters_RenderThread(DepthFactors, ScreenToDepthMatrices, nullptr, SwapchainIndex))
		{
			return;
		}

		if (SwapchainIndex >= EnvironmentDepthSwapchain.Num())
		{
			UE_LOG(LogHMD, Error, TEXT("Depth texture swapchain index %d outside of boundaries"), SwapchainIndex);
			return;
		}

		FRHITexture* DepthTexture = EnvironmentDepthSwapchain[SwapchainIndex];

		RenderHardOcclusions_RenderThread(RendererModule, DepthFactors, ScreenToDepthMatrices, DepthTexture, RHICmdList, InView);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FPostBasePassViewExtensionParameters, )
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTextures)
	RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	void FEnvironmentDepthExtensionPlugin::PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView,
		const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
	{
		if (bHardOcclusionsEnabled)
		{
			auto* PassParameters = GraphBuilder.AllocParameters<FPostBasePassViewExtensionParameters>();
			PassParameters->RenderTargets = RenderTargets;
			PassParameters->SceneTextures = SceneTextures;

			GraphBuilder.AddPass(RDG_EVENT_NAME("RenderHardOcclusions_RenderThread"), PassParameters, ERDGPassFlags::Raster,
				[this, &InView](FRHICommandListImmediate& RHICmdList) {
					PrepareAndRenderHardOcclusions_RenderThread(RHICmdList, InView);
				});
		}
	}

	bool FEnvironmentDepthExtensionPlugin::InitializeEnvironmentDepth_RenderThread()
	{
		if (!bExtEnvironmentDepthAvailable)
		{
			return false;
		}
		if (EnvironmentDepthProviderMeta != XR_NULL_HANDLE)
		{
			return false;
		}

		constexpr XrEnvironmentDepthProviderCreateInfoMETA DepthProviderCreateInfo{ XR_TYPE_ENVIRONMENT_DEPTH_PROVIDER_CREATE_INFO_META };

		XR_ENSURE(xrCreateEnvironmentDepthProviderMETA(Session, &DepthProviderCreateInfo, &EnvironmentDepthProviderMeta));

		constexpr XrEnvironmentDepthSwapchainCreateInfoMETA DepthSwapchainCreateInfo{ XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_CREATE_INFO_META };
		XR_ENSURE(xrCreateEnvironmentDepthSwapchainMETA(EnvironmentDepthProviderMeta, &DepthSwapchainCreateInfo, &EnvironmentDepthSwapchainMeta));

		XR_ENSURE(xrGetEnvironmentDepthSwapchainStateMETA(EnvironmentDepthSwapchainMeta, &EnvironmentDepthSwapchainStateMeta));

		UE_LOG(LogHMD, Log, TEXT("Initialize env depth: swapchain size=%dx%d"),
			EnvironmentDepthSwapchainStateMeta.width, EnvironmentDepthSwapchainStateMeta.height);

		uint32 ImageCount = 0;
		XR_ENSURE(xrEnumerateEnvironmentDepthSwapchainImagesMETA(EnvironmentDepthSwapchainMeta, 0, &ImageCount, nullptr));

		TArray<XrSwapchainImageVulkanKHR> DepthSwapChainImages;
		DepthSwapChainImages.SetNum(ImageCount);
		for (uint32 i = 0; i < ImageCount; ++i)
		{
			DepthSwapChainImages[i] = { GetEnvironmentDepthSwapchainImageType() };
		}

		XR_ENSURE(xrEnumerateEnvironmentDepthSwapchainImagesMETA(EnvironmentDepthSwapchainMeta, ImageCount, &ImageCount,
			reinterpret_cast<XrSwapchainImageBaseHeader*>(DepthSwapChainImages.GetData())));

		{
			std::lock_guard EnvTexLock(EnvironmentDepthTextureMutex);
			EnvironmentDepthTextures.Empty();
			for (auto DepthSwapChainImage : DepthSwapChainImages)
			{
				EnvironmentDepthTextures.Push(reinterpret_cast<TextureHandle>(DepthSwapChainImage.image));
			}
		}

		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::DestroyEnvironmentDepth_RenderThread()
	{
		if (!bExtEnvironmentDepthAvailable)
		{
			return false;
		}

		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE)
		{
			return false;
		}

		{
			std::lock_guard EnvTexLock(EnvironmentDepthTextureMutex);
			EnvironmentDepthTextures.Empty();
		}

		XR_ENSURE(xrDestroyEnvironmentDepthSwapchainMETA(EnvironmentDepthSwapchainMeta));
		EnvironmentDepthSwapchainMeta = XR_NULL_HANDLE;

		XR_ENSURE(xrDestroyEnvironmentDepthProviderMETA(EnvironmentDepthProviderMeta));
		EnvironmentDepthProviderMeta = XR_NULL_HANDLE;

		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::GetEnvironmentDepthTextureStageCount_RenderThread(int& OutStageCount)
	{
		std::lock_guard EnvTexLock(EnvironmentDepthTextureMutex);
		OutStageCount = EnvironmentDepthTextures.Num();
		return !EnvironmentDepthTextures.IsEmpty();
	}

	bool FEnvironmentDepthExtensionPlugin::GetEnvironmentDepthTexture_RenderThread(int Stage, TextureHandle& OutHandle)
	{
		if (!bExtEnvironmentDepthAvailable)
		{
			return false;
		}

		std::lock_guard EnvTexLock(EnvironmentDepthTextureMutex);

		if (Stage >= EnvironmentDepthTextures.Num())
		{
			return false;
		}

		OutHandle = EnvironmentDepthTextures[Stage];

		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::SetEnvironmentDepthHandRemoval_RenderThread(bool Enabled)
	{
		// Save the value. If environment depth is not yet started it will enable hand removal when it starts.
		bEnvironmentDepthHandRemovalEnabled = Enabled;

		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE || !bExtEnvironmentDepthAvailable || !bHandsRemovalSupported)
		{
			return false;
		}

		const XrEnvironmentDepthHandRemovalSetInfoMETA SetInfo{
			/*type=*/XR_TYPE_ENVIRONMENT_DEPTH_HAND_REMOVAL_SET_INFO_META,
			/*next=*/nullptr,
			/*enabled=*/Enabled,
		};
		XR_ENSURE(xrSetEnvironmentDepthHandRemovalMETA(EnvironmentDepthProviderMeta, &SetInfo));

		bEnvironmentDepthHandRemovalEnabled = Enabled;

		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::StopEnvironmentDepth()
	{
		ExecuteOnRenderThread_DoNotWait([this]() {
			auto& EnvDepthPlugin = FOculusXRHMDModule::Get().GetExtensionPluginManager().GetEnvironmentDepthExtensionPlugin();
			if (!EnvironmentDepthSwapchain.IsEmpty())
			{
				EnvironmentDepthSwapchain.Empty();
			}
			if (EnvDepthPlugin.StopEnvironmentDepth_RenderThread())
			{
				EnvDepthPlugin.DestroyEnvironmentDepth_RenderThread();
			}
			EnvironmentDepthMinMaxTexture = nullptr;
			PrevEnvironmentDepthMinMaxSwapchainIndex = -1;
		});
		return true;
	}

	TArray<FTextureRHIRef> FEnvironmentDepthExtensionPlugin::CreateSwapChainTextures_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat,
		const FClearValueBinding& InBinding, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, ERHIResourceType InResourceType,
		const TArray<TextureHandle>& InTextures, ETextureCreateFlags InTexCreateFlags, const TCHAR* DebugName) const
	{
		OculusXRHMD::CheckInRenderThread();

		TArray<FTextureRHIRef> RHITextureSwapChain;
		for (int32 TextureIndex = 0; TextureIndex < InTextures.Num(); ++TextureIndex)
		{
			FTextureRHIRef TexRef = CreateTexture_RenderThread_Fn(InSizeX, InSizeY, InFormat, InBinding, InNumMips, InNumSamples,
				InNumSamplesTileMem, InResourceType, InTextures[TextureIndex], InTexCreateFlags);

			FString TexName = FString::Printf(TEXT("%s (%d/%d)"), DebugName, TextureIndex, InTextures.Num());
			TexRef->SetName(*TexName);
			RHIBindDebugLabelName(TexRef, *TexName);

			RHITextureSwapChain.Add(TexRef);
		}
		return RHITextureSwapChain;
	}

	bool FEnvironmentDepthExtensionPlugin::StartEnvironmentDepth()
	{
#if PLATFORM_ANDROID
		static const FString USE_SCENE_PERMISSION_NAME("com.oculus.permission.USE_SCENE");

		// Check and request scene permissions (this is needed for environment depth to work)
		// bind delegate for handling permission request result
		if (!UAndroidPermissionFunctionLibrary::CheckPermission(USE_SCENE_PERMISSION_NAME))
		{
			TArray<FString> Permissions;
			Permissions.Add(USE_SCENE_PERMISSION_NAME);
			UAndroidPermissionCallbackProxy* Proxy = UAndroidPermissionFunctionLibrary::AcquirePermissions(Permissions);
			static FDelegateHandle DelegateHandle;
			DelegateHandle = Proxy->OnPermissionsGrantedDelegate.AddLambda([this, Proxy](const TArray<FString>& Permissions, const TArray<bool>& GrantResults) {
				int PermIndex = Permissions.Find(USE_SCENE_PERMISSION_NAME);
				if (PermIndex != INDEX_NONE && GrantResults[PermIndex])
				{
					UE_LOG(LogHMD, Verbose, TEXT("%s permission granted"), *USE_SCENE_PERMISSION_NAME);
					StartEnvironmentDepth();
				}
				else
				{
					UE_LOG(LogHMD, Log, TEXT("%s permission denied"), *USE_SCENE_PERMISSION_NAME);
				}
				Proxy->OnPermissionsGrantedDelegate.Remove(DelegateHandle);
			});
			return true;
		}
#endif // PLATFORM_ANDROID

		ExecuteOnRenderThread_DoNotWait([this]() {
			auto& EnvDepthPlugin = FOculusXRHMDModule::Get().GetExtensionPluginManager().GetEnvironmentDepthExtensionPlugin();
			if (!EnvDepthPlugin.InitializeEnvironmentDepth_RenderThread())
			{
				UE_LOG(LogHMD, Error, TEXT("Failed to initialize env depth"));
				return;
			}

			TArray<TextureHandle> DepthTextures;
			int32 TextureCount;
			if (!EnvDepthPlugin.GetEnvironmentDepthTextureStageCount_RenderThread(TextureCount))
			{
				UE_LOG(LogHMD, Error, TEXT("Failed to get depth texture stage count"));
				return;
			}

			// We don't really do different depth texture formats right now and it's always a
			// single multiview texture, so no need for a separate right eye texture for now.
			// We may need a separate Left/RightDepthTextures in the future.
			DepthTextures.SetNum(TextureCount);

			for (int32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
			{
				if (!EnvDepthPlugin.GetEnvironmentDepthTexture_RenderThread(TextureIndex, DepthTextures[TextureIndex]))
				{
					UE_LOG(LogHMD, Error, TEXT("Failed to create insight depth texture. NOTE: This causes a leak of %d other texture(s), which will go unused."), TextureIndex);
					return;
				}
			}

			const uint32 SizeX = EnvironmentDepthSwapchainStateMeta.width;
			const uint32 SizeY = EnvironmentDepthSwapchainStateMeta.height;
			constexpr EPixelFormat DepthFormat = PF_ShadowDepth;
			constexpr uint32 NumMips = 1;
			constexpr uint32 NumSamples = 1;
			constexpr uint32 NumSamplesTileMem = 1;
			constexpr ETextureCreateFlags DepthTexCreateFlags = TexCreate_ShaderResource | TexCreate_InputAttachmentRead;
			const FClearValueBinding DepthTextureBinding = FClearValueBinding::DepthFar;
			constexpr ERHIResourceType ResourceType = RRT_Texture2DArray;

			if (!EnvironmentDepthSwapchain.IsEmpty())
			{
				EnvironmentDepthSwapchain.Empty();
			}
			EnvironmentDepthSwapchain = CreateSwapChainTextures_RenderThread(SizeX, SizeY, DepthFormat, DepthTextureBinding, NumMips,
				NumSamples, NumSamplesTileMem, ResourceType, DepthTextures, DepthTexCreateFlags,
				*FString::Printf(TEXT("Oculus Environment Depth Swapchain")));

			ETextureCreateFlags MinMaxTextureCreateFlags = TexCreate_ShaderResource | TexCreate_RenderTargetable;
			FRHITextureCreateDesc MinMaxTextureDesc = FRHITextureCreateDesc::Create(TEXT("EnvironmentDepthMinMaxTexture"),
				ETextureDimension::Texture2DArray)
														  .SetExtent(SizeX, SizeY)
														  // Note: PF_R16G16B16A16_UNORM would be better from a precision perspective but is less performant.
														  .SetFormat(PF_FloatRGBA)
														  .SetNumMips(NumMips)
														  .SetNumSamples(NumSamples)
														  .SetClearValue(FClearValueBinding::None);
			MinMaxTextureDesc.SetArraySize(2);
			MinMaxTextureCreateFlags |= TexCreate_TargetArraySlicesIndependently;
			MinMaxTextureDesc.SetFlags(MinMaxTextureCreateFlags);
			EnvironmentDepthMinMaxTexture = RHICreateTexture(MinMaxTextureDesc);

			if (bEnvironmentDepthHandRemovalEnabled)
			{
				EnvDepthPlugin.SetEnvironmentDepthHandRemoval_RenderThread(bEnvironmentDepthHandRemovalEnabled);
			}
			EnvDepthPlugin.StartEnvironmentDepth_RenderThread();
		});
		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::StartEnvironmentDepth_RenderThread()
	{
		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE || bEnvironmentDepthRunning)
		{
			return false;
		}

		XR_ENSURE(xrStartEnvironmentDepthProviderMETA(EnvironmentDepthProviderMeta));

		bEnvironmentDepthRunning = true;

		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::StopEnvironmentDepth_RenderThread()
	{
		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE || !bEnvironmentDepthRunning)
		{
			return false;
		}

		XR_ENSURE(xrStopEnvironmentDepthProviderMETA(EnvironmentDepthProviderMeta));

		bEnvironmentDepthRunning = false;

		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::AcquireEnvironmentDepthTexture_RHIThread(XrTime predictedDisplayTime)
	{
		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE || !bEnvironmentDepthRunning)
		{
			return false;
		}

		XrEnvironmentDepthImageAcquireInfoMETA AcquireInfo{ XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_ACQUIRE_INFO_META };
		AcquireInfo.space = StageSpace_RHIThread;

		const XrTime PredictedDisplayTime = predictedDisplayTime;
		AcquireInfo.displayTime = PredictedDisplayTime ? PredictedDisplayTime : 0;

		XrEnvironmentDepthImageMETA DepthImage{ XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_META };
		DepthImage.views[0].type = XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META;
		DepthImage.views[1].type = XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META;
		XrResult Result = xrAcquireEnvironmentDepthImageMETA(EnvironmentDepthProviderMeta, &AcquireInfo, &DepthImage);

		std::lock_guard EnvTexLock(EnvironmentDepthTextureMutex);

		if (Result == XR_ENVIRONMENT_DEPTH_NOT_AVAILABLE_META)
		{
			// ignore this one and keep the old one
		}
		else if (XR_FAILED(Result))
		{
			UE_LOG(LogHMD, Error, TEXT("Can not set environment depth frame desc: %d"), Result);
			EnvironmentDepthFrameDesc.Reset();
			return false;
		}
		else
		{
			EnvironmentDepthFrameDesc = DepthImage;
		}

		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::GetEnvironmentDepthFrameDesc_RenderThread(TOptional<XrEnvironmentDepthImageMETA>& OutEnvironmentDepthFrameDesc)
	{
		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE || !bEnvironmentDepthRunning)
		{
			return false;
		}

		OutEnvironmentDepthFrameDesc = EnvironmentDepthFrameDesc;

		return true;
	}

	void FEnvironmentDepthExtensionPlugin::SetXROcclusionsMode(UObject* WorldContextObject, EOculusXROcclusionsMode Mode)
	{
		bHardOcclusionsEnabled = (Mode == EOculusXROcclusionsMode::HardOcclusions_Deprecated);
		bSoftOcclusionsEnabled = (Mode == EOculusXROcclusionsMode::SoftOcclusions);

#if defined(WITH_OCULUS_BRANCH)
		WorldContextObject->GetWorld()->Scene->SetEnableXRPassthroughSoftOcclusions(Mode == EOculusXROcclusionsMode::SoftOcclusions);
#else
		ensureMsgf(Mode != EOculusXROcclusionsMode::SoftOcclusions, TEXT("Soft occlusions are only supported with the Oculus branch of the Unreal Engine"));
#endif
	}

	bool FEnvironmentDepthExtensionPlugin::IsEnvironmentDepthStarted() const
	{
		return !EnvironmentDepthSwapchain.IsEmpty();
	}

	bool FEnvironmentDepthExtensionPlugin::OnStartGameFrame(FWorldContext& WorldContext)
	{
		FXRTrackingSystemBase* TS = static_cast<FXRTrackingSystemBase*>(GEngine->XRSystem.Get());
		FVector Pos;
		TS->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, HeadOrientation, Pos);
		BaseOrientation = TS->GetBaseOrientation();
		return true;
	}

	bool FEnvironmentDepthExtensionPlugin::OnEndGameFrame(FWorldContext& WorldContext)
	{
		FXRTrackingSystemBase* TS = static_cast<FXRTrackingSystemBase*>(GEngine->XRSystem.Get());
		WorldToMeters = WorldContext.World()->GetWorldSettings()->WorldToMeters;
		TrackingToWorld = TS->GetTrackingToWorldTransform();
		return true;
	}

	void FEnvironmentDepthExtensionPlugin::OnDestroySession(XrSession InSession)
	{
		StopEnvironmentDepth();
	}

	const void* FEnvironmentDepthExtensionPlugin::OnLocateViews(XrSession InSession, XrTime InDisplayTime, XrViewConfigurationType ViewConfigurationType, const void* InNext)
	{
		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE || !bEnvironmentDepthRunning)
		{
			return InNext;
		}

		XrViewLocateInfo ViewInfo = {};
		ViewInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
		ViewInfo.next = nullptr;
		ViewInfo.viewConfigurationType = ViewConfigurationType;
		ViewInfo.space = StageSpace;
		ViewInfo.displayTime = InDisplayTime;

		uint32_t Count = 0;
		XrViewState ViewState{ XR_TYPE_VIEW_STATE };
		XR_ENSURE(xrLocateViews(InSession, &ViewInfo, &ViewState, 0, &Count, nullptr));

		if (EyeViews.Num() < (int)Count)
		{
			int OldCount = EyeViews.Num();
			EyeViews.SetNum(Count, EAllowShrinking::No);
			for (int i = OldCount; i < (int)Count; ++i)
			{
				memset(&EyeViews[i], 0, sizeof(XrView));
				EyeViews[i].type = XR_TYPE_VIEW;
				EyeViews[i].pose = ToXrPose(FTransform::Identity);
			}
		}
		XR_ENSURE(xrLocateViews(InSession, &ViewInfo, &ViewState, EyeViews.Num(), &Count, EyeViews.GetData()));

		return InNext;
	}

	XrStructureType FEnvironmentDepthExtensionPlugin::GetEnvironmentDepthSwapchainImageType() const
	{
		if (RHIString == TEXT("Vulkan"))
		{
			return XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
		}
		if (RHIString == TEXT("D3D12"))
		{
			return XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR;
		}
		if (RHIString == TEXT("D3D11"))
		{
			return XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
		}
		UE_LOG(LogHMD, Error, TEXT("Unknown RHIString '%s'. Fallback to Vulkan"), *RHIString);
		return XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
	}

#if defined(WITH_OCULUS_BRANCH)
	bool FEnvironmentDepthExtensionPlugin::FindEnvironmentDepthTexture_RenderThread(FTextureRHIRef& OutTexture, FTextureRHIRef& OutMinMaxTexture,
		FVector2f& OutDepthFactors, FMatrix44f OutScreenToDepthMatrices[2], FMatrix44f OutDepthViewProjMatrices[2])
	{
		OculusXRHMD::CheckInRenderThread();

		if (EnvironmentDepthProviderMeta == XR_NULL_HANDLE || !bEnvironmentDepthRunning)
		{
			return false;
		}

		int SwapchainIndex = 0;
		if (ComputeEnvironmentDepthParameters_RenderThread(OutDepthFactors, OutScreenToDepthMatrices, OutDepthViewProjMatrices, SwapchainIndex))
		{
			if (SwapchainIndex >= EnvironmentDepthSwapchain.Num())
			{
				return false;
			}
			OutTexture = EnvironmentDepthSwapchain[SwapchainIndex];
			OutMinMaxTexture = EnvironmentDepthMinMaxTexture;
			return true;
		}
		return false;
	}
#endif // defined(WITH_OCULUS_BRANCH)
#endif
} // namespace OculusXR
