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

/** ����ģ�� */
void FWirelessSignalModule::StartupModule()
{
	UE_LOG(LogWirelessSignal, Log, TEXT("WirelessSignal module is loaded"));

	// Maps virtual shader source directory /Plugin/WirelessSignal to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("WirelessSignal"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/WirelessSignal"), PluginShaderDir);

	FStaticLightingSystemInterface::Get()->RegisterImplementation(FName(TEXT("WirelessSignal")), this);
}

/** ����ģ�� */
void FWirelessSignalModule::ShutdownModule()
{
	FStaticLightingSystemInterface::Get()->UnregisterImplementation(FName(TEXT("WirelessSignal")));
	{
		check(StaticLightingSystems.Num() == 0);
	}
}

/** �� World ��������õľ�̬����ϵͳ */
IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorldWithSettings(UWorld* InWorld, UWirelessSignalSettings* Settings)
{
	check(StaticLightingSystems.Find(InWorld) == nullptr);
	// ʵ������̬����ϵͳ WirelessSignal
	FWirelessSignal* WirelessSignal = new FWirelessSignal(InWorld, this, Settings);
	// ����̬����ϵͳ�� World ӳ�� ����Map
	StaticLightingSystems.Add(InWorld, WirelessSignal);

	FlushRenderingCommands();
	// �㲥���
	OnStaticLightingSystemsChanged.Broadcast();
	// ���ؾ�̬����ϵͳʵ��
	return WirelessSignal;
}

/** �� World ���侲̬����ϵͳ */
IStaticLightingSystem* FWirelessSignalModule::AllocateStaticLightingSystemForWorld(UWorld* InWorld)
{
	// ��ȡ Settings ָ��,��ץȡ���� 
	UWirelessSignalSettings* Settings = NewObject<UWirelessSignalSettings>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UWirelessSignalSettings::StaticClass()));
	Settings->GatherSettingsFromCVars();
	// ��������õľ�̬����ϵͳ
	return AllocateStaticLightingSystemForWorldWithSettings(InWorld, Settings);
}

/** �Ƴ���̬����ϵͳ */
void FWirelessSignalModule::RemoveStaticLightingSystemForWorld(UWorld* InWorld)
{
	if (StaticLightingSystems.Find(InWorld) != nullptr)
	{
		// ��ȡ World ��Ӧ����ϵͳָ��
		FWirelessSignal* WirelessSignal = StaticLightingSystems[InWorld];
		
		// �Ƴ� World ��Ӧ����ϵͳ
		StaticLightingSystems.Remove(InWorld);
		
		// ������Ϸ�߳�
		WirelessSignal->GameThreadDestroy();

		// ɾ������ϵͳָ��
		ENQUEUE_RENDER_COMMAND(DeleteWirelessSignalCmd)([WirelessSignal](FRHICommandListImmediate& RHICmdList) { delete WirelessSignal; });
		
		// ˢ����Ⱦָ��
		FlushRenderingCommands();
		
		// �㲥��̬����ϵͳ���
		OnStaticLightingSystemsChanged.Broadcast();
	}
}

/** ��ӳ����л�ȡ World ��Ӧ��̬����ϵͳ */
IStaticLightingSystem* FWirelessSignalModule::GetStaticLightingSystemForWorld(UWorld* InWorld)
{
	return StaticLightingSystems.Find(InWorld) != nullptr ? *StaticLightingSystems.Find(InWorld) : nullptr;
}

/** �༭��ʱ���¼� */
void FWirelessSignalModule::EditorTick()
{
	TArray<FWirelessSignal*> FinishedStaticLightingSystems;
	
	// �������еľ�̬����ϵͳ
	for (auto& StaticLightingSystem : StaticLightingSystems)
	{
		// ��ȡ WirelessSignal ϵͳ
		FWirelessSignal* WirelessSignal = StaticLightingSystem.Value;
		// ִ�� �༭��ʱ���¼�
		WirelessSignal->EditorTick();
		// �����չ������ �� ģʽ���ں�����������ʱ
		if (WirelessSignal->LightBuildPercentage >= 100 && WirelessSignal->Settings->Mode != EWirelessSignalMode::BakeWhatYouSee)
		{
			// ����ϵͳ������ɱ�
			FinishedStaticLightingSystems.Add(WirelessSignal);
		}
	}
	// ������ɱ�
	for (auto& StaticLightingSystem : FinishedStaticLightingSystems)
	{
		// [���ò���] �²����¼���ʾ���
		extern ENGINE_API void ToggleLightmapPreview_GameThread(UWorld * InWorld);

		ToggleLightmapPreview_GameThread(StaticLightingSystem->World);
	}
}

/** �ж�ϵͳ�Ƿ���ִ�� */
bool FWirelessSignalModule::IsStaticLightingSystemRunning()
{
	// ����̬����ϵͳMap�д���Ԫ��,��ϵͳ����ִ��
	return StaticLightingSystems.Num() > 0;
}

#undef LOCTEXT_NAMESPACE