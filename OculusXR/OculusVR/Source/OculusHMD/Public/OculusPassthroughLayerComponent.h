// Copyright Epic Games, Inc. All Rights Reserved.
// OculusEventComponent.h: Component to handle receiving events from Oculus HMDs

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ObjectMacros.h"
#include "Components/StereoLayerComponent.h"
#include "OculusPassthroughLayerShapes.h"
#include "OculusPassthroughLayerComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOculusPassthrough, Log, All);

UCLASS(Abstract, meta = (DisplayName = "Passthrough Layer Base"))
class OCULUSHMD_API UPassthroughLayerBase : public UStereoLayerShape
{
	GENERATED_BODY()

public:

	/** Ordering of passthrough layer in relation to scene rendering */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", DisplayName = "Layer Placement")
	TEnumAsByte<enum EPassthroughLayerOrder> LayerOrder;

	/** Opacity of the (main) passthrough texture.  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0))
	float TextureOpacityFactor = 1.0f;

	/** Enable edge color */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties")
	bool bEnableEdgeColor = false;

	/** Color of the passthrough edge rendering effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties")
	FLinearColor EdgeColor;

	/** Enable color mapping */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties")
	bool bEnableColorMap = false;

	/** Type of colormapping to perform */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (EditCondition = "bEnableColorMap", EditConditionHides))
	TEnumAsByte<enum EColorMapType> ColorMapType;

	/** Whether to use color map curve or gradient*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (EditCondition = "bEnableColorMap && ColorMapType == 1", EditConditionHides))
	bool bUseColorMapCurve = false;

	/** Passthrough color mapping gradient converts grayscale to color*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (EditCondition = "bEnableColorMap && bUseColorMapCurve && ColorMapType == 1", EditConditionHides))
	UCurveLinearColor* ColorMapCurve;

	/** Contrast setting for color mapping*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (ClampMin = "-1", ClampMax = "1", EditCondition = "bEnableColorMap && ColorMapType > 0", EditConditionHides))
	float Contrast = 0.0f;

	/** Brightness setting for color mapping*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta=(ClampMin = "-1", ClampMax = "1", EditCondition = "bEnableColorMap && ColorMapType > 0", EditConditionHides))
	float Brightness = 0.0f;

	/** Posterize setting for grayscale and grayscale to color mapping*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bEnableColorMap && ColorMapType > 0 && ColorMapType < 3", EditConditionHides))
	float Posterize = 0.0f;

	/** Saturation setting for color adjustment mapping*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (ClampMin = "-1", ClampMax = "1", EditCondition = "bEnableColorMap && ColorMapType == 3", EditConditionHides))
	float Saturation = 0.0f;

	/** Color value that will be multiplied to the current color map*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (EditCondition = "bEnableColorMap", EditConditionHides))
	FLinearColor ColorScale = FLinearColor::White;

	/** Color value that will be added to the current color map*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passthrough Properties", meta = (EditCondition = "bEnableColorMap", EditConditionHides))
	FLinearColor ColorOffset = FLinearColor::Black;

	UFUNCTION(BlueprintCallable, Category ="Components|Stereo Layer")
	void SetTextureOpacity(float InOpacity);

	UFUNCTION(BlueprintCallable, Category ="Components|Stereo Layer")
	void EnableEdgeColor(bool bInEnableEdgeColor);

	UFUNCTION(BlueprintCallable, Category ="Components|Stereo Layer")
	void EnableColorMap(bool bInEnableColorMap);

	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
	void EnableColorMapCurve(bool bInEnableColorMapCurve);

	UFUNCTION(BlueprintCallable, Category ="Components|Stereo Layer")
	void SetEdgeRenderingColor(FLinearColor InEdgeColor);

	/** Set color map controls for grayscale and grayscale to rgb color mapping*/
	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
	void SetColorMapControls(float InContrast = 0, float InBrightness = 0, float InPosterize = 0);

	/** Set color map controls for color adjustment color mapping*/
	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
	void SetBrightnessContrastSaturation(float InContrast = 0, float InBrightness = 0, float InSaturation = 0);

	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
	void SetColorScaleAndOffset(FLinearColor InColorScale = FLinearColor::White, FLinearColor InColorOffset = FLinearColor::Black);

	/** Set color curve that will be added to the color map in grayscale modes --> will be converted into a gradient*/
	UFUNCTION(BlueprintCallable, Category ="Components|Stereo Layer")
	void SetColorMapCurve(UCurveLinearColor* InColorMapCurve);

	/** Set color map gradient directly instead through a color curve*/
	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
	void SetColorMapGradient(const TArray<FLinearColor>& InColorMapGradient);

	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
	void ClearColorMap();

	UFUNCTION(BlueprintCallable, Category ="Components|Stereo Layer")
	void SetColorMapType(EColorMapType InColorMapType);

	UFUNCTION(BlueprintCallable, Category = "Passthrough Properties")
	void SetLayerPlacement(EPassthroughLayerOrder InLayerOrder);

