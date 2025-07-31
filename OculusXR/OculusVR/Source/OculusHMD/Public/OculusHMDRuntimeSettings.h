// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OculusHMDTypes.h"
#include "OculusFunctionLibrary.h"
#include "OculusHMDRuntimeSettings.generated.h"

/**
* Implements the settings for the OculusVR plugin.
*/
UCLASS(config = Engine, defaultconfig)
class OCULUSHMD_API UOculusHMDRuntimeSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Configure System Splash Screen background type. To configure Splash Image go to Project Settings > Platforms > Android > Launch Image. */
	UPROPERTY(config, EditAnywhere, Category = "System SplashScreen", meta = (DisplayName = "System Splash Screen Background"))
	ESystemSplashBackgroundType SystemSplashBackground;

	/** Whether the Splash screen is enabled. */
	UPROPERTY(config, EditAnywhere, Category = "Engine SplashScreen")
	bool bAutoEnabled;

	/** An array of splash screen descriptors listing textures to show and their positions. */
	UPROPERTY(config, EditAnywhere, Category = "Engine SplashScreen")
	TArray<FOculusSplashDesc> SplashDescs;

	/** This selects the XR API that the engine will use. If unsure, OVRPlugin OpenXR is the recommended API. */
	UPROPERTY(config, EditAnywhere, Category = General, meta = (DisplayName = "XR API", ConfigRestartRequired = true))
	EOculusXrApi XrApi;

	/** The target color space */
	UPROPERTY(config, EditAnywhere, Category = General)
	EColorSpace	ColorSpace;

	/** Whether the controller hand poses align to the OculusVR pose definitions or the OpenXR pose definitions */
	UPROPERTY(config, EditAnywhere, Category = General, meta = (EditCondition = "XrApi != EOculusXrApi::NativeOpenXR"))
	EOculusControllerPoseAlignment ControllerPoseAlignment;

	/** Whether Dash is supported by the app, which will keep the app in foreground when the User presses the oculus button (needs the app to handle input focus loss!) */
	UPROPERTY(config, EditAnywhere, Category = PC)
	bool bSupportsDash;

	/** Whether the app's depth buffer is shared with the Rift Compositor, for layer (including Dash) compositing, PTW, and potentially more. */
	UPROPERTY(config, EditAnywhere, Category = PC)
	bool bCompositesDepth;

	/** Computes mipmaps for the eye buffers every frame, for a higher quality distortion */
	UPROPERTY(config, EditAnywhere, Category = PC)
	bool bHQDistortion;

	/** Maximum allowed pixel density. */
	UPROPERTY(config, EditAnywhere, Category = "Mobile|Dynamic Resolution", DisplayName = "Enable Dynamic Resolution")
	bool bDynamicResolution;

	/** Minimum allowed pixel density. */
	UPROPERTY(config, EditAnywhere, Category = "Mobile|Dynamic Resolution")
	float PixelDensityMin;

	/** Maximum allowed pixel density. */
	UPROPERTY(config, EditAnywhere, Category = "Mobile|Dynamic Resolution")
	float PixelDensityMax;

	/** A png for Mobile-OS-driven launch splash screen. It will show up instantly at app launch and disappear upon first engine-driven frame (regardless of said frame being UE4 splashes or 3D scenes) */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "OS Splash Screen", FilePathFilter = "png", RelativeToGameDir))
	FFilePath OSSplashScreen;

	/** Default CPU level controlling CPU frequency on the mobile device */
	UPROPERTY(config, meta = (DeprecatedProperty, 
		DeprecationMessage = "Use Blueprint Function Get/SetSuggestedCpuAndGpuPerformanceLevels instead."))
	int CPULevel_DEPRECATED;

	/** Default GPU level controlling GPU frequency on the mobile device */
	UPROPERTY(config, meta = (DeprecatedProperty,
		DeprecationMessage = "Use Blueprint Function Get/SetSuggestedCpuAndGpuPerformanceLevels instead."))
	int GPULevel_DEPRECATED;

	/** Suggested CPU perf level when application starts on Oculus Quest */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	EProcessorPerformanceLevel SuggestedCpuPerfLevel;

	/** Suggested GPU perf level when application starts on Oculus Quest */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	EProcessorPerformanceLevel SuggestedGpuPerfLevel;

	/** Foveated rendering method */
	UPROPERTY(config, EditAnywhere, Category = "Mobile|Foveated Rendering", meta = (EditCondition = "XrApi == EOculusXrApi::OVRPluginOpenXR"))
	EFoveatedRenderingMethod FoveatedRenderingMethod;

	/** Foveated rendering level */
	UPROPERTY(config, EditAnywhere, Category = "Mobile|Foveated Rendering", meta = (EditCondition = "XrApi != EOculusXrApi::NativeOpenXR"))
	EFoveatedRenderingLevel FoveatedRenderingLevel;

	/** Whether foveated rendering levels will change dynamically based on performance headroom or not (up to the set Foveation Level) */
	UPROPERTY(config, EditAnywhere, Category = "Mobile|Foveated Rendering", meta = (EditCondition = "XrApi != EOculusXrApi::NativeOpenXR"))
	bool bDynamicFoveatedRendering;

	/** Whether eye tracked foveated rendering can be used with the app. */
	UPROPERTY(config, EditAnywhere, Category = "Mobile|Foveated Rendering", meta = (EditCondition = "XrApi == EOculusXrApi::OVRPluginOpenXR"))
	bool bSupportEyeTrackedFoveatedRendering;

	/** Whether the app's depth buffer is shared with the compositor to enable depth testing against other layers.
	Mobile depth composition has performance overhead both on the engine (for resolving depth) and on the compositor (for depth testing against other layers) */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Composite Depth"))
	bool bCompositeDepthMobile;

	/** If enabled the app will be focus aware. This will keep the app in foreground when the User presses the oculus button (needs the app to handle input focus loss!) */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (EditCondition = "false"))
	bool bFocusAware;

	/** [Experimental]Enable Late latching for reducing HMD and controller latency, improve tracking prediction quality, multiview and vulkan must be enabled for this feature. */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	bool bLateLatching;

	/** If enabled the app will use the Oculus system keyboard for input fields. This requires that the app be focus aware. */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	bool bRequiresSystemKeyboard;

	/** Whether controllers and/or hands can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	EHandTrackingSupport HandTrackingSupport;

	/** Note that a higher tracking frequency will reserve some performance headroom from the application's budget. */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	EHandTrackingFrequency HandTrackingFrequency;

	/** The version of hand tracking algorithm */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	EHandTrackingVersion HandTrackingVersion;

	/** Whether passthrough functionality can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Passthrough Enabled"))
	bool bInsightPassthroughEnabled;

	/** Whether Scene and Spatial Anchors can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Anchor Support"))
	bool bAnchorSupportEnabled;

	/** Whether Spatial Anchor Sharing can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Anchor Sharing"))
	bool bAnchorSharingEnabled;
	
	/** Whether Scene can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Scene Support"))
	bool bSceneSupportEnabled;

	/** Whether body tracking functionality can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Body Tracking Enabled", EditCondition = "XrApi == EOculusXrApi::OVRPluginOpenXR"))
	bool bBodyTrackingEnabled;

	/** Whether eye tracking functionality can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Eye Tracking Enabled", EditCondition = "XrApi == EOculusXrApi::OVRPluginOpenXR"))
	bool bEyeTrackingEnabled;

	/** Whether face tracking functionality can be used with the app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Face Tracking Enabled", EditCondition = "XrApi == EOculusXrApi::OVRPluginOpenXR"))
	bool bFaceTrackingEnabled;

	/** Select preffered Face Tracking data sources */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Face Tracking Source", EditCondition = "XrApi == EOculusXRXrApi::OVRPluginOpenXR"))
	TSet<EFaceTrackingDataSourceConfig> FaceTrackingDataSource;

	/** On supported Oculus mobile platforms, copy compiled .so directly to device. Allows updating compiled code without rebuilding and installing an APK. */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Deploy compiled .so directly to device"))
	bool bDeploySoToDevice;

	/** Enable phase sync on mobile, reducing HMD and controller latency, improve tracking prediction quality */
	UPROPERTY(config, EditAnywhere, Category = Mobile)
	bool bPhaseSync;

	/** Whether experimental features listed below can be used with the app. */
	UPROPERTY(config, EditAnywhere, Category = Experimental)
	bool bSupportExperimentalFeatures;

	/** Whether Tile Turn Off is enabled in app */
	UPROPERTY(config, EditAnywhere, Category = Mobile, meta = (DisplayName = "Tile Turn Off", EditCondition = "false"))
	bool bTileTurnOffEnabled;

private:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	void LoadFromIni();
	void RenameProperties();
};
