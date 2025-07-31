/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once
#include "UObject/Class.h"
#include "OculusAnchorTypes.h"
#include "OculusAnchorComponents.generated.h"

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusBaseAnchorComponent : public UObject
{
	GENERATED_BODY()
public:
	template <typename T>
	static T* FromSpace(uint64 space, UObject* Outer)
	{
		T* Component = NewObject<T>(Outer);
		Component->Space = space;
		return Component;
	}

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	bool IsComponentEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	EOculusSpaceComponentType GetType() const;

	uint64 GetSpace() const;

protected:
	uint64 Space;
	EOculusSpaceComponentType Type = EOculusSpaceComponentType::Undefined;
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusLocatableAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusLocatableAnchorComponent()
	{
		Type = EOculusSpaceComponentType::Locatable;
	}

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	bool GetTransform(FTransform& outTransform) const;
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusPlaneAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusPlaneAnchorComponent()
	{
		Type = EOculusSpaceComponentType::ScenePlane;
	}

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	bool GetPositionAndSize(FVector& outPosition, FVector& outSize) const;
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusVolumeAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusVolumeAnchorComponent()
	{
		Type = EOculusSpaceComponentType::SceneVolume;
	}

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	bool GetPositionAndSize(FVector& outPosition, FVector& outSize) const;
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusSemanticClassificationAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusSemanticClassificationAnchorComponent()
	{
		Type = EOculusSpaceComponentType::SemanticClassification;
	}

	UFUNCTION(BlueprintCallable, Category = "OculusVR|SpatialAnchor")
	bool GetSemanticClassifications(TArray<FString>& outClassifications) const;
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusXRRoomLayoutAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusXRRoomLayoutAnchorComponent()
	{
		Type = EOculusSpaceComponentType::RoomLayout;
	}

	UFUNCTION(BlueprintCallable, Category = "OculusVR|SpatialAnchor")
	bool GetRoomLayout(FUUID& outFloorUUID, FUUID& outCeilingUUID, TArray<FUUID>& outWallsUUIDs) const;
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusSpaceContainerAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusSpaceContainerAnchorComponent()
	{
		Type = EOculusSpaceComponentType::SpaceContainer;
	}

	UFUNCTION(BlueprintCallable, Category = "OculusXR|SpatialAnchor")
	bool GetUUIDs(TArray<FUUID>& outUUIDs) const;
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusXRSharableAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusXRSharableAnchorComponent()
	{
		Type = EOculusSpaceComponentType::Sharable;
	}
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusStorableAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusStorableAnchorComponent()
	{
		Type = EOculusSpaceComponentType::Storable;
	}
};

UCLASS(Blueprintable)
class OCULUSANCHORS_API UOculusTriangleMeshAnchorComponent : public UOculusBaseAnchorComponent
{
	GENERATED_BODY()
public:
	UOculusTriangleMeshAnchorComponent()
	{
		Type = EOculusSpaceComponentType::TriangleMesh;
	}
};
