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
#include "GameplayTagContainer.h"
#include "Subsystem/IsdkWidgetSubsystem.h"
#include "IsdkRigComponent.generated.h"

class UIsdkRigComponent;
class UIsdkInteractionGroupRigComponent;
class UIsdkInputActionsRigComponent;
class UIsdkGrabInteractionRigComponent;
class UIsdkPokeInteractionRigComponent;
class UIsdkRayInteractionRigComponent;
class UIsdkControllerVisualsRigComponent;
class UIsdkHandVisualsRigComponent;
class UIsdkRayInteractor;
class UIsdkPokeLimiterVisual;
class UIsdkPokeInteractor;
class UIsdkGrabberComponent;
class UIsdkHandFingerPinchGrabRecognizer;
class UIsdkRigModifier;
class UIsdkTrackedDataSourceRigComponent;
class UEnhancedInputComponent;
class IIsdkIHmdDataSource;
enum class EIsdkHandBones : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIsdkRigComponentLifecycleEvent);

const static FName NAME_isdk_PointerPoseSocket(TEXT("PointerPose"));

DECLARE_MULTICAST_DELEGATE_OneParam(FIsdkRigComponentSelectDelegate, UIsdkRigComponent*);

/**
 * @class UIsdkRigComponent
 * @brief Abstract base class for initializing interaction components
 *
 * Abstract base class used to initialize the base components necessary
 * for interaction.  It also provides an interface for interactors to get input from controllers
 * hands, agnostic of which is actively being used.
 * @addtogroup InteractionSDKPrebuiltsPrebuilts
 */
