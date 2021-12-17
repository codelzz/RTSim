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

	// ��ȡ��Ԫ Mesh ��ͼ��������
	virtual const FMeshMapBuildData* GetPrimitiveMeshMapBuildData(const UPrimitiveComponent* Component, int32 LODIndex) override;
	// ��ȡ��Դ�����ͼ��������
	virtual const FLightComponentMapBuildData* GetLightComponentMapBuildData(const ULightComponent* Component) override;
	// ��ȡԤ���� Volumetric light map
	virtual const FPrecomputedVolumetricLightmap* GetPrecomputedVolumetricLightmap() override;

	UWorld* World;
	FWirelessSignalModule* WirelessSignalModule;
	UWirelessSignalSettings* Settings;
	TUniquePtr<FGCObjectScopeGuard> SettingsGuard;

	void InstallGameThreadEventHooks();
	void RemoveGameThreadEventHooks();

	// void StartRecordingVisibleTiles();
	// void EndRecordingVisibleTiles();

	// ����
	WirelessSignal::FScene Scene;

	TSharedPtr<SNotificationItem> LightBuildNotification;
	int32 LightBuildPercentage;

	// ʵ����ʼʱ��
	double StartTime = 0;

	void EditorTick();

private:
	// ---��Ϸ�߳��¼�����---------------------------------------------------------------------------------

	// Ԥ�����������
	void OnPreWorldFinishDestroy(UWorld* World);
	// ��Ԫ���ע��
	void OnPrimitiveComponentRegistered(UPrimitiveComponent* InComponent);
	// ��Ԫ�������
	void OnPrimitiveComponentUnregistered(UPrimitiveComponent* InComponent);
	// ��Դ���ע��
	void OnLightComponentRegistered(ULightComponentBase* InComponent);
	// ��Դ�������
	void OnLightComponentUnregistered(ULightComponentBase* InComponent);
	// ��̬��Դͨ���ط���
	void OnStationaryLightChannelReassigned(ULightComponentBase* InComponent, int32 NewShadowMapChannel);
	// �����ź���Ҫ�������
	void OnWirelessSignalImportanceVolumeModified();
	// ������Ч��
	void OnMaterialInvalidated(FMaterialRenderProxy* Material);
};
