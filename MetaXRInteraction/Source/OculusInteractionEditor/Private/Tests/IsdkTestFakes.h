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
#include "Core/IsdkConditional.h"
#include "Core/IsdkConditionalBool.h"
#include "IsdkFunctionLibrary.h"
#include "DataSources/IsdkExternalHandDataSource.h"
#include "Interaction/IsdkSceneInteractorComponent.h"
#include "Interaction/IsdkIInteractableState.h"
#include "Subsystem/IsdkITrackingDataSubsystem.h"
#include "UObject/Object.h"
#include <functional>

#include "Rig/IsdkControllerRigComponent.h"
#include "Rig/IsdkHandVisualsRigComponent.h"
#include "IsdkTestFakes.generated.h"

UCLASS()
class OCULUSINTERACTIONEDITOR_API UIsdkFakeTrackingDataSubsystem
    : public UWorldSubsystem,
      public IIsdkITrackingDataSubsystem
{
  GENERATED_BODY()

 public:
  UIsdkFakeTrackingDataSubsystem();
  virtual bool ShouldCreateSubsystem(UObject* Outer) const override
  {
    return GIsAutomationTesting;
  }

  virtual FIsdkTrackingDataSources GetOrCreateHandDataSourceComponent_Implementation(
      UMotionControllerComponent* SourceMotionController,
      EIsdkHandedness Handedness) override;

  virtual FIsdkTrackingDataSources GetOrCreateControllerDataSourceComponent_Implementation(
      UMotionControllerComponent* SourceMotionController,
      EIsdkHandedness Handedness) override;

  virtual void SetControllerHandBehavior(EControllerHandBehavior InControllerHandBehavior) override
  {
    ControllerHandBehavior = InControllerHandBehavior;
  }

  virtual EControllerHandBehavior GetControllerHandBehavior() override
  {
    return ControllerHandBehavior;
  }

  virtual TScriptInterface<IIsdkIHmdDataSource> GetOrCreateHmdDataSourceComponent_Implementation(
      AActor* TrackingSpaceRoot) override
  {
    return {};
  }

  virtual FIsdkTrackedDataSubsystem_ControllerHandsBehaviorChangedDelegate*
  GetControllerHandBehaviorChangedDelegate() override
  {
    return &ControllerHandsBehaviorChangedDelegate;
  }

  virtual bool IsEnabled_Implementation() override
  {
    return true;
  }
  EControllerHandBehavior ControllerHandBehavior = EControllerHandBehavior::BothAnimated;

  UPROPERTY()
  UIsdkFakeHandDataSource* HandDataSource{};
  UPROPERTY()
  UIsdkFakeHandDataSource* ControllerDataSource{};

  FIsdkTrackedDataSubsystem_ControllerHandsBehaviorChangedDelegate
      ControllerHandsBehaviorChangedDelegate;
};

