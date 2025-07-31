// Copyright 1998-2020 Epic Games, Inc. All Rights Reserved.

#include "OculusPassthroughLayerComponent.h"
#include "OculusHMD.h"
#include "OculusPassthroughLayerShapes.h"
#include "Curves/CurveLinearColor.h"

DEFINE_LOG_CATEGORY(LogOculusPassthrough);

void UStereoLayerShapeReconstructed::ApplyShape(IStereoLayers::FLayerDesc& LayerDesc)
{
	const FEdgeStyleParameters EdgeStyleParameters(
		bEnableEdgeColor, 
		bEnableColorMap, 
		TextureOpacityFactor, 
		Brightness, 
		Contrast, 
		Posterize, 
		Saturation, 
		EdgeColor, 
		ColorScale, 
		ColorOffset, 
		ColorMapType, 
		GetColorMapGradient(bUseColorMapCurve, ColorMapCurve));
	LayerDesc.SetShape<FReconstructedLayer>(EdgeStyleParameters, LayerOrder);
}

void UStereoLayerShapeUserDefined::ApplyShape(IStereoLayers::FLayerDesc& LayerDesc)
{
	//If there is no user geometry, set the layer hidden to avoid unnecessary cost
	if (UserGeometryList.IsEmpty())
		LayerDesc.Flags |= IStereoLayers::LAYER_FLAG_HIDDEN;

	const FEdgeStyleParameters EdgeStyleParameters(
		bEnableEdgeColor, 
		bEnableColorMap, 
		TextureOpacityFactor,
		Brightness, 
		Contrast, 
		Posterize, 
		Saturation, 
		EdgeColor, 
		ColorScale, 
		ColorOffset, 
		ColorMapType, 
		GetColorMapGradient(bUseColorMapCurve, ColorMapCurve));
	LayerDesc.SetShape<FUserDefinedLayer>(UserGeometryList,EdgeStyleParameters, LayerOrder);
}

void UStereoLayerShapeUserDefined::AddGeometry(const FString& MeshName, OculusHMD::FOculusPassthroughMeshRef PassthroughMesh, FTransform Transform, bool bUpdateTransform)
{
	FUserDefinedGeometryDesc UserDefinedGeometryDesc(
		MeshName,
		PassthroughMesh,
		Transform,
		bUpdateTransform);

	UserGeometryList.Add(UserDefinedGeometryDesc);
}

void UStereoLayerShapeUserDefined::RemoveGeometry(const FString& MeshName)
{
	UserGeometryList.RemoveAll([MeshName](const FUserDefinedGeometryDesc& Desc) {
		return Desc.MeshName == MeshName;
	});
}

UOculusPassthroughLayerComponent::UOculusPassthroughLayerComponent(const FObjectInitializer& ObjectInitializer)
	:	Super(ObjectInitializer)
{
}

void UOculusPassthroughLayerComponent::OnComponentDestroyed(bool bDestroyingHierarchy) {
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	IStereoLayers* StereoLayers;
	if (LayerId && GEngine->StereoRenderingDevice.IsValid() && (StereoLayers = GEngine->StereoRenderingDevice->GetStereoLayers()) != nullptr)
	{
		StereoLayers->DestroyLayer(LayerId);
		LayerId = 0;
	}
}

void UOculusPassthroughLayerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (Texture == nullptr && !LayerRequiresTexture())
	{
		// UStereoLayerComponent hides components without textures
		Texture = GEngine->DefaultTexture;
	}

	UpdatePassthroughObjects();
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UOculusPassthroughLayerComponent::UpdatePassthroughObjects()
{
	UStereoLayerShapeUserDefined* UserShape = Cast<UStereoLayerShapeUserDefined>(Shape);
	if (UserShape)
	{
		bool bDirty = false;
		for (FUserDefinedGeometryDesc& Entry : UserShape->GetUserGeometryList())
		{
			if (Entry.bUpdateTransform)
			{
				const UMeshComponent** MeshComponent = PassthroughComponentMap.Find(Entry.MeshName);
				if (MeshComponent)
				{
					Entry.Transform = (*MeshComponent)->GetComponentTransform();
					bDirty = true;
					
				}
			}
		}
		if (bDirty) 
		{
			MarkStereoLayerDirty();
		}
	}

}

