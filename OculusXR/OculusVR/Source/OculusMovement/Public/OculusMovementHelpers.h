/*
Copyright (c) Meta Platforms, Inc. and affiliates.
All rights reserved.
This source code is licensed under the license found in the
LICENSE file in the root directory of this source tree.
*/

#pragma once

namespace OculusUtility
{
	template <typename T>
	T* FindComponentByName(AActor* Actor, const FName& ComponentName)
	{
		if (IsValid(Actor) && (ComponentName != NAME_None))
		{
			TArray<T*> ComponentsOfType;
			Actor->GetComponents<T>(ComponentsOfType);
			T** FoundComponent = ComponentsOfType.FindByPredicate([Name = ComponentName.ToString()](T* Component) { return Component->GetName().Equals(Name); });

			if (FoundComponent != nullptr)
			{
				return *FoundComponent;
			}
		}

		return nullptr;
	}
} // namespace OculusUtility
