// Copyright Epic Games, Inc. All Rights Reserved.

#include "WirelessSignalModule.h"
#include "CoreMinimal.h"
#include "Internationalization/Internationalization.h"
#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "RenderingThread.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "WirelessSignal.h"
#include "SceneInterface.h"

#define LOCTEXT_NAMESPACE "StaticLightingSystem"

DEFINE_LOG_CATEGORY(LogWirelessSignal);

IMPLEMENT_MODULE(FWirelessSignalModule, WirelessSignal);

/** 启动模块 */
void FWirelessSignalModule::StartupModule()
{
	UE_LOG(LogWirelessSignal, Log, TEXT("WirelessSignal module is loaded"));

	// Maps virtual shader source directory /Plugin/WirelessSignal to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("WirelessSignal"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/WirelessSignal"), PluginShaderDir);

	FStaticLightingSystemInterface::Get()->RegisterImplementation(FName(TEXT("WirelessSignal")), this);
}

/** 销毁模块 */
void FWirelessSignalModule::ShutdownModule()
{
	FStaticLightingSystemInterface::Get()->UnregisterImplementation(FName(TEXT("WirelessSignal")));
	{
		check(StaticLightingSystems.Num() == 0);
	}
}

/** 给 World 分配带设置的静态光照系统 */
IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorldWithSettings(UWorld* InWorld, UWirelessSignalSettings* Settings)
{
	check(StaticLightingSystems.Find(InWorld) == nullptr);
	// 实例化静态光照系统 WirelessSignal
	FWirelessSignal* WirelessSignal = new FWirelessSignal(InWorld, this, Settings);
	// 将静态光照系统与 World 映射 加入Map
	StaticLightingSystems.Add(InWorld, WirelessSignal);

	FlushRenderingCommands();
	// 广播变更
	OnStaticLightingSystemsChanged.Broadcast();
	// 返回静态光照系统实例
	return WirelessSignal;
}

/** 给 World 分配静态光照系统 */
IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorld(UWorld* InWorld)
{
	// 获取 Settings 指针,并抓取配置 
	UWirelessSignalSettings* Settings = NewObject<UWirelessSignalSettings>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UWirelessSignalSettings::StaticClass()));
	Settings->GatherSettingsFromCVars();
	// 分配带配置的静态光照系统
	return AllocateStaticLightingSystemForWorldWithSettings(InWorld, Settings);
}

/** 移除静态光照系统 */
void FWirelessSignalModule::RemoveStaticLightingSystemForWorld(UWorld* InWorld)
{
	if (StaticLightingSystems.Find(InWorld) != nullptr)
	{
		// 获取 World 对应光照系统指针
		FWirelessSignal* WirelessSignal = StaticLightingSystems[InWorld];
		
		// 移除 World 对应光照系统
		StaticLightingSystems.Remove(InWorld);
		
		// 销毁游戏线程
		WirelessSignal->GameThreadDestroy();

		// 删除光照系统指令
		ENQUEUE_RENDER_COMMAND(DeleteWirelessSignalCmd)([WirelessSignal](FRHICommandListImmediate& RHICmdList) { delete WirelessSignal; });
		
		// 刷新渲染指令
		FlushRenderingCommands();
		
		// 广播静态光照系统变更
		OnStaticLightingSystemsChanged.Broadcast();
	}
}

/** 从映射表中获取 World 对应静态光照系统 */
IStaticLightingSystem* FWirelessSignalModule::GetStaticLightingSystemForWorld(UWorld* InWorld)
{
	return StaticLightingSystems.Find(InWorld) != nullptr ? *StaticLightingSystems.Find(InWorld) : nullptr;
}

/** 编辑器时钟事件 */
void FWirelessSignalModule::EditorTick()
{
	TArray<FWirelessSignal*> FinishedStaticLightingSystems;
	
	// 遍历所有的静态光照系统
	for (auto& StaticLightingSystem : StaticLightingSystems)
	{
		// 获取 WirelessSignal 系统
		FWirelessSignal* WirelessSignal = StaticLightingSystem.Value;
		// 执行 编辑器时钟事件
		WirelessSignal->EditorTick();
		// 若光照构建完成 且 模式处于烘培所有区域时
		if (WirelessSignal->LightBuildPercentage >= 100 && WirelessSignal->Settings->Mode != EWirelessSignalMode::BakeWhatYouSee)
		{
			// 将子系统加入完成表
			FinishedStaticLightingSystems.Add(WirelessSignal);
		}
	}
	// 遍历完成表
	for (auto& StaticLightingSystem : FinishedStaticLightingSystems)
	{
		// [作用不明] 猜测与事件显示相关
		extern ENGINE_API void ToggleLightmapPreview_GameThread(UWorld * InWorld);

		ToggleLightmapPreview_GameThread(StaticLightingSystem->World);
	}
}

/** 判断系统是否在执行 */
bool FWirelessSignalModule::IsStaticLightingSystemRunning()
{
	// 若静态光照系统Map中存在元素,则系统正在执行
	return StaticLightingSystems.Num() > 0;
}

#undef LOCTEXT_NAMESPACE