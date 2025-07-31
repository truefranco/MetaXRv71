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
#include "Components/ActorComponent.h"
#include "Interaction/IsdkPokeInteractable.h"
#include "Curves/CurveFloat.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/UnrealMathUtility.h"
#include "IsdkInteractableColorVisual.generated.h"

USTRUCT(BlueprintType)
struct FIsdkColorState
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  FLinearColor Color = FLinearColor::Black;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  UCurveFloat* TransitionCurve = nullptr;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  float TransitionDuration = 0.1f;
};

USTRUCT(BlueprintType)
struct FIsdkColorTransition
{
  GENERATED_BODY()

 private:
  bool Active = false;
  FLinearColor StartColor = FLinearColor::Black;
  FIsdkColorState TargetState;
  float Timer = 0.f;
  FLinearColor CurrentColor = FLinearColor::Black;

 public:
  FLinearColor GetCurrentColor() const
  {
    return CurrentColor;
  }
  bool IsActive() const
  {
    return Active;
  }
  void UpdateTransition(float DeltaTime)
  {
    Timer += DeltaTime;
    float NormalizedTimer =
        TargetState.TransitionDuration < 0.001 ? 1.0 : (Timer / TargetState.TransitionDuration);
    NormalizedTimer = FMath::Clamp(NormalizedTimer, 0.0f, 1.0f);
    float Parameter = IsValid(TargetState.TransitionCurve)
        ? TargetState.TransitionCurve->GetFloatValue(NormalizedTimer)
        : NormalizedTimer;
    CurrentColor = FLinearColor::LerpUsingHSV(StartColor, TargetState.Color, Parameter);
    if (IsTransitionFinished())
    {
      Active = false;
    }
  }

  bool IsTransitionFinished() const
  {
    return Timer > TargetState.TransitionDuration;
  }
  static FIsdkColorTransition ImmediateTransitionTo(const FIsdkColorState& ColorState)
  {
    FIsdkColorTransition Transition = FIsdkColorTransition();
    Transition.CurrentColor = ColorState.Color;
    Transition.StartColor = ColorState.Color;
    Transition.TargetState = ColorState;
    Transition.Timer = ColorState.TransitionDuration + 1.0;
    Transition.Active = true;
    return Transition;
  }
  FIsdkColorTransition TransitionTo(const FIsdkColorState& ColorState) const
  {
    FIsdkColorTransition Transition = FIsdkColorTransition();
    Transition.CurrentColor = CurrentColor;
    Transition.StartColor = CurrentColor;
    Transition.TargetState = ColorState;
    Transition.Timer = 0.0;
    Transition.Active = true;
    return Transition;
  }
};

UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Interactable Color Visual"))
class OCULUSINTERACTIONPREBUILTS_API UIsdkInteractableColorVisual : public UActorComponent
{
  GENERATED_BODY()

 public:
  // Sets default values for this component's properties
  UIsdkInteractableColorVisual();

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetTransitionToState(EIsdkInteractableState State);
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetImmediateTransitionToState(EIsdkInteractableState State);
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetColorState(EIsdkInteractableState State, FIsdkColorState ColorState);
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  FIsdkColorState GetColorState(EIsdkInteractableState State) const;

  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  void SetColorStates(
      FIsdkColorState InNormalColorState,
      FIsdkColorState InHoverColorState,
      FIsdkColorState InSelectColorState,
      FIsdkColorState InDisabledColorState)
  {
    NormalColorState = InNormalColorState;
    HoverColorState = InHoverColorState;
    SelectColorState = InSelectColorState;
    DisabledColorState = InDisabledColorState;

    if (IsValid(Interactable.GetObject()))
    {
      if (HasBegunPlay())
      {
        SetTransitionToState(Interactable->GetInteractableState());
      }
    }
  }

  UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, Category = InteractionSDK)
  TScriptInterface<IIsdkIInteractableState> GetInteractable() const
  {
    return Interactable;
  }
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  FName ParameterName;
  UPROPERTY(BlueprintReadWrite, Category = InteractionSDK)
  UMaterialInstanceDynamic* DynamicMaterial{};

  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetInteractable(TScriptInterface<IIsdkIInteractableState> InInteractable);
  virtual void OnRegister() override;

  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  FIsdkColorState GetNormalColorState() const
  {
    return NormalColorState;
  }
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  FIsdkColorState GetHoverColorState() const
  {
    return HoverColorState;
  }
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  FIsdkColorState GetSelectColorState() const
  {
    return SelectColorState;
  }
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  FIsdkColorState GetDisabledColorState() const
  {
    return DisabledColorState;
  }

  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetNormalColorState(const FIsdkColorState& ColorState)
  {
    SetColorState(EIsdkInteractableState::Normal, ColorState);
  }
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetHoverColorState(const FIsdkColorState& ColorState)
  {
    SetColorState(EIsdkInteractableState::Hover, ColorState);
  }
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetSelectColorState(const FIsdkColorState& ColorState)
  {
    SetColorState(EIsdkInteractableState::Select, ColorState);
  }
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetDisabledColorState(const FIsdkColorState& ColorState)
  {
    SetColorState(EIsdkInteractableState::Disabled, ColorState);
  }

 private:
  virtual void BeginPlay() override;
  virtual void BeginDestroy() override;
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;
  UPROPERTY(
      BlueprintGetter = GetInteractable,
      BlueprintSetter = SetInteractable,
      Category = InteractionSDK)
  TScriptInterface<IIsdkIInteractableState> Interactable;
  FIsdkColorTransition ColorTransition;
  UPROPERTY(
      BlueprintGetter = GetNormalColorState,
      BlueprintSetter = SetNormalColorState,
      EditAnywhere,
      Category = InteractionSDK)
  FIsdkColorState NormalColorState;
  UPROPERTY(
      BlueprintGetter = GetHoverColorState,
      BlueprintSetter = SetHoverColorState,
      EditAnywhere,
      Category = InteractionSDK)
  FIsdkColorState HoverColorState;
  UPROPERTY(
      BlueprintGetter = GetSelectColorState,
      BlueprintSetter = SetSelectColorState,
      EditAnywhere,
      Category = InteractionSDK)
  FIsdkColorState SelectColorState;
  UPROPERTY(
      BlueprintGetter = GetDisabledColorState,
      BlueprintSetter = SetDisabledColorState,
      EditAnywhere,
      Category = InteractionSDK)
  FIsdkColorState DisabledColorState;

  UFUNCTION()
  void HandleStateChange(const FIsdkInteractableStateEvent& Event);
};
