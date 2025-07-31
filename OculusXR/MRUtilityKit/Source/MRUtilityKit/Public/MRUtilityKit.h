/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GameFramework/Actor.h"
//#include "VectorUtil.h"
#include "MRUtilityKit.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMRUK, Log, All);

UENUM(BlueprintType)
enum class EMRUKInitStatus : uint8
{
	/// Not Initialized.
	None,
	/// Is busy Initializing.
	Busy,
	/// Has finished Initializing.
	Complete,
	/// Failed to initialize.
	Failed,
};

UENUM(BlueprintType)
enum class EMRUKCoordModeU : uint8
{
	/// The texture coordinates start at 0 and increase by 1 unit every meter.
	Metric,
	/// The texture coordinates start at 0 and increase by 1 unit every meter but are adjusted to end on a whole number to avoid seams.
	MetricSeamless,
	/// The texture coordinates are adjusted to the other dimensions to ensure the aspect ratio is maintained.
	MaintainAspectRatio,
	/// The texture coordinates are adjusted to the other dimensions to ensure the aspect ratio is maintained but are adjusted to end on a whole number to avoid seams.
	MaintainAspectRatioSeamless,
	/// The texture coordinates range from 0 to 1.
	Stretch,
};

UENUM(BlueprintType)
enum class EMRUKCoordModeV : uint8
{
	/// The texture coordinates start at 0 and increase by 1 unit every meter.
	Metric,
	/// The texture coordinates are adjusted to the other dimensions to ensure the aspect ratio is maintained.
	MaintainAspectRatio,
	/// The texture coordinates range from 0 to 1.
	Stretch,
};

UENUM(BlueprintType)
enum class EMRUKSpawnerSelectionMode : uint8
{
	/// Pick one at random.
	Random,
	/// Pick the closest size.
	ClosestSize,
};

UENUM(BlueprintType)
enum class EMRUKSpawnerScalingMode : uint8
{
	/// Stretch each axis to exactly match the size of the Plane/Volume.
	Stretch,
	/// Scale each axis by the same amount to maintain the correct aspect ratio.
	UniformScaling,
	/// Scale the X and Y axes uniformly but the Z scale can be different.
	UniformXYScale,
	/// Don't perform any scaling.
	NoScaling,
};

USTRUCT(BlueprintType)
struct MRUTILITYKIT_API FMRUKHit
{
	GENERATED_BODY()

	/**
	 * The position where the raycast hit.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MR Utility Kit")
	FVector HitPosition;

	/**
	 * The normal of the surface that was hit.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MR Utility Kit")
	FVector HitNormal;

	/**
	 * The distance between the origin of the ray to the hit position.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "MR Utility Kit")
	float HitDistance;
};

USTRUCT(BlueprintType)
struct MRUTILITYKIT_API FMRUKLabelFilter
{
	GENERATED_BODY()

	/**
	 * If included labels is not empty then the anchor must have at
	 * least one of the labels in this list.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	TArray<FString> IncludedLabels;

	/**
	 * Anchors with any of the labels in this exclusion list
	 * will be ignored.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	TArray<FString> ExcludedLabels;

	/**
	 * Check if the labels pass the given label filter
	 * @Param Labels The labels to check.
	 * @Return Whether the filter passes or not.
	 */
	bool PassesFilter(const TArray<FString>& Labels) const;
};

USTRUCT(BlueprintType)
struct MRUTILITYKIT_API FMRUKPlaneUV
{
	GENERATED_BODY()

	/**
	 * Offset applied to the UV texture coordinates.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	FVector2D Offset = FVector2D::ZeroVector;

	/**
	 * Scale applied to the UV texture coordinates.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	FVector2D Scale = FVector2D::UnitVector;
};

USTRUCT(BlueprintType)
struct MRUTILITYKIT_API FMRUKTexCoordModes
{
	GENERATED_BODY()

	/**
	 * Texture Coordinate mode for the U direction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	EMRUKCoordModeU U = EMRUKCoordModeU::Metric;

	/**
	 * Texture Coordinate mode for the V direction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	EMRUKCoordModeV V = EMRUKCoordModeV::Metric;
};

USTRUCT(BlueprintType)
struct MRUTILITYKIT_API FMRUKSpawnActor
{
	GENERATED_BODY()

	/**
	 * The class of actor to spawn.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	TSubclassOf<AActor> Actor;

	/**
	 * When match aspect ratio is enabled then the actor will be rotated
	 * to try and match the aspect ratio of the volume as closely as possible.
	 * This is most useful for long and thin volumes, keep this disabled for
	 * objects with an aspect ratio close to 1:1. Only applies to volumes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	bool MatchAspectRatio;

	/**
	 * When calculate facing direction is enabled the actor will be rotated to
	 * face away from the closest wall. If match aspect ratio is also enabled
	 * then that will take precedence and it will be constrained to a choice
	 * between 2 directions only. Only applies to volumes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	bool CalculateFacingDirection;

	/**
	 * Set what scaling mode to apply to the actor. By default the actor will
	 * be stretched to fit the size of the plane/volume. But in some cases
	 * this may not be desirable and can be customized here.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	EMRUKSpawnerScalingMode ScalingMode;
};

USTRUCT(BlueprintType)
struct MRUTILITYKIT_API FMRUKSpawnGroup
{
	GENERATED_BODY()

	/**
	 * List of actors to choose from, multiple actors can be specified and
	 * the selection criteria will be determined by the SelectionMode option.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	TArray<FMRUKSpawnActor> Actors;

	/**
	 * Set the selection mode when multiple different actors are specified.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	EMRUKSpawnerSelectionMode SelectionMode = EMRUKSpawnerSelectionMode::Random;

	/**
	 * If an actor is not specified then fallback to using a procedural mesh instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR Utility Kit")
	bool FallbackToProcedural = true;
};

/**
 * Implements the settings for the MRUtilityKit plugin.
 */
UCLASS(config = Game, defaultconfig)
class MRUTILITYKIT_API UMRUKSettings : public UObject
{
	GENERATED_BODY()

public:
	UMRUKSettings(const FObjectInitializer& obj);

	/**
	 * When world locking is enabled the position of the VR Pawn will be adjusted each frame to ensure
	 * the room anchors are where they should be relative to the camera position. This is necessary to
	 * ensure the position of the virtual objects in the world do not get out of sync with the real world.
	 */
	UPROPERTY(config, EditAnywhere, Category = "MR Utility Kit")
	bool EnableWorldLock = true;
};

struct MRUTILITYKIT_API FMRUKLabels
{
	static const FString Floor;
	static const FString WallFace;
	static const FString Ceiling;
	static const FString DoorFrame;
	static const FString WindowFrame;
	static const FString Couch;
	static const FString Table;
	static const FString Screen;
	static const FString Bed;
	static const FString Lamp;
	static const FString Plant;
	static const FString Storage;
	static const FString WallArt;
	static const FString GlobalMesh;
	static const FString Other;
};

class FMRUKModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
