/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.

This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#include "MRUtilityKitEditor.h"
#include "MRUtilityKitTelemetry.h"

#define LOCTEXT_NAMESPACE "FMRUKEditorModule"

DEFINE_LOG_CATEGORY(LogMRUKEditor);

void FMRUKEditorModule::StartupModule()
{
	/*OculusXRTelemetry::IfActiveThen([]() {
		MRUKTelemetry::FLoadPluginMarker().Start().End(OculusXRTelemetry::EAction::Success);
	});*/
}

void FMRUKEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMRUKEditorModule, MRUtilityKit)
