/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "OculusAnchorTypes.generated.h"

#define OCULUS_UUID_SIZE 16

typedef uint8 ovrpUuidArray[OCULUS_UUID_SIZE];

USTRUCT(BlueprintType)
struct OCULUSANCHORS_API FUUID
{
	GENERATED_BODY()

	FUUID();
	FUUID(const ovrpUuidArray& UuidArray);
	
	bool operator==(const FUUID& Other) const;
	bool operator!=(const FUUID& Other) const;

	bool IsValidUUID() const;

	bool IsEqual(const FUUID& Other) const;
	friend uint32 GetTypeHash(const FUUID& Other);
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	OCULUSANCHORS_API friend FArchive& operator<<(FArchive& Ar, FUUID& UUID);
	bool Serialize(FArchive& Ar);

	FString ToString() const;

	uint8 UUIDBytes[OCULUS_UUID_SIZE];
};

template<>
struct TStructOpsTypeTraits<FUUID> : public TStructOpsTypeTraitsBase2<FUUID>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithSerializer = true
	};
};

USTRUCT( BlueprintType )
struct OCULUSANCHORS_API FUInt64
{
	GENERATED_BODY()

	FUInt64() : FUInt64(0) { }
	FUInt64( const uint64& Value ) { this->Value = Value; }

	operator uint64() const { return Value; }
	bool operator==(const FUInt64& Right) const;
	bool operator!=(const FUInt64& Right) const;

	UPROPERTY()
	uint64 Value;
	
	bool IsEqual(const FUInt64& Other) const
	{
		return Other.Value == Value;
	}

	friend uint32 GetTypeHash(const FUInt64& Other)
	{
		return FCrc::MemCrc_DEPRECATED(&Other.Value, sizeof(Other.Value));
	}

	uint64 GetValue() const { return Value; };
	
	void SetValue(const uint64 Val) { Value = Val; };
};

template<>
struct TStructOpsTypeTraits<FUInt64> : public TStructOpsTypeTraitsBase2<FUInt64>
{
	enum
	{
		WithIdenticalViaEquality = true,
	};
};

UENUM(BlueprintType)
enum class EOculusSpaceQueryFilterType : uint8
{
	None = 0                  UMETA(DisplayName = "No Filter"),
	FilterByIds = 1			  UMETA(DisplayName = "Filter queries by UUIDs"),
	FilterByComponentType = 2 UMETA(DisplayName = "Filter queries by component type")
};

// This is used as a bit-mask
UENUM(BlueprintType)
enum class EOculusSpaceStorageLocation : uint8
{
	Invalid = 0		UMETA(DisplayName = "Invalid"),
	Local = 1 << 0 	UMETA(DisplayName = "Local"),
	Cloud = 1 << 1 	UMETA(DisplayName = "Cloud")
};

UENUM(BlueprintType)
enum class EOculusSpaceStoragePersistenceMode : uint8
{
	Invalid = 0		UMETA(Hidden),
	Indefinite = 1	UMETA(DisplayName = "Indefinite"),
};

UENUM(BlueprintType)
enum class EOculusSpaceComponentType : uint8
{
	Locatable = 0				UMETA(DisplayName = "Locatable"),
	Storable = 1				UMETA(DisplayName = "Storable"),
	Sharable = 2				UMETA(DisplayName = "Sharable"),
	ScenePlane = 3				UMETA(DisplayName = "ScenePlane"),
	SceneVolume = 4				UMETA(DisplayName = "SceneVolume"),
	SemanticClassification = 5	UMETA(DisplayName = "SemanticClassification"),
	RoomLayout = 6				UMETA(DisplayName = "RoomLayout"),
	SpaceContainer = 7			UMETA(DisplayName = "SpaceContainer"),
	Undefined = 8				UMETA(DisplayName = "Not defined"),
	TriangleMesh = 9            UMETA(DisplayName = "TriangleMesh"),
};

USTRUCT(BlueprintType)
struct OCULUSANCHORS_API FOculusSpaceQueryInfo
{
	GENERATED_BODY()
public:
	FOculusSpaceQueryInfo() :
		MaxQuerySpaces(1024),
		Timeout(0),
		Location(EOculusSpaceStorageLocation::Local),
		FilterType(EOculusSpaceQueryFilterType::None)
	{}

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	int MaxQuerySpaces;

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	float Timeout;

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	EOculusSpaceStorageLocation Location;

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	EOculusSpaceQueryFilterType FilterType;

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	TArray<FUUID> IDFilter;

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	TArray<EOculusSpaceComponentType> ComponentFilter;
};

USTRUCT(BlueprintType)
struct OCULUSANCHORS_API FOculusSpaceQueryResult
{
	GENERATED_BODY()
public:
	FOculusSpaceQueryResult() : Space(0), UUID(), Location(EOculusSpaceStorageLocation::Invalid) {}
	FOculusSpaceQueryResult(FUInt64 SpaceHandle, FUUID ID, EOculusSpaceStorageLocation SpaceLocation) : Space(SpaceHandle), UUID(ID), Location(SpaceLocation) {}

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	FUInt64 Space;

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	FUUID UUID;

	UPROPERTY(BlueprintReadWrite, Category = "Oculus|SpatialAnchor")
	EOculusSpaceStorageLocation Location;
};

USTRUCT(BlueprintType)
struct OCULUSANCHORS_API FOculusSpaceQueryFilterValues
{
	GENERATED_BODY()
public:
	TArray<FUUID> Uuids; // used if filtering by UUIDs
	TArray<EOculusSpaceComponentType> ComponentTypes; // used if filtering by component types
};

// Represents a room layout within a specific space
USTRUCT(BlueprintType)
struct OCULUSANCHORS_API FOculusRoomLayout
{
	GENERATED_USTRUCT_BODY()

	FUUID FloorUuid;
	FUUID CeilingUuid;
	TArray<FUUID> WallsUuid;
	TArray<FUUID> RoomObjectUUIDs;
};