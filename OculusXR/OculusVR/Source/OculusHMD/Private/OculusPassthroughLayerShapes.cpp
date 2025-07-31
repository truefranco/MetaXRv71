#include "OculusPassthroughLayerShapes.h"
#include "Curves/CurveLinearColor.h"

const FName FReconstructedLayer::ShapeName = FName("ReconstructedLayer");
const FName FUserDefinedLayer::ShapeName = FName("UserDefinedLayer");


TArray<uint8> FEdgeStyleParameters::GenerateColorMapData(EColorMapType InColorMapType, const TArray<FLinearColor>& InColorMapGradient)
{
	switch (InColorMapType) {
		case ColorMapType_GrayscaleToColor: {
			TArray<uint8> NewColorMapData = GenerateMonoBrightnessContrastPosterizeMap();
			return GenerateMonoToRGBA(InColorMapGradient, NewColorMapData);
		}
		case ColorMapType_Grayscale:
			return GenerateMonoBrightnessContrastPosterizeMap();
		case ColorMapType_ColorAdjustment:
			return GenerateBrightnessContrastSaturationColorMap();
		default:
			return TArray<uint8>();
	}
}

TArray<uint8> FEdgeStyleParameters::GenerateMonoToRGBA(const TArray<FLinearColor>& InColorMapGradient, const TArray<uint8>& InColorMapData)
{
	TArray<uint8> NewColorMapData;
	FInterpCurveLinearColor InterpCurve;
	const uint32 TotalEntries = 256;

	for (int32 Index = 0; Index < InColorMapGradient.Num(); ++Index)
	{
		InterpCurve.AddPoint(Index, (InColorMapGradient[Index] * ColorScale) + ColorOffset);
	}	

	NewColorMapData.SetNum(TotalEntries * sizeof(ovrpColorf));
	uint8* Dest = NewColorMapData.GetData();
	for (int32 Index = 0; Index < TotalEntries; ++Index)
	{
		const ovrpColorf Color = OculusHMD::ToOvrpColorf(InterpCurve.Eval(InColorMapData[Index]));
		FMemory::Memcpy(Dest, &Color, sizeof(Color));
		Dest += sizeof(ovrpColorf);
	}
	return NewColorMapData;
}

TArray<uint8> FEdgeStyleParameters::GenerateMonoBrightnessContrastPosterizeMap()
{
	TArray<uint8> NewColorMapData;
	const int32 TotalEntries = 256;
	NewColorMapData.SetNum(TotalEntries * sizeof(uint8));
	for (int32 Index = 0; Index < TotalEntries; ++Index)
	{
		float Alpha = ((float)Index / TotalEntries);
		float ContrastFactor = Contrast + 1.0;
		Alpha = (Alpha - 0.5) * ContrastFactor + 0.5 + Brightness;

		if (Posterize > 0.0f)
		{
			const float PosterizationBase = 50.0f;
			float FinalPosterize = (FMath::Pow(PosterizationBase, Posterize) - 1.0) / (PosterizationBase - 1.0);
			Alpha = FMath::RoundToFloat(Alpha / FinalPosterize) * FinalPosterize;
		}

		NewColorMapData[Index] = (uint8)(FMath::Min(FMath::Max(Alpha, 0.0f), 1.0f) * 255.0f);
	}
	return NewColorMapData;
}

TArray<uint8> FEdgeStyleParameters::GenerateBrightnessContrastSaturationColorMap()
{
	TArray<uint8> NewColorMapData;
	NewColorMapData.SetNum(3 * sizeof(float));
	float newB = Brightness * 100.0f;
	float newC = Contrast + 1.0f;
	float newS = Saturation + 1.0f;  

	uint8* Dest = NewColorMapData.GetData();
	FMemory::Memcpy(Dest, &newB, sizeof(float));
	Dest += sizeof(float);
	FMemory::Memcpy(Dest, &newC, sizeof(float));
	Dest += sizeof(float);
	FMemory::Memcpy(Dest, &newS, sizeof(float));

	return NewColorMapData;
}

ovrpInsightPassthroughColorMapType FEdgeStyleParameters::GetOVRPColorMapType(EColorMapType InColorMapType)
{
	switch (InColorMapType) {
	case ColorMapType_GrayscaleToColor: 
		return ovrpInsightPassthroughColorMapType_MonoToRgba;
	case ColorMapType_Grayscale: 
		return ovrpInsightPassthroughColorMapType_MonoToMono;
	case ColorMapType_ColorAdjustment:
		return ovrpInsightPassthroughColorMapType_BrightnessContrastSaturation;
	default:
		return ovrpInsightPassthroughColorMapType_None;
	}
}