UCLASS(
    Abstract,
    Blueprintable,
    ClassGroup = ("InteractionSDK|Rig"),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Rig Component"))
class OCULUSINTERACTIONPREBUILTS_API UIsdkRigComponent : public USceneComponent
{
  GENERATED_BODY()

  friend class FIsdkTestControllerVisualsCreateDataSources;
  friend class FIsdkTestHandVisualsCreateDataSources;
  friend class FIsdkTestRigHandCheckInteractorEnabledValue;

 public:
  // Default constructor required for Unreal object system.
  UIsdkRigComponent();

  /*
   * @brief Called on BeginPlay. Implemented from UActorComponent
   */
  virtual void BeginPlay() override;

  /*
   * @brief Called on EndPlay. Implemented from UActorComponent
   */
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  /*
   * @brief Returns if component has any sockets present. Implemented from USceneComponent
   * @return bool Whether or not this component has any sockets. True by default.
   */
  virtual bool HasAnySockets() const override;

  /*
   * @brief Passes array of FComponentSocketDescription by reference. Implemented from
   * UActorComponent
   */
  virtual void QuerySupportedSockets(
      TArray<FComponentSocketDescription>& OutSockets) const override;

  /*
   * @brief Returns transform of given socket, using given Transform Space. Implemented from
   * UActorComponent
   * @return FTransform The transform for a given socket, in the transform space requested
   */
  virtual FTransform GetSocketTransform(
      FName InSocketName,
      ERelativeTransformSpace TransformSpace = RTS_World) const override;

  /*
   * @brief Given a socket name, returns whether or not it exists on this ActorComponent.
   * Implemented from UActorComponent
   * @return bool If a given socket name exists on this ActorComponent.
   */
  virtual bool DoesSocketExist(FName InSocketName) const override;

  /*
   * @brief Returns the socket name associated with attaching the Raycast Pointer
   * @return FName The socket used for attaching the ray pointer, hardcoded in UIsdkRigComponent but
   * may be overridden by children.
   */
  static FName GetPointerPoseSocketName();

  /*
   * Delegate broadcast when a pinch grab select event occurs
   */
  FIsdkRigComponentSelectDelegate OnPinchSelectDelegate;

  /*
   * Delegate broadcast when a pinch grab unselect event occurs
   */
  FIsdkRigComponentSelectDelegate OnPinchUnselectDelegate;

  /*
   * Delegate broadcast when a palm grab select event occurs
   */
  FIsdkRigComponentSelectDelegate OnPalmSelectDelegate;

  /*
   * Delegate broadcast when a palm grab unselect event occurs
   */
  FIsdkRigComponentSelectDelegate OnPalmUnselectDelegate;

  /**
   * @brief Returns the current pinch strength, defined by the implementing child class
   * @return float Current pinch strength, further defined by implementing child class, returns 0
   * otherwise.
   */
  virtual float GetPinchStrength() const PURE_VIRTUAL(GetPinchStrength, return 0.f;);

  /**
   * @brief Returns a pointer to the PokeInteractor attached to this component's Poke Interaction
   * Rig Component.
   * @return UIsdkPokeInteractor PokeInteractor attached to this component's PokeInteraction
   * component
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkPokeInteractor* GetPokeInteractor() const;

  /**
   * @brief Returns a pointer to the RayInteractor attached to this component's Ray Interaction Rig
   * Component.
   * @return UIsdkRayInteractor RayInteractor attached to this component's RayInteraction
   * component
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkRayInteractor* GetRayInteractor() const;

  /**
   * @brief Returns a pointer to the Poke Limiter Visual attached to this component's Poke
   * Interaction Rig Component.
   * @return UIsdkPokeLimiterVisual Poke Limiter Visual attached to this component's PokeInteraction
   * component
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkPokeLimiterVisual* GetPokeLimiterVisual() const;

  /**
   * @brief Returns a pointer to the Grabber attached to this component's
   * Grab Interaction Rig Component.
   * @return UIsdkGrabberComponent Grabber Component attached to this component's GrabInteraction
   * component
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkGrabberComponent* GetGrabber() const;

  /**
   * @brief Returns a pointer to this rig component's Ray Interaction Rig Component
   * @return UIsdkRayInteractionRigComponent Rig component used for handling ray interactions
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkRayInteractionRigComponent* GetRayInteraction() const;

  /**
   * @brief Returns a pointer to this rig component's Poke Interaction Rig Component
   * @return UIsdkPokeInteractionRigComponent Rig component used for handling poke interactions
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkPokeInteractionRigComponent* GetPokeInteraction() const;

  /**
   * @brief Returns a pointer to this rig component's Grab Interaction Rig Component
   * @return UIsdkGrabInteractionRigComponent Rig component used for handling grab interactions
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkGrabInteractionRigComponent* GetGrabInteraction() const;

  /**
   * @brief Returns a pointer to this rig component's Input Actions Rig Component
   * @return UIsdkInputActionsRigComponent Rig component used for binding input actions
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkInputActionsRigComponent* GetInputActions() const;

  /**
   * @brief Returns a pointer to this rig component's Interaction Group Rig Component
   * @return UIsdkInteractionGroupRigComponent Rig component used for activating/deactivating groups
   * of interactors
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  UIsdkInteractionGroupRigComponent* GetInteractionGroup() const;

  /**
   * @brief Returns struct reference representing a virtual user for interacting with widgets
   * @see UIsdkWidgetSubsystem
   * @return FIsdkVirtualUserInfo Struct for abstracting different handed interactors as distinct
   * widget users
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  const FIsdkVirtualUserInfo& GetWidgetVirtualUser() const;

  /**
   * @brief Sets a virtual user info for interacting with widgets
   * @see UIsdkWidgetSubsystem
   * @param InWidgetVirtualUser the FIsdkVirtualUserInfo to utilize for abstracting different handed
   * interactors as distinct widget users
   */
  UFUNCTION(BlueprintSetter, Category = InteractionSDK)
  void SetWidgetVirtualUser(const FIsdkVirtualUserInfo& InWidgetVirtualUser);

  /*
   * @brief Returns all of the active rig modifiers successfully spawned from RigModifiersToSpawn
   * @return TArray<UIsdkRigModifier*>& All RigModifiers that this RigComponent spawned
   */
  UFUNCTION(BlueprintGetter, Category = InteractionSDK)
  const TArray<UIsdkRigModifier*>& GetActiveRigModifiers() const;

  /*
   * @brief Returns reference to the HMD DataSource being utilized by this Rig Component
   * @return bool True if HMD DataSource passed by reference is valid
   */
  UFUNCTION(BlueprintCallable, Category = InteractionSDK)
  bool GetHmdDataSource(TScriptInterface<IIsdkIHmdDataSource>& HmdDataSourceOut) const;

  /*
   * @brief Returns reference to Tracked Data Source Rig Component used by this Rig Component
   * @return UIsdkTrackedDataSourceRigComponent Tracked Visuals, as defined by child class
   */
  virtual UIsdkTrackedDataSourceRigComponent* GetVisuals() const;

  /*
   * @brief Returns the current state of the Handedness enum on this Rig Component
   * @return EIsdkHandedness Handedness of this Rig Component (left or right)
   */
  EIsdkHandedness GetHandedness() const;

  /*
   * @brief Gets list of tags that should be added to interactions stemming from this rig.
   * @return FGameplayTagContainer All gameplay tags associated with this rig component
   */
  const FGameplayTagContainer& GetInteractionTags() const
  {
    return InteractionTags;
  }

  /*
   * DataSourcesReady is broadcast when HMD and hand / controller data are valid and ready to be
   * used
   */
  UPROPERTY(BlueprintAssignable, Category = InteractionSDK)
  FIsdkRigComponentLifecycleEvent DataSourcesReady;

 protected:
  /**
   * @brief Binds data sources for Ray, Poke and Grab interaction rig components and updates mesh
   * dependencies.
   */
  virtual void UpdateComponentDataSources();

  /**
   * @brief Registers the Poke and Ray interactors of this rig component with the WidgetSubsystem,
   * using the set Widget Virtual Users
   * @see UIsdkWidgetSubsystem
   */
  virtual void RegisterInteractorWidgetIndices();

  /**
   * @brief Unregisters the Poke and Ray interactors of this rig component with the WidgetSubsystem
   * @see UIsdkWidgetSubsystem
   */
  virtual void UnregisterInteractorWidgetIndices();

  /**
   * @brief Returns the current offset for the palm collider
   * @return FVector Local space offset for palm grab collider. Returns identity in base class.
   */
  virtual FVector GetPalmColliderOffset() const;

  /**
   * @brief Returns the current offset for the pinch collider
   * @return FVector Local space offset for pinch grab collider. Returns identity in base class.
   */
  virtual FVector GetPinchColliderOffset() const;

  /**
   * @brief Returns the hand mesh associated with this controller rig
   * @return USkinnedMeshComponent Hand mesh associated with pinch interactions for this rig
   * component. Null in base class.
   */
  virtual USkinnedMeshComponent* GetPinchAttachMesh() const;

  virtual void BindInputActions(UEnhancedInputComponent* EnhancedInputComponent) {}

  /**
   * @brief Initializes defaults which rely on handeness, which is only valid when called from a
   * concrete rig component base class due to how Handedness is defined
   */
  void SetRigComponentDefaults();

  /**
   * @brief Attempts to find another already created HmdDataSource on other RigComponents. If
   * unable, creates one. Sets either result to member variable.
   */
  void InitializeHmdDataSource();

  /**
   * When true, during BeginPlay this actor will bind the configured input actions to the
   * PlayerController at index 0.
   * If false, a manual call to BindInputActionEvents must be made to bind the input actions.
   */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  bool bAutoBindInputActions = true;

  /**
   * Handedness indicates which hand this rig component belongs to
   */
  UPROPERTY(BlueprintReadOnly, Category = InteractionSDK)
  EIsdkHandedness Handedness = EIsdkHandedness::Left;

  /**
   * RayInteractorSocket indicates which bone the ray interactor will attach to on either the
   * synthetic hand mesh (if using hands) or on the poseable hand mesh (if using controller hands)
   */
  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "InteractionSDK|Customization")
  EIsdkHandBones RayInteractorSocket;

  /**
   * PokeInteractorSocket indicates which bone the poke interactor will attach to on either the
   * synthetic hand mesh (if using hands) or on the poseable hand mesh (if using controller hands)
   */
  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "InteractionSDK|Customization")
  EIsdkHandBones PokeInteractorSocket;

  /**
   * GrabberSocket indicates which bone the grabber component will attach to on either the
   * synthetic hand mesh (if using hands) or on the poseable hand mesh (if using controller hands)
   */
  UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "InteractionSDK|Customization")
  EIsdkHandBones GrabberSocket;

  /* All Rig Modifiers that this Rig Component should spawn and initialize */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = InteractionSDK)
  TArray<TSubclassOf<UIsdkRigModifier>> RigModifiersToSpawn;

  /* The local offset to apply for the palm grab collider on a controller-based rig */
  static const FVector ControllerPalmColliderOffset;

  /* The local offset to apply for the palm grab collider on a hand-based rig */
  static const FVector HandPalmColliderOffset;

  /**
   * RayInteraction is a reference to this rig component's RayInteractionRigComponent, which binds
   * hand and controller input to ray interactions
   */
  UPROPERTY(BlueprintGetter = GetRayInteraction, EditAnywhere, Category = InteractionSDK)
  TObjectPtr<UIsdkRayInteractionRigComponent> RayInteraction;

  /**
   * PokeInteraction is a reference to this rig component's PokeInteractionRigComponent, which binds
   * hand and controller input to poke interactions
   */
  UPROPERTY(BlueprintGetter = GetPokeInteraction, EditAnywhere, Category = InteractionSDK)
  TObjectPtr<UIsdkPokeInteractionRigComponent> PokeInteraction;

  /**
   * GrabInteraction is a reference to this rig component's GrabInteractionRigComponent, which binds
   * hand and controller input to grab interactions
   */
  UPROPERTY(BlueprintGetter = GetGrabInteraction, EditAnywhere, Category = InteractionSDK)
  TObjectPtr<UIsdkGrabInteractionRigComponent> GrabInteraction;

  /**
   * InputActions is a reference to this rig component's InputActionsRigComponent, which is used
   * to store input actions used for interaction
   */
  UPROPERTY(BlueprintGetter = GetInputActions, EditAnywhere, Category = InteractionSDK)
  TObjectPtr<UIsdkInputActionsRigComponent> InputActions;

  /**
   * InteractionGroup is a reference to this rig component's InteractionGroupComponent, which is
   * used to help drive which interactors are enabled and disabled.
   */
  UPROPERTY(BlueprintGetter = GetInteractionGroup, EditAnywhere, Category = InteractionSDK)
  TObjectPtr<UIsdkInteractionGroupRigComponent> InteractionGroup;

  /**
   * WidgetVirtualUser is used with UI interactions to determine which hand is interacting with
   * a widget.
   */
  UPROPERTY(
      BlueprintGetter = GetWidgetVirtualUser,
      BlueprintSetter = SetWidgetVirtualUser,
      EditAnywhere,
      Category = InteractionSDK,
      meta = (ExposeOnSpawn = true))
  FIsdkVirtualUserInfo WidgetVirtualUser;

  /**
   * All of the rig successfully modifiers spawned by this rig component
   * @see UIsdkRigComponent#GetActiveRigModifiers
   */
  UPROPERTY()
  TArray<UIsdkRigModifier*> ActiveRigModifiers;

  /**
   * Array of input indices currently bound to the UEnhancedInputComponent
   */
  TArray<uint32> InputActionBindingHandles;

  /**
   * The HMD data source used for propagating data to this rig component
   */
  UPROPERTY()
  TScriptInterface<IIsdkIHmdDataSource> HmdDataSource;

  /**
   * Gameplay Tag container for this rig component, used in propagating tags to children components
   * for interaction queries
   * @see UIsdkRigComponent#GetInteractionTags
   */
  UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = InteractionSDK)
  FGameplayTagContainer InteractionTags{};

  /**
   * @brief Returns the FName of the bone representing the thumb tip. Used to attach pinch colliders
   * @return FName Fully qualified name of the bone joint representing the thumb tip. Returns
   * NAME_None in base class
   */
  virtual FName GetThumbTipSocketName() const;

  /**
   * @brief Updates all component data sources and broadcasts that they are ready (DataSourcesReady
   * delegate)
   */
  UFUNCTION()
  void HandleVisualsDataSourcesCreatedEvent();

 private:
  void CreateInteractionGroupConditionals();
};
