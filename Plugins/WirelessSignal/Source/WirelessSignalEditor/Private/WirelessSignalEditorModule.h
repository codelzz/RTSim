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
	/** IModuleInterface ʵ�� */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// ����U
	TSharedPtr<IDetailsView> SettingsView;
	// ��������Tab
	TSharedRef<SDockTab> SpawnSettingsTab(const FSpawnTabArgs& Args);
	// ��������Tab
	void UpdateSettingsTab();
	// ע��Tab������
	void RegisterTabSpawner();
	// ��ͼ�ı��¼�
	void OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType);

	// ��༭��Build�˵�չ���¼�
	TSharedRef<FExtender> OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList);
	// ����Build�˵�
	void CreateBuildMenu(FMenuBuilder& Builder);

	// ʵʱģʽ�Ƿ��ѿ���
	static bool IsRealtimeOn();
	// �Ƿ�����ִ��
	static bool IsRunning();
	// ����ģʽ�Ƿ�ֻ����ɼ�����
	static bool IsBakeWhatYouSeeMode();

	// ��ʼ��ť����¼�
	FReply OnStartClicked();
	// �����ֹͣ��ť����¼�
	FReply OnSaveAndStopClicked();
	// ȡ����ť����¼�
	FReply OnCancelClicked();

	// ��Ϣ
	TSharedPtr<STextBlock> Messages;
};
