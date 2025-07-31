/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "MRUtilityKitGuardianSpawner.h"
#include "MRUtilityKitTelemetry.h"
#include "MRUtilityKitAnchor.h"
#include "MRUtilityKitSubsystem.h"
#include "MRUtilityKitGuardian.h"
#include "IXRTrackingSystem.h"
#include "Materials/MaterialInstanceDynamic.h"

AMRUKGuardianSpawner::AMRUKGuardianSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void AMRUKGuardianSpawner::SpawnGuardian()
{
	if (GuardianMaterial)
	{
		SetGuardianMaterial(GuardianMaterial);
	}

	for (auto GuardianActor : GuardianActors)
	{
		GuardianActor->SetActorHiddenInGame(IsHidden());
	}
}

void AMRUKGuardianSpawner::SetGuardianMaterial(UMaterialInstance* Material)
{
	if (!Material)
	{
		return;
	}

	GuardianMaterial = Material;
	DynamicGuardianMaterial = UMaterialInstanceDynamic::Create(GuardianMaterial, this);
	DynamicGuardianMaterial->SetVectorParameterValue(TEXT("WallScale"), FVector(GridDensity));

	auto GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		// We are probably currently just in the editor and didn't started a play session.
		return;
	}
	auto Subsystem = GameInstance->GetSubsystem<UMRUKSubsystem>();
	auto CurrentRoom = Subsystem->GetCurrentRoom();
	if (!CurrentRoom)
	{
		return;
	}

	// Destroy old guardians if there are any
	DestroyGuardians();

	// Generate new guardians

	const auto SpawnGuardian = [this](AMRUKAnchor* Anchor, const TArray<FMRUKPlaneUV>& PlaneUVAdjustments) {
		// Create guardian actor
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActorSpawnParams.Owner = Anchor;
		AMRUKGuardian* GuardianActor = GetWorld()->SpawnActor<AMRUKGuardian>(ActorSpawnParams);
		GuardianActor->AttachToActor(Anchor, FAttachmentTransformRules::KeepRelativeTransform);
		GuardianActors.Push(GuardianActor);

		// Generate procedural mesh
		auto ProceduralMesh = NewObject<UProceduralMeshComponent>(GuardianActor, TEXT("GuardianMesh"));
		Anchor->GenerateProceduralAnchorMesh(*ProceduralMesh, PlaneUVAdjustments, true, false);
		ProceduralMesh->SetMaterial(0, DynamicGuardianMaterial);
		GuardianActor->CreateGuardian(ProceduralMesh);
	};

	// Attach procedural meshes to the walls first because they are connected.
	TArray<FMRUKAnchorWithPlaneUVs> AnchorsWithPlaneUVs;
	TArray<FMRUKTexCoordModes> WallTextureCoordinateModes = { { EMRUKCoordModeU::Metric, EMRUKCoordModeV::Metric } };
	CurrentRoom->ComputeWallMeshUVAdjustments(WallTextureCoordinateModes, AnchorsWithPlaneUVs);
	for (const auto& AnchorWithPlaneUVs : AnchorsWithPlaneUVs)
	{
		SpawnGuardian(AnchorWithPlaneUVs.Anchor, AnchorWithPlaneUVs.PlaneUVs);
	}

	// Attach procedural meshes to the rest of the anchors. The walls have already meshes applied
	// because of the first step and will therefore be ignored by this code automatically.
	for (const auto Anchor : CurrentRoom->AllAnchors)
	{
		if (!Anchor || Anchor == CurrentRoom->FloorAnchor || Anchor == CurrentRoom->CeilingAnchor || CurrentRoom->IsWallAnchor(Anchor))
		{
			continue;
		}

		SpawnGuardian(Anchor, {});
	}
}

void AMRUKGuardianSpawner::SetGridDensity(float Density)
{
	GridDensity = (double)Density;

	if (DynamicGuardianMaterial)
	{
		DynamicGuardianMaterial->SetVectorParameterValue(TEXT("WallScale"), FVector(GridDensity));
	}
}

void AMRUKGuardianSpawner::Tick(float DeltaSeconds)
{
	if (!DynamicGuardianMaterial)
	{
		return;
	}

	if (EnableFade)
	{
		auto Subsystem = GetGameInstance()->GetSubsystem<UMRUKSubsystem>();
		auto CurrentRoom = Subsystem->GetCurrentRoom();
		if (!CurrentRoom)
		{
			return;
		}

		FQuat HeadsetOrientation;
		FVector HeadsetPosition(0.f);
		GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, HeadsetOrientation, HeadsetPosition);

		FVector SurfacePosition = FVector::ZeroVector;
		double SurfaceDistance = 0.0;
		FMRUKLabelFilter LabelFilter;
		LabelFilter.ExcludedLabels = { FMRUKLabels::Ceiling, FMRUKLabels::Floor };
		CurrentRoom->TryGetClosestSurfacePosition(HeadsetPosition, SurfacePosition, (float&)SurfaceDistance, LabelFilter);

		const auto WorldToMeters = GetWorldSettings()->WorldToMeters;
		const auto GuardianFade = FMath::Clamp(1.0 - ((SurfaceDistance / WorldToMeters) / GuardianDistance), 0.0, 1.0);
		DynamicGuardianMaterial->SetScalarParameterValue(TEXT("Fade"), GuardianFade);
	}
}

void AMRUKGuardianSpawner::BeginPlay()
{
	Super::BeginPlay();

/*#if WITH_EDITOR
	if (OculusXRTelemetry::IsActive())
	{
		MRUKTelemetry::FLoadGuardianMarker().Start().End(OculusXRTelemetry::EAction::Success);
	}
#endif*/

	if (SpawnOnStart)
	{
		const auto Subsystem = GetGameInstance()->GetSubsystem<UMRUKSubsystem>();
		if (Subsystem->SceneLoadStatus == EMRUKInitStatus::Complete)
		{
			SpawnGuardian();
		}
		// Register to the event because we don't want to miss future updates
		Subsystem->OnSceneLoaded.AddUniqueDynamic(this, &AMRUKGuardianSpawner::SceneLoaded);
	}
}

void AMRUKGuardianSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyGuardians();
}

#if WITH_EDITOR
void AMRUKGuardianSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const auto PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMRUKGuardianSpawner, GridDensity))
	{
		SetGridDensity(GridDensity);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AMRUKGuardianSpawner, GuardianMaterial))
	{
		SetGuardianMaterial(GuardianMaterial);
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AMRUKGuardianSpawner::DestroyGuardians()
{
	for (const auto GuardianActor : GuardianActors)
	{
		if (IsValid(GuardianActor))
		{
			GuardianActor->Destroy();
		}
	}
	GuardianActors.Empty();
}

void AMRUKGuardianSpawner::SceneLoaded(bool Success)
{
	if (Success)
	{
		SpawnGuardian();
	}
}
