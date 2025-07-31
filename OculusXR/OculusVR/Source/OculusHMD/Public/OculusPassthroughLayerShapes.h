#pragma once

#include "StereoLayerShapes.h"
#include "OculusPassthroughMesh.h"
#include "OculusHMDPrivate.h"

UENUM()
enum EColorMapType
{
	/** None*/
	ColorMapType_None UMETA(DisplayName = "None"),

	/** Grayscale to color */
	ColorMapType_GrayscaleToColor UMETA(DisplayName = "Grayscale To Color"),

	/** Grayscale */
	ColorMapType_Grayscale UMETA(DisplayName = "Grayscale"),

	/** Color Adjustment */
	ColorMapType_ColorAdjustment UMETA(DisplayName = "Color Adjustment"),

	ColorMapType_MAX,
};

UENUM()
enum EPassthroughLayerOrder
{
	/** Layer is rendered on top of scene */
	PassthroughLayerOrder_Overlay UMETA(DisplayName = "Overlay"),

	/** Layer is rendered under scene */
	PassthroughLayerOrder_Underlay UMETA(DisplayName = "Underlay"),

	PassthroughLayerOrder_MAX,
};

struct FEdgeStyleParameters {
	FEdgeStyleParameters()
		: bEnableEdgeColor(false)
		, bEnableColorMap(false)
		, TextureOpacityFactor(1.0f)
		, EdgeColor{}
		, ColorMapType{}
		, ColorMapData{}
	{

	};

	FEdgeStyleParameters(
		bool bEnableEdgeColor, 
		bool bEnableColorMap, 
		float TextureOpacityFactor, 
		float Brightness, 
		float Contrast, 
		float Posterize, 
		float Saturation,
		FLinearColor EdgeColor, 
		FLinearColor ColorScale, 
		FLinearColor ColorOffset, 
		EColorMapType InColorMapType, 
		const TArray<FLinearColor>& InColorMapGradient)
		: bEnableEdgeColor(bEnableEdgeColor)
		, bEnableColorMap(bEnableColorMap)
		, TextureOpacityFactor(TextureOpacityFactor)
		, Brightness(Brightness)
		, Contrast(Contrast)
		, Posterize(Posterize)
		, Saturation(Saturation)
		, EdgeColor(EdgeColor)
		, ColorScale(ColorScale)
		, ColorOffset(ColorOffset)
		, ColorMapType(GetOVRPColorMapType(InColorMapType))
	{
		ColorMapData = GenerateColorMapData(InColorMapType, InColorMapGradient);
	};

	bool bEnableEdgeColor;
	bool bEnableColorMap;
	float TextureOpacityFactor;
	float Brightness;
	float Contrast;
	float Posterize;
	float Saturation;
	FLinearColor EdgeColor;
	FLinearColor ColorScale;
	FLinearColor ColorOffset;
	ovrpInsightPassthroughColorMapType ColorMapType;
	TArray<uint8> ColorMapData;

private:

	/** Generates the corresponding color map based on given color map type */
	TArray<uint8> GenerateColorMapData(EColorMapType InColorMapType, const TArray<FLinearColor>& InColorMapGradient);

	/** Generates a grayscale to color color map based on given gradient --> It also applies the color scale and offset */
	TArray<uint8> GenerateMonoToRGBA(const TArray<FLinearColor>& InGradient, const TArray<uint8>& InColorMapData);

	/** Generates a grayscale color map with given Brightness/Contrast/Posterize settings */
	TArray<uint8> GenerateMonoBrightnessContrastPosterizeMap();

	/** Generates a luminance based colormap from the the Brightness/Contrast */
	TArray<uint8> GenerateBrightnessContrastSaturationColorMap();
	
	/** Converts `EColorMapType` to `ovrpInsightPassthroughColorMapType` */
	ovrpInsightPassthroughColorMapType GetOVRPColorMapType(EColorMapType InColorMapType);
};

class OCULUSHMD_API FReconstructedLayer : public IStereoLayerShape
{
	STEREO_LAYER_SHAPE_BOILERPLATE(FReconstructedLayer)

public:
	FReconstructedLayer() {};
	FReconstructedLayer(const FEdgeStyleParameters& EdgeStyleParameters, EPassthroughLayerOrder PassthroughLayerOrder)
		: EdgeStyleParameters(EdgeStyleParameters),
		PassthroughLayerOrder(PassthroughLayerOrder)
	{
	};
	FEdgeStyleParameters EdgeStyleParameters;
	EPassthroughLayerOrder PassthroughLayerOrder;
};

struct FUserDefinedGeometryDesc
{
	FUserDefinedGeometryDesc(const FString& MeshName, OculusHMD::FOculusPassthroughMeshRef PassthroughMesh, const FTransform& Transform, bool bUpdateTransform)
		: MeshName(MeshName)
		, PassthroughMesh(PassthroughMesh)
		, Transform(Transform)
		, bUpdateTransform(bUpdateTransform)
	{
	};

	FString MeshName;
	OculusHMD::FOculusPassthroughMeshRef PassthroughMesh;
	FTransform  Transform;
	bool bUpdateTransform;
};

class OCULUSHMD_API FUserDefinedLayer : public IStereoLayerShape
{
	STEREO_LAYER_SHAPE_BOILERPLATE(FUserDefinedLayer)

public:
	FUserDefinedLayer() {};
	FUserDefinedLayer(TArray<FUserDefinedGeometryDesc> InUserGeometryList, const FEdgeStyleParameters& EdgeStyleParameters, EPassthroughLayerOrder PassthroughLayerOrder)
		: UserGeometryList{}
		, EdgeStyleParameters(EdgeStyleParameters)
		, PassthroughLayerOrder(PassthroughLayerOrder)
	{
		UserGeometryList = InUserGeometryList;
	}

	TArray<FUserDefinedGeometryDesc> UserGeometryList;
	FEdgeStyleParameters EdgeStyleParameters;
	EPassthroughLayerOrder PassthroughLayerOrder;

private:

};
