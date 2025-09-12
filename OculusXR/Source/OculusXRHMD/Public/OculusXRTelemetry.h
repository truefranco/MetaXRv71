// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include "OculusXRQPL.h"
#include <Containers/StringConv.h>
#include <Templates/Function.h>
#include <Templates/Tuple.h>

namespace OculusXRTelemetry
{
#ifndef TURN_OFF_META_TELEMETRY
	using FTelemetryBackend = FQPLBackend;
#else
	using FTelemetryBackend = FEmptyBackend;
#endif

	constexpr int UNREAL_TOOL_ID = 2;
	OCULUSXRHMD_API bool IsActive();
	OCULUSXRHMD_API void IfActiveThen(TUniqueFunction<void()> Function);
	OCULUSXRHMD_API void PropagateTelemetryConsent();

	OCULUSXRHMD_API TTuple<FString, FString> GetProjectIdAndName();

	OCULUSXRHMD_API bool IsConsentGiven();

	template <typename Backend = FTelemetryBackend>
	class OCULUSXRHMD_API TMarkerPoint : FNoncopyable
	{
	public:
		explicit TMarkerPoint(const char* Name)
			: bCreated(Backend::CreateMarkerHandle(Name, Handle)) {}
		~TMarkerPoint()
		{
			if (bCreated)
			{
				Backend::DestroyMarkerHandle(Handle);
			}
		}
		int GetHandle() const { return Handle; }

	private:
		const bool bCreated{ false };
		const int Handle{ -1 };
	};

	enum class EAnnotationType
	{
		Required,
		Optional,
	};

	template <int MarkerId, typename Backend = FTelemetryBackend>
	class TMarker
	{
	public:
		explicit TMarker(const FTelemetryInstanceKey InstanceKey = DefaultTelemetryInstance)
			: InstanceKey(InstanceKey)
		{
		}
		TMarker(const TMarker&& Other) noexcept
			: InstanceKey(Other.GetMarkerId()) {}

		const TMarker& Start(const FTelemetryTimestamp Timestamp = AutoSetTimestamp) const
		{
			Backend::MarkerStart(MarkerId, InstanceKey, Timestamp);
			return *this;
		}
		const TMarker& AddAnnotation(const char* Key, const char* Value, EAnnotationType bExtraAnnotation = EAnnotationType::Required) const
		{
			if (bExtraAnnotation == EAnnotationType::Required || IsConsentGiven())
			{
				Backend::MarkerAnnotation(MarkerId, Key, Value, InstanceKey);
			}
			return *this;
		}
		const TMarker& AddPoint(const char* Name, const FTelemetryTimestamp Timestamp = AutoSetTimestamp) const
		{
			Backend::MarkerPoint(MarkerId, Name, InstanceKey, Timestamp);
			return *this;
		}
		const TMarker& AddPoint(const TMarkerPoint<Backend>& MarkerPoint, const FTelemetryTimestamp Timestamp = AutoSetTimestamp) const
		{
			Backend::MarkerPoint(MarkerId, MarkerPoint.GetHandle(), InstanceKey, Timestamp);
			return *this;
		}
		void End(EAction Result, const FTelemetryTimestamp Timestamp = AutoSetTimestamp) const
		{
			Backend::MarkerEnd(MarkerId, Result, InstanceKey, Timestamp);
		}
		constexpr static int GetMarkerId()
		{
			return MarkerId;
		}

	private:
		const FTelemetryInstanceKey InstanceKey;
	};

	struct FIgnoreNotEndedMarker
	{
		template <int MarkerId, typename Backend>
		constexpr const FIgnoreNotEndedMarker& operator=(const TMarker<MarkerId, Backend>&) const noexcept
		{
			// do nothing
			return *this;
		}
	};

	constexpr FIgnoreNotEndedMarker NotEnd{};

	enum class EScopeMode
	{
		StartAndEnd,
		Start,
		End
	};

	template <typename TMarker, EScopeMode TScope = EScopeMode::StartAndEnd>
	class TScopedMarker : public FNoncopyable
	{
		TOptional<TMarker> Marker;
		EAction Result{ EAction::Success };
		static constexpr EScopeMode Scope{ TScope };

	public:
		TScopedMarker(const FTelemetryInstanceKey InstanceKey = DefaultTelemetryInstance)
		{
			if (IsActive())
			{
				Marker.Emplace(InstanceKey);
				if constexpr (Scope != EScopeMode::End)
				{
					const auto& Self = Start();
				}

				const TTuple<FString, FString> ProjectIdAndName = GetProjectIdAndName();
				const auto& AnnotatedWithProjectHash = AddAnnotation("project_hash", StringCast<ANSICHAR>(*ProjectIdAndName.Get<0>()).Get(), EAnnotationType::Optional);
				const auto& AnnotatedWithProjectName = AddAnnotation("project_name", StringCast<ANSICHAR>(*ProjectIdAndName.Get<1>()).Get(), EAnnotationType::Optional);

			}
		}
		~TScopedMarker()
		{
			if constexpr (Scope != EScopeMode::Start)
			{
				End();
			}
		}

		const TScopedMarker& Start() const
		{
			if (Marker)
			{
				Marker->Start();
			}
			return *this;
		}

		const TScopedMarker& AddPoint(const char* Name) const
		{
			if (Marker)
			{
				Marker->AddPoint(Name);
			}
			return *this;
		}

		const TScopedMarker& AddAnnotation(const char* Key, const char* Value, EAnnotationType bExtraAnnotation = EAnnotationType::Required) const
		{
			if (Marker && (bExtraAnnotation == EAnnotationType::Required || IsConsentGiven()))
			{
				Marker->AddAnnotation(Key, Value);
			}
			return *this;
		}

		const TScopedMarker& SetResult(EAction InResult)
		{
			Result = InResult;
			return *this;
		}

		void End() const
		{
			if (Marker)
			{
				Marker->End(Result);
			}
		}
	};

	void SendEvent(const TCHAR* EventName, const TCHAR* Param);
	void SendEvent(const TCHAR* EventName, bool bParam);
	void SendEvent(const TCHAR* EventName, float Param);

} // namespace OculusXRTelemetry
