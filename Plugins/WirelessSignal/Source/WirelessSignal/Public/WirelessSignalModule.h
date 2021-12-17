// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Rendering/StaticLightingSystemInterface.h"


/**
 * ��ģ��Ĺ����ӿ�.�ڴ���������,�ýӿ�ֻ�Ե�ǰ������ֵ�ģ�鹫��
 */
class IWirelessSignalModule : public IModuleInterface
{
	/**
	 *  �Ե���(Singleton)��ʽ���ʸ�ģ��ӿ�(����ʹ��). ��Ҫע��������ٽ׶ε��ø÷���ʱ,��ģ��
	 *  �����ѱ�ж��.
	 * 
	 *  @return Returns ��ʵ��,��ģ����Ҫʱ����
	 */
	static inline IWirelessSignalModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IWirelessSignalModule>("WirelessSignal");
	}

	/**
	 *  ���ģ���Ƿ��Ѽ��ز�����. Get() �������� IsAvailable() ����trueʱ��Ч.
	 * 
	 *  @return True ��ģ���Ѽ��ز���׼������
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("WirelessSignal");
	}
};


class FWirelessSignalModule : public IWirelessSignalModule, public IStaticLightingSystemImpl
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// ֧��ʵʱԤ��
	virtual bool SupportsRealtimePreview() override { return true; }

	// �� World ���侲̬����ϵͳ
	virtual IStaticLightingSystem* AllocateStaticLightingSystemForWorld(UWorld* InWorld) override;
	// �� World ����������õľ�̬����ϵͳ
	virtual IStaticLightingSystem* AllocateStaticLightingSystemForWorldWithSettings(UWorld* InWorld, class UWirelessSignalSettings* Settings);
	// �� World �Ƴ���̬����ϵͳ
	virtual void RemoveStaticLightingSystemForWorld(UWorld* InWorld) override;
	// �� World ��ȡ�������ϵͳ
	virtual IStaticLightingSystem* GetStaticLightingSystemForWorld(UWorld* InWorld) override;
	// �༭��ʱ��
	virtual void EditorTick() override;
	// �жϾ�̬����ϵͳ�Ƿ�ִ��
	virtual bool IsStaticLightingSystemRunning() override;

	// Due to limitations in our TMap implementation I cannot use TUniquePtr here
	// But the GPULightmassModule is the only owner of all static lighting systems, and all worlds weak refer to the systems
	// ��̬����ϵͳӳ���
	TMap<UWorld*, class FWirelessSignal*> StaticLightingSystems;

	// ��̬����ϵͳ����¼�
	FSimpleMulticastDelegate OnStaticLightingSystemsChanged;

};

DECLARE_LOG_CATEGORY_EXTERN(LogWirelessSignal, Log, All);
