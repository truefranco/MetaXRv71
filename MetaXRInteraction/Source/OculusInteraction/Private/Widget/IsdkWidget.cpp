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

#include "Widget/IsdkWidget.h"

#include "Subsystem/IsdkWidgetSubsystem.h"
#include "OculusInteractionLog.h"

void UIsdkWidget::HandleVirtualUserPointerEvent(
    FIsdkVirtualUserPointerEvent VirtualUserPointerEvent,
    UWidgetComponent* AttachedWidget,
    FIsdkWidgetEventDelegate WidgetEventDelegate,
    UIsdkWidgetSubsystem& WidgetSubsystem,
    float MinMoveTravelDistance,
    bool bStopBroadcastOnDrag)
{
  auto* VirtualUserStatePtr =
      WidgetSubsystem.FindOrCreateInteractorVirtualUserState(VirtualUserPointerEvent);
  auto& VirtualUserState = *VirtualUserStatePtr;

  switch (VirtualUserPointerEvent.Type)
  {
    // Unreal doesn't have the notion of a cancel, so treat cancel as an unhover.
    case EIsdkPointerEventType::Cancel:
    case EIsdkPointerEventType::Unhover:
    {
      // This removes the hover from any element in the widget in the event it was hovered but
      // never touched.
      UE_LOG(
          LogOculusInteraction,
          Verbose,
          TEXT("VirtualUserPointerEvent Unhover (VirtualUser Index: %u)"),
          VirtualUserState.VirtualUser->GetVirtualUserIndex());
      VirtualUserState.CurrentLocalHitLocation = FVector2D(0.0, 0.0);
      VirtualUserState.LastLocalHitLocation = FVector2D(0.0, 0.0);
      VirtualUserState.bMoving = false;
      RouteMouseHoverPointerEvent(AttachedWidget, VirtualUserState);

      WidgetSubsystem.DestroyInteractorVirtualUserState(VirtualUserPointerEvent.Interactor);
      break;
    }

    case EIsdkPointerEventType::Hover:
    {
      RouteMouseHoverPointerEvent(AttachedWidget, VirtualUserState);
      break;
    }

    case EIsdkPointerEventType::Unselect:
    {
      UE_LOG(
          LogOculusInteraction,
          Verbose,
          TEXT("RoutePointerEvent TouchUp (VirtualUser Index: %u)"),
          VirtualUserState.VirtualUser->GetVirtualUserIndex());
      const auto reply = RouteTouchUpPointerEvent(AttachedWidget, VirtualUserState);
      if (WidgetEventDelegate.IsBound() && !(bStopBroadcastOnDrag && VirtualUserState.bMoving))
      {
        if (reply.IsEventHandled())
        {
          const FIsdkWidgetEvent Event{
              VirtualUserState.Interactor, EIsdkWidgetEventType::UnselectedHovered};
          WidgetEventDelegate.Broadcast(Event);
        }
        else
        {
          const FIsdkWidgetEvent Event{
              VirtualUserState.Interactor, EIsdkWidgetEventType::UnselectedEmpty};
          WidgetEventDelegate.Broadcast(Event);
        }
      }
      VirtualUserState.bTouching = false;
      VirtualUserState.bMoving = false;

      // Send another hover event with LastLocalHitLocation at (0,0) to re-hover
      // the correct element.
      VirtualUserState.CurrentHitPosition = FVector{VirtualUserPointerEvent.Position};
      UpdateLocalHitLocation(AttachedWidget, VirtualUserState);
      VirtualUserState.LastLocalHitLocation = FVector2D(0.0, 0.0);
      RouteMouseHoverPointerEvent(AttachedWidget, VirtualUserState);
      break;
    }

    case EIsdkPointerEventType::Select:
    {
      UE_LOG(
          LogOculusInteraction,
          Verbose,
          TEXT("RoutePointerEvent TouchDown (VirtualUser Index: %u)"),
          VirtualUserState.VirtualUser->GetVirtualUserIndex());
      VirtualUserState.CurrentHitPosition = FVector{VirtualUserPointerEvent.Position};
      VirtualUserState.HitPositionOnSelect = VirtualUserState.CurrentHitPosition;
      VirtualUserState.bTouching = true;
      VirtualUserState.bMoving = false;
      UpdateLocalHitLocation(AttachedWidget, VirtualUserState);
      const auto Reply = RouteTouchDownPointerEvent(AttachedWidget, VirtualUserState);
      if (WidgetEventDelegate.IsBound())
      {
        if (Reply.IsEventHandled())
        {
          const FIsdkWidgetEvent Event{
              VirtualUserState.Interactor, EIsdkWidgetEventType::SelectedHovered};
          WidgetEventDelegate.Broadcast(Event);
        }
        else
        {
          const FIsdkWidgetEvent Event{
              VirtualUserState.Interactor, EIsdkWidgetEventType::SelectedEmpty};
          WidgetEventDelegate.Broadcast(Event);
        }
      }
      break;
    }

    case EIsdkPointerEventType::Move:
    {
      VirtualUserState.CurrentHitPosition = FVector{VirtualUserPointerEvent.Position};
      UpdateLocalHitLocation(AttachedWidget, VirtualUserState);
      if (!VirtualUserState.bTouching)
      {
        RouteMouseHoverPointerEvent(AttachedWidget, VirtualUserState);
      }
      else
      {
        if (VirtualUserState.bMoving ||
            TraveledEnoughForMove(VirtualUserState, MinMoveTravelDistance))
        {
          VirtualUserState.bMoving = true;
          RouteTouchMovePointerEvent(AttachedWidget, VirtualUserState);
        }
      }
      break;
    }

    default:
      break;
  }
}
