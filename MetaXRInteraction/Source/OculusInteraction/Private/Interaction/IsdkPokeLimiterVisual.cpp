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

#include "Interaction/IsdkPokeLimiterVisual.h"
#include "Templates/PimplPtr.h"
#include "isdk_api/isdk_api.hpp"
#include "ApiImpl.h"
#include "IsdkChecks.h"
#include "OculusInteractionLog.h"
#include "Subsystem/IsdkWorldSubsystem.h"

using isdk::api::HandPokeLimiterVisual;
using isdk::api::HandPokeLimiterVisualPtr;
using isdk::api::IInteractable;

namespace isdk {
	namespace api {
		namespace helper
		{
			class FPokeLimiterVisualImpl : public FApiImpl<HandPokeLimiterVisual, HandPokeLimiterVisualPtr>
			{
			public:
				explicit FPokeLimiterVisualImpl(std::function<HandPokeLimiterVisualPtr()> CreateFn)
					: FApiImpl(std::move(CreateFn))
				{
				}
			};
		}
	}
} // namespace isdk::api::helper

UIsdkPokeLimiterVisual::UIsdkPokeLimiterVisual()
{
  PrimaryComponentTick.bCanEverTick = false;
  PrimaryComponentTick.TickGroup = TG_PrePhysics;

  PokeLimiterVisualImpl =
      MakePimpl<isdk::api::helper::FPokeLimiterVisualImpl>(
          [this]() -> HandPokeLimiterVisualPtr
          {
            // Check dependencies
            bool validInputs = true;
            validInputs &= UIsdkChecks::EnsureMsgfDependencyIsValid(
                PokeInteractor, this, TEXT("PokeInteractor"));
            validInputs &= UIsdkChecks::EnsureMsgfDependencyIsValid(
                SyntheticHand, this, TEXT("SyntheticHand"));
            validInputs &=
                UIsdkChecks::EnsureMsgfDependencyIsValid(DataSource, this, TEXT("DataSource"));
            if (!validInputs)
            {
              return nullptr;
            }

            const auto ApiPokeInteractor = PokeInteractor->GetApiPokeInteractor();
            const auto ApiSyntheticHand = SyntheticHand->GetApiSyntheticHand();
            const auto ApiDataSource = DataSource->GetApiIHandDataSource();

            validInputs &= UIsdkChecks::EnsureMsgfDependencyApiIsInitialized(
                ApiPokeInteractor, this, TEXT("PokeInteractor"));
            validInputs &= UIsdkChecks::EnsureMsgfDependencyApiIsInitialized(
                ApiPokeInteractor, this, TEXT("SyntheticHand"));
            validInputs &= UIsdkChecks::EnsureMsgfDependencyApiIsInitialized(
                ApiPokeInteractor, this, TEXT("DataSource"));
            if (!validInputs)
            {
              return nullptr;
            }

            // Create native instance
            auto Instance = HandPokeLimiterVisual::create(
                ApiDataSource->getIHandDataSourceHandle(), *ApiPokeInteractor, *ApiSyntheticHand);
            if (!UIsdkChecks::EnsureMsgfApiInstanceIsValid(Instance.IsValid(), this))
            {
              return nullptr;
            }

            return Instance;
          });
}

void UIsdkPokeLimiterVisual::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  if (UpdateEventToken)
  {
    UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
    WorldSubsystem.UnregisterUpdateEventHandler(UpdateEventToken);
  }

  PokeLimiterVisualImpl->DestroyInstance();
}

void UIsdkPokeLimiterVisual::BeginDestroy()
{
  Super::BeginDestroy();

  PokeLimiterVisualImpl.Reset();
}

void UIsdkPokeLimiterVisual::SetPokeInteractor(UIsdkPokeInteractor* InPokeInteractor)
{
  if (PokeInteractor != InPokeInteractor)
  {
    PokeInteractor = InPokeInteractor;
    SetupImplDependencies();
  }
}

void UIsdkPokeLimiterVisual::SetSyntheticHand(UIsdkSyntheticHand* InSyntheticHand)
{
  if (SyntheticHand != InSyntheticHand)
  {
    SyntheticHand = InSyntheticHand;
    SetupImplDependencies();
  }
}

void UIsdkPokeLimiterVisual::SetDataSource(UIsdkHandDataSource* InHandDataSource)
{
  if (DataSource != InHandDataSource)
  {
    DataSource = InHandDataSource;
    SetupImplDependencies();
  }
}

void UIsdkPokeLimiterVisual::SetupImplDependencies()
{
  // Dependencies changed. The native instance needs to be recreated to get the new values -
  // destroying the existing instance here will cause it to be lazy-created again on next access.
  PokeLimiterVisualImpl->DestroyInstance();

  // Make sure that we are subscribed to Update on the world subsystem ONLY if our dependencies are
  // set. If the dependencies were set to null, unsubscribe from the Update (if we previously had
  // subscribed)
  UIsdkWorldSubsystem& WorldSubsystem = UIsdkWorldSubsystem::Get(GetWorld());
  if (IsValid(PokeInteractor) && IsValid(SyntheticHand) && IsValid(DataSource))
  {
    if (!UpdateEventToken)
    {
      UE_LOG(
          LogOculusInteraction, Verbose, TEXT("UIsdkPokeLimiterVisual: Registered update event"));
      UpdateEventToken = WorldSubsystem.RegisterUpdateEventHandler(
          [this]() -> isdk::api::IUpdate* { return PokeLimiterVisualImpl->GetOrCreateInstance(); },
          Updated);
    }
  }
  else
  {
    if (UpdateEventToken)
    {
      WorldSubsystem.UnregisterUpdateEventHandler(UpdateEventToken);
      UpdateEventToken = {};
      UE_LOG(
          LogOculusInteraction, Verbose, TEXT("UIsdkPokeLimiterVisual: Unregistered update event"));
    }
  }
}
