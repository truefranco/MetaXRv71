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

#include "Interaction/IsdkThrowable.h"
#include "Engine/World.h"
#include "Utilities/IsdkMathUtils.h"
#include "IsdkChecks.h"
#include "DrawDebugHelpers.h"
#include "Logging/LogVerbosity.h"
#include "VisualLogger/VisualLogger.h"

UIsdkThrowable::UIsdkThrowable()
{
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UIsdkThrowable::StartTracking(USceneComponent* InTrackedComponent)
{
  TrackedComponent = InTrackedComponent;
  LastNPositions.Init(TPair<FVector, float>(FVector::ZeroVector, 0.0f), Settings.SampleSize);
  LastNRotations.Init(TPair<FQuat, float>(FQuat::Identity, 0.0f), Settings.SampleSize);
  SetComponentTickEnabled(true);
}

void UIsdkThrowable::StopTracking()
{
  TrackedComponent = nullptr;
  SetComponentTickEnabled(false);
}

void UIsdkThrowable::SetSamplePositions(const TArray<TPair<FVector, float>>& Positions)
{
  LastNPositions = Positions;
}

void UIsdkThrowable::SetSampleRotations(const TArray<TPair<FQuat, float>>& Rotations)
{
  LastNRotations = Rotations;
}

void UIsdkThrowable::BeginPlay()
{
  Super::BeginPlay();
  IsFirstFrame = true;
  LastNPositions.Init(TPair<FVector, float>(FVector::ZeroVector, 0.0f), Settings.SampleSize);
  LastNRotations.Init(TPair<FQuat, float>(FQuat::Identity, 0.0f), Settings.SampleSize);
  CurrentPositionIndex = 0;
  CurrentRotationIndex = 0;
  PreviousPositionIndex = 0;

  ResetKalmanFilter();

  KalmanParams.Q = KalmanProcessNoise;
  KalmanParams.R = KalmanObservationNoise;

  Settings.LastTime = GetWorld()->GetTimeSeconds();
}

// Called every frame
void UIsdkThrowable::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
  if (!IsValid(TrackedComponent) || !IsValid(GetWorld()))
  {
    return;
  }

  // If we get a bad delta between now and last frame, don't insert the delta, just skip this frame.
  if (!CheckPositionConstraints())
  {
    return;
  }

  if (Settings.VelocityEstimationMethod == EIsdkVelocityEstimationMethod::VE_KalmanFilter)
  {
    ProcessKalmanFilter();
  }

  // Store the current position and timestamp
  LastNPositions[CurrentPositionIndex] =
      TPair<FVector, float>(TrackedComponent->GetComponentLocation(), GetWorld()->GetTimeSeconds());
  PreviousPositionIndex = CurrentPositionIndex;
  CurrentPositionIndex = (CurrentPositionIndex + 1) % Settings.SampleSize;
  LastNRotations[CurrentRotationIndex] =
      TPair<FQuat, float>(TrackedComponent->GetComponentQuat(), GetWorld()->GetTimeSeconds());
  CurrentRotationIndex = (CurrentRotationIndex + 1) % Settings.SampleSize;

  // Debug visualization for throwable
#if !UE_BUILD_SHIPPING
  for (int i = 0; i < LastNPositions.Num(); i++)
  {
    const auto Position = LastNPositions[i].Key;
    const auto Coefficient = (1.f - i) / static_cast<float>(LastNPositions.Num());
    UE_VLOG_LOCATION(
        GetOwner(),
        LogOculusInteraction,
        Log,
        Position,
        1.f,
        (Coefficient * FColor::White).ToFColor(true),
        TEXT_EMPTY);
  }
#endif
}

void UIsdkThrowable::GetLastNPositions(int32 N, TArray<TPair<FVector, float>>& OutPositions) const
{
  OutPositions.Empty();

  for (int32 i = 0; i < N; i++)
  {
    const int32 Index = (CurrentPositionIndex - i - 1 + Settings.SampleSize) % Settings.SampleSize;
    TPair<FVector, float> PositionTimestampPair = LastNPositions[Index];

    // Add initialized positions only
    if (PositionTimestampPair.Key != FVector::ZeroVector)
    {
      OutPositions.Insert(PositionTimestampPair, 0);
    }
  }
}

