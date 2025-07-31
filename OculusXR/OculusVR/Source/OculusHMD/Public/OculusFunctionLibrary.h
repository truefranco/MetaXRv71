// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IHeadMountedDisplay.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IOculusHMDModule.h"
#include "OculusFunctionLibrary.generated.h"

namespace OculusHMD
{
	class FOculusHMD;
}

/* Tracked device types corresponding to ovrTrackedDeviceType enum*/
UENUM(BlueprintType)
enum class ETrackedDeviceType : uint8
{
	None UMETA(DisplayName = "No Devices"),
	HMD	UMETA(DisplayName = "HMD"),
	LTouch	UMETA(DisplayName = "Left Hand"),
	RTouch	UMETA(DisplayName = "Right Hand"),
	Touch		UMETA(DisplayName = "All Hands"),
    DeviceObjectZero    UMETA(DisplayName = "DeviceObject Zero"),
	All	UMETA(DisplayName = "All Devices")
};

USTRUCT(BlueprintType, meta = (DisplayName = "HMD User Profile Data Field"))
struct FHmdUserProfileField
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	FString FieldName;

	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	FString FieldValue;

	FHmdUserProfileField() {}
	FHmdUserProfileField(const FString& Name, const FString& Value) :
		FieldName(Name), FieldValue(Value) {}
};

USTRUCT(BlueprintType, meta = (DisplayName = "HMD User Profile Data"))
struct FHmdUserProfile
{
	GENERATED_USTRUCT_BODY()

	/** Name of the user's profile. */
	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	FString Name;

	/** Gender of the user ("male", "female", etc). */
	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	FString Gender;

	/** Height of the player, in meters */
	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	float PlayerHeight;

	/** Height of the player, in meters */
	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	float EyeHeight;

	/** Interpupillary distance of the player, in meters */
	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	float IPD;

	/** Neck-to-eye distance, in meters. X - horizontal, Y - vertical. */
	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	FVector2D NeckToEyeDistance;

	UPROPERTY(BlueprintReadWrite, Category = "Input|HeadMountedDisplay")
	TArray<FHmdUserProfileField> ExtraFields;

	FHmdUserProfile() :
		PlayerHeight(0.f), EyeHeight(0.f), IPD(0.f), NeckToEyeDistance(FVector2D::ZeroVector) {}
};

UENUM(BlueprintType)
enum class EFoveatedRenderingMethod : uint8
{
	FixedFoveatedRendering = 0,
	EyeTrackedFoveatedRendering = 1,
};

UENUM(BlueprintType)
enum class EFoveatedRenderingLevel : uint8
{
	Off = 0,
	Low = 1,
	Medium = 2,
	High = 3,
	// High foveation setting with more detail toward the bottom of the view and more foveation near the top
	HighTop = 4
};

/* Guardian boundary types*/
UENUM(BlueprintType)
enum class EBoundaryType : uint8
{
	Boundary_Outer	UMETA(DisplayName = "Outer Boundary"),
	Boundary_PlayArea	UMETA(DisplayName = "Play Area"),
};

UENUM(BlueprintType)
enum class EColorSpace : uint8
{
	/// The default value from GetHmdColorSpace until SetClientColorDesc is called. Only valid on PC, and will be remapped to Quest on Mobile
	Unknown = 0,
	/// No color correction, not recommended for production use. See documentation for more info
	Unmanaged = 1,
	/// Preferred color space for standardized color across all Oculus HMDs with D65 white point
	Rec_2020 = 2,
	/// Rec. 709 is used on Oculus Go and shares the same primary color coordinates as sRGB
	Rec_709 = 3,
	/// Oculus Rift CV1 uses a unique color space, see documentation for more info
	Rift_CV1 = 4,
	/// Oculus Rift S uses a unique color space, see documentation for more info
	Rift_S = 5,
	/// Oculus Quest's native color space is slightly different than Rift CV1
	Quest = 6,
	/// Similar to DCI-P3. See documentation for more details on P3
	P3 = 7,
	/// Similar to sRGB but with deeper greens using D65 white point
	Adobe_RGB = 8,
};

UENUM(BlueprintType)
enum class EOculusXROcclusionsMode : uint8
{
	/// Environment depth occlusions disabled.
	Disabled = 0,
	/// Hard occlusions, good performance, shows hard edges between the real and virtual world.
	HardOcclusions = 1,
	/// Soft occlusions, most expensive, shows soft edges between the real and virtual world.
	SoftOcclusions = 2,
};

