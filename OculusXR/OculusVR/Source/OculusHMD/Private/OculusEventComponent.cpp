// Copyright 1998-2020 Epic Games, Inc. All Rights Reserved.
#include "OculusEventComponent.h"
#include "OculusHMD.h"
#include "OculusDelegates.h"

void UOculusEventComponent::OnRegister()
{
	Super::OnRegister();

	FOculusEventDelegates::OculusDisplayRefreshRateChanged.AddUObject(this, &UOculusEventComponent::OculusDisplayRefreshRateChanged_Handler);
	FOculusEventDelegates::OculusEyeTrackingStateChanged.AddUObject(this, &UOculusEventComponent::OculusEyeTrackingStateChanged_Handler);
}

void UOculusEventComponent::OnUnregister()
{
	Super::OnUnregister();

	FOculusEventDelegates::OculusDisplayRefreshRateChanged.RemoveAll(this);
	FOculusEventDelegates::OculusEyeTrackingStateChanged.RemoveAll(this);
}