OculusHMD::FOculusPassthroughMeshRef UOculusPassthroughLayerComponent::CreatePassthroughMesh(UProceduralMeshComponent* ProceduralMeshComponent)
{
	if (!ProceduralMeshComponent)
	{
		UE_LOG(LogOculusPassthrough, Error, TEXT("Passthrough Procedural Mesh is nullptr"));
		return nullptr;
	}

	TArray<int32> Triangles;
	TArray<FVector> Vertices;
	int32 NumSections = ProceduralMeshComponent->GetNumSections();
	int VertexOffset = 0; //Each section start with vertex IDs of 0, in order to create a single mesh from all sections we need to offset those IDs by the amount of previous vertices
	for (int32 s = 0; s < NumSections; ++s)
	{
		FProcMeshSection* ProcMeshSection = ProceduralMeshComponent->GetProcMeshSection(s);
		for (int32 i = 0; i < ProcMeshSection->ProcIndexBuffer.Num(); ++i)
		{
			Triangles.Add(VertexOffset + ProcMeshSection->ProcIndexBuffer[i]);
		}

		for (int32 i = 0; i < ProcMeshSection->ProcVertexBuffer.Num(); ++i)
		{
			Vertices.Add(ProcMeshSection->ProcVertexBuffer[i].Position);
		}

		VertexOffset += ProcMeshSection->ProcVertexBuffer.Num();
	}

	OculusHMD::FOculusPassthroughMeshRef PassthroughMesh = new OculusHMD::FOculusPassthroughMesh(Vertices, Triangles);
	return PassthroughMesh;
}


OculusHMD::FOculusPassthroughMeshRef UOculusPassthroughLayerComponent::CreatePassthroughMesh(UStaticMeshComponent* StaticMeshComponent)
{
	if (!StaticMeshComponent)
	{
		UE_LOG(LogOculusPassthrough, Error, TEXT("Passthrough Static Mesh is nullptr"));
		return nullptr;
	}

	UStaticMesh* Mesh = StaticMeshComponent->GetStaticMesh();

	if (!Mesh || !Mesh->GetRenderData())
	{
		UE_LOG(LogOculusPassthrough, Error, TEXT("Passthrough Static Mesh has no Renderdata"));
		return nullptr;
	}

	if (Mesh->GetNumLODs() == 0)
	{
		UE_LOG(LogOculusPassthrough, Error, TEXT("Passthrough Static Mesh has no LODs"));
		return nullptr;
	}

	if (!Mesh->bAllowCPUAccess)
	{
		UE_LOG(LogOculusPassthrough, Error, TEXT("Passthrough Static Mesh Requires CPU Access"));
		return nullptr;
	}

	const int32 LODIndex = 0;
	FStaticMeshLODResources& LOD = Mesh->GetRenderData()->LODResources[LODIndex];

	TArray<int32> Triangles;
	const int32 NumIndices = LOD.IndexBuffer.GetNumIndices();
	for (int32 i = 0; i < NumIndices; ++i)
	{
		Triangles.Add(LOD.IndexBuffer.GetIndex(i));
	}

	TArray<FVector> Vertices;
	const int32 NumVertices = LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();
	for (int32 i = 0; i < NumVertices; ++i)
	{
		Vertices.Add((FVector)LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(i));
	}

	OculusHMD::FOculusPassthroughMeshRef PassthroughMesh = new OculusHMD::FOculusPassthroughMesh(Vertices, Triangles);
	return PassthroughMesh;
}

