/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * Licensed under the Oculus SDK License Agreement (the "License");
 * you may not use the Oculus SDK except in compliance with the License,
 * which is provided at the time of installation or download, or which
 * otherwise accompanies this software in either electronic or hard copy form.
 *
 * You may obtain a copy of the License at
 *
 * https://developer.oculus.com/licenses/oculussdk/
 *
 * Unless required by applicable law or agreed to in writing, the Oculus SDK
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "CoreMinimal.h"
#include "IsdkGrabTransformerComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Math/UnrealMathUtility.h"

#include "IsdkGrabbableAudio.generated.h"

/// Raises events when an object is scaled up or down. Events are raised in steps,
/// meaning scale changes are only responded to when the scale magnitude delta since
/// last step exceeds a provided amount.
UCLASS(
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Grabbable Audio"))
class OCULUSINTERACTION_API UIsdkGrabbableAudio final : public UActorComponent
{
  GENERATED_BODY()

 public:
  UIsdkGrabbableAudio();

  /**
   * Sets the grab transformer this component plays audio for
   */
  UFUNCTION(BlueprintCallable, Category = "InteractionSDK")
  void SetGrabTransformer(UIsdkGrabTransformerComponent* InGrabTransformer);

  /**
   * Gets the grab transformer this component plays autio for
   */
  UFUNCTION(
      BlueprintPure,
      Category = "InteractionSDK",
      meta = (ReturnDisplayName = "GrabTransformer"))
  UIsdkGrabTransformerComponent* GetGrabTransformer() const;

 protected:
  /**
   * The grab transformer this component plays audio for
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FName GrabTransformerName;

  /**
   * The increase in scale magnitude that will fire the step event
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK|Scaling")
  float StepSize = 0.6f;

  /**
   * Events will not be fired more frequently than this many times per second
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK|Scaling")
  int MaxEventFreq = 25;

  /**
   * A collection of sounds to be played at random when scaling the grab transformer
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK|Scaling")
  TArray<USoundBase*> ScalingSounds;

  /**
   * A collection of sounds to be played at random when grabbing the grab transformer
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK|Grabbing")
  TArray<USoundBase*> GrabbingSounds;

  /**
   * A collection of sounds to be played at random when releasing the grab transformer
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK|Grabbing")
  TArray<USoundBase*> ReleasingSounds;

  /**
   * Sets the default pitch for the grabbing sound (1 is normal pitch)
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "InteractionSDK|Grabbing",
      meta = (ClampMin = ".1", ClampMax = "3.0", UIMin = ".1", UIMax = "3.0"))
  float GrabbingBasePitch = 1;

  /**
   * Adjust the slider value for randomized pitch of the grabbing sound (0 is no randomization)
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "InteractionSDK|Grabbing",
      meta = (ClampMin = "-3", ClampMax = "3", UIMin = "-3", UIMax = "3"))
  float GrabbingPitchRange = 0.1f;

  /**
   * Sets the default volume for the grabbing sound (0 = silent, 1 = full volume)
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK|Grabbing")
  float GrabbingBaseVolume = 1.f;

  /**
   * Adjust the slider value for randomized volume level playback if the grabbing sound (0 is no
   * randomization)
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractionSDK|Grabbing")
  float GrabbingVolumeRange = 0.f;

  /**
   * Sets the default pitch for the scaling sound (1 is normal pitch)
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "InteractionSDK|Scaling",
      meta = (ClampMin = ".1", ClampMax = "3.0", UIMin = ".1", UIMax = "3.0"))
  float ScalingBasePitch = 1.f;

  /**
   * Sets the pitch range for the scaling sound
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "InteractionSDK|Scaling",
      meta = (ClampMin = "-3", ClampMax = "3", UIMin = "-3", UIMax = "3"))
  float ScalingPitchRange = 0.1f;

 private:
  /**
   * The grab transformer to play audio for
   */
  UPROPERTY(
      BlueprintGetter = GetGrabTransformer,
      BlueprintSetter = SetGrabTransformer,
      Category = InteractionSDK)
  UIsdkGrabTransformerComponent* GrabTransformer;

  UPROPERTY()
  UAudioComponent* GrabbableAudioComponent = nullptr;

  FVector PreviousScale = FVector::OneVector;
  float LastEventTime = 0;
  int GrabPoseCount = 0;

  FVector GetCurrentScale();
  float GetScaleDelta();
  virtual void BeginPlay() override;
  void PreloadSounds(const TArray<USoundBase*>& Sounds);
  void PlaySound(
      const TArray<USoundBase*>& Sounds,
      float BasePitch,
      float HalfPitchRange,
      float BaseVolume,
      float HalfVolumeRange,
      bool bUsePitchRangeAsOffset = false,
      bool bUseVolumeRangeAsOffset = false);
  void UpdateScaling();

  UFUNCTION()
  void HandleGrabTransformerEvent(
      TransformEvent Event,
      const UIsdkGrabTransformerComponent* InGrabTransformer);
};
