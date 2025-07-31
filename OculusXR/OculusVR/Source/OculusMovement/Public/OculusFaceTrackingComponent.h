/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"

#include "OculusMorphTargetsController.h"
#include "OculusMovementTypes.h"

#include "OculusFaceTrackingComponent.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Oculus Face Tracking Component"), ClassGroup = OculusHMD)
class OCULUSMOVEMENT_API UOculusFaceTrackingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UOculusFaceTrackingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Set face expression value with expression key and value(0-1).
	 *
	 * @param Expression : The expression key that will be modified.
	 * @param Value : The new value to assign to the expression, 0 will remove all changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|OculusFaceTracking", meta = (UnsafeDuringActorConstruction = "true"))
	void SetExpressionValue(EOculusFaceExpression Expression, float Value);

	/**
	 * Get a face expression value given an expression key.
	 *
	 * @param Expression : The expression key that will be queried.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|OculusFaceTracking")
	float GetExpressionValue(EOculusFaceExpression Expression) const;

	/**
	 * Clears all face expression values.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|OculusFaceTracking")
	void ClearExpressionValues();

	/**
	 * The name of the skinned mesh component that this component targets for facial expression.
	 * This must be the name of a component on this actor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oculus|Movement")
	FName TargetMeshComponentName;

	/**
	 * If the face data is invalid for at least this or longer than this time then all face blendshapes/morph targets are reset to zero.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oculus|Movement")
	float InvalidFaceDataResetTime;

	/**
	 * The list of expressions that this component supports.
	 * Names are validated on startup so only valid morph targets on the skeletal mesh will be targeted.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oculus|Movement")
	TMap<EOculusFaceExpression, FName> ExpressionNames;

	/**
	* This flag determines if the face should be updated or not during the components tick.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oculus|Movement")
	bool bUpdateFace;

private:
	bool InitializeFaceTracking();

	// The mesh component targeted for expressions
	UPROPERTY()
	USkinnedMeshComponent* TargetMeshComponent;

	// Which mapped expressions are valid
	TStaticArray<bool, static_cast<uint32>(EOculusFaceExpression::COUNT)> ExpressionValid;

	// Morph targets controller
	FOculusMorphTargetsController MorphTargets;

	FOculusFaceState FaceState;

	// Timer that counts up until we reset morph curves if we've failed to get face state
	float InvalidFaceStateTimer;

	// Stop the tracker just once.
	static int TrackingInstanceCount;
};
