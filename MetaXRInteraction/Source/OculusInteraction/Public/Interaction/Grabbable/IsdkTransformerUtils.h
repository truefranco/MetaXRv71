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
#include "IsdkTransformerUtils.generated.h"

USTRUCT(BlueprintType)
struct FIsdkAxisConstraints
{
  GENERATED_BODY()

  FIsdkAxisConstraints() = default;
  FIsdkAxisConstraints(bool bUseConstraint, float MinVal, float MaxVal)
      : bUseConstraint(bUseConstraint), Min(MinVal), Max(MaxVal)
  {
  }

  UPROPERTY(BlueprintReadWrite, meta = (Input), Category = "InteractionSDK")
  bool bUseConstraint = false;

  UPROPERTY(BlueprintReadWrite, meta = (Input), Category = "InteractionSDK")
  float Min = 0;

  UPROPERTY(BlueprintReadWrite, meta = (Input), Category = "InteractionSDK")
  float Max = 0;
};

USTRUCT(BlueprintType)
struct FIsdkConstraintAxes
{
  GENERATED_BODY()

  FIsdkConstraintAxes() = default;

  UPROPERTY(BlueprintReadWrite, meta = (Input), Category = "InteractionSDK")
  FIsdkAxisConstraints XAxis;

  UPROPERTY(BlueprintReadWrite, meta = (Input), Category = "InteractionSDK")
  FIsdkAxisConstraints YAxis;

  UPROPERTY(BlueprintReadWrite, meta = (Input), Category = "InteractionSDK")
  FIsdkAxisConstraints ZAxis;

  bool HasConstraints() const
  {
    return XAxis.bUseConstraint || YAxis.bUseConstraint || ZAxis.bUseConstraint;
  }

  void SetAxisConstraint(EAxis::Type Axis, FIsdkAxisConstraints Constraint)
  {
    switch (Axis)
    {
      case EAxis::X:
        XAxis = Constraint;
        break;
      case EAxis::Y:
        YAxis = Constraint;
        break;
      case EAxis::Z:
        ZAxis = Constraint;
        break;
      default:
        XAxis = YAxis = ZAxis = Constraint;
        break;
    }
  }
};

using Pose = FTransform;
using ScaleConstraints = FIsdkAxisConstraints;
using PositionConstraints = FIsdkConstraintAxes;
using ConstrainedAxis = FIsdkAxisConstraints;
using RotationConstraints = FIsdkConstraintAxes;

USTRUCT()
struct FIsdkTransformerUtils
{
  GENERATED_BODY()

  static PositionConstraints GenerateParentPositionConstraints(
      const PositionConstraints& Constraints,
      const FVector& InitialPosition)
  {
    PositionConstraints ParentConstraints;

    if (Constraints.XAxis.bUseConstraint)
    {
      ParentConstraints.XAxis.bUseConstraint = true;
      ParentConstraints.XAxis.Min = Constraints.XAxis.Min + InitialPosition.X;
      ParentConstraints.XAxis.Max = Constraints.XAxis.Max + InitialPosition.X;
    }
    if (Constraints.YAxis.bUseConstraint)
    {
      ParentConstraints.YAxis.bUseConstraint = true;
      ParentConstraints.YAxis.Min = Constraints.YAxis.Min + InitialPosition.Y;
      ParentConstraints.YAxis.Max = Constraints.YAxis.Max + InitialPosition.Y;
    }
    if (Constraints.ZAxis.bUseConstraint)
    {
      ParentConstraints.ZAxis.bUseConstraint = true;
      ParentConstraints.ZAxis.Min = Constraints.ZAxis.Min + InitialPosition.Z;
      ParentConstraints.ZAxis.Max = Constraints.ZAxis.Max + InitialPosition.Z;
    }

    return ParentConstraints;
  }

  static ScaleConstraints GenerateParentScaleConstraints(
      const ScaleConstraints& Constraints,
      const FVector& InitialScale)
  {
    ScaleConstraints ParentConstraints;

    if (Constraints.bUseConstraint)
    {
      ParentConstraints.bUseConstraint = true;
      ParentConstraints.Min = Constraints.Min * InitialScale.X;
      ParentConstraints.Max = Constraints.Max * InitialScale.X;
    }

    return ParentConstraints;
  }

  static FVector GetConstrainedTransformPosition(
      const FVector& UnconstrainedPosition,
      const PositionConstraints& PositionConstraints)
  {
    FVector ConstrainedPosition = UnconstrainedPosition;

    if (PositionConstraints.XAxis.bUseConstraint)
    {
      ConstrainedPosition.X = FMath::Clamp(
          ConstrainedPosition.X, PositionConstraints.XAxis.Min, PositionConstraints.XAxis.Max);
    }
    if (PositionConstraints.YAxis.bUseConstraint)
    {
      ConstrainedPosition.Y = FMath::Clamp(
          ConstrainedPosition.Y, PositionConstraints.YAxis.Min, PositionConstraints.YAxis.Max);
    }
    if (PositionConstraints.ZAxis.bUseConstraint)
    {
      ConstrainedPosition.Z = FMath::Clamp(
          ConstrainedPosition.Z, PositionConstraints.ZAxis.Min, PositionConstraints.ZAxis.Max);
    }

    return ConstrainedPosition;
  }

