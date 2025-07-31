/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchorTypes.h"
#include "OculusHMDPrivate.h"

bool FUInt64::operator==(const FUInt64& Right) const { return IsEqual(Right); }
bool FUInt64::operator!=(const FUInt64& Right) const { return !IsEqual(Right); }

FUUID::FUUID()
{
	FMemory::Memzero(&UUIDBytes, OCULUS_UUID_SIZE);
}

FUUID::FUUID(const ovrpUuidArray& UuidArray)
{
	FMemory::Memcpy(UUIDBytes, UuidArray);
}

bool FUUID::operator==(const FUUID& Right) const 
{ 
	return IsEqual(Right); 
}

bool FUUID::operator!=(const FUUID& Right) const 
{ 
	return !IsEqual(Right); 
}

bool FUUID::IsValidUUID() const
{
	static uint8 InvalidUUID[OCULUS_UUID_SIZE] = { 0 };

	return FMemory::Memcmp(UUIDBytes, InvalidUUID, OCULUS_UUID_SIZE) != 0;
}

bool FUUID::IsEqual(const FUUID& Other) const
{
	return FMemory::Memcmp(UUIDBytes, Other.UUIDBytes, OCULUS_UUID_SIZE) == 0;
}

uint32 GetTypeHash(const FUUID& Other)
{
	return FCrc::MemCrc32(&Other.UUIDBytes, sizeof(Other.UUIDBytes));
}

bool FUUID::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	uint8 data[16] = { 0 };

	for (uint8 i = 0; i < OCULUS_UUID_SIZE; ++i)
	{
		data[i] = UUIDBytes[i];
	};

	for (uint8 i = 0; i < OCULUS_UUID_SIZE; ++i)
	{
		Ar << data[i];
	};

	for (uint8 i = 0; i < OCULUS_UUID_SIZE; ++i)
	{
		UUIDBytes[i] = data[i];
	};

	bOutSuccess = true;

	return true;
}

FArchive& operator<<(FArchive& Ar, FUUID& UUID)
{
	bool bOutSuccess = false;
	UUID.NetSerialize(Ar, nullptr, bOutSuccess);

	return Ar;
}

bool FUUID::Serialize(FArchive& Ar)
{
	Ar << *this;
	return true;
}

FString FUUID::ToString() const
{
	return BytesToHex(UUIDBytes, OCULUS_UUID_SIZE);
}