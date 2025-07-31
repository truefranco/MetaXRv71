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

#include "Interaction/Grabbable/IsdkGrabbableAudio.h"

UIsdkGrabbableAudio::UIsdkGrabbableAudio()
{
  PrimaryComponentTick.bCanEverTick = false;
  GrabbableAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Grabbable Audio"));
}

void UIsdkGrabbableAudio::SetGrabTransformer(UIsdkGrabTransformerComponent* InGrabTransformer)
{
  if (IsValid(GrabTransformer))
  {
    GrabTransformer->GetGrabTransformerEventDelegate()->RemoveDynamic(
        this, &UIsdkGrabbableAudio::HandleGrabTransformerEvent);
  }
  GrabTransformer = InGrabTransformer;
  if (IsValid(GrabTransformer))
  {
    GrabTransformerName = GrabTransformer->GetFName();
    GrabTransformer->GetGrabTransformerEventDelegate()->AddUniqueDynamic(
        this, &UIsdkGrabbableAudio::HandleGrabTransformerEvent);
  }
}

UIsdkGrabTransformerComponent* UIsdkGrabbableAudio::GetGrabTransformer() const
{
  return GrabTransformer;
}

void UIsdkGrabbableAudio::PlaySound(
    const TArray<USoundBase*>& Sounds,
    float BasePitch,
    float HalfPitchRange,
    float BaseVolume,
    float HalfVolumeRange,
    bool bUsePitchRangeAsOffset,
    bool bUseVolumeRangeAsOffset)
{
  if (!Sounds.IsEmpty())
  {
    auto SelectedIndex = FMath::RandRange(0, Sounds.Num() - 1);
    GrabbableAudioComponent->SetSound(Sounds[SelectedIndex]);
    auto PitchOffset =
        bUsePitchRangeAsOffset ? HalfPitchRange : FMath::RandRange(-HalfPitchRange, HalfPitchRange);
    GrabbableAudioComponent->SetPitchMultiplier(BasePitch + PitchOffset);
    auto VolumeOffset = bUseVolumeRangeAsOffset
        ? HalfVolumeRange
        : FMath::RandRange(-HalfVolumeRange, HalfVolumeRange);
    GrabbableAudioComponent->VolumeMultiplier = (BaseVolume + VolumeOffset);

    GrabbableAudioComponent->Play();
  }
}

FVector UIsdkGrabbableAudio::GetCurrentScale()
{
  if (IsValid(GrabTransformer))
  {
    auto Target = GrabTransformer->GetTransformTarget();
    if (IsValid(Target))
    {
      return Target->GetRelativeScale3D();
    }
  }
  return PreviousScale;
}

float UIsdkGrabbableAudio::GetScaleDelta()
{
  const float PrevMagnitude = PreviousScale.Length();
  const float NewMagnitude = GetCurrentScale().Length();
  return NewMagnitude - PrevMagnitude;
}

void UIsdkGrabbableAudio::BeginPlay()
{
  Super::BeginPlay();

  PreloadSounds(GrabbingSounds);
  PreloadSounds(ReleasingSounds);
  PreloadSounds(ScalingSounds);

  if (!IsValid(GrabTransformer) && GrabTransformerName != NAME_None)
  {
    auto Components = GetOwner()->GetComponents();
    for (auto* Component : Components)
    {
      if (Component->GetFName() == GrabTransformerName)
      {
        auto* FoundGrabTransformer = Cast<UIsdkGrabTransformerComponent>(Component);
        if (IsValid(FoundGrabTransformer))
        {
          SetGrabTransformer(FoundGrabTransformer);
          break;
        }
      }
    }
  }
  else
  {
    SetGrabTransformer(GrabTransformer);
  }
  PreviousScale = GetCurrentScale();
}

void UIsdkGrabbableAudio::PreloadSounds(const TArray<USoundBase*>& Sounds)
{
  for (int i = 0; i < Sounds.Num(); i++)
  {
    USoundBase* Sound = Sounds[i];
    GrabbableAudioComponent->SetSound(Sound);
  }
}

void UIsdkGrabbableAudio::UpdateScaling()
{
  if (StepSize <= 0 || MaxEventFreq <= 0)
  {
    return;
  }

  if (ScalingSounds.Num())
  {
    float TotalDelta = GetScaleDelta();
    float AbsTotalDelta = FMath::Abs(TotalDelta);

    if (AbsTotalDelta > StepSize)
    {
      PreviousScale = GetCurrentScale();
      auto CurrentTime = GetWorld()->GetRealTimeSeconds();
      if ((CurrentTime - LastEventTime) >= 1.f / MaxEventFreq)
      {
        LastEventTime = CurrentTime;
        auto PitchOffset = TotalDelta > 0.0 ? ScalingPitchRange : -ScalingPitchRange;
        PlaySound(ScalingSounds, ScalingBasePitch, PitchOffset, GrabbingBaseVolume, 0.0, true);
      }
    }
  }
}

void UIsdkGrabbableAudio::HandleGrabTransformerEvent(
    TransformEvent Event,
    const UIsdkGrabTransformerComponent* InGrabTransformer)
{
  switch (Event)
  {
    case TransformEvent::BeginTransform:
    case TransformEvent::EndTransform:
    {
      int CurrentPoseCount = GrabTransformer->GetGrabPoses().GetSelectPoses().Num();
      // Added a pose
      if (CurrentPoseCount > GrabPoseCount)
      {
        PlaySound(
            GrabbingSounds,
            GrabbingBasePitch,
            GrabbingPitchRange,
            GrabbingBaseVolume,
            GrabbingVolumeRange);
      }
      // Removed a pose
      else if (CurrentPoseCount < GrabPoseCount)
      {
        PlaySound(
            ReleasingSounds,
            GrabbingBasePitch,
            GrabbingPitchRange,
            GrabbingBaseVolume,
            GrabbingVolumeRange);
      }
      GrabPoseCount = CurrentPoseCount;
      break;
    }
    case TransformEvent::UpdateTransform:
      UpdateScaling();
      break;
    default:
      break;
  }
}
