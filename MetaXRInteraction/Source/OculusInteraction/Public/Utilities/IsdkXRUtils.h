/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * Licensed under the Oculus SDK License Agreement (the "License");
 * you may not use the Oculus SDK except in compliance with the License,
 * which is provided at the time of installation or download, or which
 * otherwise accompanies this software in either electronic or hard copy form.
 *
 * You may obtain a copy of the License at
 *
 * https://developer.oculus.com/licenses/oculussdk/
 *
 * Unless required by applicable law or agreed to in writing, the Oculus SDK
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "CoreMinimal.h"
#include "IXRTrackingSystem.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/ConfigCacheIni.h"

namespace IsdkXRUtils
{
const FName OpenXRName = TEXT("OpenXR");
static const FName LeftSourceName("Left");
static const FName RightSourceName("Right");
static const FName LeftAimSourceName("LeftAim");
static const FName RightAimSourceName("RightAim");

inline bool IsUsingOpenXR()
{
  const IXRTrackingSystem* XRTrackingSystem = GEngine->XRSystem.Get();
  return XRTrackingSystem && XRTrackingSystem->GetSystemName() == OpenXRName;
}

inline bool IsUsingOculusXR()
{
  FString XrApi;
  const bool bValueFound = GConfig->GetString(
      TEXT("/Script/OculusHMD.OculusHMDRuntimeSettings"), TEXT("XrApi"), XrApi, GEngineIni);

  // Default value is to use OVR Plugin as the XRApi.  The value will not be present in config
  // unless it has been manually changed, so we are using the OVR if the value is not found or if
  // the value corresponds to the OVR option.
  return XrApi.Equals(FString("OVRPluginOpenXR")) || !bValueFound;
}

namespace OXR
{
// OpenXR hand constants
const FQuat HandRootFixupRotation =
    FQuat(FVector::UnitZ(), HALF_PI) * FQuat(FVector::UnitX(), -HALF_PI);
const FQuat HandJointFixupRotation =
    FQuat(FVector::UnitX(), -HALF_PI) * FQuat(FVector::UnitY(), -HALF_PI);
const FQuat HandRelativePointerRotation = FQuat(FVector::UnitY(), PI / 4);
const FVector HandRootWristOffsetLeft = FVector(-0.089, -0.131, 4.917);
const FVector HandRootWristOffsetRight = FVector(0.089, -0.131, 4.917);
const FVector HandRelativePointerOffset = FVector(5, 0, 0);
// OpenXR controller constants
const FVector ControllerRelativePointerOffset = FVector(4.0, 0, 0);
} // namespace OXR

namespace OVR
{
// OVR hand constants
const FQuat HandRootFixupRotationLeft = FQuat(FVector::UnitX(), HALF_PI);
const FQuat HandRootFixupRotationRight = FQuat(FVector::UnitX(), -HALF_PI);
const FQuat HandRootInvFixupRotationLeft = FQuat(FVector::UnitX(), -HALF_PI);
const FQuat HandRootInvFixupRotationRight = FQuat(FVector::UnitX(), HALF_PI);
const FTransform LeftControllerHandTransform =
    FTransform(FRotator(90, 0.f, 15.f), FVector(-10.5f, -3.f, -4.2), FVector::OneVector);
const FTransform RightControllerHandTransform =
    FTransform(FRotator(-90.f, 0.f, 195.f), FVector(-10.5f, 3.0f, -4.2f), FVector::OneVector);
const FQuat OVRToOXRLeft = FQuat(FVector::UnitY(), -HALF_PI) * FQuat(FVector::UnitZ(), PI);
const FQuat OVRToOXRRight = FQuat(FVector::UnitY(), HALF_PI);
// OVR controller constants
const FVector ControllerRelativePointerOffset = FVector(5.5, 0, 0);
} // namespace OVR

} // namespace IsdkXRUtils