/* 
* Hand tracking settings. Please check https://developer.oculus.com/documentation/unreal/unreal-hand-tracking/
* for detailed information.
*/
UENUM(BlueprintType)
enum class EHandTrackingSupport : uint8
{
	ControllersOnly,
	ControllersAndHands,
	HandsOnly,
};

UENUM(BlueprintType)
enum class EHandTrackingFrequency : uint8
{
	LOW,
	HIGH,
	MAX,
};

UENUM(BlueprintType)
enum class EHandTrackingVersion : uint8
{
	Default,
	V1,
	V2,
};

UENUM(BlueprintType)
enum class EProcessorPerformanceLevel : uint8
{
	PowerSavings = 0 UMETA(DisplayName = "PowerSavings", ToolTip = "Usually used in non-XR section (head-locked / static screen), during which power savings are to be prioritized"),
	SustainedLow = 1 UMETA(DisplayName = "SustainedLow", ToolTip = "App enters a low and stable complexity section, during which reducing power is more important than occasional late rendering frames"),
	SustainedHigh = 2 UMETA(DisplayName = "SustainedHigh", ToolTip = "Let XR Runtime to perform consistent XR compositing and frame rendering within a thermally sustainable range"),
	Boost = 3 UMETA(DisplayName = "Boost(*)", ToolTip = "Allow XR Runtime to step up beyond the thermally sustainable range for short period. (Currently equivalent to SustainedHigh and not recommended to be used on Quest)")
};

UENUM()
enum class EProcessorFavor : int8
{
	FavorEqually = 0 UMETA(DisplayName = "Favor Equally"),
	FavorCPU = 1 UMETA(DisplayName = "Favor CPU"),
	FavorGPU = 2 UMETA(DisplayName = "Favor GPU"),
};

UENUM(BlueprintType)
enum class EOculusHMDBodyTrackingFidelity : uint8
{
	Unset = 0 UMETA(Hidden),
	Low = 1,
	High = 2,
};

UENUM(BlueprintType)
enum class EOculusHMDBodyJointSet : uint8
{
	UpperBody = 0,
	FullBody = 1,
};

UENUM()
enum class EFaceTrackingDataSourceConfig : int8
{
	Visual = 0 UMETA(DisplayName = "Visual"),
	Audio = 1 UMETA(DisplayName = "Audio"),
	MAX = 2 UMETA(Hidden),
};

UENUM()
enum class ESystemSplashBackgroundType : int8
{
	Black = 0 UMETA(DisplayName = "Black"),
	Contextual = 1 UMETA(DisplayName = "Passthrough (Contextual)"),
};

UENUM(BlueprintType)
enum class EOculusDeviceType : uint8
{
	//mobile HMDs 
	OculusMobile_Deprecated0 = 0,
	OculusQuest_Deprecated,
	OculusQuest2,
	MetaQuestPro,
	MetaQuest3,

	//PC HMDs
	Rift = 100,
	Rift_S,
	Quest_Link_Deprecated,
	Quest2_Link,
	MetaQuestProLink,
	MetaQuest3Link,

	//default
	OculusUnknown = 200,
};

UENUM(BlueprintType)
enum class EOculusControllerType : uint8
{
	None = 0,
	MetaQuestTouch = 1,
	MetaQuestTouchPro = 2,
	MetaQuestTouchPlus = 3,
	Unknown = 0x7f,
};

UENUM(BlueprintType)
enum class EOculusXrApi : uint8
{
	OVRPluginOpenXR = 0 UMETA(DisplayName = "OVRPlugin OpenXR", ToolTip = "OVRPlugin using an OpenXR backend."),
	NativeOpenXR = 1 UMETA(DisplayName = "Native OpenXR", ToolTip = "Disable OVRPlugin in favor of the native OpenXR implementation. Must enable the OpenXR plugin."),
};

/*
* Information about relationships between a triggered boundary (EBoundaryType::Boundary_Outer or
* EBoundaryType::Boundary_PlayArea) and a device or point in the world.
* All dimensions, points, and vectors are returned in Unreal world coordinate space.
*/
USTRUCT(BlueprintType)
struct FGuardianTestResult
{
	GENERATED_BODY()