UCLASS()
class UIsdkFakeHandDataSource : public UIsdkExternalHandDataSource,
                                public IIsdkIRootPose,
                                public IIsdkIHandPointerPose
{
  GENERATED_BODY()

 public:
  UIsdkFakeHandDataSource()
  {
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.SetTickFunctionEnable(true);

    bWantsInitializeComponent = true;
    IsConnected = NewObject<UIsdkConditionalBool>(this, TEXT("IsConnected"));

    bUpdateInTick = true;
  }

  virtual FTransform GetRootPose_Implementation() override
  {
    return FakeRootPose;
  }

  virtual bool IsRootPoseValid_Implementation() override
  {
    return bIsFakeRootPoseValid;
  }

  virtual UIsdkConditional* GetRootPoseConnectedConditional_Implementation() override
  {
    return IsConnected;
  }

  virtual bool IsPointerPoseValid_Implementation() override
  {
    return bIsFakePointerPoseValid;
  }

  virtual void GetPointerPose_Implementation(FTransform& PointerPose, bool& IsValid) override
  {
    IsValid = bIsFakePointerPoseValid;
    PointerPose = FakeRootPose * FakePointerPoseRelative;
  }

  virtual void GetRelativePointerPose_Implementation(FTransform& PointerRelativePose, bool& IsValid)
      override
  {
    IsValid = bIsFakePointerPoseValid;
    PointerRelativePose = FakePointerPoseRelative;
  }

  virtual bool IsHandJointDataValid_Implementation() override
  {
    return bIsFakeJointDataValid;
  }

  void SetHandJointsToPinchPose()
  {
    const TArray<FTransform> Poses = {
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(-0.145789, 5.974506, 1.9), FVector::One()),
        FTransform(FRotator(-0.091246, -0.045623, -90.0), FVector(0.0, 0.0, 0.0), FVector::One()),
        // Thumb
        FTransform(
            FRotator(48.9851, -113.701698, 172.928802),
            FVector(2.139567, 3.824452, 2.453524),
            FVector::One()),
        FTransform(
            FRotator(57.150665, -100.346275, 164.179504),
            FVector(2.752405, 6.216104, 4.569017),
            FVector::One()),
        FTransform(
            FRotator(47.236786, -111.416557, 159.337891),
            FVector(2.336923, 9.070005, 6.330366),
            FVector::One()),
        FTransform(
            FRotator(47.236782, -111.416557, 159.337891),
            FVector(2.262305, 10.992578, 7.866672),
            FVector::One()),
        // Index
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(2.004335, 3.77878, 0.284375), FVector::One()),
        FTransform(
            FRotator(4.715132, 1.606428, -138.849655),
            FVector(2.355067, 9.599621, 0.731645),
            FVector::One()),
        FTransform(
            FRotator(3.462922, 4.358374, 178.681244),
            FVector(2.047428, 12.087732, 3.577596),
            FVector::One()),
        FTransform(
            FRotator(6.449874, 5.817866, 160.726379),
            FVector(1.903415, 12.020663, 6.002761),
            FVector::One()),
        FTransform(
            FRotator(6.449874, 5.817866, 160.726379),
            FVector(1.697909, 11.35505, 8.130509),
            FVector::One()),
        // Middle
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(0.383252, 3.780457, -0.507344), FVector::One()),
        FTransform(
            FRotator(-0.750007, 5.529117, -115.12471),
            FVector(0.172591, 9.564658, 0.254315),
            FVector::One()),
        FTransform(
            FRotator(-1.730706, 6.580639, -133.013306),
            FVector(-0.18154, 13.435093, 2.076834),
            FVector::One()),
        FTransform(
            FRotator(-5.628296, 10.355316, -137.51918),
            FVector(-0.358187, 15.442487, 3.955384),
            FVector::One()),
        FTransform(
            FRotator(-5.628296, 10.355315, -137.51918),
            FVector(-0.538097, 17.208802, 5.714391),
            FVector::One()),
        // Ring
        FTransform(
            FRotator(0.0, 0.0, 0.0), FVector(-1.035655, 3.796854, -0.203718), FVector::One()),
        FTransform(
            FRotator(-5.503546, 11.743522, -108.604683),
            FVector(-1.746524, 8.869377, 0.652931),
            FVector::One()),
        FTransform(
            FRotator(-9.39999, 13.546801, -127.83728),
            FVector(-2.384943, 12.511496, 1.891606),
            FVector::One()),
        FTransform(
            FRotator(-12.411263, 11.693164, -137.136856),
            FVector(-2.619735, 14.613632, 3.500127),
            FVector::One()),
        FTransform(
            FRotator(-12.411263, 11.693164, -137.136856),
            FVector(-2.653035, 16.416984, 5.140535),
            FVector::One()),
        // Pinky
        FTransform(
            FRotator(-23.404991, 17.665907, -95.831917),
            FVector(-2.299857, 3.407354, 0.941983),
            FVector::One()),
        FTransform(
            FRotator(-13.076055, 20.247158, -109.732986),
            FVector(-3.505405, 7.789581, 1.369117),
            FVector::One()),
        FTransform(
            FRotator(-16.281265, 27.057407, -130.770691),
            FVector(-4.288184, 10.582926, 2.380006),
            FVector::One()),
        FTransform(
            FRotator(-20.059959, 22.687025, -135.843262),
            FVector(-4.658125, 12.121253, 3.653663),
            FVector::One()),
        FTransform(
            FRotator(-20.059959, 22.687025, -135.843262),
            FVector(-4.789843, 13.815973, 5.043576),
            FVector::One())};
    HandData->GetJointPoses() = Poses;
    SetImplHandData(FTransform::Identity);
  }

  void SetHandJointsToPalmGrabPose()
  {
    const TArray<FTransform> Poses = {
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(-0.145789, 5.974506, 1.9), FVector::One()),
        FTransform(FRotator(-0.091246, -0.045622, -90.0), FVector(0.0, 0.0, 0.0), FVector::One()),
        // Thumb
        FTransform(
            FRotator(52.050983, -108.273201, -172.723114),
            FVector(2.521744, 3.719525, 2.192914),
            FVector::One()),
        FTransform(
            FRotator(57.551983, -93.126663, 162.632339),
            FVector(3.710712, 6.006842, 4.17418),
            FVector::One()),
        FTransform(
            FRotator(50.80867, -99.382751, 106.833755),
            FVector(2.852018, 8.780964, 5.902362),
            FVector::One()),
        FTransform(
            FRotator(50.80867, -99.382751, 106.833755),
            FVector(0.668735, 9.822522, 6.361195),
            FVector::One()),
        // Index
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(2.004335, 3.77878, 0.284375), FVector::One()),
        FTransform(
            FRotator(5.245879, 4.482617, -159.614227),
            FVector(2.355067, 9.599621, 0.731645),
            FVector::One()),
        FTransform(
            FRotator(5.051727, 7.398309, 102.752708),
            FVector(1.924755, 10.8911, 4.271658),
            FVector::One()),
        FTransform(
            FRotator(6.258151, 5.315538, 31.693542),
            FVector(2.181224, 8.534093, 4.805893),
            FVector::One()),
        FTransform(
            FRotator(6.258151, 5.315538, 31.693542),
            FVector(2.467975, 7.293154, 2.964582),
            FVector::One()),
        // Middle
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(0.383252, 3.780457, -0.507344), FVector::One()),
        FTransform(
            FRotator(-0.337989, 9.835943, -168.096588),
            FVector(0.17259, 9.564658, 0.254315),
            FVector::One()),
        FTransform(
            FRotator(0.101305, 11.329015, 107.865074),
            FVector(0.042381, 10.44072, 4.45465),
            FVector::One()),
        FTransform(
            FRotator(10.996787, 10.799524, 37.899353),
            FVector(0.55386, 7.868971, 5.299803),
            FVector::One()),
        FTransform(
            FRotator(10.996787, 10.799524, 37.899353),
            FVector(1.182541, 6.336322, 3.428417),
            FVector::One()),
        // Ring
        FTransform(
            FRotator(0.0, 0.0, 0.0), FVector(-1.035655, 3.796854, -0.203718), FVector::One()),
        FTransform(
            FRotator(-5.221974, 9.86189, -171.752533),
            FVector(-1.746524, 8.869377, 0.652931),
            FVector::One()),
        FTransform(
            FRotator(-4.364793, 15.120712, 105.406532),
            FVector(-1.499322, 9.48014, 4.496469),
            FVector::One()),
        FTransform(
            FRotator(0.092187, 16.545042, 32.913406),
            FVector(-0.781224, 7.020455, 5.20055),
            FVector::One()),
        FTransform(
            FRotator(0.092187, 16.545042, 32.913406),
            FVector(-0.38968, 5.617006, 3.245774),
            FVector::One()),
        // Pinky
        FTransform(
            FRotator(-23.405005, 17.665907, -95.831924),
            FVector(-2.299857, 3.407354, 0.941983),
            FVector::One()),
        FTransform(
            FRotator(-16.267633, 9.181431, -172.712067),
            FVector(-3.505405, 7.789581, 1.369118),
            FVector::One()),
        FTransform(
            FRotator(-10.04005, 17.072643, 107.503662),
            FVector(-2.727241, 8.310124, 4.295021),
            FVector::One()),
        FTransform(
            FRotator(-8.762888, 21.234495, 31.231606),
            FVector(-2.058257, 6.489198, 4.896848),
            FVector::One()),
        FTransform(
            FRotator(-8.762888, 21.234495, 31.231606),
            FVector(-1.844879, 5.241109, 3.102962),
            FVector::One())};
    HandData->GetJointPoses() = Poses;
    SetImplHandData(FTransform::Identity);
  }

  void SetHandJointsToNeutralPose()
  {
    const TArray<FTransform> Poses = {
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(-0.145789, 5.974506, 1.9), FVector::One()),
        FTransform(FRotator(-0.091246, -0.045623, -90.0), FVector(0.0, 0.0, 0.0), FVector::One()),
        // Thumb
        FTransform(
            FRotator(51.723598, -107.057266, -168.919556),
            FVector(2.617297, 3.683103, 2.109033),
            FVector::One()),
        FTransform(
            FRotator(56.412647, -87.714256, -158.379303),
            FVector(3.949862, 5.895951, 4.083484),
            FVector::One()),
        FTransform(
            FRotator(52.270557, -108.61412, -157.979355),
            FVector(5.089581, 8.562062, 5.819171),
            FVector::One()),
        FTransform(
            FRotator(52.270554, -108.61412, -157.979355),
            FVector(6.632873, 9.957041, 7.136128),
            FVector::One()),
        // Index
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(2.004335, 3.77878, 0.284375), FVector::One()),
        FTransform(
            FRotator(3.672261, -1.9105, -107.535576),
            FVector(2.355067, 9.599621, 0.731645),
            FVector::One()),
        FTransform(
            FRotator(1.124207, -0.217482, -123.104958),
            FVector(2.399471, 13.21663, 1.871841),
            FVector::One()),
        FTransform(
            FRotator(1.271785, 3.251185, -128.768814),
            FVector(2.37922, 15.252568, 3.198947),
            FVector::One()),
        FTransform(
            FRotator(1.271785, 3.251185, -128.768814),
            FVector(2.216313, 17.054053, 4.518301),
            FVector::One()),
        // Middle
        FTransform(FRotator(0.0, 0.0, 0.0), FVector(0.383252, 3.780457, -0.507344), FVector::One()),
        FTransform(
            FRotator(-0.665182, 8.208684, -107.802528),
            FVector(0.172591, 9.564658, 0.254315),
            FVector::One()),
        FTransform(
            FRotator(-1.77126, 9.125257, -125.248825),
            FVector(-0.39927, 13.611616, 1.566699),
            FVector::One()),
        FTransform(
            FRotator(-6.186466, 12.376503, -130.331497),
            FVector(-0.709723, 15.840434, 3.155968),
            FVector::One()),
        FTransform(
            FRotator(-6.186466, 12.376504, -130.331497),
            FVector(-1.004189, 17.799576, 4.67958),
            FVector::One()),
        // Ring
        FTransform(
            FRotator(0.0, 0.0, 0.0), FVector(-1.035655, 3.796854, -0.203718), FVector::One()),
        FTransform(
            FRotator(-5.242828, 15.561725, -110.46283),
            FVector(-1.746524, 8.869377, 0.652931),
            FVector::One()),
        FTransform(
            FRotator(-9.052396, 17.465261, -128.119843),
            FVector(-2.609654, 12.421574, 2.010807),
            FVector::One()),
        FTransform(
            FRotator(-12.057053, 15.611009, -137.121216),
            FVector(-2.992892, 14.492616, 3.631102),
            FVector::One()),
        FTransform(
            FRotator(-12.057053, 15.611009, -137.121216),
            FVector(-3.159324, 16.287161, 5.273063),
            FVector::One()),
        // Pinky
        FTransform(
            FRotator(-23.404987, 17.665911, -95.831924),
            FVector(-2.299857, 3.407354, 0.941983),
            FVector::One()),
        FTransform(
            FRotator(-12.44035, 27.304699, -118.198044),
            FVector(-3.505405, 7.789581, 1.369118),
            FVector::One()),
        FTransform(
            FRotator(-14.571406, 34.404102, -136.763519),
            FVector(-4.471629, 10.337714, 2.78716),
            FVector::One()),
        FTransform(
            FRotator(-18.740183, 30.47057, -140.962524),
            FVector(-4.951909, 11.695146, 4.219716),
            FVector::One()),
        FTransform(
            FRotator(-18.740183, 30.47057, -140.962524),
            FVector(-5.2308, 13.242616, 5.752309),
            FVector::One()),
    };
    HandData->GetJointPoses() = Poses;
    SetImplHandData(FTransform::Identity);
  }

  virtual void InitializeComponent() override
  {
    Super::InitializeComponent();

    const auto ThumbJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRThumbMapping();
    const auto FingerJointMappings = UIsdkFunctionLibrary::GetDefaultOpenXRFingerMapping();
    SetHandJointMappings(ThumbJointMappings, FingerJointMappings);
  }

  UPROPERTY()
  UIsdkConditionalBool* IsConnected{};

  bool bIsFakeRootPoseValid = true;
  bool bIsFakePointerPoseValid = true;
  bool bIsFakeJointDataValid = true;

  FTransform FakeRootPose = FTransform::Identity;
  FTransform FakePointerPoseRelative = FTransform::Identity;
};

