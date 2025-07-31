/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusSceneGlobalMeshComponent.h"
#include "OculusSceneModule.h"
#include "OculusRoomLayoutManagerComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInterface.h"

const FString UOculusSceneGlobalMeshComponent::GlobalMeshSemanticLabel = TEXT("GLOBAL_MESH");

UOculusSceneGlobalMeshComponent::UOculusSceneGlobalMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UOculusSceneGlobalMeshComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UOculusSceneGlobalMeshComponent::HasCollision() const
{
	return Collision;
}

bool UOculusSceneGlobalMeshComponent::IsVisible() const
{
	return Visible;
}

UClass* UOculusSceneGlobalMeshComponent::GetAnchorComponentClass() const
{
	UClass* sceneAnchorComponentInstanceClass = SceneAnchorComponent ? SceneAnchorComponent.LoadSynchronous() : nullptr;
	return sceneAnchorComponentInstanceClass;
}

void UOculusSceneGlobalMeshComponent::CreateMeshComponent(const FUInt64& Space, AActor* GlobalMeshAnchor, const UOculusRoomLayoutManagerComponent* RoomLayoutManagerComponent) const
{
	bool hasCollision = HasCollision();
	UProceduralMeshComponent* proceduralMeshComponent = NewObject<UProceduralMeshComponent>(GlobalMeshAnchor);
	proceduralMeshComponent->RegisterComponent();

	bool bLoaded = RoomLayoutManagerComponent->LoadTriangleMesh(Space.Value, proceduralMeshComponent, hasCollision);
	ensure(bLoaded);
	UMaterialInterface* refMaterial = Material;
	if (refMaterial != nullptr)
	{
		UE_LOG(LogOculusScene, Verbose, TEXT("GLOBAL MESH Set Material %s"), *refMaterial->GetName());
		proceduralMeshComponent->SetMaterial(0, refMaterial);
	}
	if (hasCollision)
	{
		FName refCollisionProfile = CollisionProfileName.Name;
		proceduralMeshComponent->SetCollisionProfileName(refCollisionProfile);
		UE_LOG(LogOculusScene, Verbose, TEXT("GLOBAL MESH Set Collision Profile %s"), *refCollisionProfile.ToString());
	}
	GlobalMeshAnchor->AddOwnedComponent(proceduralMeshComponent);
	proceduralMeshComponent->AttachToComponent(GlobalMeshAnchor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	proceduralMeshComponent->SetRelativeLocation(FVector::ZeroVector, false, nullptr, ETeleportType::ResetPhysics);

	proceduralMeshComponent->SetVisibility(IsVisible());

	const float worldToMeters = GetWorld()->GetWorldSettings()->WorldToMeters;
	proceduralMeshComponent->SetRelativeScale3D(FVector(worldToMeters, worldToMeters, worldToMeters));
}
