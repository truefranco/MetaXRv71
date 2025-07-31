// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_DeferredDeletionQueue.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusHMDPrivate.h"
#include "XRThreadUtils.h"
#include "OculusHMDModule.h"

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FDeferredDeletionQueue
//-------------------------------------------------------------------------------------------------
uint32 GOculusHMDLayerDeletionFrameNumber = 0;
const uint32 NUM_FRAMES_TO_WAIT_FOR_LAYER_DELETE = 3;
const uint32 NUM_FRAMES_TO_WAIT_FOR_OVRP_LAYER_DELETE = 7;

void FDeferredDeletionQueue::AddLayerToDeferredDeletionQueue(const FLayerPtr& ptr)
{
	DeferredDeletionEntry Entry;
	Entry.Layer = ptr;
	Entry.FrameEnqueued = GOculusHMDLayerDeletionFrameNumber;
	Entry.EntryType = DeferredDeletionEntry::DeferredDeletionEntryType::Layer;
	DeferredDeletionArray.Add(Entry);
}

void FDeferredDeletionQueue::AddOVRPLayerToDeferredDeletionQueue(const uint32 layerID)
{
	DeferredDeletionEntry Entry;
	Entry.OvrpLayerId = layerID;
	Entry.FrameEnqueued = GOculusHMDLayerDeletionFrameNumber;
	Entry.EntryType = DeferredDeletionEntry::DeferredDeletionEntryType::OvrpLayer;
	DeferredDeletionArray.Add(Entry);
}

void FDeferredDeletionQueue::HandleLayerDeferredDeletionQueue_RenderThread(bool bDeleteImmediately)
{
	// Traverse list backwards so the swap switches to elements already tested
	for (int32 Index = DeferredDeletionArray.Num() - 1; Index >= 0; --Index)
	{
		DeferredDeletionEntry* Entry = &DeferredDeletionArray[Index];
		if (Entry->EntryType == DeferredDeletionEntry::DeferredDeletionEntryType::Layer)
		{
			if (bDeleteImmediately || GOculusHMDLayerDeletionFrameNumber > Entry->FrameEnqueued + NUM_FRAMES_TO_WAIT_FOR_LAYER_DELETE)
			{
				DeferredDeletionArray.RemoveAtSwap(Index, 1, false);
			}
		}
		else if (Entry->EntryType == DeferredDeletionEntry::DeferredDeletionEntryType::OvrpLayer)
		{
			if (bDeleteImmediately || GOculusHMDLayerDeletionFrameNumber > Entry->FrameEnqueued + NUM_FRAMES_TO_WAIT_FOR_OVRP_LAYER_DELETE)
			{
				ExecuteOnRHIThread_DoNotWait([OvrpLayerId = Entry->OvrpLayerId]()
				{
					UE_LOG(LogHMD, Log, TEXT("Destroying layer %d"), OvrpLayerId);
					FOculusHMDModule::GetPluginWrapper().DestroyLayer(OvrpLayerId);
				});
				DeferredDeletionArray.RemoveAtSwap(Index, 1, false);
			}
		}

	}

	// if the function is to be called multiple times, move this increment somewhere unique!
	++GOculusHMDLayerDeletionFrameNumber;
}

} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
