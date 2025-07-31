// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OculusInputFunctionLibrary.generated.h"

UENUM(BlueprintType)
enum class EOculusHandType : uint8
{
	None,
	HandLeft,
	HandRight,
};

UENUM(BlueprintType)
enum class ETrackingConfidence : uint8
{
	Low,
	High
};

UENUM(BlueprintType)
enum class EOculusFinger : uint8
{
	Thumb,
	Index,
	Middle,
	Ring,
	Pinky,
	Invalid
};

/**
* EBone is enum representing the Bone Ids that come from the Oculus Runtime. 
*/
UENUM(BlueprintType)
enum class EBone : uint8
{
	Wrist_Root UMETA(DisplayName = "Wrist Root"),
	Hand_Start = Wrist_Root UMETA(DisplayName = "Hand Start"),
	Forearm_Stub UMETA(DisplayName = "Forearm Stub"),
	Thumb_0 UMETA(DisplayName = "Thumb0"),
	Thumb_1 UMETA(DisplayName = "Thumb1"),
	Thumb_2 UMETA(DisplayName = "Thumb2"),
	Thumb_3 UMETA(DisplayName = "Thumb3"),
	Index_1 UMETA(DisplayName = "Index1"),
	Index_2 UMETA(DisplayName = "Index2"),
	Index_3 UMETA(DisplayName = "Index3"),
	Middle_1 UMETA(DisplayName = "Middle1"),
	Middle_2 UMETA(DisplayName = "Middle2"),
	Middle_3 UMETA(DisplayName = "Middle3"),
	Ring_1 UMETA(DisplayName = "Ring1"),
	Ring_2 UMETA(DisplayName = "Ring2"),
	Ring_3 UMETA(DisplayName = "Ring3"),
	Pinky_0 UMETA(DisplayName = "Pinky0"),
	Pinky_1 UMETA(DisplayName = "Pinky1"),
	Pinky_2 UMETA(DisplayName = "Pinky2"),
	Pinky_3 UMETA(DisplayName = "Pinky3"),
	Thumb_Tip UMETA(DisplayName = "Thumb Tip"),
	Max_Skinnable = Thumb_Tip UMETA(DisplayName = "Max Skinnable"),
	Index_Tip UMETA(DisplayName = "Index Tip"),
	Middle_Tip UMETA(DisplayName = "Middle Tip"),
	Ring_Tip UMETA(DisplayName = "Ring Tip"),
	Pinky_Tip UMETA(DisplayName = "Pinky Tip"),
	Hand_End UMETA(DisplayName = "Hand End"),
	Bone_Max = Hand_End UMETA(DisplayName = "Hand Max"),
	Invalid UMETA(DisplayName = "Invalid")
};

/** Defines the haptics location of controller hands for tracking. */
UENUM(BlueprintType)
enum class EOculusHandHapticsLocation : uint8
{
	Hand = 0,   // Haptics is applied to the whole controller 
	Thumb,      // Haptics is applied to the thumb finger location
	Index,      // Haptics is applied to the index finger location

	HandHapticsLocation_Count UMETA(Hidden, DisplayName = "<INVALID>"),
};

/** Define how a controllers button touches will be used to generate a hand pose. */
UENUM(BlueprintType)
enum class EOculusXRControllerDrivenHandPoseTypes : uint8
{
	None = 0,	// Controllers do not generate any hand poses.
	Natural,	// Controller button inputs will be used to generate a normal hand pose.
	Controller, // Controller button inputs will be used to generate a hand pose holding a controller.
};

struct FOculusHapticsDesc
{
	FOculusHapticsDesc(
		EOculusHandHapticsLocation ILocation = EOculusHandHapticsLocation::Hand,
		bool bIAppend = false) :
		Location(ILocation),
		bAppend(bIAppend)
	{}

	void Restart()
	{
		Location = EOculusHandHapticsLocation::Hand;
		bAppend = false;
	}
	EOculusHandHapticsLocation Location;
	bool bAppend;
};

/**
* FOculusCapsuleCollider is a struct that contains information on the physics/collider capsules created by the runtime for hands.
*
* @var Capsule		The UCapsuleComponent that is the collision capsule on the bone. Use this to register for overlap/collision events
* @var BoneIndex	The Bone that this collision capsule is parented to. Corresponds to the EBone enum.
*
*/
USTRUCT(BlueprintType)
struct OCULUSINPUT_API FOculusCapsuleCollider
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "OculusLibrary|HandTracking")
	UCapsuleComponent* Capsule = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "OculusLibrary|HandTracking")
	EBone BoneId = EBone::Wrist_Root;
};

