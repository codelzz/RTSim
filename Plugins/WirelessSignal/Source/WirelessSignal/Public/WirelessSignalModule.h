// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Rendering/StaticLightingSystemInterface.h"


/**
 * 该模块的公共接口.在大多数情况下,该接口只对当前插件的兄弟模块公开
 */
class IWirelessSignalModule : public IModuleInterface
{
	/**
	 *  以单例(Singleton)形式访问该模块接口(便于使用). 需要注意的在销毁阶段调用该方法时,该模块
	 *  可能已被卸载.
	 * 
	 *  @return Returns 单实例,在模块需要时加载
	 */
	static inline IWirelessSignalModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IWirelessSignalModule>("WirelessSignal");
	}

	/**
	 *  检查模块是否已加载并就绪. Get() 方法仅在 IsAvailable() 返回true时有效.
	 * 
	 *  @return True 若模块已加载并且准备就绪
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

	// 支持实时预览
	virtual bool SupportsRealtimePreview() override { return true; }

	// 给 World 分配静态光照系统
	virtual IStaticLightingSystem* AllocateStaticLightingSystemForWorld(UWorld* InWorld) override;
	// 给 World 分配基于配置的静态光照系统
	virtual IStaticLightingSystem* AllocateStaticLightingSystemForWorldWithSettings(UWorld* InWorld, class UWirelessSignalSettings* Settings);
	// 从 World 移除静态光照系统
	virtual void RemoveStaticLightingSystemForWorld(UWorld* InWorld) override;
	// 从 World 获取今天光照系统
	virtual IStaticLightingSystem* GetStaticLightingSystemForWorld(UWorld* InWorld) override;
	// 编辑器时钟
	virtual void EditorTick() override;
	// 判断静态光照系统是否执行
	virtual bool IsStaticLightingSystemRunning() override;

	// Due to limitations in our TMap implementation I cannot use TUniquePtr here
	// But the GPULightmassModule is the only owner of all static lighting systems, and all worlds weak refer to the systems
	// 静态光照系统映射表
	TMap<UWorld*, class FWirelessSignal*> StaticLightingSystems;

	// 静态光照系统变更事件
	FSimpleMulticastDelegate OnStaticLightingSystemsChanged;

};

DECLARE_LOG_CATEGORY_EXTERN(LogWirelessSignal, Log, All);
