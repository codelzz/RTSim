// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScenePrivate.h"
#include "VolumetricLightmap.h"
#include "WirelessSignalModule.h"
#include "WirelessSignalSettings.h"
#include "Scene/Scene.h"

class FWirelessSignal : public IStaticLightingSystem
{
public:
	FWirelessSignal(UWorld* InWorld, FWirelessSignalModule* WirelessSignalModule, UWirelessSignalSettings* InSettings = nullptr);
	virtual ~FWirelessSignal();
	void GameThreadDestroy();

	virtual const FMeshMapBuildData* GetPrimitiveMeshMapBuildData(const UPrimitiveComponent* Component, int32 LODIndex) override;
	virtual const FLightComponentMapBuildData* GetLightComponentMapBuildData(const ULightComponent* Component) override;
	virtual const FPrecomputedVolumetricLightmap* GetPrecomputedVolumetricLightmap() override;

	UWorld* World;
	FWirelessSignalModule* WirelessSignalModule;
	UWirelessSignalSettings* Settings;
	TUniquePtr<FGCObjectScopeGuard> SettingsGuard;

	WirelessSignal::FScene Scene;

	void InstallGameThreadEventHooks();
	void RemoveGameThreadEventHooks();

	void StartRecordingVisibleTiles();
	void EndRecordingVisibleTiles();
	
	// 通知, 显示在编辑器右下方
	TSharedPtr<SNotificationItem> LightBuildNotification;
	int32 LightBuildPercentage;
	
	double StartTime = 0;

	void EditorTick();

private:
	// Game thread event hooks
	void OnPreWorldFinishDestroy(UWorld* World);

	void OnPrimitiveComponentRegistered(UPrimitiveComponent* InComponent);
	void OnPrimitiveComponentUnregistered(UPrimitiveComponent* InComponent);
	void OnLightComponentRegistered(ULightComponentBase* InComponent);
	void OnLightComponentUnregistered(ULightComponentBase* InComponent);
	void OnStationaryLightChannelReassigned(ULightComponentBase* InComponent, int32 NewShadowMapChannel);
	void OnLightmassImportanceVolumeModified();
	void OnMaterialInvalidated(FMaterialRenderProxy* Material);
};
