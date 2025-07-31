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
#include "IsdkTransformer.h"
#include "IsdkTransformerUtils.h"
#include "IsdkITransformer.h"
#include "IsdkGrabFreeTransformer.generated.h"

/**
 * @struct FIsdkGrabPointDelta
 * @brief A struct containing the deltas (rotation and centroid offset) of a grab point, used by
 * GrabTransformers to track changes in grab heuristics.
 * @see UIsdkGrabFreeTransformer
 */
USTRUCT()
struct FIsdkGrabPointDelta
{
  GENERATED_BODY()

  FIsdkGrabPointDelta() : FIsdkGrabPointDelta(FVector::Zero(), FQuat::Identity) {}
  FIsdkGrabPointDelta(const FVector& CentroidPoseOffset, const FQuat& PoseRotation)
  {
    PrevCentroidOffset = CentroidOffset = CentroidPoseOffset;
    PrevRotation = Rotation = PoseRotation;
  }

  static constexpr float _epsilon = 0.000001f;

  FVector PrevCentroidOffset;
  FVector CentroidOffset;
  FQuat PrevRotation;
  FQuat Rotation;

  /**
   * @brief Called by transformer update methods, passes in new offset and rotations, and builds
   * respective deltas from them
   */
  void UpdateData(const FVector& CentroidPoseOffset, const FQuat& PoseRotation)
  {
    PrevCentroidOffset = CentroidOffset;
    CentroidOffset = CentroidPoseOffset;
    PrevRotation = Rotation;

    auto NewRotation = PoseRotation;

    // Quaternions have two ways of expressing the same rotation.
    // This code ensures that the result is the same rotation but expressed in the desired sign.
    if ((NewRotation | Rotation) < 0)
    {
      NewRotation.X = -NewRotation.X;
      NewRotation.Y = -NewRotation.Y;
      NewRotation.Z = -NewRotation.Z;
      NewRotation.W = -NewRotation.W;
    }

    Rotation = NewRotation;
  }

  /** @brief Determines if the current centroid offset has a valid axis */
  bool IsValidAxis() const
  {
    return CentroidOffset.SizeSquared() > _epsilon;
  }
};

/**
 * @struct FIsdkGrabFreeTransformerConfig
 * @brief A struct defining how the a Grab Free Transformer should function, including axis
 * constraints and relative versus world translations.
 * @see UIsdkGrabFreeTransformer
 */
USTRUCT(BlueprintType)
struct FIsdkGrabFreeTransformerConfig
{
  GENERATED_BODY()

  FIsdkGrabFreeTransformerConfig(
      bool bUseRelativeTranslation = true,
      const FIsdkConstraintAxes& TranslateConstraints = FIsdkConstraintAxes(),
      const FIsdkConstraintAxes& RotateConstraints = FIsdkConstraintAxes(),
      bool bUseRelativeScale = true,
      const FIsdkAxisConstraints& ScaleConstraints = FIsdkAxisConstraints())
      : bUseRelativeTranslation(bUseRelativeTranslation),
        TranslateConstraintAxes(TranslateConstraints),
        RotateConstraintAxes(RotateConstraints),
        bUseRelativeScale(bUseRelativeScale),
        ScaleConstraint(ScaleConstraints)
  {
  }
  /** If this transformer should use relative translations (versus world)*/
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool bUseRelativeTranslation = false;

  /** Constraints that should be applied to translation/location axes of this transformer*/
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkConstraintAxes TranslateConstraintAxes;

  /** Constraints that should be applied to rotation axes of this transformer*/
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkConstraintAxes RotateConstraintAxes;

  /** Whether or not this transformer's scale is treated as relative */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  bool bUseRelativeScale = false;

  /** Constraints that should be applied to scale axes of this transformer*/
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  FIsdkAxisConstraints ScaleConstraint;
};

/**
 * @class UIsdkGrabFreeTransformer
 * @brief Scene component utilized for transforming a grabbable object
 *
 * Transforming including scale and axis constraints and physics considerations, with multiple
 * potential configurations
 * @see UIsdkTransformer
 * @addtogroup InteractionSDK
 */
UCLASS(
    Blueprintable,
    CollapseCategories,
    ClassGroup = (InteractionSDK),
    meta = (DisplayName = "ISDK Grab Free Transformer"))
class OCULUSINTERACTION_API UIsdkGrabFreeTransformer : public UIsdkTransformer
{
  GENERATED_BODY()

 public:
  UIsdkGrabFreeTransformer();

  /**
   * @brief Returns whether or not this transformer is currently active.
   * @return bool If this transformer is active
   */
  virtual bool IsTransformerActive() const override;

  /**
   * @brief Returns maximum grab points, -1 if infinite (GrabFreeTransformer always returns -1).
   * @return int Number of points
   */
  virtual int MaxGrabPoints() const override;

  /**
   * @brief Initialize the transformation.
   *
   * Empties selected poses and deltas, adds
   * @param GrabPoses Array of GrabPose structs to initialize this transform with
   * @param Target Initial transform of the target object
   * @see IIsdkITransformer
   */
  virtual void BeginTransform(
      const TArray<FIsdkGrabPose>& GrabPoses,
      const FIsdkTargetTransform& Target) override;

  /**
   * @brief Generate the transform update,the result should be a FTransform relative to the parent.
   * @param GrabPoses Array of grab poses to update this transform with
   * @param Target Current transform of the target object
   * @return FTransform Resulting updated transform
   */
  virtual FTransform UpdateTransform(
      const TArray<FIsdkGrabPose>& GrabPoses,
      const FIsdkTargetTransform& Target) override;

  /**
   * @brief Flush any local state and generate a final update to the target transform, for example
   * finishing some interpolation.
   * @return FTransform The final resulting transform
   */
  virtual FTransform EndTransform(const FIsdkTargetTransform& Target) override;

  /**
   * @brief Generate constraints based on the state of the target and how this transformer is
   * configured.
   * @param Target The current transform of the target object
   */
  virtual void UpdateConstraints(const FIsdkTargetTransform& Target) override;

  /** Returns the number of grab points on this transformer */
  UFUNCTION(BlueprintPure, Category = InteractionSDK)
  int GetGrabCount() const;

  /** Returns the current interactable state, driven by events */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = InteractionSDK,
      meta = (ShowOnlyInnerProperties))
  FIsdkGrabFreeTransformerConfig Config;

 private:
  TArray<FIsdkGrabPose> SelectedPoses;
  FVector RelativeCentroidToTargetVector = FVector::ZeroVector;
  FQuat InitialTargetWorldRotation = FQuat::Identity;
  FQuat LastRotation = FQuat::Identity;
  FVector LastScale = FVector::ZeroVector;
  TArray<FIsdkGrabPointDelta> Deltas;

  // These might be the constraints from the config or relative constraints depending on the
  // configuration
  FIsdkConstraintAxes TargetTranslateConstraintAxes;
  FIsdkAxisConstraints TargetScaleConstraint;

  // helper functions
  void UpdateSelectedPoses(const TArray<FIsdkGrabPose>& GrabPoses);
  static FVector GetCentroid(const TArray<FIsdkGrabPose>& Poses);
  float GetDeltaScale() const;
  FQuat GetDeltaRotation() const;
  void UpdateGrabPointDeltas(const FVector& Centroid);
  void DrawTransformAxis(const FTransform& Pose, float Thickness = 0.3f) const;
};
