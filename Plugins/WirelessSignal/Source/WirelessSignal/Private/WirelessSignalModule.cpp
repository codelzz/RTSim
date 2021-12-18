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

void FWirelessSignalModule::StartupModule()
{
	UE_LOG(LogWirelessSignal, Log, TEXT("WirelessSignal module is loaded"));
	
	// Maps virtual shader source directory /Plugin/WirelessSignal to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("WirelessSignal"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/WirelessSignal"), PluginShaderDir);

	FStaticLightingSystemInterface::Get()->RegisterImplementation(FName(TEXT("WirelessSignal")), this);
}

void FWirelessSignalModule::ShutdownModule()
{
	FStaticLightingSystemInterface::Get()->UnregisterImplementation(FName(TEXT("WirelessSignal")));

	{
		check(StaticLightingSystems.Num() == 0);
	}
}

IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorldWithSettings(UWorld* InWorld, UWirelessSignalSettings* Settings)
{
	check(StaticLightingSystems.Find(InWorld) == nullptr);

	FWirelessSignal* WirelessSignal = new FWirelessSignal(InWorld, this, Settings);

	StaticLightingSystems.Add(InWorld, WirelessSignal);

	FlushRenderingCommands();

	OnStaticLightingSystemsChanged.Broadcast();

	return WirelessSignal;
}

IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorld(UWorld* InWorld)
{
	// Gather settings from CVars
	UWirelessSignalSettings* Settings = NewObject<UWirelessSignalSettings>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UWirelessSignalSettings::StaticClass()));
	Settings->GatherSettingsFromCVars();

	return AllocateStaticLightingSystemForWorldWithSettings(InWorld, Settings);
}

void FWirelessSignalModule::RemoveStaticLightingSystemForWorld(UWorld* InWorld)
{
	if (StaticLightingSystems.Find(InWorld) != nullptr)
	{
		FWirelessSignal* WirelessSignal = StaticLightingSystems[InWorld];

		StaticLightingSystems.Remove(InWorld);

		WirelessSignal->GameThreadDestroy();

		ENQUEUE_RENDER_COMMAND(DeleteWirelessSignalCmd)([WirelessSignal](FRHICommandListImmediate& RHICmdList) { delete WirelessSignal; });

		FlushRenderingCommands();

		OnStaticLightingSystemsChanged.Broadcast();
	}
}

IStaticLightingSystem* FWirelessSignalModule::GetStaticLightingSystemForWorld(UWorld* InWorld)
{
	return StaticLightingSystems.Find(InWorld) != nullptr ? *StaticLightingSystems.Find(InWorld) : nullptr;
}

void FWirelessSignalModule::EditorTick()
{
	TArray<FWirelessSignal*> FinishedStaticLightingSystems;

	for (auto& StaticLightingSystem : StaticLightingSystems)
	{
		FWirelessSignal* WirelessSignal = StaticLightingSystem.Value;
		WirelessSignal->EditorTick();
		if (WirelessSignal->LightBuildPercentage >= 100 && WirelessSignal->Settings->Mode != EWirelessSignalMode::BakeWhatYouSee)
		{
			FinishedStaticLightingSystems.Add(WirelessSignal);
		}
	}

	for (auto& StaticLightingSystem : FinishedStaticLightingSystems)
	{
		extern ENGINE_API void ToggleLightmapPreview_GameThread(UWorld * InWorld);

		ToggleLightmapPreview_GameThread(StaticLightingSystem->World);
	}
}

bool FWirelessSignalModule::IsStaticLightingSystemRunning()
{
	return StaticLightingSystems.Num() > 0;
}

#undef LOCTEXT_NAMESPACE