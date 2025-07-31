/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "GameFramework/Actor.h"
#include "Dom/JsonObject.h"
#include "MRUtilityKitAnchorActorSpawner.h"

#include "OculusAnchorTypes.h"
#include "ProceduralMeshComponent.h"
#include "MRUtilityKitAnchor.generated.h"

class AMRUKRoom;
class UMRUKAnchorData;

UCLASS(ClassGroup = MRUtilityKit, meta = (DisplayName = "MR Utility Kit Anchor"))
class MRUTILITYKIT_API AMRUKAnchor : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * This contains the UUID of the anchor along with other meta data that can be used to
	 * identify it.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	FOculusSpaceQueryResult SpaceQueryResult;

	/**
	 * The semantic classification of the anchor, also sometimes refered to as labels for short.
	 * This can be for example FLOOR, COUCH, TABLE, SCREEN, BED, LAMP, etc...
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	TArray<FString> SemanticClassifications;

	/**
	 * If the anchor has a plane attached to it, this represents the bounds of that plane in
	 * local coordinate space.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	FBox2D PlaneBounds{ ForceInit };

	/**
	 * If the anchor has a plane attached to it, this represents the boundary of it in 
	 * local coordinate space. For rectangular boundaries this will be the same as the
	 * PlaneBounds.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	TArray<FVector2D> PlaneBoundary2D;

	/**
	 * If the anchor has a volume attached to it, this represents the bounds of that volume in
	 * local coordinate space.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	FBox VolumeBounds{ ForceInit };

	/**
	 * Procedural mesh that is generated from the anchor geometry.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadWrite, Category = "MR Utility Kit")
	UProceduralMeshComponent* ProceduralMeshComponent;

	/**
	 * Pointer to the parent anchor, e.g. if this is a door or window frame the parent will
	 * be a wall. If this is a screen it could have a desk parent.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	AMRUKAnchor* ParentAnchor;

	/**
	 * Array of all children attached to it, e.g. if this is a wall, it could have an array
	 * of door/window frames. If this is a desk it could have an array of screens on it.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	TArray<AMRUKAnchor*> ChildAnchors;

	/**
	 * The room this anchor is placed in.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "MR Utility Kit")
	AMRUKRoom* Room;

	/**
	 * Check if a 2D position is within the boundary of the plane. The position should be in
	 * the local coordinate system NOT world coordinates.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool IsPositionInBoundary(const FVector2D& Position);

	/**
	 * Generate a uniform random position within the boundary of the plane.
	 * @Return					The random position in local coordinate space.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	FVector GenerateRandomPositionOnPlane();

	/**
	 * Generate a uniform random position within the boundary of the plane from a random stream.
	 * @Param RandomStream		A random generator used to generate the position on the plane.
	 * @Return					The random position in local coordinate space.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	FVector GenerateRandomPositionOnPlaneFromStream(const FRandomStream& RandomStream);

	/**
	 * Cast a ray and return the closest hit against the volume and plane bounds.
	 * @Param Origin    Origin The origin of the ray.
	 * @Param Direction Direction The direction of the ray.
	 * @Param MaxDist   The maximum distance the ray should travel.
	 * @Param OutHit    The closest hit.
	 * @Return          Whether the ray hit anything
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool Raycast(const FVector& Origin, const FVector& Direction, float MaxDist, FMRUKHit& OutHit);

	/**
	 * Cast a ray and collect hits against the volume and plane bounds. The order of the hits in the array is not specified.
	 * @Param Origin    Origin The origin of the ray.
	 * @Param Direction Direction The direction of the ray.
	 * @Param MaxDist   The maximum distance the ray should travel.
	 * @Param OutHits   The hits the ray collected.
	 * @Return          Whether the ray hit anything
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool RaycastAll(const FVector& Origin, const FVector& Direction, float MaxDist, TArray<FMRUKHit>& OutHits);

	/**
	 * Attach a procedural mesh to the anchor. The mesh will match the size, position and shape of the volume and/or plane
	 * if they are set.
	 * @Param PlaneUVAdjustments    Scale and offset to apply to the UV texture coordinates. If more than one is specified
	 *								then multiple UV texture coordinates are created (up to 4) and adjustments applied to
	 *								each. This can be left empty in which case a single set of UV texture coordinates are
	 *								created in the range 0 to 1 for the plane.
	 * @Param GenerateCollision     Whethere to generate collision geometry or not
	 * @Param ProceduralMaterial    Material to use on the procedural generated mesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit", meta = (AutoCreateRefTerm = "PlaneUVAdjustments"))
	void AttachProceduralMesh(TArray<FMRUKPlaneUV> PlaneUVAdjustments, bool GenerateCollision = true, UMaterialInterface* ProceduralMaterial = nullptr);

	/**
	 * Check if the anchor has the given label.
	 * @Param Label The label to check.
	 * @Return Whether the anchor has the given label.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool HasLabel(const FString& Label) const;

	/**
	 * Check if the anchor has any of the given labels.
	 * @Param Labels The labels to check.
	 * @Return Whether the anchor has any of the given labels.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool HasAnyLabel(const TArray<FString>& Labels) const;

	/**
	 * Check if the anchor passes the given label filter
	 * @Param Labels The labels to check.
	 * @Return Whether the anchor has any of the given labels.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool PassesLabelFilter(const FMRUKLabelFilter& LabelFilter) const;

	/**
	 * Calculate the closest surface position on this anchor.
	 * @param TestPosition       The position in world space for which the closes surface position should be obtained.
	 * @param OutSurfacePosition The closest surface position
	 * @return                   The distance between TestPosition and OutSurfacePosition
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	float GetClosestSurfacePosition(const FVector& TestPosition, FVector& OutSurfacePosition);

	/**
	 *	Checks if the given position is on or inside the volume bounds.
	 *  Floor, ceiling and wall anchors will be excluded from the search.
	 *	@param WorldPosition      The position in world space to check
	 *	@param TestVerticalBounds Whether the vertical bounds should be checked or not
	 *	@return					  The anchor the WorldPosition is in. A null pointer otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	bool IsPositionInVolumeBounds(const FVector& Position, bool TestVerticalBounds = true, float Tolerance = 0.0);

	/**
	 * Gets a natural “forward” direction for anchors; for planes, this is always Z-forward. 
	 * For volumes, it’s the X/Y cardinal axis that aligns best with the normal of the closest wall.
	 * @Param Anchor The anchor which forward direction should be calculated.
	 * @Return       The forward facing direction.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	FVector GetFacingDirection() const;

	/**
	 * Spawn a mesh on the position of this anchor.
	 * The actor should have Z as up, Y as right and X as forward.
	 * @Param ActorClass The Class to spawn at the anchors position.
	 * @Param MatchAspectRatio If true the actor will be rotated to best match the aspect ratio of the volume (applies to volumes only).
	 * @Param CalculateFacingDirection If true then actor will be rotated to face away from the closest wall (applies to volumes only).
	 * @Param ScalingMode Sets how to scale the actor to fit the size of the volume/plane.
	 * @Return The spawned actor or null if nothing was spawned.
	 */
	UFUNCTION(BlueprintCallable, Category = "MR Utility Kit")
	AActor* SpawnInterior(const TSubclassOf<class AActor>& ActorClass, bool MatchAspectRatio = false, bool CalculateFacingDirection = false, EMRUKSpawnerScalingMode ScalingMode = EMRUKSpawnerScalingMode::Stretch);

public:
	AMRUKAnchor(const FObjectInitializer& ObjectInitializer);

	bool LoadFromData(UMRUKAnchorData* AnchorData);

	void AttachProceduralMesh(bool GenerateCollision = true, UMaterialInterface* ProceduralMaterial = nullptr);
	void GenerateProceduralAnchorMesh(UProceduralMeshComponent& ProceduralMesh, const TArray<FMRUKPlaneUV>& PlaneUVAdjustments, bool PreferVolume = false, bool GenerateCollision = true, double Offset = 0.0);

	TSharedRef<FJsonObject> JsonSerialize();

private:
	bool RayCastPlane(const FRay& LocalRay, float MaxDist, FMRUKHit& OutHit);
	bool RayCastVolume(const FRay& LocalRay, float MaxDist, FMRUKHit& OutHit);
	FVector GetDirectionAwayFromClosestWall(int* OutCardinalAxisIndex = nullptr, const TArray<int> ExcludedAxes = {}) const;

	struct TriangulatedMeshCache
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<float> Areas;
		float TotalArea;

		void Clear();
	};

	AActor* Interior = nullptr;
	TOptional<TriangulatedMeshCache> CachedMesh;
};
