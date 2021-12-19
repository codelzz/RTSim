// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Info.h"
#include "Subsystems/WorldSubsystem.h"
#include "WirelessSignalSettings.generated.h"

UENUM()
enum class EWirelessSignalMode : uint8
{
	FullBake,		// 烘培所有区域 
	BakeWhatYouSee,	// 烘培可见区域
	// BakeSelected  UMETA(DisplayName = "Bake Selected (Not Implemented)")
};

UENUM()
enum class EWirelessSignalDenoisingOptions : uint8
{
	None,						// 不降噪
	OnCompletion,				// 构建完成时降噪
	DuringInteractivePreview	// 交互预览时降噪
};

UCLASS(BlueprintType)
class WIRELESSSIGNAL_API UWirelessSignalSettings : public UObject
{
	GENERATED_BODY()

public:

	// 值为真时, 在每个瓦片上绘制表达渲染进度的绿色进度条
	// 红色进度条表示 First Bounce Ray Guiding 处理中
	// 在高亮度场景, 由于曝光影响进度条可能为黑色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = General)
	bool bShowProgressBars = true;

	// Full Bake 模式, 给每个场景中的物体渲染 full lightmap resolution
	// BWYS 模式, 只烘培视图中可见到的虚拟纹理瓦片,所处的次层级由虚拟纹理系统所决定. 可以通过移动相机
	// 来渲染更多的区域. BWYS 模式仅在点击保存按钮时保存结果.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = General)
	EWirelessSignalMode Mode;

	// 降噪开启时, 渲染结果会被 CPU 进行降噪.
	// On Completion 模式 - 在lightmap 完成后对整个 lightmap 进行降噪.
	// 在交互预览模式	 - 对每个瓦片完成时降噪,这有助于预览但较为抵消.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = General, DisplayName = "Denoise")
	EWirelessSignalDenoisingOptions DenoisingOptions = EWirelessSignalDenoisingOptions::OnCompletion;

	// 是否压缩 lightmap texture
	// 不开启时有利于减少下次但会增加4倍内存使用.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = General)
	bool bCompressLightmaps = true;

	// Total number of ray paths executed per texel across all bounces.
	// Set this to the lowest value that gives artifact-free results with the denoiser.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GlobalIllumination, DisplayName = "GI Samples", meta = (ClampMin = "32", ClampMax = "65536", UIMax = "8192"))
	int32 GISamples = 512;

	// Number of samples for stationary shadows, which are calculated and stored separately from GI.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GlobalIllumination, meta = (ClampMin = "32", ClampMax = "65536", UIMax = "8192"))
	int32 StationaryLightShadowSamples = 128;

	// Irradiance Caching should be enabled with interior scenes to achieve more physically correct GI intensities,
	// albeit with some biasing. Without IC the results may be darker than expected. It should be disabled for exterior scenes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GlobalIllumination)
	bool bUseIrradianceCaching = true;

	// If Irradiance Caching is enabled, First Bounce Ray Guiding will search the hemisphere over
	// each first bounce sample to find the brightest directions to weigh the rest of the samples towards.
	// This improves results for interior scenes with specific sources of light like a window.
	// The quality of this pass is controlled with the Trial Samples setting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GlobalIllumination, meta = (EditCondition = "bUseIrradianceCaching"))
	bool bUseFirstBounceRayGuiding = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VolumetricLightmap, DisplayName = "Quality Multiplier", meta = (ClampMin = "1", ClampMax = "256", UIMax = "32"))
	int32 VolumetricLightmapQualityMultiplier = 4;

	// Size of an Volumetric Lightmap voxel at the highest density (used around geometry), in world space units.
	// This setting has a large impact on build times and memory, use with caution.
	// Halving the DetailCellSize can increase memory by up to a factor of 8x.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VolumetricLightmap, DisplayName = "Detail Cell Size", meta = (ClampMin = "1", ClampMax = "20000", UIMax = "2000"))
	int32 VolumetricLightmapDetailCellSize = 200;

	// Number of samples per Irradiance Cache cell.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = IrradianceCaching, DisplayName = "Quality", meta = (EditCondition = "bUseIrradianceCaching", ClampMin = "4", ClampMax = "65536", UIMax = "8192"))
	int32 IrradianceCacheQuality = 128;

	// Size of each Irradiance Cache cell. Smaller sizes will be slower but more accurate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = IrradianceCaching, DisplayName = "Size", meta = (EditCondition = "bUseIrradianceCaching", ClampMin = "4", ClampMax = "1024"))
	int32 IrradianceCacheSpacing = 32;

	// Reject IC entries around corners to help reduce leaking and artifacts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = IrradianceCaching, DisplayName = "Corner Rejection", meta = (EditCondition = "bUseIrradianceCaching", ClampMin = "0.0", ClampMax = "8.0"))
	float IrradianceCacheCornerRejection = 1.0f;

	// If true, visualize the Irradiance Cache cells. This can be useful for setting the IC size and quality.
	// The visualization may appear black in very bright scenes that have been exposed down. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = IrradianceCaching, DisplayName = "Debug: Visualize", meta = (EditCondition = "bUseIrradianceCaching"))
	bool bVisualizeIrradianceCache = false;

	// Number of samples used for First Bounce Ray Guiding, which are thrown away before sampling for lighting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FirstBounceRayGuiding, DisplayName = "Trial Samples", meta = (EditCondition = "bUseFirstBounceRayGuiding"))
	int32 FirstBounceRayGuidingTrialSamples = 128;

	// Baking speed multiplier when Realtime is enabled in the viewer.
	// Setting this too high can cause the editor to become unresponsive with heavy scenes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = System, DisplayName = "Realtime Workload Factor", meta = (ClampMin = "1", ClampMax = "64"))
	int32 TilePassesInSlowMode = 1;

	// Baking speed multiplier when Realtime is disabled in the viewer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = System, DisplayName = "Non-Realtime Workload Factor", meta = (ClampMin = "1", ClampMax = "64"))
	int32 TilePassesInFullSpeedMode = 8;

	// Wireless Signal manages a pool for calculations of visible tiles. The pool size should be set based on the size of the
	// viewport and how many tiles will be visible on screen at once. Increasing this number increases GPU memory usage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = System, meta = (ClampMin = "16", ClampMax = "128"))
	int32 LightmapTilePoolSize = 55;

public:
	void GatherSettingsFromCVars();
	void ApplyImmediateSettingsToRunningInstances();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif // WITH_EDITOR
};

UCLASS(NotPlaceable)
class WIRELESSSIGNAL_API AWirelessSignalSettingsActor : public AInfo
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	UWirelessSignalSettings* Settings;
};

UCLASS()
class WIRELESSSIGNAL_API UWirelessSignalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	UWirelessSignalSettings* GetSettings();

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	void Launch();

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	void Stop();

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	bool IsRunning();

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	void StartRecordingVisibleTiles();

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	void EndRecordingVisibleTiles();

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	int32 GetPercentage();

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	void SetRealtime(bool bInRealtime);

	UFUNCTION(BlueprintCallable, Category = WirelessSignal)
	void Save();

	/* Accessor for the delegate called when the light build finishes successfully or is cancelled */
	FSimpleMulticastDelegate& OnLightBuildEnded()
	{
		return LightBuildEnded;
	}

private:
	AWirelessSignalSettingsActor* GetSettingsActor();

private:
	/* Called when the light build finishes successfully or is cancelled */
	FSimpleMulticastDelegate LightBuildEnded;
};
