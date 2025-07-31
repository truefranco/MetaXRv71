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
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "IsdkObjectDebugUI.generated.h"

UCLASS(
    Blueprintable,
    ClassGroup = (InteractionSDK),
    meta = (BlueprintSpawnableComponent, DisplayName = "ISDK Object Debug UI"))
class UIsdkObjectDebugUI : public USceneComponent
{
  GENERATED_BODY()
 public:
  UIsdkObjectDebugUI()
  {
    TextComp = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IsdkObjectDebugUI"));
    TextComp->SetTextRenderColor(FColor::Red);
    TextComp->SetRelativeScale3D(FVector(.05f));
  }

  virtual void SetText(FText text)
  {
    if (IsValid(TextComp))
    {
      TextComp->SetText(text);
    }
  }

 private:
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override
  {
    if (IsValid(TextComp))
    {
      const auto HMDLocation =
          GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
      FVector origin;
      FVector boxExtent;
      GetOwner()->GetActorBounds(false, origin, boxExtent, true);
      const auto TextLoc = GetOwner()->GetActorLocation() + FVector(0, boxExtent.Y + 1, 0);
      const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(TextLoc, HMDLocation);
      FVector Euler = LookAtRot.Euler();
      Euler.X = 0; // no roll please
      const FQuat FinalRot = FQuat::MakeFromEuler(Euler);

      TextComp->SetWorldRotation(FinalRot);
      TextComp->SetWorldLocation(TextLoc);
    }
  }

  UPROPERTY()
  UTextRenderComponent* TextComp{};
};