	/** Is there a triggering interaction between the device/point and specified boundary? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary Test Result")
	bool IsTriggering = false;

	/** Device type triggering boundary (ETrackedDeviceType::None if BoundaryTestResult corresponds to a point rather than a device) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary Test Result")
	ETrackedDeviceType DeviceType = ETrackedDeviceType::None;

	/** Distance of device/point to surface of boundary specified by BoundaryType */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary Test Result")
	float ClosestDistance = 0.0f;

	/** Closest point on surface corresponding to specified boundary */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary Test Result")
	FVector ClosestPoint = FVector(0.0f);

	/** Normal of closest point */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boundary Test Result")
	FVector ClosestPointNormal = FVector(0.0f, 0.0f, 1.0f);
};

UENUM()
enum class EOculusControllerPoseAlignment : uint8
{
	Default = 0 UMETA(ToolTip = "Default pose alignment used in all versions of the OculusVR plugin. Recommended pose for compatibility with previous assets designed for the Meta XR plugin."),

	Grip = 1 UMETA(ToolTip = "Grip pose alignment as defined by OpenXR. Use this for cross-plugin compatibility with assets designed for the native OpenXR grip pose."),

	Aim = 2 UMETA(ToolTip = "Aim pose alignment as defined by OpenXR. Use this for cross-plugin compatibility with assets designed for the native OpenXR aim pose."),
};

UENUM(BlueprintType)
namespace EOculusResult
{
	enum Type
	{
		Success = 0,
		Success_EventUnavailable = 1,
		Success_Pending = 2,

		/// Failure
		Failure = -1000,
		Failure_InvalidParameter = -1001,
		Failure_NotInitialized = -1002,
		Failure_InvalidOperation = -1003,
		Failure_Unsupported = -1004,
		Failure_NotYetImplemented = -1005,
		Failure_OperationFailed = -1006,
		Failure_InsufficientSize = -1007,
		Failure_DataIsInvalid = -1008,
		Failure_DeprecatedOperation = -1009,
		Failure_ErrorLimitReached = -1010,
		Failure_ErrorInitializationFailed = -1011,

		/// Space error cases
		Failure_SpaceCloudStorageDisabled = -2000,
		Failure_SpaceMappingInsufficient = -2001,
		Failure_SpaceLocalizationFailed = -2002,
		Failure_SpaceNetworkTimeout = -2003,
		Failure_SpaceNetworkRequestFailed = -2004
	};
}