  static FVector GetConstrainedTransformPosition(
      const FVector& UnconstrainedPosition,
      const PositionConstraints& PositionConstraints,
      const FTransform& RelativeTransform)
  {
    FVector ConstrainedPosition = UnconstrainedPosition;

    if (PositionConstraints.HasConstraints())
    {
      ConstrainedPosition = RelativeTransform.InverseTransformPosition(ConstrainedPosition);
      ConstrainedPosition =
          GetConstrainedTransformPosition(ConstrainedPosition, PositionConstraints);
      ConstrainedPosition = RelativeTransform.TransformPosition(ConstrainedPosition);
    }

    return ConstrainedPosition;
  }

  static FQuat GetConstrainedTransformRotation(
      const FQuat& UnconstrainedRotation,
      const RotationConstraints& RotationConstraints)
  {
    auto NewRotationEuler = UnconstrainedRotation.Euler();

    if (RotationConstraints.XAxis.bUseConstraint)
    {
      NewRotationEuler.X = FMath::Clamp(
          NewRotationEuler.X, RotationConstraints.XAxis.Min, RotationConstraints.XAxis.Max);
    }
    if (RotationConstraints.YAxis.bUseConstraint)
    {
      NewRotationEuler.Y = FMath::Clamp(
          NewRotationEuler.Y, RotationConstraints.YAxis.Min, RotationConstraints.YAxis.Max);
    }
    if (RotationConstraints.ZAxis.bUseConstraint)
    {
      NewRotationEuler.Z = FMath::Clamp(
          NewRotationEuler.Z, RotationConstraints.ZAxis.Min, RotationConstraints.ZAxis.Max);
    }

    return FQuat::MakeFromEuler(NewRotationEuler);
  }

  static FQuat GetConstrainedTransformRotation(
      const FQuat& UnconstrainedRotation,
      const RotationConstraints& RotationConstraints,
      const FTransform& RelativeTransform)
  {
    FQuat ConstrainedRotation = UnconstrainedRotation;

    if (RotationConstraints.HasConstraints())
    {
      ConstrainedRotation = RelativeTransform.InverseTransformRotation(ConstrainedRotation);
      ConstrainedRotation =
          GetConstrainedTransformRotation(ConstrainedRotation, RotationConstraints);
      ConstrainedRotation = RelativeTransform.TransformRotation(ConstrainedRotation);
    }

    return ConstrainedRotation;
  }

  static FVector GetConstrainedTransformScale(
      const FVector& UnconstrainedScale,
      const ScaleConstraints& ScaleConstraints)
  {
    FVector ConstrainedScale = UnconstrainedScale;

    if (ScaleConstraints.bUseConstraint)
    {
      ConstrainedScale.X =
          FMath::Clamp(ConstrainedScale.X, ScaleConstraints.Min, ScaleConstraints.Max);
      ConstrainedScale.Y =
          FMath::Clamp(ConstrainedScale.Y, ScaleConstraints.Min, ScaleConstraints.Max);
      ConstrainedScale.Z =
          FMath::Clamp(ConstrainedScale.Z, ScaleConstraints.Min, ScaleConstraints.Max);
    }

    return ConstrainedScale;
  }

  static FQuat GetRotationTwistAroundAxis(
      const FQuat& Rotation,
      const FVector& Axis,
      bool bIsAxisNormalized = false)
  {
    const auto TwistAxis = bIsAxisNormalized ? Axis : Axis.GetSafeNormal();
    const auto TwistAngle = Rotation.GetTwistAngle(Axis);
    return FQuat(TwistAxis, TwistAngle);
  }

  /*
   * @brief This transformation ensures that the angle between the vectors Axis and (Rotation *
   * @brief Axis) is not larger than MaxAngleFromAxis
   */
  static FQuat GetConstrainedRotationFromAxis(
      const FQuat& UnconstrainedRotation,
      const FVector& Axis,
      float MaxAngleFromAxis)
  {
    const auto RotatedAxis = UnconstrainedRotation * Axis;

    const auto Rotation = FQuat::FindBetweenNormals(Axis, RotatedAxis);
    const auto AngleConstraint = FMath::DegreesToRadians(MaxAngleFromAxis);

    if (Rotation.GetAngle() > AngleConstraint)
    {
      auto ExtraRotation = Rotation.GetAngle() - AngleConstraint;
      return FQuat(Rotation.GetRotationAxis(), ExtraRotation).Inverse() * UnconstrainedRotation;
    }
    return UnconstrainedRotation;
  }

  static FVector GetAxis(EAxis::Type Axis)
  {
    auto UnitVector = FVector::Zero();
    UnitVector.SetComponentForAxis(Axis, 1.0);
    return UnitVector;
  }

  static FQuat GetConstrainedTwistRotation(
      const FQuat& UnconstrainedRotation,
      const FVector& Axis,
      FIsdkAxisConstraints Constraint)
  {
    if (Constraint.bUseConstraint)
    {
      const auto TwistAngleRad = UnconstrainedRotation.GetTwistAngle(Axis);
      const auto TwistAngleDeg = -1.0 * FMath::RadiansToDegrees(TwistAngleRad);
      const auto ConstrainedAngleDeg = FMath::Clamp((float)TwistAngleDeg, Constraint.Min, Constraint.Max);
      return FQuat(Axis, -1.0 * FMath::DegreesToRadians(ConstrainedAngleDeg));
    }
    else
    {
      return UnconstrainedRotation;
    }
  }
};