UCLASS()
class UIsdkFakeHmdDataSource : public UActorComponent, public IIsdkIHmdDataSource
{
  GENERATED_BODY()

 public:
  virtual bool IsHmdTracked_Implementation() override
  {
    return bFakeIsTracked;
  }

  virtual void GetHmdPose_Implementation(FTransform& HmdPose, bool& IsTracked) override
  {
    HmdPose = FakeHmdPose;
    IsTracked = bFakeIsTracked;
  }

  virtual void GetRelativeHmdPose_Implementation(FTransform& HmdRelativePose, bool& IsTracked)
      override
  {
    HmdRelativePose = FakeHmdRelativePose;
    IsTracked = bFakeIsTracked;
  }

  FTransform FakeHmdPose{};
  FTransform FakeHmdRelativePose{};
  bool bFakeIsTracked{false};
};

UCLASS()
class UIsdkFakeControllerRigComponentLeft : public UIsdkControllerRigComponentLeft
{
  GENERATED_BODY()

 public:
  virtual void OnControllerVisualsAttached() override {}
};

UCLASS()
class UIsdkFakeSceneInteractor : public UIsdkSceneInteractorComponent
{
  GENERATED_BODY()

 public:
  void SetStateImpl(EIsdkInteractorState NewState)
  {
    SetState(NewState);
  }
};

