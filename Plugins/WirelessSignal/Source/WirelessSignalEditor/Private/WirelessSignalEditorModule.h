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

	// 配置 View
	TSharedPtr<IDetailsView> SettingsView;
	// 生成配置 Tab
	TSharedRef<SDockTab> SpawnSettingsTab(const FSpawnTabArgs& Args);
	// 更新配置 Tab
	void UpdateSettingsTab();
	// 注册Tab生成器
	void RegisterTabSpawner();
	// 地图变更事件
	void OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType);

	// 层编辑器 Build 菜单展开事件
	TSharedRef<FExtender> OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList);
	// 构建 Build 菜单
	void CreateBuildMenu(FMenuBuilder& Builder);

	// 实时模式是否开启
	static bool IsRealtimeOn();
	// 模块是否正在执行
	static bool IsRunning();
	// 是否只烘培可见区域
	static bool IsBakeWhatYouSeeMode();

	// 开始按钮单击事件
	FReply OnStartClicked();
	// 保存和停止按钮单击事件
	FReply OnSaveAndStopClicked();
	// 取消按钮单击事件
	FReply OnCancelClicked();
	
	// 消息
	TSharedPtr<STextBlock> Messages;
};