UCLASS()
class OCULUSINPUT_API UOculusInputFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|HandTracking")
	static EOculusFinger ConvertBoneToFinger(const EBone Bone);
	
	DECLARE_MULTICAST_DELEGATE_FourParams(FHandMovementFilterDelegate, EControllerHand, FVector*, FRotator*, bool*);
	static FHandMovementFilterDelegate HandMovementFilter; /// Called to modify Hand position and orientation whenever it is queried
	
	/**
	 * Creates a new runtime hand skeletal mesh.
	 *
	 * @param HandSkeletalMesh			(out) Skeletal Mesh object that will be used for the runtime hand mesh
	 * @param SkeletonType				(in) The skeleton type that will be used for generating the hand bones
	 * @param MeshType					(in) The mesh type that will be used for generating the hand mesh
	 * @param WorldTometers				(in) Optional change to the world to meters conversion value
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|HandTracking")
	static bool GetHandSkeletalMesh(USkeletalMesh* HandSkeletalMesh, EOculusHandType SkeletonType, EOculusHandType MeshType, const float WorldToMeters = 100.0f);

	/**
	 * Initializes physics capsules for collision and physics on the runtime mesh
	 *
	 * @param SkeletonType				(in) The skeleton type that will be used to generated the capsules
	 * @param HandComponent				(in) The skinned mesh component that the capsules will be attached to
	 * @param WorldTometers				(in) Optional change to the world to meters conversion value
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|HandTracking")
	static TArray<FOculusCapsuleCollider> InitializeHandPhysics(EOculusHandType SkeletonType, USkinnedMeshComponent* HandComponent, const float WorldToMeters = 100.0f);

	/**
	 * Get the rotation of a specific bone
	 *
	 * @param DeviceHand				(in) The hand to get the rotations from
	 * @param BoneId					(in) The specific bone to get the rotation from
	 * @param ControllerIndex			(in) Optional different controller index
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static FQuat GetBoneRotation(const EOculusHandType DeviceHand, const EBone BoneId, const int32 ControllerIndex = 0);
	
	/**
	 * Get the pointer pose
	 *
	 * @param DeviceHand				(in) The hand to get the pointer pose from
	 * @param ControllerIndex			(in) Optional different controller index
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static FTransform GetPointerPose(const EOculusHandType DeviceHand, const int32 ControllerIndex = 0);

	/**
	 * Check if the pointer pose is a valid pose
	 *
	 * @param DeviceHand				(in) The hand to get the pointer status from
	 * @param ControllerIndex			(in) Optional different controller index
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static bool IsPointerPoseValid(const EOculusHandType DeviceHand, const int32 ControllerIndex = 0);

	/**
	 * Get the tracking confidence of the hand
	 *
	 * @param DeviceHand				(in) The hand to get tracking confidence of
	 * @param ControllerIndex			(in) Optional different controller index
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static ETrackingConfidence GetTrackingConfidence(const EOculusHandType DeviceHand, const int32 ControllerIndex = 0);

	/**
	* Get the tracking confidence of a finger
	*
	* @param DeviceHand				(in) The hand to get tracking confidence of
	* @param ControllerIndex			(in) Optional different controller index
	* @param Finger			(in) The finger to get tracking confidence of
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static ETrackingConfidence GetFingerTrackingConfidence(const EOculusHandType DeviceHand, const EOculusFinger Finger, const int32 ControllerIndex = 0);

	/**
	 * Get the scale of the hand
	 *
	 * @param DeviceHand				(in) The hand to get scale of
	 * @param ControllerIndex			(in) Optional different controller index
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static float GetHandScale(const EOculusHandType DeviceHand, const int32 ControllerIndex = 0);

	/**
	 * Get the user's dominant hand
	 *
	 * @param ControllerIndex			(in) Optional different controller index
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static EOculusHandType GetDominantHand(const int32 ControllerIndex = 0);

	/**
	 * Check if hand tracking is enabled currently
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static bool IsHandTrackingEnabled();

	/**
	* Check if the hand position is valid
	*
	* @param DeviceHand				(in) The hand to get the position from
	* @param ControllerIndex			(in) Optional different controller index
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static bool IsHandPositionValid(const EOculusHandType DeviceHand, const int32 ControllerIndex = 0);

	/**
	* Enable or disable support for multimodal controller and hand tracking
	*
	* @param Supported				(in) Whether multimodal is supported or not
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|HandTracking")
	static void SetMultimodalHandsControllersSupported(const bool Supported);

	/**
	* Enable or disable support for simultaneous controller and hand tracking
	*
	* @param Enabled				(in) Whether simultaneous hand and controller tracking is enabled or not
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|HandTracking")
	static void SetSimultaneousHandsAndControllersEnabled(const bool Enabled);

	/**
	* Get whether the controller is currently in the provided hand
	*
	* @param DeviceHand				(in) The hand in which to check if the controller is being held
	*/
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static bool GetControllerIsInHand(EOculusHandType DeviceHand);

	/**
	* Set whether the system should be querying for hand poses powered by controller data
	*
	* @param ControllerDrivenPoses				(in) Whether the system should be querying for hand poses powered by controller data
	*/
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static void SetControllerDrivenHandPoses(EOculusXRControllerDrivenHandPoseTypes Type);

	/**
	 * Get if / how controller inputs are used to build a syntheic hand pose.
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|Controller")
	static EOculusXRControllerDrivenHandPoseTypes GetControllerDrivenHandPoses();

	/**
	 * Get the bone name from the bone index
	 *
	 * @param BoneId					(in) Bone index to get the name of
	 */
	UFUNCTION(BlueprintPure, Category = "OculusLibrary|HandTracking")
	static FString GetBoneName(EBone BoneId);

	/**
	 * Play a haptic feedback curve on the player's controller with location support.
	 * The curve data will be sampled and sent to controller to vibrate a specific location at each frame.
	 * @param	HapticEffect			The haptic effect to play
	 * @param	Hand					Which hand to play the effect on
	 * @param	Location				Which hand location to play the effect on
	 * @param	Scale					Scale between 0.0 and 1.0 on the intensity of playback
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static void PlayCurveHapticEffect(class UHapticFeedbackEffect_Curve* HapticEffect, EControllerHand Hand, EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand, float Scale = 1.f, bool bLoop = false);

	/**
	 * Play a haptic feedback buffer on the player's controller with location support.
	 * In each frame, the buffer data will be sampled and the individual sampled data will be sent to controller to vibrate a specific location.
	 * @param	HapticEffect			The haptic effect to play
	 * @param	Hand					Which hand to play the effect on
	 * @param	Location				Which hand location to play the effect on
	 * @param	Scale					Scale between 0.0 and 1.0 on the intensity of playback
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static void PlayBufferHapticEffect(class UHapticFeedbackEffect_Buffer* HapticEffect, EControllerHand Hand, EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand, float Scale = 1.f, bool bLoop = false);

	/**
	 * Play a haptic feedback buffer on the player's controller.
	 * All buffer data will be sent to controller together in one frame.
	 * Data duration should be no greater than controller's maximum haptics duration which can be queried with GetMaxHapticDuration.
	 * @param	HapticEffect			The haptic effect to play
	 * @param	Hand					Which hand to play the effect on
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static void PlayAmplitudeEnvelopeHapticEffect(class UHapticFeedbackEffect_Buffer* HapticEffect, EControllerHand Hand);

	/**
	 * Play a haptic feedback soundwave on the player's controller.
	 * In each frame, the soundwave data will be splitted into a batch of data and sent to controller.
	 * The data duration of each frame is equal to controller's maximum haptics duration which can be queried with GetMaxHapticDuration.
	 * @param	HapticEffect			The haptic effect to play
	 * @param	Hand					Which hand to play the effect on
	 * @param	bAppend					False: any existing samples will be cleared and a new haptic effect will begin; True: samples will be appended to the currently playing effect
	 * @param	Scale					Scale between 0.0 and 1.0 on the intensity of playback
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static void PlaySoundWaveHapticEffect(class UHapticFeedbackEffect_SoundWave* HapticEffect, EControllerHand Hand, bool bAppend = false, float Scale = 1.f, bool bLoop = false);

	/**
	 * Stops a playing haptic feedback curve at a specific location.
	 * @param	HapticEffect			The haptic effect to stop
	 * @param	Hand					Which hand to stop the effect for
	 * @param	Location				Which hand location to play the effect on
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static void StopHapticEffect(EControllerHand Hand, EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand);

	/**
	 * Set the value of the haptics for the specified hand and location directly, using frequency and amplitude.  NOTE:  If a curve is already
	 * playing for this hand, it will be cancelled in favour of the specified values.
	 *
	 * @param	Frequency				The normalized frequency [0.0, 1.0] to play through the haptics system
	 * @param	Amplitude				The normalized amplitude [0.0, 1.0] to set the haptic feedback to
	 * @param	Hand					Which hand to play the effect on
	 * @param	Location				Which hand location to play the effect on
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static void SetHapticsByValue(const float Frequency, const float Amplitude, EControllerHand Hand, EOculusHandHapticsLocation Location = EOculusHandHapticsLocation::Hand);

	/**
	 * Get the controller haptics sample rate.
	 * @param	Hand					Which hand to play the effect on
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static float GetControllerSampleRateHz(EControllerHand Hand);

	/**
	 * Get the maximum duration (in seconds) that the controller haptics can handle each time.
	 * @param	Hand					Which hand to play the effect on
	 */
	UFUNCTION(BlueprintCallable, Category = "OculusLibrary|Controller")
	static int GetMaxHapticDuration(EControllerHand Hand);
};

