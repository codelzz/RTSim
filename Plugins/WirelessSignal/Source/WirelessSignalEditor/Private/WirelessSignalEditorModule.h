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
	/** IModuleInterface 实现 */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// 设置U
	TSharedPtr<IDetailsView> SettingsView;
	// 生成设置Tab
	TSharedRef<SDockTab> SpawnSettingsTab(const FSpawnTabArgs& Args);
	// 更新设置Tab
	void UpdateSettingsTab();
	// 注册Tab生成器
	void RegisterTabSpawner();
	// 地图改变事件
	void OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType);

	// 层编辑器Build菜单展开事件
	TSharedRef<FExtender> OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList);
	// 构建Build菜单
	void CreateBuildMenu(FMenuBuilder& Builder);

	// 实时模式是否已开启
	static bool IsRealtimeOn();
	// 是否正在执行
	static bool IsRunning();
	// 烘培模式是否只烘培可见区域
	static bool IsBakeWhatYouSeeMode();

	// 开始按钮点击事件
	FReply OnStartClicked();
	// 保存和停止按钮点击事件
	FReply OnSaveAndStopClicked();
	// 取消按钮点击事件
	FReply OnCancelClicked();

	// 消息
	TSharedPtr<STextBlock> Messages;
};
