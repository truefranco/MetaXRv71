/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/
#include "MRUtilityKitAnchorActorSpawner.h"
#include "MRUtilityKitTelemetry.h"
#include "MRUtilityKitSubsystem.h"

void AMRUKAnchorActorSpawner::BeginPlay()
{
	Super::BeginPlay();

/*#if WITH_EDITOR
	if (OculusXRTelemetry::IsActive())
	{
		MRUKTelemetry::FLoadAnchorActorSpawnerMarker().Start().End(OculusXRTelemetry::EAction::Success);
	}
#endif*/

	if (SpawnOnStart)
	{
		const auto Subsystem = GetGameInstance()->GetSubsystem<UMRUKSubsystem>();
		if (Subsystem->SceneLoadStatus == EMRUKInitStatus::Complete)
		{
			SpawnInterior();
		}
		// Register for the OnSceneLoaded event anyway, because we don't want to miss future updates
		Subsystem->OnSceneLoaded.AddUniqueDynamic(this, &AMRUKAnchorActorSpawner::OnSceneLoaded);
	}
}

void AMRUKAnchorActorSpawner::OnSceneLoaded(bool Success)
{
	if (Success)
	{
		SpawnInterior();
	}
}

void AMRUKAnchorActorSpawner::SpawnInterior()
{
	for (auto InteriorActor : InteriorActors)
	{
		if (InteriorActor)
		{
			InteriorActor->Destroy();
		}
	}
	InteriorActors.Empty();

	const auto Subsystem = GetGameInstance()->GetSubsystem<UMRUKSubsystem>();

	// Use last seed if possible to keep spawing determenistic after the first spawn.
	// In case the anchor random spawn seed has been changed it will be used instead
	// of the last seed.
	int32 Seed = -1;
	if (LastSeed >= 0)
	{
		if ((AnchorRandomSpawnSeed >= 0) && (LastSeed != AnchorRandomSpawnSeed))
		{
			Seed = AnchorRandomSpawnSeed;
		}
		else
		{
			Seed = LastSeed;
		}
	}
	else if (AnchorRandomSpawnSeed >= 0)
	{
		Seed = AnchorRandomSpawnSeed;
	}

	FRandomStream RandomStream(Seed);
	if (Seed < 0)
	{
		RandomStream.GenerateNewSeed();
	}
	LastSeed = RandomStream.GetCurrentSeed();
	InteriorActors = Subsystem->SpawnInteriorFromStream(SpawnGroups, RandomStream, ProceduralMaterial);

	OnActorsSpawned.Broadcast();
}
