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

#include "Rig/IsdkInputActionsRigComponent.h"


#include "IsdkHandData.h"

#include "Misc/UObjectToken.h"

#include "Logging/MessageLog.h"

UIsdkInputActionsRigComponent::UIsdkInputActionsRigComponent()
{
  PrimaryComponentTick.bCanEverTick = false;
}

void UIsdkInputActionsRigComponent::SetSubobjectPropertyDefaults(EIsdkHandedness InHandedness)
{
	if (InHandedness == EIsdkHandedness::Left)
	{
		SelectAction = FName("Left_HandPinchSelect");
		SelectStrengthAction = FName("Left_HandPinchSelectStrength");
		//GrabSelectAction = FName("GrabAxisLeft");
		//GrabSelectStrengthAction = FName("GrabAxisLeft");

		PinchGrabAction = FName("Left_HandPinchGrab");
		PalmGrabAction = FName("Left_HandPalmGrab");
	}
	else // Right Hand
	{
		SelectAction = FName("Right_HandPinchSelect");
		SelectStrengthAction = FName("Right_HandPinchSelectStrength");
		//GrabSelectAction = FName("GrabAxisRight");
		//GrabSelectStrengthAction = FName("GrabAxisRight");

		PinchGrabAction = FName("Right_HandPinchGrab");
		PalmGrabAction = FName("Right_HandPalmGrab");
	}

	// Common controller animation actions
	AButtonDownAction = FName("Oculus_A_Button");
	BButtonDownAction = FName("Oculus_B_Button");
	XButtonDownAction = FName("Oculus_X_Button");
	YButtonDownAction = FName("Oculus_Y_Button");

	AButtonTouchedAction = FName("Oculus_A_Button_Touched");
	BButtonTouchedAction = FName("Oculus_B_Button_Touched");
	XButtonTouchedAction = FName("Oculus_X_Button_Touched");
	YButtonTouchedAction = FName("Oculus_Y_Button_Touched");

	LeftMenuButtonDownAction = FName("MenuToggleLeft");

	LeftFrontTriggerTouchedAction = FName("Oculus_Left_Trigger_Touched");
	RightFrontTriggerTouchedAction = FName("Oculus_Right_Trigger_Touched");
	LeftGripTriggerAxisAction = FName("GrabAxisLeft");
	RightGripTriggerAxisAction = FName("GrabAxisRight");
	LeftFrontTriggerAxisAction = FName("TriggerAxisLeft");
	RightFrontTriggerAxisAction = FName("TriggerAxisRight");

	LeftThumbstickTouchedAction = FName("Oculus_Left_Thumbstick_Touched");
	RightThumbstickTouchedAction = FName("Oculus_Right_Thumbstick_Touched");
	LeftThumbstickXAxisAction = FName("MovementAxisLeft_X");
	LeftThumbstickYAxisAction = FName("MovementAxisLeft_Y");
	RightThumbstickXAxisAction = FName("MovementAxisRight_X");
	RightThumbstickYAxisAction = FName("MovementAxisRight_Y");

	LeftPanelTouchedAction = FName("Left_Panel_Touched");
	RightPanelTouchedAction = FName("Right_Panel_Touched");
}

#if WITH_EDITOR
void UIsdkInputActionsRigComponent::CheckForErrors()
{
  Super::CheckForErrors();

#define LOCTEXT_NAMESPACE "InteractionSDK"
  if (!SelectStrengthAction.IsNone())
  {
    FMessageLog("MapCheck")
        .Warning()
        ->AddToken(FUObjectToken::Create(this))
        ->AddToken(FTextToken::Create(LOCTEXT(
            "MapCheck_Message_IsdkInputActionsRigComponent",
            "SelectStrengthAction field must be set.")));
  }
#undef LOCTEXT_NAMESPACE
}
#endif