protected:
	TArray<FLinearColor> ColorMapGradient;
	TArray<FLinearColor> NeutralMapGradient;

	TArray<FLinearColor> GenerateColorGradientFromColorCurve(const UCurveLinearColor* InColorMapCurve) const;
	TArray<FLinearColor> GetOrGenerateNeutralColorMapGradient();
	TArray<FLinearColor> GetColorMapGradient(bool bInUseColorMapCurve, const UCurveLinearColor* InColorMapCurve);
	TArray<FLinearColor> GenerateColorMapGradient(bool bInUseColorMapCurve, const UCurveLinearColor* InColorMapCurve);
};

/* Reconstructed Passthrough Layer*/
UCLASS(meta = (DisplayName = "Reconstructed Passthrough Layer"))
class OCULUSHMD_API UStereoLayerShapeReconstructed : public UPassthroughLayerBase
{
	GENERATED_BODY()
public:

	virtual void ApplyShape(IStereoLayers::FLayerDesc& LayerDesc) override;

};

/* User Defined Passthrough Layer*/
UCLASS(meta = (DisplayName = "User Defined Passthrough Layer"))
class OCULUSHMD_API UStereoLayerShapeUserDefined : public UPassthroughLayerBase
{
	GENERATED_BODY()
public:
	void AddGeometry(const FString& MeshName, OculusHMD::FOculusPassthroughMeshRef PassthroughMesh, FTransform Transform, bool bUpdateTransform);
	void RemoveGeometry(const FString& MeshName);

	virtual void ApplyShape(IStereoLayers::FLayerDesc& LayerDesc) override;
	TArray<FUserDefinedGeometryDesc>& GetUserGeometryList() { return UserGeometryList; };

private:
	TArray<FUserDefinedGeometryDesc> UserGeometryList;
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = OculusHMD)
class OCULUSHMD_API UOculusPassthroughLayerComponent : public UStereoLayerComponent
{
	GENERATED_UCLASS_BODY()

public:
	void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	void UpdatePassthroughObjects();

	UFUNCTION(BlueprintCallable, Category ="Passthrough")
	void AddSurfaceGeometry(AStaticMeshActor* StaticMeshActor, bool updateTransform );
	UFUNCTION(BlueprintCallable, Category = "Passthrough")
	void AddStaticSurfaceGeometry(UStaticMeshComponent* StaticMeshComponent, bool updateTransform);
	UFUNCTION(BlueprintCallable, Category = "Passthrough")
	void AddProceduralSurfaceGeometry(UProceduralMeshComponent* ProceduralMeshComponent, bool updateTransform);

	UFUNCTION(BlueprintCallable, Category ="Passthrough")
	void RemoveSurfaceGeometry(AStaticMeshActor* StaticMeshActor );
	UFUNCTION(BlueprintCallable, Category = "Passthrough")
	void RemoveStaticSurfaceGeometry(UStaticMeshComponent* StaticMeshComponent);
	UFUNCTION(BlueprintCallable, Category = "Passthrough")
	void RemoveProceduralSurfaceGeometry(UProceduralMeshComponent* ProceduralMeshComponent);

	UFUNCTION(BlueprintCallable, Category = "Passthrough")
	bool IsSurfaceGeometry(AStaticMeshActor* StaticMeshActor ) const;
	UFUNCTION(BlueprintPure, Category = "Passthrough")
	bool IsSurfaceGeometryComponent(const UMeshComponent* MeshComponent) const;

	// Manually mark the stereo layer passthrough effect for updating
	UFUNCTION(BlueprintCallable, Category="Components|Stereo Layer")
	void MarkPassthroughStyleForUpdate();

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif // WITH_EDITOR

protected:
	virtual bool LayerRequiresTexture() override;
	virtual void RemoveSurfaceGeometryComponent(UMeshComponent* MeshComponent);

	UPROPERTY(Transient)
	TMap<FString, const UMeshComponent*> PassthroughComponentMap;

private:

	OculusHMD::FOculusPassthroughMeshRef CreatePassthroughMesh(UProceduralMeshComponent* ProceduralMeshComponent);
	OculusHMD::FOculusPassthroughMeshRef CreatePassthroughMesh(UStaticMeshComponent* StaticMeshComponent);

	/** Passthrough style needs to be marked for update **/
	bool bPassthroughStyleNeedsUpdate;

};
