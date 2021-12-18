// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "TickableEditorObject.h"
#include "Input/Reply.h"

class FExtender;
class FMenuBuilder;
class FSpawnTabArgs;
class FUICommandList;
class IDetailsView;
class SDockTab;
class STextBlock;

enum class EMapChangeType : uint8;

class FWirelessSignalEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// ���� View
	TSharedPtr<IDetailsView> SettingsView;
	// �������� Tab
	TSharedRef<SDockTab> SpawnSettingsTab(const FSpawnTabArgs& Args);
	// �������� Tab
	void UpdateSettingsTab();
	// ע��Tab������
	void RegisterTabSpawner();
	// ��ͼ����¼�
	void OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType);

	// ��༭�� Build �˵�չ���¼�
	TSharedRef<FExtender> OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList);
	// ���� Build �˵�
	void CreateBuildMenu(FMenuBuilder& Builder);

	// ʵʱģʽ�Ƿ���
	static bool IsRealtimeOn();
	// ģ���Ƿ�����ִ��
	static bool IsRunning();
	// �Ƿ�ֻ����ɼ�����
	static bool IsBakeWhatYouSeeMode();

	// ��ʼ��ť�����¼�
	FReply OnStartClicked();
	// �����ֹͣ��ť�����¼�
	FReply OnSaveAndStopClicked();
	// ȡ����ť�����¼�
	FReply OnCancelClicked();
	
	// ��Ϣ
	TSharedPtr<STextBlock> Messages;
};