UCLASS()
class OCULUSINTERACTIONEDITOR_API UIsdkFakeHandPointerPose : public UActorComponent,
                                                             public IIsdkIHandPointerPose
{
  GENERATED_BODY()

 public:
  virtual bool IsPointerPoseValid_Implementation() override
  {
    return bIsPointerPoseValid;
  }
  virtual void GetPointerPose_Implementation(FTransform& OutPointerPose, bool& OutIsValid) override
  {
    OutPointerPose = PointerPose;
    OutIsValid = bIsPointerPoseValid;
  }
  virtual void GetRelativePointerPose_Implementation(
      FTransform& OutPointerRelativePose,
      bool& OutIsValid) override
  {
    OutPointerRelativePose = RelativePointerPose;
    OutIsValid = bIsPointerPoseValid;
  }

  bool bIsPointerPoseValid{false};
  FTransform PointerPose{};
  FTransform RelativePointerPose{};
};

UCLASS()
class UIsdkFakeInteractorStateChangedHandler : public UObject
{
  GENERATED_BODY()

 public:
  UFUNCTION()
  void HandleStateChanged(const FIsdkInteractorStateEvent& InteractorState)
  {
    OnStateChange(InteractorState);
  }

  void SetLambda(std::function<void(const FIsdkInteractorStateEvent&)>&& InOnStateChange)
  {
    OnStateChange = std::move(InOnStateChange);
  }