void UOculusPassthroughLayerComponent::AddSurfaceGeometry(AStaticMeshActor* StaticMeshActor, bool updateTransform)
{
	if (StaticMeshActor)
	{
		UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
		if (StaticMeshComponent)
			AddStaticSurfaceGeometry(StaticMeshComponent, updateTransform);
	}
}

void UOculusPassthroughLayerComponent::AddStaticSurfaceGeometry(UStaticMeshComponent* StaticMeshComponent, bool updateTransform)
{
	if (!StaticMeshComponent)
		return;

	UStereoLayerShapeUserDefined* UserShape = Cast<UStereoLayerShapeUserDefined>(Shape);
	if (!UserShape)
		return;

	OculusHMD::FOculusPassthroughMeshRef PassthroughMesh = CreatePassthroughMesh(StaticMeshComponent);
	if (!PassthroughMesh)
		return;

	const FString MeshName = StaticMeshComponent->GetFullName();
	const FTransform Transform = StaticMeshComponent->GetComponentTransform();
	UserShape->AddGeometry(MeshName, PassthroughMesh, Transform, updateTransform);

	PassthroughComponentMap.Add(MeshName, StaticMeshComponent);
	MarkStereoLayerDirty();
}

void UOculusPassthroughLayerComponent::AddProceduralSurfaceGeometry(UProceduralMeshComponent* ProceduralMeshComponent, bool updateTransform)
{
	if (!ProceduralMeshComponent)
		return;

	UStereoLayerShapeUserDefined* UserShape = Cast<UStereoLayerShapeUserDefined>(Shape);
	if (!UserShape)
		return;

	OculusHMD::FOculusPassthroughMeshRef PassthroughMesh = CreatePassthroughMesh(ProceduralMeshComponent);
	if (!PassthroughMesh)
		return;

	const FString MeshName = ProceduralMeshComponent->GetFullName();
	const FTransform Transform = ProceduralMeshComponent->GetComponentTransform();
	UserShape->AddGeometry(MeshName, PassthroughMesh, Transform, updateTransform);

	PassthroughComponentMap.Add(MeshName, ProceduralMeshComponent);
	MarkStereoLayerDirty();
}

void UOculusPassthroughLayerComponent::RemoveSurfaceGeometry(AStaticMeshActor* StaticMeshActor)
{
	if (StaticMeshActor)
		RemoveSurfaceGeometryComponent(StaticMeshActor->GetStaticMeshComponent());
}

void UOculusPassthroughLayerComponent::RemoveStaticSurfaceGeometry(UStaticMeshComponent* StaticMeshComponent)
{
	RemoveSurfaceGeometryComponent(StaticMeshComponent);
}

void UOculusPassthroughLayerComponent::RemoveProceduralSurfaceGeometry(UProceduralMeshComponent* ProceduralMeshComponent)
{
	RemoveSurfaceGeometryComponent(ProceduralMeshComponent);
}

void UOculusPassthroughLayerComponent::RemoveSurfaceGeometryComponent(UMeshComponent* MeshComponent)
{
	if (!MeshComponent)
		return;

	UStereoLayerShapeUserDefined* UserShape = Cast<UStereoLayerShapeUserDefined>(Shape);
	if (!UserShape)
		return;

	const FString MeshName = MeshComponent->GetFullName();

	UserShape->RemoveGeometry(MeshName);
	PassthroughComponentMap.Remove(MeshName);

	MarkStereoLayerDirty();
}

bool UOculusPassthroughLayerComponent::IsSurfaceGeometry(AStaticMeshActor* StaticMeshActor) const
{
	return StaticMeshActor ? IsSurfaceGeometryComponent(StaticMeshActor->GetStaticMeshComponent()) : false;
}

bool UOculusPassthroughLayerComponent::IsSurfaceGeometryComponent(const UMeshComponent* MeshComponent) const
{
	return MeshComponent ? PassthroughComponentMap.Contains(MeshComponent->GetFullName()) : false;
}

void UOculusPassthroughLayerComponent::MarkPassthroughStyleForUpdate()
{
	bPassthroughStyleNeedsUpdate = true;
}

