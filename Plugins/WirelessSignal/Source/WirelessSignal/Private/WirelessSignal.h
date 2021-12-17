#pragma once

// #include "ScenePrivate.h"
#include "UObject/GCObjectScopeGuard.h"
// #include "VolumetricLightmap.h"
#include "WirelessSignalModule.h"
#include "WirelessSignalSettings.h"
#include "Scene/Scene.h"

class FWirelessSignal : public IStaticLightingSystem
{
public: 
	FWirelessSignal(UWorld* InWorld, FWirelessSignalModule* GPULightmassModule, UWirelessSignalSettings* InSettings = nullptr);
	virtual ~FWirelessSignal();
	void GameThreadDestroy();

	// 获取基元 Mesh 地图构造数据
	virtual const FMeshMapBuildData* GetPrimitiveMeshMapBuildData(const UPrimitiveComponent* Component, int32 LODIndex) override;
	// 获取光源组件地图构造数据
	virtual const FLightComponentMapBuildData* GetLightComponentMapBuildData(const ULightComponent* Component) override;
	// 获取预计算 Volumetric light map
	virtual const FPrecomputedVolumetricLightmap* GetPrecomputedVolumetricLightmap() override;

	UWorld* World;
	FWirelessSignalModule* WirelessSignalModule;
	UWirelessSignalSettings* Settings;
	TUniquePtr<FGCObjectScopeGuard> SettingsGuard;

	void InstallGameThreadEventHooks();
	void RemoveGameThreadEventHooks();

	// void StartRecordingVisibleTiles();
	// void EndRecordingVisibleTiles();

	// 场景
	WirelessSignal::FScene Scene;

	TSharedPtr<SNotificationItem> LightBuildNotification;
	int32 LightBuildPercentage;

	// 实例起始时间
	double StartTime = 0;

	void EditorTick();

private:
	// ---游戏线程事件钩子---------------------------------------------------------------------------------

	// 预世界完成销毁
	void OnPreWorldFinishDestroy(UWorld* World);
	// 基元组件注册
	void OnPrimitiveComponentRegistered(UPrimitiveComponent* InComponent);
	// 基元组件销毁
	void OnPrimitiveComponentUnregistered(UPrimitiveComponent* InComponent);
	// 光源组件注册
	void OnLightComponentRegistered(ULightComponentBase* InComponent);
	// 光源组件销毁
	void OnLightComponentUnregistered(ULightComponentBase* InComponent);
	// 静态光源通道重分配
	void OnStationaryLightChannelReassigned(ULightComponentBase* InComponent, int32 NewShadowMapChannel);
	// 无线信号重要体积更改
	void OnWirelessSignalImportanceVolumeModified();
	// 材质无效化
	void OnMaterialInvalidated(FMaterialRenderProxy* Material);
};