 private:
  std::function<void(const FIsdkInteractorStateEvent&)> OnStateChange =
      [](const FIsdkInteractorStateEvent&) {};
};

UCLASS()
class UIsdkFakeInteractableStateChangedHandler : public UObject
{
  GENERATED_BODY()

 public:
  UFUNCTION()
  void HandleStateChanged(const FIsdkInteractableStateEvent& InteractableState)
  {
    OnStateChange(InteractableState);
  }

  UFUNCTION()
  void HandleInteractablePointerEvent(const FIsdkInteractionPointerEvent& PointerEvent)
  {
    OnPointerEvent(PointerEvent);
  }

  void SetOnStateChangeLambda(
      std::function<void(const FIsdkInteractableStateEvent&)>&& InOnStateChange)
  {
    OnStateChange = std::move(InOnStateChange);
  }
  void SetOnPointerEventLambda(
      std::function<void(const FIsdkInteractionPointerEvent&)>&& InOnPointerEvent)
  {
    OnPointerEvent = std::move(InOnPointerEvent);
  }

 private:
  std::function<void(const FIsdkInteractableStateEvent&)> OnStateChange =
      [](const FIsdkInteractableStateEvent&) {};
  std::function<void(const FIsdkInteractionPointerEvent&)> OnPointerEvent =
      [](const FIsdkInteractionPointerEvent&) {};
};