UCLASS()
class OCULUSHMD_API UOculusFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/**
	 * Grabs the current orientation and position for the HMD.  If positional tracking is not available, DevicePosition will be a zero vector
	 *
	 * @param DeviceRotation	(out) The device's current rotation
	 * @param DevicePosition	(out) The device's current position, in its own tracking space
	 * @param NeckPosition		(out) The estimated neck position, calculated using NeckToEye vector from User Profile. Same coordinate space as DevicePosition.
	 * @param bUseOrienationForPlayerCamera	(in) Should be set to 'true' if the orientation is going to be used to update orientation of the camera manually.
	 * @param bUsePositionForPlayerCamera	(in) Should be set to 'true' if the position is going to be used to update position of the camera manually.
	 * @param PositionScale		(in) The 3D scale that will be applied to position.
	 */
	UFUNCTION(BlueprintPure, Category="OculusLibrary")
	static void GetPose(FRotator& DeviceRotation, FVector& DevicePosition, FVector& NeckPosition, bool bUseOrienationForPlayerCamera = false, bool bUsePositionForPlayerCamera = false, const FVector PositionScale = FVector::ZeroVector);

	/**
	* Reports raw sensor data. If HMD doesn't support any of the parameters then it will be set to zero.
	*
	* @param AngularAcceleration	(out) Angular acceleration in radians per second per second.
	* @param LinearAcceleration		(out) Acceleration in meters per second per second.
	* @param AngularVelocity		(out) Angular velocity in radians per second.
	* @param LinearVelocity			(out) Velocity in meters per second.
	* @param TimeInSeconds			(out) Time when the reported IMU reading took place, in seconds.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static void GetRawSensorData(FVector& AngularAcceleration, FVector& LinearAcceleration, FVector& AngularVelocity, FVector& LinearVelocity, float& TimeInSeconds, ETrackedDeviceType DeviceType = ETrackedDeviceType::HMD);

	/**
	* Returns if the device is currently tracked by the runtime or not.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static bool IsDeviceTracked(ETrackedDeviceType DeviceType);

	/**
	* Set the CPU and GPU levels as hints to the Oculus device (Deprecated).
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary", meta=(DeprecatedFunction, 
		DeprecatedMessage="Deprecated. Please use Get/SetSuggestedCpuAndGpuPerformanceLevels instead"))
	static void SetCPUAndGPULevels(int CPULevel, int GPULevel);

	/**
	* Get the suggested CPU and GPU levels to the Oculus device.
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void GetSuggestedCpuAndGpuPerformanceLevels(EProcessorPerformanceLevel& CpuPerfLevel, EProcessorPerformanceLevel& GpuPerfLevel);

	/**
	* Set the suggested CPU and GPU levels to the Oculus device.
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetSuggestedCpuAndGpuPerformanceLevels(EProcessorPerformanceLevel CpuPerfLevel, EProcessorPerformanceLevel GpuPerfLevel);

	/**
	* Returns current user profile.
	*
	* @param Profile		(out) Structure to hold current user profile.
	* @return (boolean)	True, if user profile was acquired.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static bool GetUserProfile(FHmdUserProfile& Profile);

	/**
	* Sets 'base rotation' - the rotation that will be subtracted from
	* the actual HMD orientation.
	* Sets base position offset (in meters). The base position offset is the distance from the physical (0, 0, 0) position
	* to current HMD position (bringing the (0, 0, 0) point to the current HMD position)
	* Note, this vector is set by ResetPosition call; use this method with care.
	* The axis of the vector are the same as in Unreal: X - forward, Y - right, Z - up.
	*
	* @param Rotation			(in) Rotator object with base rotation
	* @param BaseOffsetInMeters (in) the vector to be set as base offset, in meters.
	* @param Options			(in) specifies either position, orientation or both should be set.
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetBaseRotationAndBaseOffsetInMeters(FRotator Rotation, FVector BaseOffsetInMeters, EOrientPositionSelector::Type Options);

	/**
	* Returns current base rotation and base offset.
	* The base offset is currently used base position offset, previously set by the
	* ResetPosition or SetBasePositionOffset calls. It represents a vector that translates the HMD's position
	* into (0,0,0) point, in meters.
	* The axis of the vector are the same as in Unreal: X - forward, Y - right, Z - up.
	*
	* @param OutRotation			(out) Rotator object with base rotation
	* @param OutBaseOffsetInMeters	(out) base position offset, vector, in meters.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static void GetBaseRotationAndBaseOffsetInMeters(FRotator& OutRotation, FVector& OutBaseOffsetInMeters);

	/**
	 * Scales the HMD position that gets added to the virtual camera position.
	 *
	 * @param PosScale3D	(in) the scale to apply to the HMD position.
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary", meta = (DeprecatedFunction, DeprecationMessage = "This feature is no longer supported."))
	static void SetPositionScale3D(FVector PosScale3D) { }

	/**
	 * Sets 'base rotation' - the rotation that will be subtracted from
	 * the actual HMD orientation.
	 * The position offset might be added to current HMD position,
	 * effectively moving the virtual camera by the specified offset. The addition
	 * occurs after the HMD orientation and position are applied.
	 *
	 * @param BaseRot			(in) Rotator object with base rotation
	 * @param PosOffset			(in) the vector to be added to HMD position.
	 * @param Options			(in) specifies either position, orientation or both should be set.
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary", meta = (DeprecatedFunction, DeprecationMessage = "A hack, proper camera positioning should be used"))
	static void SetBaseRotationAndPositionOffset(FRotator BaseRot, FVector PosOffset, EOrientPositionSelector::Type Options);

	/**
	 * Returns current base rotation and position offset.
	 *
	 * @param OutRot			(out) Rotator object with base rotation
	 * @param OutPosOffset		(out) the vector with previously set position offset.
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary", meta = (DeprecatedFunction, DeprecationMessage = "A hack, proper camera positioning should be used"))
	static void GetBaseRotationAndPositionOffset(FRotator& OutRot, FVector& OutPosOffset);

	/**
	 * Adds loading splash screen with parameters
	 *
	 * @param Texture			(in) A texture asset to be used for the splash.
	 * @param TranslationInMeters (in) Initial translation of the center of the splash screen (in meters).
	 * @param Rotation			(in) Initial rotation of the splash screen, with the origin at the center of the splash screen.
	 * @param SizeInMeters		(in) Size, in meters, of the quad with the splash screen.
	 * @param DeltaRotation		(in) Incremental rotation, that is added each 2nd frame to the quad transform. The quad is rotated around the center of the quad.
	 * @param bClearBeforeAdd	(in) If true, clears splashes before adding a new one.
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary", meta = (DeprecatedFunction, DeprecationMessage = "Use Add Loading Screen Splash from the Head Mounted Display Loading Screen functions instead."))
	static void AddLoadingSplashScreen(class UTexture2D* Texture, FVector TranslationInMeters, FRotator Rotation, FVector2D SizeInMeters = FVector2D(1.0f, 1.0f), FRotator DeltaRotation = FRotator::ZeroRotator, bool bClearBeforeAdd = false);

	/**
	 * Removes all the splash screens.
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary", meta = (DeprecatedFunction, DeprecationMessage = "Use Clear Loading Screen Splashes from the Head Mounted Display Loading Screen functions instead."))
	static void ClearLoadingSplashScreens();

	/**
	* Returns true, if the app has input focus.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static bool HasInputFocus();

	/**
	* Returns true, if the system overlay is present.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static bool HasSystemOverlayPresent();

	/**
	* Returns the GPU utilization availability and value
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static void GetGPUUtilization(bool& IsGPUAvailable, float& GPUUtilization);

	/**
	* Returns the GPU frame time on supported mobile platforms (Go for now)
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static float GetGPUFrameTime();

	/**
	* Returns the foveated rendering method currently being used
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static EFoveatedRenderingMethod GetFoveatedRenderingMethod();

	/**
	* Set the requested foveated rendering method
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetFoveatedRenderingMethod(EFoveatedRenderingMethod Method);

	/**
	* Returns the current foveated rendering level
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static EFoveatedRenderingLevel GetFoveatedRenderingLevel();

	/**
	* Set the requested foveated rendering level, and whether the level should be adjusted dynamically or not.
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetFoveatedRenderingLevel(EFoveatedRenderingLevel level, bool isDynamic);

	/**
	* Returns whether eye-tracked foveated rendering is supported or not
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static bool GetEyeTrackedFoveatedRenderingSupported();


	/**
	* Returns the current device's name
	*/
	UE_DEPRECATED(4.22, "UOculusFunctionLibrary::GetDeviceName has been deprecated and no longer functions as before. Please use the enum-based GetDeviceType instead.")
	UFUNCTION(BlueprintPure, Category = "OculusLibrary", meta = (DeprecatedFunction, DeprecationMessage = "UOculusFunctionLibrary::GetDeviceName has been deprecated and no longer functions as before. Please use the enum-based GetDeviceType instead."))
	static FString GetDeviceName();

	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static EOculusDeviceType GetDeviceType();

	/**
	* Returns the current controller's type
	* @param deviceHand				(in) The hand to get the position from
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static EOculusControllerType GetControllerType(EControllerHand deviceHand);

	/**
	* Returns the current available frequencies
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static TArray<float> GetAvailableDisplayFrequencies();

	/**
	* Returns the current display frequency
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static float GetCurrentDisplayFrequency();

	/**
	* Sets the requested display frequency
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetDisplayFrequency(float RequestedFrequency);

	/**
	* Enables/disables positional tracking on devices that support it.
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void EnablePositionTracking(bool bPositionTracking);

	/**
	* Enables/disables orientation tracking on devices that support it.
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void EnableOrientationTracking(bool bOrientationTracking);

	/**
	* Set the Color Scale/Offset
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetColorScaleAndOffset(FLinearColor ColorScale, FLinearColor ColorOffset, bool bApplyToAllLayers = false);

	/**
	* Returns true if system headset is in 3dof mode 
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static bool GetSystemHmd3DofModeEnabled();

	/**
	* Returns the color space of the target HMD
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary")
	static EColorSpace GetHmdColorDesc();

	/**
	* Sets the target HMD to do color space correction to a specific color space
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetClientColorDesc(EColorSpace ColorSpace);

	/**
	* Checks if passthrough is supported
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static bool IsPassthroughSupported();

	/**
	* Checks if color passthrough is supported
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static bool IsColorPassthroughSupported();

	/**
	* Turns on or off local dimming
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetLocalDimmingOn(bool LocalDimmingOn);

	/**
	* Determines if an OVRPlugin result value is a success case
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static bool IsResultSuccess(EOculusResult::Type result);


	/**
	* Sets the eyebuffer sharpen type. This amplifies contrast and fine details.
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static void SetEyeBufferSharpenType(EOculusEyeBufferSharpenType EyeBufferSharpenType);

	/**
	 * Get a system recommendation on whether Passthrough should be active.
	 * When set, it is recommended for apps which optionally support an MR experience
	 * with Passthrough to default to that mode.
	 * Currently, this is determined based on whether the user has Passthrough active in the home environment.
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary")
	static bool IsPassthroughRecommended();

	/**
	 * Returns IStereoLayers interface to work with overlays.
	 */
	static class IStereoLayers* GetStereoLayers();

	/* GUARDIAN API */
	/**
	* Returns true if the Guardian Outer Boundary is being displayed
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|Guardian")
	static bool IsGuardianDisplayed();

	/* GUARDIAN API */
	/**
	* Returns true if the Guardian has been set up by the user, false if the user is in "seated" mode and has not set up a play space.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|Guardian")
	static bool IsGuardianConfigured();

	/**
	* Returns the list of points in UE world space of the requested Boundary Type 
	* @param BoundaryType			(in) An enum representing the boundary type requested, either Outer Boundary (exact guardian bounds) or PlayArea (rectangle inside the Outer Boundary)
	* @param UsePawnSpace			(in) Boolean indicating to return the points in world space or pawn space
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|Guardian")
	static TArray<FVector> GetGuardianPoints(EBoundaryType BoundaryType, bool UsePawnSpace = false);

	/**
	* Returns the dimensions in UE world space of the requested Boundary Type
	* @param BoundaryType			(in) An enum representing the boundary type requested, either Outer Boundary (exact guardian bounds) or PlayArea (rectangle inside the Outer Boundary)
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|Guardian")
	static FVector GetGuardianDimensions(EBoundaryType BoundaryType);

	/**
	* Returns the transform of the play area rectangle, defining its position, rotation and scale to apply to a unit cube to match it with the play area.
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|Guardian")
	static FTransform GetPlayAreaTransform();

	/**
	* Get the intersection result between a UE4 coordinate and a guardian boundary
	* @param Point					(in) Point in UE space to test against guardian boundaries
	* @param BoundaryType			(in) An enum representing the boundary type requested, either Outer Boundary (exact guardian bounds) or PlayArea (rectangle inside the Outer Boundary)
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Guardian")
	static FGuardianTestResult GetPointGuardianIntersection(const FVector Point, EBoundaryType BoundaryType);

	/**
	* Get the intersection result between a tracked device (HMD or controllers) and a guardian boundary
	* @param DeviceType             (in) Tracked Device type to test against guardian boundaries
	* @param BoundaryType			(in) An enum representing the boundary type requested, either Outer Boundary (exact guardian bounds) or PlayArea (rectangle inside the Outer Boundary)
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Guardian")
	static FGuardianTestResult GetNodeGuardianIntersection(ETrackedDeviceType DeviceType, EBoundaryType BoundaryType);

	/**
	* Forces the runtime to render guardian at all times or not
	* @param GuardianVisible			(in) True will display guardian, False will hide it
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Guardian")
	static void SetGuardianVisibility(bool GuardianVisible);

	/** When player triggers the Guardian boundary */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOculusGuardianTriggeredEvent, FGuardianTestResult);

	/** When player returns within outer bounds */
	DECLARE_MULTICAST_DELEGATE(FOculusGuardianReturnedEvent);

	/**
	* For outer boundary only. Devs can bind delegates via something like: BoundaryComponent->OnOuterBoundaryTriggered.AddDynamic(this, &UCameraActor::PauseGameForBoundarySystem) where
	* PauseGameForBoundarySystem() takes a TArray<FBoundaryTestResult> parameter.
	*/
	//UPROPERTY(BlueprintAssignable, Category = "Input|OculusLibrary|Guardian")
	//static FOculusGuardianTriggeredEvent OnGuardianTriggered;

	/** For outer boundary only. Devs can bind delegates via something like: BoundaryComponent->OnOuterBoundaryReturned.AddDynamic(this, &UCameraActor::ResumeGameForBoundarySystem) */
	//UPROPERTY(BlueprintAssignable, Category = "OculusLibrary|Guardian")
	//FOculusGuardianReturnedEvent OnGuardianReturned;

protected:
	static class OculusHMD::FOculusHMD* GetOculusHMD();
};