FVector UIsdkThrowable::GetVelocity() const
{
  TArray<TPair<FVector, float>> Positions;
  GetLastNPositions(LastNPositions.Num(), Positions);

  int32 numPositions = Positions.Num();

  // Check if there are enough tracked positions
  if (numPositions <= 1)
  {
    return FVector::ZeroVector;
  }

  TArray<FVector> PositionValues;
  for (const TPair<FVector, float>& PositionTimestampPair : Positions)
  {
    PositionValues.Add(PositionTimestampPair.Key);
  }

  const FVector Mean = FIsdkMathUtils::GetMean(PositionValues);
  const FVector StandardDeviation = FIsdkMathUtils::GetStandardDeviation(PositionValues, Mean);

  // Filter out positions that are more than a certain number of standard deviations away from the
  // mean
  TArray<TPair<FVector, float>> FilteredPositions;
  FilteredPositions.Reserve(numPositions);

  for (const TPair<FVector, float>& PositionTimestampPair : Positions)
  {
    FVector Position = PositionTimestampPair.Key;
    FVector ZScore = (Position - Mean) / StandardDeviation;
    if (ZScore.GetMax() <= Settings.Z_Score_Threshold)
    {
      FilteredPositions.Add(PositionTimestampPair);
    }
  }

  FVector OutVelocity;
  switch (Settings.VelocityEstimationMethod)
  {
    case EIsdkVelocityEstimationMethod::VE_LeastSquares:
      OutVelocity = FIsdkMathUtils::LeastSquares(FilteredPositions)
                        .GetClampedToSize(MinVelocity, MaxVelocity);
      break;
    case EIsdkVelocityEstimationMethod::VE_RANSAC:
      OutVelocity =
          FIsdkMathUtils::Ransac(FilteredPositions, Ransac_Iterations, Ransac_Score_Threshold);
      break;
    case EIsdkVelocityEstimationMethod::VE_KalmanFilter:
      OutVelocity = KalmanParams.V;
      break;
    default:
      OutVelocity = FVector::ZeroVector;
  }

    // Debug visualization at time of throw
#if !UE_BUILD_SHIPPING
  {
    for (int i = 0; i < FilteredPositions.Num(); i++)
    {
      const auto Position = FilteredPositions[i].Key;
      const auto Coefficient = (1.f - i) / static_cast<float>(LastNPositions.Num());
      UE_VLOG_LOCATION(
          GetOwner(),
          LogOculusInteraction,
          Log,
          Position,
          1.f,
          (Coefficient * FColor::White).ToFColor(true),
          TEXT_EMPTY);
    }

    const auto TraceStart = LastNPositions[CurrentPositionIndex].Key;
    const auto TraceEnd = TraceStart + OutVelocity.GetSafeNormal() * 10.f;
    UE_VLOG_ARROW(
        GetOwner(), LogOculusInteraction, Log, TraceStart, TraceEnd, FColor::Red, TEXT_EMPTY);
  }
#endif

  return OutVelocity;
}

