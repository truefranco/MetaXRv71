// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusMR_Settings.h"
#include "OculusMRPrivate.h"
#include "OculusHMD.h"
#include "Engine/Engine.h"

UOculusMR_Settings::UOculusMR_Settings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ClippingReference(EOculusMR_ClippingReference::CR_Head)
	, bUseTrackedCameraResolution(true)
	, WidthPerView(960)
	, HeightPerView(540)
	, CastingLatency(0.0f)
	, BackdropColor(FColor::Green)
	, ExternalCompositionPostProcessEffects(EOculusMR_PostProcessEffects::PPE_Off)
	, bIsCasting(false)
	, CompositionMethod(EOculusMR_CompositionMethod::ExternalComposition)
	, BindToTrackedCameraIndex(-1)
{
}

void UOculusMR_Settings::SetCompositionMethod(EOculusMR_CompositionMethod val)
{
	if (CompositionMethod == val)
	{
		return;
	}
	auto old = CompositionMethod;
	CompositionMethod = val;
	CompositionMethodChangeDelegate.Execute(old, val);
}

void UOculusMR_Settings::SetCapturingCamera(EOculusMR_CameraDeviceEnum_DEPRECATED val)
{
	// deprecated
}

void UOculusMR_Settings::SetIsCasting(bool val)
{
	if (bIsCasting == val)
	{
		return;
	}
	auto old = bIsCasting;
	bIsCasting = val;
	IsCastingChangeDelegate.Execute(old, val);
}

void UOculusMR_Settings::BindToTrackedCameraIndexIfAvailable(int InTrackedCameraIndex)
{
	if (BindToTrackedCameraIndex == InTrackedCameraIndex)
	{
		return;
	}
	auto old = BindToTrackedCameraIndex;
	BindToTrackedCameraIndex = InTrackedCameraIndex;
	TrackedCameraIndexChangeDelegate.Execute(old, InTrackedCameraIndex);
}

void UOculusMR_Settings::LoadFromIni()
{
	if (!GConfig)
	{
		UE_LOG(LogMR, Warning, TEXT("GConfig is NULL"));
		return;
	}

	// Flushing the GEngineIni is necessary to get the settings reloaded at the runtime, but the manual flushing
	// could cause an assert when loading audio settings if launching through editor at the 2nd time. Disabled temporarily.
	//GConfig->Flush(true, GEngineIni);

	const TCHAR* OculusMRSettings = TEXT("Oculus.Settings.MixedReality");
	bool v;
	float f;
	int32 i;
	FVector vec;
	FColor color;
	if (GConfig->GetInt(OculusMRSettings, TEXT("CompositionMethod"), i, GEngineIni))
	{
		SetCompositionMethod((EOculusMR_CompositionMethod)i);
	}
	if (GConfig->GetInt(OculusMRSettings, TEXT("ClippingReference"), i, GEngineIni))
	{
		ClippingReference = (EOculusMR_ClippingReference)i;
	}
	if (GConfig->GetBool(OculusMRSettings, TEXT("bUseTrackedCameraResolution"), v, GEngineIni))
	{
		bUseTrackedCameraResolution = v;
	}
	if (GConfig->GetInt(OculusMRSettings, TEXT("WidthPerView"), i, GEngineIni))
	{
		WidthPerView = i;
	}
	if (GConfig->GetInt(OculusMRSettings, TEXT("HeightPerView"), i, GEngineIni))
	{
		HeightPerView = i;
	}
	if (GConfig->GetFloat(OculusMRSettings, TEXT("CastingLatency"), f, GEngineIni))
	{
		CastingLatency = f;
	}
	if (GConfig->GetColor(OculusMRSettings, TEXT("BackdropColor"), color, GEngineIni))
	{
		BackdropColor = color;
	}
	if (GConfig->GetInt(OculusMRSettings, TEXT("BindToTrackedCameraIndex"), i, GEngineIni))
	{
		BindToTrackedCameraIndexIfAvailable(i);
	}
	if (GConfig->GetInt(OculusMRSettings, TEXT("ExternalCompositionPostProcessEffects"), i, GEngineIni))
	{
		ExternalCompositionPostProcessEffects = (EOculusMR_PostProcessEffects)i;
	}

	UE_LOG(LogMR, Log, TEXT("MixedReality settings loaded from Engine.ini"));
}

void UOculusMR_Settings::SaveToIni() const
{
	if (!GConfig)
	{
		UE_LOG(LogMR, Warning, TEXT("GConfig is NULL"));
		return;
	}

	const TCHAR* OculusMRSettings = TEXT("Oculus.Settings.MixedReality");
	GConfig->SetInt(OculusMRSettings, TEXT("CompositionMethod"), (int32)CompositionMethod, GEngineIni);
	GConfig->SetInt(OculusMRSettings, TEXT("ClippingReference"), (int32)ClippingReference, GEngineIni);
	GConfig->SetBool(OculusMRSettings, TEXT("bUseTrackedCameraResolution"), bUseTrackedCameraResolution, GEngineIni);
	GConfig->SetInt(OculusMRSettings, TEXT("WidthPerView"), WidthPerView, GEngineIni);
	GConfig->SetInt(OculusMRSettings, TEXT("HeightPerView"), HeightPerView, GEngineIni);
	GConfig->SetFloat(OculusMRSettings, TEXT("CastingLatency"), CastingLatency, GEngineIni);
	GConfig->SetColor(OculusMRSettings, TEXT("BackdropColor"), BackdropColor, GEngineIni);
	GConfig->SetInt(OculusMRSettings, TEXT("BindToTrackedCameraIndex"), (int32)BindToTrackedCameraIndex, GEngineIni);
	GConfig->SetInt(OculusMRSettings, TEXT("ExternalCompositionPostProcessEffects"), (int32)ExternalCompositionPostProcessEffects, GEngineIni);

	GConfig->Flush(false, GEngineIni);

	UE_LOG(LogMR, Log, TEXT("MixedReality settings saved to Engine.ini"));
}
