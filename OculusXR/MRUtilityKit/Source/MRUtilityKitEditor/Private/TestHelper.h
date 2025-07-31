/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "GameFramework/Actor.h"
#include "MRUtilityKitAnchor.h"
#include "TestHelper.generated.h"

UCLASS()
class URoomAndAnchorObserver : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<AMRUKAnchor*> AnchorsCreated;
	UPROPERTY()
	TArray<AMRUKAnchor*> AnchorsUpdated;
	UPROPERTY()
	TArray<AMRUKAnchor*> AnchorsRemoved;

	UPROPERTY()
	TArray<AMRUKRoom*> RoomsCreated;
	UPROPERTY()
	TArray<AMRUKRoom*> RoomsUpdated;
	UPROPERTY()
	TArray<AMRUKRoom*> RoomsRemoved;

	UFUNCTION()
	void OnAnchorCreated(AMRUKAnchor* Anchor);
	UFUNCTION()
	void OnAnchorUpdated(AMRUKAnchor* Anchor);
	UFUNCTION()
	void OnAnchorRemoved(AMRUKAnchor* Anchor);
	UFUNCTION()
	void OnRoomCreated(AMRUKRoom* Room);
	UFUNCTION()
	void OnRoomUpdated(AMRUKRoom* Room);
	UFUNCTION()
	void OnRoomRemoved(AMRUKRoom* Room);

	void Clear();
};

extern const TCHAR* ExampleRoomJson;
extern const TCHAR* ExampleRoomFurnitureAddedJson;
extern const TCHAR* ExampleRoomMoreFurnitureAddedJson;
extern const TCHAR* ExampleRoomFurnitureModifiedJson;
extern const TCHAR* ExampleOtherRoomJson;