FQuat UIsdkThrowable::GetAngularVelocity() const
{
  // If for whatever reason our sample size has somehow been set to zero or 1, return an
  // FQuat::Identity
  if (!UIsdkChecks::ValidateCondition(
          Settings.SampleSize > 1,
          this,
          TEXT("Throwable SampleSize Setting < 2"),
          ANSI_TO_TCHAR(__FUNCTION__),
          nullptr))
  {
    return FQuat::Identity;
  }

  FVector SumAngularVelocity = FVector::ZeroVector;
  for (int i = 0; i < LastNRotations.Num() - 1; i++)
  {
    const FQuat CurrentRotation = LastNRotations[i].Key;
    const FQuat NextRotation = LastNRotations[i + 1].Key;
    const FQuat RotationDifference = FQuat::Slerp(
        CurrentRotation,
        NextRotation,
        (LastNRotations[i + 1].Value - LastNRotations[i].Value) / Settings.SampleSize);
    FVector Axis;
    float Angle;
    RotationDifference.ToAxisAndAngle(Axis, Angle);
    Angle = FMath::Fmod(Angle, 2 * PI);
    const float RotationDelta = LastNRotations[i + 1].Value - LastNRotations[i].Value;
    // If the current and next rotation are zero, there is no angular velocity delta to add to the
    // running sum, avoid Div0 and move on
    if (RotationDelta == 0.0f)
    {
      continue;
    }
    const FVector AngularVelocity = Axis * Angle / RotationDelta;
    SumAngularVelocity += AngularVelocity;
  }

  FVector AverageAngularVelocity = SumAngularVelocity / (LastNRotations.Num() - 1);
  AverageAngularVelocity.X = FMath::Clamp(AverageAngularVelocity.X, -MaxAngularSpeed, MaxAngularSpeed);
  AverageAngularVelocity.Y = FMath::Clamp(AverageAngularVelocity.Y, -MaxAngularSpeed, MaxAngularSpeed);
  AverageAngularVelocity.Z = FMath::Clamp(AverageAngularVelocity.Z, -MaxAngularSpeed, MaxAngularSpeed);
  const FQuat AverageAngularVelocityQuat =
      FQuat::MakeFromEuler(AverageAngularVelocity * Settings.AngularVelocityScale);
  return AverageAngularVelocityQuat;
}

void UIsdkThrowable::GetThrowableSettings(FIsdkThrowableSettings& OutSettings)
{
  OutSettings = Settings;
}

void UIsdkThrowable::SetThrowableSettings(const FIsdkThrowableSettings& InSettings)
{
  Settings = InSettings;
}

void UIsdkThrowable::ResetKalmanFilter()
{
  if (IsValid(TrackedComponent))
  {
    KalmanParams.X = TrackedComponent->GetComponentLocation();
  }
  else
  {
    KalmanParams.X = this->GetComponentLocation();
  }
  KalmanParams.V = FVector::ZeroVector;
  KalmanParams.P = KalmanErrorCovariance;
  KalmanParams.K = FVector::ZeroVector;
}

// WIP TODO: Implement Exponential Smoothing Filter for Position Data T=1082914832757572
void UIsdkThrowable::ProcessKalmanFilter()
{
  FVector Z = TrackedComponent->GetComponentLocation();
  double T = GetWorld()->GetTimeSeconds();
  double DT = T - Settings.LastTime;

  if (DT >= MAX_TIME_DELTA)
  {
    Settings.LastTime = T;
    return;
  }

  FIsdkMathUtils::UpdateKalmanFilter(KalmanParams, Z, DT);

  KalmanParams.V.X = FMath::Clamp(KalmanParams.V.X, MinVelocity, MaxVelocity);
  KalmanParams.V.Y = FMath::Clamp(KalmanParams.V.Y, MinVelocity, MaxVelocity);
  KalmanParams.V.Z = FMath::Clamp(KalmanParams.V.Z, MinVelocity, MaxVelocity);

  Settings.LastTime = GetWorld()->GetTimeSeconds();
}

bool UIsdkThrowable::CheckPositionConstraints()
{
  if (!IsFirstFrame)
  {
    if (LastNPositions[PreviousPositionIndex].Key != FVector::ZeroVector)
    {
      // Check for erratic movement and lack of movement
      if (auto Distance = FVector::Dist(
              LastNPositions[PreviousPositionIndex].Key, TrackedComponent->GetComponentLocation());
          Distance < MinPositionThreshold || Distance > MaxPositionThreshold)
      {
        UE_VLOG(
            GetOwner(),
            LogOculusInteraction,
            Warning,
            TEXT(
                "Bad Delta in UIsdkThrowable.  Distance: %.2f | Previous Position: %s | CurrentPosition: %s"),
            Distance,
            *LastNPositions[PreviousPositionIndex].Key.ToCompactString(),
            *TrackedComponent->GetComponentLocation().ToCompactString());
        return false;
      }
    }
  }
  else
  {
    IsFirstFrame = false;
  }

  return true;
}
