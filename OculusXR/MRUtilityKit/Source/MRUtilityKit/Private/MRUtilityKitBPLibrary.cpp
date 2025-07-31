/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "MRUtilityKitBPLibrary.h"
#include "MRUtilityKit.h"
#include "MRUtilityKitSubsystem.h"
#include "MRUtilityKitSerializationHelpers.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	TArray<FVector> RecalculateNormals(const TArray<FVector>& Vertices, const TArray<uint32>& Triangles)
	{
		TArray<FVector> Normals;

		// Initialize the normals array with zero vectors
		Normals.Init(FVector::ZeroVector, Vertices.Num());

		// Iterate through each triangle
		for (int32 TriIndex = 0; TriIndex < Triangles.Num(); TriIndex += 3)
		{
			// Get the vertices of the triangle
			FVector VertexA = Vertices[Triangles[TriIndex]];
			FVector VertexB = Vertices[Triangles[TriIndex + 1]];
			FVector VertexC = Vertices[Triangles[TriIndex + 2]];

			// Calculate the triangle's normal
			FVector TriangleNormal = FVector::CrossProduct(VertexC - VertexA, VertexB - VertexA).GetSafeNormal();

			// Add the triangle's normal to each of its vertices' normals
			Normals[Triangles[TriIndex]] += TriangleNormal;
			Normals[Triangles[TriIndex + 1]] += TriangleNormal;
			Normals[Triangles[TriIndex + 2]] += TriangleNormal;
		}

		// Normalize the vertex normals
		for (FVector& Normal : Normals)
		{
			if (!Normal.IsNearlyZero())
			{
				Normal.Normalize();
			}
			else
			{
				Normal = FVector::UpVector;
			}
		}

		return Normals;
	}

	TArray<FProcMeshTangent> RecalculateTangents(const TArray<FVector>& Normals)
	{
		TArray<FProcMeshTangent> Tangents;

		// Initialize the tangents array with zero tangents
		Tangents.Init(FProcMeshTangent(0.f, 0.f, 0.f), Normals.Num());

		// Iterate through each normal
		for (int32 NormalIndex = 0; NormalIndex < Normals.Num(); NormalIndex++)
		{
			const FVector& Normal = Normals[NormalIndex];

			// Calculate a tangent based on the normal
			FVector TangentX = FVector(1.0f, 0.0f, 0.0f);
			FVector TangentY = FVector(0.0f, 1.0f, 0.0f);

			// Gram-Schmidt orthogonalization
			TangentX -= Normal * FVector::DotProduct(TangentX, Normal);
			if (!TangentX.IsNearlyZero())
			{
				TangentX.Normalize();
			}
			else
			{
				TangentX = FVector::UpVector;
			}

			// Calculate the bitangent (TangentY) as the cross product of normal and tangent
			TangentY = FVector::CrossProduct(Normal, TangentX);

			// Store the tangent in the array
			Tangents[NormalIndex] = FProcMeshTangent(TangentX, false);
		}

		return Tangents;
	}
} // namespace

UMRUKLoadFromDevice* UMRUKLoadFromDevice::LoadSceneFromDeviceAsync(const UObject* WorldContext)
{
	// We must have a valid contextual world for this action, so we don't even make it
	// unless we can resolve the UWorld from WorldContext.
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return nullptr;
	}

	// Create a new UMyDelayAsyncAction, and store function arguments in it.
	UMRUKLoadFromDevice* NewAction = NewObject<UMRUKLoadFromDevice>();
	NewAction->World = World;
	NewAction->RegisterWithGameInstance(World->GetGameInstance());
	return NewAction;
}

void UMRUKLoadFromDevice::Activate()
{
	const auto Subsystem = World->GetGameInstance()->GetSubsystem<UMRUKSubsystem>();
	Subsystem->OnSceneLoaded.AddDynamic(this, &UMRUKLoadFromDevice::OnSceneLoaded);
	Subsystem->LoadSceneFromDevice();
}

void UMRUKLoadFromDevice::OnSceneLoaded(bool Succeeded)
{
	const auto Subsystem = World->GetGameInstance()->GetSubsystem<UMRUKSubsystem>();
	Subsystem->OnSceneLoaded.RemoveDynamic(this, &UMRUKLoadFromDevice::OnSceneLoaded);
	if (Succeeded)
	{
		Success.Broadcast();
	}
	else
	{
		Failure.Broadcast();
	}
	SetReadyToDestroy();
}

