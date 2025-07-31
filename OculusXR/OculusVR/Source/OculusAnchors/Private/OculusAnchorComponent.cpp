/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "OculusAnchorComponent.h"
#include "OculusAnchors.h"
#include "OculusHMD.h"
#include "OculusAnchorBPFunctionLibrary.h"
#include "OculusAnchorsPrivate.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarOVRVerboseAnchorDebug(
	TEXT("ovr.VerboseAnchorDebug"),
	0,
	TEXT("Enables or disables verbose logging for Oculus anchors.\n")
	TEXT("<=0: disabled (no printing)\n")
	TEXT("  1: enabled  (verbose logging)\n"));
#endif

UOculusAnchorComponent::UOculusAnchorComponent(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
	, bUpdateHeadSpaceTransform(true)
	, AnchorHandle(0)
	, StorageLocations(0)
{
	AnchorHandle = 0;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UOculusAnchorComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (IsValid(PlayerController))
		{
			PlayerCameraManager = PlayerController->PlayerCameraManager;
		}
	}
}

void UOculusAnchorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateAnchorTransform();
}

void UOculusAnchorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HasValidHandle())
	{
		EOculusResult::Type AnchorResult;
		OculusAnchors::FOculusAnchors::DestroyAnchor(AnchorHandle.GetValue(), AnchorResult);
	}
}

FUInt64 UOculusAnchorComponent::GetHandle() const
{
	return AnchorHandle;
}

void UOculusAnchorComponent::SetHandle(FUInt64 Handle)
{
	AnchorHandle = Handle;
}

bool UOculusAnchorComponent::HasValidHandle() const
{
	return AnchorHandle != FUInt64(0);
}

FUUID UOculusAnchorComponent::GetUUID() const
{
	return AnchorUUID;
}

void UOculusAnchorComponent::SetUUID(FUUID NewUUID)
{
	if (AnchorUUID.IsValidUUID())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Anchor component already has valid UUID, cannot re-assign a new UUID. Component: %s  --  Space: %llu  --  UUID: %s"), 
			*GetName(), AnchorHandle, *AnchorUUID.ToString());
		return;
	}

	if (!NewUUID.IsValidUUID())
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("New UUID provided to component is invalid, cannot assign. Component: %s  --  Space: %llu"), *GetName(), AnchorHandle);
		return;
	}

	UE_LOG(LogOculusAnchors, Verbose, TEXT("Assigned new Oculus UUID: %s"), *NewUUID.ToString());

	AnchorUUID = NewUUID;
}

bool UOculusAnchorComponent::IsStoredAtLocation(EOculusSpaceStorageLocation Location) const
{
	UE_LOG(LogOculusAnchors, Verbose, TEXT("Anchor UUID: %s  -  Saved Local: %d  -  Saved Cloud: %d"),
		*GetUUID().ToString(),
		StorageLocations & static_cast<int32>(EOculusSpaceStorageLocation::Local),
		StorageLocations & static_cast<int32>(EOculusSpaceStorageLocation::Cloud));

	return (StorageLocations & static_cast<int32>(Location)) > 0;
}

void UOculusAnchorComponent::SetStoredLocation(EOculusSpaceStorageLocation Location, bool Stored)
{
	if (Stored)
	{
		StorageLocations |= static_cast<int32>(Location);
	}
	else
	{
		StorageLocations = StorageLocations & ~static_cast<int32>(Location);
	}

	UE_LOG(LogOculusAnchors, Verbose, TEXT("Anchor UUID: %s  -  Saved Local: %d  -  Saved Cloud: %d"),
		*GetUUID().ToString(),
		StorageLocations & static_cast<int32>(EOculusSpaceStorageLocation::Local),
		StorageLocations & static_cast<int32>(EOculusSpaceStorageLocation::Cloud));
}

bool UOculusAnchorComponent::IsSaved() const
{
	return StorageLocations > 0;
}

void UOculusAnchorComponent::UpdateAnchorTransform() const
{
	if (GetWorld() == nullptr)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to retrieve World Context"));
		return;
	}

	AActor* Parent = GetOwner();
	if (Parent)
	{
		if(AnchorHandle.Value)
		{
			FTransform OutTransform;
			if(UOculusAnchorBPFunctionLibrary::GetAnchorTransformByHandle(AnchorHandle, OutTransform))
			{
#if WITH_EDITOR
				// Link only head-space transform update
				if (bUpdateHeadSpaceTransform && PlayerCameraManager != nullptr)
				{
					FTransform MainCameraTransform;
					MainCameraTransform.SetLocation(PlayerCameraManager->GetCameraLocation());
					MainCameraTransform.SetRotation(FQuat(PlayerCameraManager->GetCameraRotation()));

					if (!ToWorldSpacePose(MainCameraTransform, OutTransform))
					{
						UE_LOG(LogOculusAnchors, Display, TEXT("Was not able to transform anchor to world space pose"));
					}
				}
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (CVarOVRVerboseAnchorDebug.GetValueOnGameThread() > 0)
				{
					UE_LOG(LogOculusAnchors, Display, TEXT("UpdateAnchor Pos %s"), *OutTransform.GetLocation().ToString());
					UE_LOG(LogOculusAnchors, Display, TEXT("UpdateAnchor Rot %s"), *OutTransform.GetRotation().ToString());
				}
#endif
			}

			Parent->SetActorLocationAndRotation(OutTransform.GetLocation(), OutTransform.GetRotation(), false, 0, ETeleportType::ResetPhysics);
		}
	}
}

bool UOculusAnchorComponent::ToWorldSpacePose(FTransform CameraTransform, FTransform& OutTrackingSpaceTransform) const
{
	OculusHMD::FOculusHMD* OculusHMD = GEngine->XRSystem.IsValid() ? (OculusHMD::FOculusHMD*)(GEngine->XRSystem->GetHMDDevice()) : nullptr;
	if (!OculusHMD)
	{
		UE_LOG(LogOculusAnchors, Warning, TEXT("Unable to retrieve OculusHMD, cannot calculate anchor world space pose."));
		return false;
	}

	OculusHMD::FPose MainCameraPose(CameraTransform.GetRotation(), CameraTransform.GetLocation());
	OculusHMD::FPose TrackingSpacePose(OutTrackingSpaceTransform.GetRotation(), OutTrackingSpaceTransform.GetLocation());

	FVector OutHeadPosition;
	FQuat OutHeadOrientation;
	const bool bGetPose = OculusHMD->GetCurrentPose(OculusHMD->HMDDeviceId, OutHeadOrientation, OutHeadPosition);
	if (!bGetPose) return false;

	OculusHMD::FPose HeadPose(OutHeadOrientation, OutHeadPosition);

	OculusHMD::FPose poseInHeadSpace = HeadPose.Inverse() * TrackingSpacePose;

	// To world space pose
	const OculusHMD::FPose WorldTrackingSpacePose = MainCameraPose * poseInHeadSpace;

	OutTrackingSpaceTransform.SetLocation(WorldTrackingSpacePose.Position);
	OutTrackingSpaceTransform.SetRotation(WorldTrackingSpacePose.Orientation);

	return true;
}