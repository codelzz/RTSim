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

IMPLEMENT_MODULE( FWirelessSignalModule, WirelessSignal )

/** 启动模块 */
void FWirelessSignalModule::StartupModule()
{
	UE_LOG(LogWirelessSignal, Log, TEXT("WirelessSignal module is loaded"));
	
	// Maps virtual shader source directory /Plugin/WirelessSignal to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("WirelessSignal"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/WirelessSignal"), PluginShaderDir);

	FStaticLightingSystemInterface::Get()->RegisterImplementation(FName(TEXT("WirelessSignal")), this);
}

/** 关闭模块 */
void FWirelessSignalModule::ShutdownModule()
{
	FStaticLightingSystemInterface::Get()->UnregisterImplementation(FName(TEXT("WirelessSignal")));

	{
		check(StaticLightingSystems.Num() == 0);
	}
}

/** 分配静态光照系统及其配置 */
IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorldWithSettings(UWorld* InWorld, UWirelessSignalSettings* Settings)
{
	check(StaticLightingSystems.Find(InWorld) == nullptr);
	
	// 给定配置新建一个 无线信号 静态光照系统 
	FWirelessSignal* WirelessSignal = new FWirelessSignal(InWorld, this, Settings);

	StaticLightingSystems.Add(InWorld, WirelessSignal);

	FlushRenderingCommands();

	OnStaticLightingSystemsChanged.Broadcast();

	return WirelessSignal;
}

// 分配静态光照系统
IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorld(UWorld* InWorld)
{
	// Gather settings from CVars
	UWirelessSignalSettings* Settings = NewObject<UWirelessSignalSettings>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UWirelessSignalSettings::StaticClass()));
	Settings->GatherSettingsFromCVars();

	return AllocateStaticLightingSystemForWorldWithSettings(InWorld, Settings);
}

// 移除静态光照系统
void FWirelessSignalModule::RemoveStaticLightingSystemForWorld(UWorld* InWorld)
{
	// 若存在当前 world 对应的静态光照系统
	if (StaticLightingSystems.Find(InWorld) != nullptr)
	{
		FWirelessSignal* WirelessSignal = StaticLightingSystems[InWorld];
		
		// 将光照系统移除
		StaticLightingSystems.Remove(InWorld);

		// 销毁系统游戏线程
		WirelessSignal->GameThreadDestroy();

		ENQUEUE_RENDER_COMMAND(DeleteWirelessSignalCmd)([WirelessSignal](FRHICommandListImmediate& RHICmdList) { delete WirelessSignal; });

		FlushRenderingCommands();

		OnStaticLightingSystemsChanged.Broadcast();
	}
}

// 获取静态光照系统
IStaticLightingSystem* FWirelessSignalModule::GetStaticLightingSystemForWorld(UWorld* InWorld)
{
	return StaticLightingSystems.Find(InWorld) != nullptr ? *StaticLightingSystems.Find(InWorld) : nullptr;
}

// 时钟事件
void FWirelessSignalModule::EditorTick()
{
	TArray<FWirelessSignal*> FinishedStaticLightingSystems;
	
	// 遍历所有静态光照系统
	for (auto& StaticLightingSystem : StaticLightingSystems)
	{
		// 获取 WirelessSignal 子系统
		FWirelessSignal* WirelessSignal = StaticLightingSystem.Value;
		// 执行子系统时钟事件
		WirelessSignal->EditorTick();
		// 若光照构建完成 且 处于完全烘培模式时
		if (WirelessSignal->LightBuildPercentage >= 100 && WirelessSignal->Settings->Mode != EWirelessSignalMode::BakeWhatYouSee)
		{
			// 将子系统加入完成列表
			FinishedStaticLightingSystems.Add(WirelessSignal);
		}
	}
	
	// 遍历所有已执行的静态光照系统
	for (auto& StaticLightingSystem : FinishedStaticLightingSystems)
	{
		extern ENGINE_API void ToggleLightmapPreview_GameThread(UWorld * InWorld);

		ToggleLightmapPreview_GameThread(StaticLightingSystem->World);
	}
}

/** 判断静态光照系统是否正在执行 */
bool FWirelessSignalModule::IsStaticLightingSystemRunning()
{
	// 若静态光照系统列表不为空,意味着还有未处理任务,系统仍在执行
	return StaticLightingSystems.Num() > 0;
}

#undef LOCTEXT_NAMESPACE