bool UMRUKBPLibrary::LoadGlobalMeshFromDevice(FOculusSpaceQueryResult SpaceQuery, UProceduralMeshComponent* OutProceduralMesh, bool LoadCollision, const UObject* WorldContext)
{
	ensure(OutProceduralMesh);

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	if (!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
	{
		return false;
	}

	auto RoomLayoutManager = World->GetGameInstance()->GetSubsystem<UMRUKSubsystem>()->GetRoomLayoutManager();
	bool LoadResult = RoomLayoutManager->LoadTriangleMesh(SpaceQuery.Space.Value, OutProceduralMesh, LoadCollision);
	if (!LoadResult)
	{
		UE_LOG(LogMRUK, Warning, TEXT("Could not load triangle mesh from layout manager"));
		return false;
	}

	return true;
}

bool UMRUKBPLibrary::LoadGlobalMeshFromJsonString(const FString& JsonString, FOculusSpaceQueryResult SpaceQuery, UProceduralMeshComponent* OutProceduralMesh, bool LoadCollision)
{
	ensure(OutProceduralMesh);

	TSharedPtr<FJsonValue> JsonValue;
	auto JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonValue))
	{
		UE_LOG(LogMRUK, Warning, TEXT("Could not deserialize global mesh JSON data"));
		return false;
	}

	auto JsonObject = JsonValue->AsObject();

	// Find room
	auto RoomsJson = JsonObject->GetArrayField(TEXT("Rooms"));
	for (const auto& RoomJson : RoomsJson)
	{
		auto RoomObject = RoomJson->AsObject();
		FUUID RoomUUID;
		MRUKDeserialize(*RoomObject->GetField<EJson::None>(TEXT("UUID")), RoomUUID);
		if (RoomUUID == SpaceQuery.UUID)
		{
			// Find global mesh anchor
			auto AnchorsJson = RoomObject->GetArrayField(TEXT("Anchors"));
			for (const auto& AnchorJson : AnchorsJson)
			{
				FUUID AnchorUUID;
				auto AnchorObject = AnchorJson->AsObject();
				if (AnchorObject->HasField(TEXT("GlobalMesh")))
				{
					auto GlobalMeshObject = AnchorObject->GetField<EJson::Object>(TEXT("GlobalMesh"))->AsObject();

					auto PositionsJson = GlobalMeshObject->GetArrayField(TEXT("Positions"));
					TArray<FVector> Positions;
					Positions.Reserve(PositionsJson.Num());
					for (const auto& PositionJson : PositionsJson)
					{
						FVector Position;
						MRUKDeserialize(*PositionJson, Position);
						Positions.Push(Position);
					}

					auto IndicesJson = GlobalMeshObject->GetArrayField(TEXT("Indices"));
					TArray<int32> Indices;
					Indices.Reserve(IndicesJson.Num());
					for (const auto& IndexJson : IndicesJson)
					{
						double Index;
						MRUKDeserialize(*IndexJson, (float&)Index);
						Indices.Push((int32)Index);
					}

					TArray<FVector> EmptyNormals;
					TArray<FVector2D> EmptyUV;
					TArray<FColor> EmptyVertexColors;
					TArray<FProcMeshTangent> EmptyTangents;
					OutProceduralMesh->CreateMeshSection(0, Positions, Indices, EmptyNormals, EmptyUV, EmptyVertexColors, EmptyTangents, LoadCollision);

					return true;
				}
			}

			break;
		}
	}

	UE_LOG(LogMRUK, Warning, TEXT("Could not find global mesh in room"));

	return false;
}

void UMRUKBPLibrary::RecalculateProceduralMeshAndTangents(UProceduralMeshComponent* Mesh)
{
	if (!IsValid(Mesh))
		return;

	for (int s = 0; s < Mesh->GetNumSections(); ++s)
	{
		FProcMeshSection* Section = Mesh->GetProcMeshSection(s);

		//Get vertices of the section
		TArray<FVector> Vertices;
		for (FProcMeshVertex Vertex : Section->ProcVertexBuffer)
		{
			Vertices.Add(Vertex.Position);
		}

		//Calculate normals and tangents
		TArray<FVector> Normals = RecalculateNormals(Vertices, Section->ProcIndexBuffer);
		TArray<FProcMeshTangent> Tangents = RecalculateTangents(Normals);
		TArray<FVector2D> EmptyUV;
		TArray<FColor> EmptyVertexColors;

		//Update mesh section
		Mesh->UpdateMeshSection(s, Vertices, Normals, EmptyUV, EmptyVertexColors, Tangents);
	}
}

bool UMRUKBPLibrary::IsUnrealEngineMetaFork()
{
#if defined(WITH_OCULUS_BRANCH)
	return true;
#else
	return false;
#endif
}