UCLASS()
class UIsdkFakeTrackedDataSourceRigComponent : public UIsdkTrackedDataSourceRigComponent
{
  GENERATED_BODY()
 public:
  UIsdkFakeTrackedDataSourceRigComponent()
  {
    FakeMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FakeMeshComponent"));
    // Do not have a synthetic visual.
    HandVisibilityMode = EIsdkRigHandVisibility::TrackedOnly;
  }
  virtual void BeginPlay() override
  {
    Super::BeginPlay();
    CustomTrackingData = GetWorld()->GetSubsystem<UIsdkFakeTrackingDataSubsystem>();
    check(CustomTrackingData);
  }

  virtual USceneComponent* GetTrackedVisual() const
  {
    return FakeMeshComponent;
  }
  virtual USceneComponent* GetSyntheticVisual() const
  {
    return nullptr;
  }
  virtual void GetInteractorSocket(
      USceneComponent*& OutSocketComponent,
      FName& OutSocketName,
      EIsdkHandBones HandBone) const override
  {
    OutSocketComponent = FakeMeshComponent;
    OutSocketName = NAME_None;
  }

  UPROPERTY()
  TObjectPtr<USkeletalMeshComponent> FakeMeshComponent;
};

/// Test implementation of abstract UIsdkRigComponent, so that we can test base class functionality.
UCLASS()
class UIsdkTestHandRigComponent : public UIsdkRigComponent
{
  GENERATED_BODY()

 public:
  UIsdkTestHandRigComponent()
  {
    Handedness = EIsdkHandedness::Left;
    FakeVisuals =
        CreateDefaultSubobject<UIsdkFakeTrackedDataSourceRigComponent>(TEXT("FakeVisuals"));
    SetRigComponentDefaults();
  }

  virtual void BeginPlay() override
  {
    Super::BeginPlay();
    FakeVisuals->TryAttachToParentMotionController(this);
    FakeVisuals->CreateDataSourcesAsTrackedHand();
  }

  virtual UIsdkTrackedDataSourceRigComponent* GetVisuals() const override
  {
    return FakeVisuals;
  }

  virtual float GetPinchStrength() const override
  {
    return TestPinchStrength;
  }

  UPROPERTY()
  UIsdkFakeTrackedDataSourceRigComponent* FakeVisuals;

  float TestPinchStrength = 0.0;
};
