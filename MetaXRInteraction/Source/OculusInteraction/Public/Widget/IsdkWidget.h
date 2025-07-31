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
#include "Components/WidgetComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Layout/WidgetPath.h"
#include "StructTypes.h"

#include "IsdkWidget.generated.h"

class FSlateVirtualUserHandle;
class UWidgetComponent;
class UIsdkWidgetSubsystem;

#pragma region WidgetEvent
UENUM(BlueprintType)
enum class EIsdkWidgetEventType : uint8
{
  SelectedHovered = 0,
  SelectedEmpty = 1,
  UnselectedHovered = 2,
  UnselectedEmpty = 3,
};

USTRUCT(BlueprintType)
struct FIsdkWidgetEvent
{
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  UObject* Interactor = nullptr;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteractionSDK)
  EIsdkWidgetEventType Type{};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FIsdkWidgetEventDelegate,
    FIsdkWidgetEvent,
    WidgetEvent);
#pragma endregion WidgetEvent

#pragma region Virtual User Pointer Event

/**
 * An interaction event for use with Slate Widgets. It contains information about which virtual user
 * is emitting the event
 */
USTRUCT(BlueprintType)
struct FIsdkVirtualUserPointerEvent
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  UObject* Interactor{};
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  EIsdkPointerEventType Type{};
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  FVector Position = FVector::ZeroVector;
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  int32 VirtualUserIndex{};
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  int32 PointerIndex{};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FIsdkVirtualUserPointerEventDelegate,
    FIsdkVirtualUserPointerEvent,
    WidgetPointerEvent);

#pragma endregion Virtual User Pointer Event

#pragma region Virtual User State - Multi - touch support
USTRUCT()
struct FIsdkWidgetVirtualUserState
{
  GENERATED_BODY()

  UPROPERTY()
  UObject* Interactor = nullptr;

  TSharedPtr<FSlateVirtualUserHandle> VirtualUser{};
  int32 PointerIndex{};
  FVector HitPositionOnSelect{};
  FVector CurrentHitPosition{};
  FVector2D LastLocalHitLocation{};
  FVector2D CurrentLocalHitLocation{};
  FWeakWidgetPath WidgetPathOnPointerDown{};
  uint8 bTouching : 1;
  uint8 bMoving : 1;
};
#pragma endregion Virtual User State - Multi - touch support

/**
 * @brief Static methods to move data from ISDK into Slate
 */
UCLASS(ClassGroup = (InteractionSDK))
class OCULUSINTERACTION_API UIsdkWidget : public UObject
{
  GENERATED_BODY()
 public:
  inline static const TSet<FKey> PressedKeysDefault = TSet<FKey>();

#pragma region Helper Methods
  // Widget Path for last local hit position
  static FWidgetPath GetWidgetPath(
      UWidgetComponent* AttachedWidget,
      const FIsdkWidgetVirtualUserState& State)
  {
    return FWidgetPath(AttachedWidget->GetHitWidgetPath(State.CurrentLocalHitLocation, false));
  }

  static bool TraveledEnoughForMove(
      const FIsdkWidgetVirtualUserState& State,
      const float MinMoveTravelDistance)
  {
    const float DistanceTraveled =
        FVector::Distance(State.CurrentHitPosition, State.HitPositionOnSelect);
    return DistanceTraveled > MinMoveTravelDistance;
  }
  static FPointerEvent GenerateTouchEvent(const FIsdkWidgetVirtualUserState& State)
  {
    return FPointerEvent(
        State.VirtualUser->GetUserIndex(),
        State.PointerIndex,
        State.CurrentLocalHitLocation,
        State.LastLocalHitLocation,
        1.0f,
        false,
        false,
        false,
        FModifierKeysState(),
        0); // Touch index needs to be updated
  }
  static FPointerEvent GenerateKeyEvent(const FIsdkWidgetVirtualUserState& State)
  {
    return FPointerEvent(
        State.VirtualUser->GetUserIndex(),
        State.PointerIndex,
        State.CurrentLocalHitLocation,
        State.LastLocalHitLocation,
        PressedKeysDefault,
        FKey(),
        0.0f,
        FModifierKeysState());
  }
  static void UpdateLocalHitLocation(
      UWidgetComponent* AttachedWidget,
      FIsdkWidgetVirtualUserState& State)
  {
    State.LastLocalHitLocation = State.CurrentLocalHitLocation;
    AttachedWidget->GetLocalHitLocation(State.CurrentHitPosition, State.CurrentLocalHitLocation);
  }
#pragma endregion Helper Methods

#pragma region Routing
  static FReply RouteTouchDownPointerEvent(
      UWidgetComponent* AttachedWidget,
      FIsdkWidgetVirtualUserState& State)
  {
    const FWidgetPath WidgetPath = GetWidgetPath(AttachedWidget, State);
    const FPointerEvent TouchEvent = GenerateTouchEvent(State);
    const auto Reply = FSlateApplication::Get().RoutePointerDownEvent(WidgetPath, TouchEvent);

    // Store the path so that it can be used in a touch-up event in the case that the move threshold
    // was not reached.
    State.WidgetPathOnPointerDown = WidgetPath;
    AttachedWidget->RequestRedraw();
    return Reply;
  }
  static void RouteTouchMovePointerEvent(
      UWidgetComponent* AttachedWidget,
      FIsdkWidgetVirtualUserState& State)
  {
    const FPointerEvent TouchEvent = GenerateTouchEvent(State);
    FSlateApplication::Get().RoutePointerMoveEvent(
        GetWidgetPath(AttachedWidget, State), TouchEvent, false);

    // Started touch move, make sure touch up is not on the same path
    // to ensure the click event is not triggered anymore
    State.WidgetPathOnPointerDown = {};
    AttachedWidget->RequestRedraw();
  }
  static FReply RouteTouchUpPointerEvent(
      UWidgetComponent* AttachedWidget,
      FIsdkWidgetVirtualUserState& State)
  {
    const FWidgetPath WidgetPath = State.WidgetPathOnPointerDown.IsValid()
        ? State.WidgetPathOnPointerDown.ToWidgetPath()
        : GetWidgetPath(AttachedWidget, State);
    const FPointerEvent TouchEvent = GenerateTouchEvent(State);
    const auto Reply = FSlateApplication::Get().RoutePointerUpEvent(WidgetPath, TouchEvent);

    State.WidgetPathOnPointerDown = {};
    AttachedWidget->RequestRedraw();
    return Reply;
  }

  static void RouteMouseHoverPointerEvent(
      UWidgetComponent* AttachedWidget,
      FIsdkWidgetVirtualUserState& State)
  {
    FWidgetPath WidgetPath = GetWidgetPath(AttachedWidget, State);
    FPointerEvent PointerEvent = GenerateKeyEvent(State);
    FSlateApplication::Get().RoutePointerMoveEvent(WidgetPath, PointerEvent, false);
    AttachedWidget->RequestRedraw();
  }
#pragma endregion Routing

#pragma region Virtual User Pointer Event Handling
  static void HandleVirtualUserPointerEvent(
      FIsdkVirtualUserPointerEvent VirtualUserPointerEvent,
      UWidgetComponent* AttachedWidget,
      FIsdkWidgetEventDelegate WidgetEventDelegate,
      UIsdkWidgetSubsystem& WidgetSubsystem,
      float MinMoveTravelDistance = 0.1f,
      bool bStopBroadcastOnDrag = true);
#pragma endregion Virtual User Pointer Event Handling
};
