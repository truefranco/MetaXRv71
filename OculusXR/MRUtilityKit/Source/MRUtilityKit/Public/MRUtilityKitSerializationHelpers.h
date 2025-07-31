/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#pragma once

#include "Dom/JsonObject.h"
#include "OculusAnchorTypes.h"
#include "OculusRoomLayoutManagerComponent.h"

TSharedPtr<FJsonValue> MRUKSerialize(const FString& String);

void MRUKDeserialize(const FJsonValue& Value, FString& String);

TSharedPtr<FJsonValue> MRUKSerialize(const FUUID& UUID);

void MRUKDeserialize(const FJsonValue& Value, FUUID& UUID);

TSharedPtr<FJsonValue> MRUKSerialize(const double& Number);

void MRUKDeserialize(const FJsonValue& Value, float& Number);

TSharedPtr<FJsonValue> MRUKSerialize(const FOculusRoomLayout& RoomLayout);

void MRUKDeserialize(const FJsonValue& Value, FOculusRoomLayout& RoomLayout);

//template <typename T>
TSharedPtr<FJsonValue> MRUKSerialize(const FVector2D& Vector)
{
	return MakeShareable(new FJsonValueArray({ MakeShareable(new FJsonValueNumber(Vector.X)), MakeShareable(new FJsonValueNumber(Vector.Y)) }));
}

//template <typename T>
void MRUKDeserialize(const FJsonValue& Value, FVector2D& Vector)
{
	auto Array = Value.AsArray();
	if (Array.Num() == 2)
	{
		MRUKDeserialize(*Array[0], Vector.X);
		MRUKDeserialize(*Array[1], Vector.Y);
	}
	else
	{
		UE_LOG(LogJson, Error, TEXT("Json Array is of length %d (expected 2) when deserializing TVector2"), Array.Num());
		Vector = FVector2D::ZeroVector;
	}
}

//template <typename T>
TSharedPtr<FJsonValue> MRUKSerialize(const FVector& Vector)
{
	return MakeShareable(new FJsonValueArray({ MakeShareable(new FJsonValueNumber(Vector.X)), MakeShareable(new FJsonValueNumber(Vector.Y)), MakeShareable(new FJsonValueNumber(Vector.Z)) }));
}

//template <typename T>
void MRUKDeserialize(const FJsonValue& Value, FVector& Vector)
{
	auto Array = Value.AsArray();
	if (Array.Num() == 3)
	{
		MRUKDeserialize(*Array[0], Vector.X);
		MRUKDeserialize(*Array[1], Vector.Y);
		MRUKDeserialize(*Array[2], Vector.Z);
	}
	else
	{
		UE_LOG(LogJson, Error, TEXT("Json Array is of length %d (expected 3) when deserializing TVector"), Array.Num());
		Vector = FVector::ZeroVector;
	}
}

//template <typename T>
TSharedPtr<FJsonValue> MRUKSerialize(const FRotator& Rotation)
{
	return MakeShareable(new FJsonValueArray({ MakeShareable(new FJsonValueNumber(Rotation.Pitch)), MakeShareable(new FJsonValueNumber(Rotation.Yaw)), MakeShareable(new FJsonValueNumber(Rotation.Roll)) }));
}

//template <typename T>
void MRUKDeserialize(const FJsonValue& Value, FRotator& Rotation)
{
	auto Array = Value.AsArray();
	if (Array.Num() == 3)
	{
		MRUKDeserialize(*Array[0], Rotation.Pitch);
		MRUKDeserialize(*Array[1], Rotation.Yaw);
		MRUKDeserialize(*Array[2], Rotation.Roll);
	}
	else
	{
		UE_LOG(LogJson, Error, TEXT("Json Array is of length %d (expected 3) when deserializing TRotator"), Array.Num());
		Rotation = FRotator::ZeroRotator;
	}
}

//template <typename T>
TSharedPtr<FJsonValue> MRUKSerialize(const FBox2D& Box)
{
	if (Box.bIsValid)
	{
		TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetField("Min", MRUKSerialize(Box.Min));
		JsonObject->SetField("Max", MRUKSerialize(Box.Max));
		return MakeShareable(new FJsonValueObject(JsonObject));
	}
	else
	{
		return MakeShareable(new FJsonValueNull());
	}
}

//template <typename T>
void MRUKDeserialize(const FJsonValue& Value, FBox2D& Box)
{
	if (Value.IsNull())
	{
		Box.Init();
	}
	else
	{
		auto Object = Value.AsObject();
		MRUKDeserialize(*Object->GetField<EJson::None>("Min"), Box.Min);
		MRUKDeserialize(*Object->GetField<EJson::None>("Max"), Box.Max);
		Box.bIsValid = true;
	}
}

//template <typename T>
TSharedPtr<FJsonValue> MRUKSerialize(const FBox& Box)
{
	if (Box.IsValid)
	{
		TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetField("Min", MRUKSerialize(Box.Min));
		JsonObject->SetField("Max", MRUKSerialize(Box.Max));
		return MakeShareable(new FJsonValueObject(JsonObject));
	}
	else
	{
		return MakeShareable(new FJsonValueNull());
	}
}

//template <typename T>
void MRUKDeserialize(const FJsonValue& Value, FBox& Box)
{
	if (Value.IsNull())
	{
		Box.Init();
	}
	else
	{
		auto Object = Value.AsObject();
		MRUKDeserialize(*Object->GetField<EJson::None>("Min"), Box.Min);
		MRUKDeserialize(*Object->GetField<EJson::None>("Max"), Box.Max);
		Box.IsValid = 1;
	}
}

//template <typename T>
TSharedPtr<FJsonValue> MRUKSerialize(const FTransform& Transform)
{
	TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetField("Translation", MRUKSerialize(Transform.GetTranslation()));
	JsonObject->SetField("Rotation", MRUKSerialize(Transform.Rotator()));
	JsonObject->SetField("Scale", MRUKSerialize(Transform.GetScale3D()));
	return MakeShareable(new FJsonValueObject(JsonObject));
}

//template <typename T>
void MRUKDeserialize(const FJsonValue& Value, FTransform& Transform)
{
	auto Object = Value.AsObject();
	FVector Translation;
	FRotator Rotation;
	FVector Scale;
	MRUKDeserialize(*Object->GetField<EJson::None>("Translation"), Translation);
	MRUKDeserialize(*Object->GetField<EJson::None>("Rotation"), Rotation);
	MRUKDeserialize(*Object->GetField<EJson::None>("Scale"), Scale);

	Transform.SetComponents(FQuat(Rotation), Translation, Scale);
}

template <typename T>
TSharedPtr<FJsonValue> MRUKSerialize(const TArray<T>& Array)
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	JsonArray.Reserve(Array.Num());
	for (const auto& Item : Array)
	{
		JsonArray.Add(MRUKSerialize(Item));
	}
	return MakeShareable(new FJsonValueArray(JsonArray));
}

template <typename T>
void MRUKDeserialize(const FJsonValue& Value, TArray<T>& OutArray)
{
	auto Array = Value.AsArray();
	OutArray.Empty();
	OutArray.Reserve(Array.Num());
	for (const auto& Item : Array)
	{
		T ItemDeserialized;
		MRUKDeserialize(*Item, ItemDeserialized);
		OutArray.Push(ItemDeserialized);
	}
}