#if WITH_EDITOR
bool UOculusPassthroughLayerComponent::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
		return false;

	if (!(Shape && Shape->IsA(UPassthroughLayerBase::StaticClass())))
	{
		return true;
	}

	const FName PropertyName = InProperty->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UOculusPassthroughLayerComponent, Texture)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOculusPassthroughLayerComponent, bQuadPreserveTextureRatio)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOculusPassthroughLayerComponent, QuadSize)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOculusPassthroughLayerComponent, UVRect)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOculusPassthroughLayerComponent, StereoLayerType))
	{
		return false;
	}

	return true;
}
#endif // WITH_EDITOR

bool UOculusPassthroughLayerComponent::LayerRequiresTexture()
{
	const bool bIsPassthroughShape = Shape && (Shape->IsA<UStereoLayerShapeReconstructed>() ||
		Shape->IsA<UStereoLayerShapeUserDefined>());
	return !bIsPassthroughShape;
}

void UPassthroughLayerBase::SetTextureOpacity(float InOpacity)
{
	if(TextureOpacityFactor == InOpacity)
	{
		return;
	}

	TextureOpacityFactor = InOpacity;
	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::EnableEdgeColor(bool bInEnableEdgeColor)
{
	if (bEnableEdgeColor == bInEnableEdgeColor)
	{
		return;
	}
	bEnableEdgeColor = bInEnableEdgeColor;
	MarkStereoLayerDirty();
}


void UPassthroughLayerBase::EnableColorMap(bool bInEnableColorMap)
{
	if (bEnableColorMap == bInEnableColorMap)
	{
		return;
	}
	bEnableColorMap = bInEnableColorMap;
	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::EnableColorMapCurve(bool bInEnableColorMapCurve)
{
	if (bUseColorMapCurve == bInEnableColorMapCurve)
	{
		return;
	}
	bUseColorMapCurve = bInEnableColorMapCurve;
	ColorMapGradient = GenerateColorMapGradient(bUseColorMapCurve, ColorMapCurve);
	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::SetEdgeRenderingColor(FLinearColor InEdgeColor)
{
	if(EdgeColor == InEdgeColor)
	{
		return;
	}
	EdgeColor = InEdgeColor;
	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::SetColorMapControls(float InContrast, float InBrightness, float InPosterize)
{
	if (ColorMapType != ColorMapType_Grayscale && ColorMapType != ColorMapType_GrayscaleToColor) {
		UE_LOG(LogOculusPassthrough, Warning, TEXT("SetColorMapControls is ignored for color map types other than Grayscale and Grayscale to color."));
		return;
	}
	Contrast = FMath::Clamp(InContrast, -1.0f, 1.0f);
	Brightness = FMath::Clamp(InBrightness, -1.0f, 1.0f);
	Posterize = FMath::Clamp(InPosterize, 0.0f, 1.0f);

	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::SetBrightnessContrastSaturation(float InContrast, float InBrightness, float InSaturation)
{
	if (ColorMapType != ColorMapType_ColorAdjustment) {
		UE_LOG(LogOculusPassthrough, Warning, TEXT("SetBrightnessContrastSaturation is ignored for color map types other than Color Adjustment."));
		return;
	}
	Contrast = FMath::Clamp(InContrast, -1.0f, 1.0f);
	Brightness = FMath::Clamp(InBrightness, -1.0f, 1.0f);
	Saturation = FMath::Clamp(InSaturation, -1.0f, 1.0f);

	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::SetColorScaleAndOffset(FLinearColor InColorScale, FLinearColor InColorOffset)
{
	if (ColorScale == InColorScale && ColorOffset == InColorOffset)
	{
		return;
	}
	ColorScale = InColorScale;
	ColorOffset = InColorOffset;
	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::SetColorMapCurve(UCurveLinearColor* InColorMapCurve)
{
	if(ColorMapCurve == InColorMapCurve)
	{
		return;
	}
	ColorMapCurve = InColorMapCurve;
	ColorMapGradient = GenerateColorMapGradient(bUseColorMapCurve, ColorMapCurve);
	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::SetColorMapGradient(const TArray<FLinearColor>& InColorMapGradient)
{
	if (InColorMapGradient.Num() == 0)
	{
		return;
	}

	if (ColorMapType != ColorMapType_GrayscaleToColor) {
		UE_LOG(LogOculusPassthrough, Warning, TEXT("SetColorMapGradient is ignored for color map types other than Grayscale to Color."));
		return;
	}

	if(bUseColorMapCurve)
	{
		UE_LOG(LogOculusPassthrough, Warning, TEXT("UseColorMapCurve is enabled on the layer. Automatic disable and use the Array for color lookup"));
	}
	bUseColorMapCurve = false;

	ColorMapGradient = InColorMapGradient;

	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::ClearColorMap()
{
	ColorMapGradient.Empty();
}

void UPassthroughLayerBase::SetColorMapType(EColorMapType InColorMapType)
{
	if(ColorMapType == InColorMapType)
	{
		return;
	}
	ColorMapType = InColorMapType;
	MarkStereoLayerDirty();
}

void UPassthroughLayerBase::SetLayerPlacement(EPassthroughLayerOrder InLayerOrder)
{
	if (LayerOrder == InLayerOrder)
	{
		UE_LOG(LogOculusPassthrough, Warning, TEXT("Same layer order as before, no change needed"));
		return;
	}

	LayerOrder = InLayerOrder;
	this->MarkStereoLayerDirty();
}

TArray<FLinearColor> UPassthroughLayerBase::GenerateColorGradientFromColorCurve(const UCurveLinearColor* InColorMapCurve) const
{
	if (InColorMapCurve == nullptr)
	{
		return TArray<FLinearColor>();
	}

	TArray<FLinearColor> Gradient;
	constexpr uint32 TotalEntries = 256;
	Gradient.Empty();
	Gradient.SetNum(TotalEntries);

	for (int32 Index = 0; Index < TotalEntries; ++Index)
	{
		const float Alpha = ((float)Index / TotalEntries);
		Gradient[Index] = InColorMapCurve->GetLinearColorValue(Alpha);
	}
	return Gradient;
}

TArray<FLinearColor> UPassthroughLayerBase::GetOrGenerateNeutralColorMapGradient()
{
	if (NeutralMapGradient.Num() == 0) {
		const uint32 TotalEntries = 256;
		NeutralMapGradient.SetNum(TotalEntries);

		for (int32 Index = 0; Index < TotalEntries; ++Index)
		{
			NeutralMapGradient[Index] = FLinearColor((float)Index / TotalEntries, (float)Index / TotalEntries, (float)Index / TotalEntries);
		}
	}

	return NeutralMapGradient;
}

TArray<FLinearColor> UPassthroughLayerBase::GenerateColorMapGradient(bool bInUseColorMapCurve, const UCurveLinearColor* InColorMapCurve)
{
	TArray<FLinearColor> NewColorGradient;
	if (bInUseColorMapCurve)
	{
		NewColorGradient = GenerateColorGradientFromColorCurve(InColorMapCurve);
	}
	// Check for existing gradient, otherwise generate a neutral one
	if (NewColorGradient.Num() == 0)
	{
		NewColorGradient = GetOrGenerateNeutralColorMapGradient();
	}
	return NewColorGradient;
}

TArray<FLinearColor> UPassthroughLayerBase::GetColorMapGradient(bool bInUseColorMapCurve, const UCurveLinearColor* InColorMapCurve)
{
	if (ColorMapGradient.Num() == 0) {
		if (bInUseColorMapCurve) {
			return GenerateColorMapGradient(bInUseColorMapCurve, InColorMapCurve);
		}
		return GetOrGenerateNeutralColorMapGradient();
	}

	return ColorMapGradient;
}
