/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "CoreMinimal.h"
#include "OculusSceneAnchorComponent.h"
#include "Engine/CollisionProfile.h"
#include "OculusRoomLayoutManagerComponent.h"
#include "OculusSceneGlobalMeshComponent.generated.h"

class UMaterialInterface;

UCLASS(meta = (DisplayName = "Oculus Scene Global Mesh Component", BlueprintSpawnableComponent))
class OCULUSSCENE_API UOculusSceneGlobalMeshComponent : public UActorComponent
{
	GENERATED_BODY()

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UOculusSceneGlobalMeshComponent(const FObjectInitializer& ObjectInitializer);

	void CreateMeshComponent(const FUInt64& Space, AActor* GlobalMeshAnchor, const UOculusRoomLayoutManagerComponent* RoomLayoutManagerComponent) const;

	static const FString GlobalMeshSemanticLabel;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Oculus")
	bool Collision = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Oculus")
	FCollisionProfileName CollisionProfileName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Oculus")
	bool Visible = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Oculus")
	UMaterialInterface* Material = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Oculus")
	TSoftClassPtr<UOculusSceneAnchorComponent> SceneAnchorComponent = TSoftClassPtr<UOculusSceneAnchorComponent>(FSoftClassPath(UOculusSceneAnchorComponent::StaticClass()));

public:
	bool HasCollision() const;

	bool IsVisible() const;

	UClass* GetAnchorComponentClass() const;
};
