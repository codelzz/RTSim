// Copyright Epic Games, Inc. All Rights Reserved.WirelessSignal
#include "WirelessSignalEditorModule.h"
#include "CoreMinimal.h"
#include "Internationalization/Internationalization.h"
#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "RenderingThread.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "SceneInterface.h"
#include "LevelEditor.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "WirelessSignalSettings.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "EditorFontGlyphs.h"
#include "WirelessSignalModule.h"
#include "LevelEditorViewport.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"

extern ENGINE_API void ToggleLightmapPreview_GameThread(UWorld* InWorld);

#define LOCTEXT_NAMESPACE "StaticLightingSystem"

IMPLEMENT_MODULE( FWirelessSignalEditorModule, WirelessSignalEditor )

FName WirelessSignalSettingsTabName = TEXT("WirelessSignalSettings");

/** 启动模块 */
void FWirelessSignalEditorModule::StartupModule()
{
	// 层编辑器事件绑定
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	LevelEditorModule.OnTabManagerChanged().AddRaw(this, &FWirelessSignalEditorModule::RegisterTabSpawner);
	LevelEditorModule.OnMapChanged().AddRaw(this, &FWirelessSignalEditorModule::OnMapChanged);
	auto BuildMenuExtender = FLevelEditorModule::FLevelEditorMenuExtender::CreateRaw(this, &FWirelessSignalEditorModule::OnExtendLevelEditorBuildMenu);
	LevelEditorModule.GetAllLevelEditorToolbarBuildMenuExtenders().Add(BuildMenuExtender);
}

/** 停止模块 */
void FWirelessSignalEditorModule::ShutdownModule()
{
}

/** 注册Tab生成器 */
void FWirelessSignalEditorModule::RegisterTabSpawner()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

	LevelEditorTabManager->RegisterTabSpawner(WirelessSignalSettingsTabName, FOnSpawnTab::CreateRaw(this, &FWirelessSignalEditorModule::SpawnSettingsTab))
		.SetDisplayName(LOCTEXT("WirelessSignalSettingsTitle", "Wireless Signal"))
		//.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.LightingScenarioIcon16x"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

/** 生成配置Tab */
TSharedRef<SDockTab> FWirelessSignalEditorModule::SpawnSettingsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropPlugin = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, true, FDetailsViewArgs::HideNameArea, false, GUnrealEd);
	DetailsViewArgs.bShowActorLabel = false;

	// 新建一个 Detail View 到 Settings View 中
	SettingsView = PropPlugin.CreateDetailView(DetailsViewArgs);

	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{	// 如果子系统存在,将系统设置加载到 Settings View
		if (World->GetSubsystem<UWirelessSignalSubsystem>())
		{
			SettingsView->SetObject(World->GetSubsystem<UWirelessSignalSubsystem>()->GetSettings());
		}
	}

	// DockTab 实现
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("Level.LightingScenarioIcon16x"))
		.Label(NSLOCTEXT("WirelessSignal", "WirelessSignalSettingsTabTitle", "Wireless Signal"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[

				// Start Build - 开始构建按钮
				// 在子系统非运行状态下显示, 与 OnStartClicked 事件绑定.
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 8.f, 0.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					.IsEnabled(IsRayTracingEnabled())
					.Visibility_Lambda([](){ return IsRunning() ? EVisibility::Collapsed : EVisibility::Visible; })
					.OnClicked_Raw(this, &FWirelessSignalEditorModule::OnStartClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
							.Text(FEditorFontGlyphs::Lightbulb_O)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0, 0, 0)
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Text_Lambda( []()
							{ 
								return FWirelessSignalEditorModule::IsBakeWhatYouSeeMode() ? 
										LOCTEXT("WirelessSignalSettingsStartInteractive", "Start Building Lighting") : 
										LOCTEXT("WirelessSignalSettingsStartFull", "Build Lighting");
							})

						]
					]
				]
	
				// Save and Stop Building - 保存和停止构建按钮
				// 在系统处于只烘培可见区域时出现,与 OnSaveAndStopClicked 事件绑定
				+SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 8.f, 0.0f)
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					.Visibility_Lambda([](){ return IsRunning() && IsBakeWhatYouSeeMode() ? EVisibility::Visible : EVisibility::Collapsed; })
					.OnClicked_Raw(this, &FWirelessSignalEditorModule::OnSaveAndStopClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
							.Text(FEditorFontGlyphs::Lightbulb_O)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0, 0, 0)
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Text(LOCTEXT("WirelessSignalSettingsSaveAndStop", "Save And Stop Building"))
						]
					]
				]

				// Cancel Build - 取消按钮
				// 在系统运行时显示,与 OnCancelClicked 绑定.
				+SHorizontalBox::Slot()
				.Padding(0.0f, 0.0f, 8.f, 0.0f)
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Danger")
					.Visibility_Lambda([](){ return IsRunning() ? EVisibility::Visible: EVisibility::Collapsed; })
					.OnClicked_Raw(this, &FWirelessSignalEditorModule::OnCancelClicked)
					.Text(LOCTEXT("WirelessSignalSettingsCancel", "Cancel Build"))
					.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
				]
				
				// Real Time Checkbox - 实时模式选择
				// 控制是否开启实时预览模式
				+SHorizontalBox::Slot()
				.FillWidth(1.0)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda( [] () { return FWirelessSignalEditorModule::IsRealtimeOn() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda( [] (ECheckBoxState NewState) 
					{
						if (GCurrentLevelEditingViewportClient)
						{
							GCurrentLevelEditingViewportClient->SetRealtime( NewState == ECheckBoxState::Checked );
						}	
					})
				]
				
				// Real Time Textblock - 实时模式文本
				// 显示实时模式状态
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(140)
					[	
						SNew(STextBlock)
						.Text_Lambda( [](){ return FWirelessSignalEditorModule::IsRealtimeOn() ? LOCTEXT("WirelessSignalRealtimeEnabled", "Viewport Realtime is ON ") : LOCTEXT("WirelessSignalRealtimeDisabled", "Viewport Realtime is OFF");})
					]
				]
			]

			// Message Textblock - 消息文本
			// 显示提示信息
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.f, 4.f)
			[
				SAssignNew(Messages, STextBlock)
				.AutoWrapText(true)
				.Text_Lambda( []() -> FText
				{

					bool bIsRayTracingEnabled = IsRayTracingEnabled();
					FText RTDisabledMsg = LOCTEXT("WirelessSignalRayTracingDisabled", "Wireless Signal requires ray tracing support which is disabled.");
					if (!bIsRayTracingEnabled)
					{
						return LOCTEXT("WirelessSignalRayTracingDisabled", "Wireless Signal requires ray tracing support which is disabled.");
					}

					// Ready, 准备完成
					static FText ReadyMsg = FText(LOCTEXT("WirelessSignalReady", "Wireless Signal is ready."));

					// Ready, BWYS, 准备完成, 光照将会基于以只烘焙可见区域重构
					static FText BWYSReadyMsg = FText(LOCTEXT("WirelessSignalReadyBWYS", "Wireless Signal is ready. Lighting will rebuild continuously in Bake What You See mode until saved or canceled."));

					// Ready, BWYS+RT OFF Warning 
					static FText RtOffBWYSWarningMsg = LOCTEXT("WirelessSignalSpeedReadyRTWarning", "Building Lighting when using Bake What You See Mode will automatically enable Viewport Realtime to start building. Lighting will rebuild continuously in Bake What You See mode until saved or canceled.");

					// Building FULL + RT Off Warning
					UWorld* World = GEditor->GetEditorWorldContext().World();
					FText BuildingMsg = FText::Format(LOCTEXT("WirelessSignalBuildingLighting", "Wireless Signal is building lighting for {0}."), FText::FromString(World->GetActiveLightingScenario() ? World->GetActiveLightingScenario()->GetOuter()->GetName() : World->GetName()));

					// Building FULL + RT ON Warning 
					static FText BuildingFullRTOnMsg = LOCTEXT("WirelessSignalBuildingFullRTOn", "Wireless Signal runs in slow mode when the viewport is realtime to avoid freezing. Uncheck Viewport Realtime to get full speed.");

					// Building BWYS + RT ON Warning 
					static FText BuildingRTOnMsg = LOCTEXT("WirelessSignalBuildingInteractiveRTOn", "Disable Viewport Realtime to speed up building.");

					// Building BWYS + RT OFF Warning 
					static FText BuidlingRTOffMsg = LOCTEXT("WirelessSignalBuildingInteractiveRTOff", "Re-enable Viewport Realtime to preview lighting.  Enabling Viewport Realtime will slow down building, to avoid freezing.");

					bool bIsRunning = IsRunning();
					bool bIsInteractive = IsBakeWhatYouSeeMode();
					bool bIsRealtime = IsRealtimeOn();
					if (bIsRunning)
					{
						if (bIsInteractive)
						{
							return bIsRealtime ? BuildingRTOnMsg : BuidlingRTOffMsg;
						}

						return bIsRealtime ? BuildingFullRTOnMsg : BuildingMsg;
					}
					else if (bIsInteractive)
					{
						return bIsRealtime ?  BWYSReadyMsg : RtOffBWYSWarningMsg;
					}

					return bIsRealtime ? BuildingFullRTOnMsg : ReadyMsg;
				})	
			]

			+ SVerticalBox::Slot()
			[
				SettingsView.ToSharedRef()
			]
		];
}

/** 更新配置Tab */
void FWirelessSignalEditorModule::UpdateSettingsTab()
{
	if (SettingsView.IsValid())
	{
		SettingsView->ForceRefresh();
	}
}

/** 是否只烘培可见区域 */
bool FWirelessSignalEditorModule::IsBakeWhatYouSeeMode()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (UWirelessSignalSubsystem* LMSubsystem = World->GetSubsystem<UWirelessSignalSubsystem>())
		{	// 获取子系统BWYS配置
			return LMSubsystem->GetSettings()->Mode == EWirelessSignalMode::BakeWhatYouSee;
		}
	}

	return false;
}

/** 是否已开启实时模式 */
bool FWirelessSignalEditorModule::IsRealtimeOn() 
{	// 获取 层编辑器 Viewport 实时状态配置
	return GCurrentLevelEditingViewportClient && GCurrentLevelEditingViewportClient->IsRealtime();
}

/** 子系统是否正在运行 */
bool FWirelessSignalEditorModule::IsRunning() 
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{	// 返回子系统运行状态
		return World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning();
	}

	return false;
}

/** 开始按钮单击事件 */
FReply FWirelessSignalEditorModule::OnStartClicked()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (!World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			if (IsBakeWhatYouSeeMode() && !IsRealtimeOn() && GCurrentLevelEditingViewportClient != nullptr)
			{	// 若BWYS开启 且 处于非实时模式 且 层编辑器Viewport client 有效,开启实时模式
				GCurrentLevelEditingViewportClient->SetRealtime(true);
			}
			// 启动子系统
			World->GetSubsystem<UWirelessSignalSubsystem>()->Launch();
			// 当光照构建完成时,响应 UpdateSettingsTab 事件
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().AddRaw(this, &FWirelessSignalEditorModule::UpdateSettingsTab);
		}
	}
	// 更新配置Tab
	UpdateSettingsTab();

	return FReply::Handled();
}

/** 保存和停止单击事件 */
FReply FWirelessSignalEditorModule::OnSaveAndStopClicked()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		// 若系统处于运行状态
		if (World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			// 保存, 停止, 移除所有与光照构建结束绑定的事件
			World->GetSubsystem<UWirelessSignalSubsystem>()->Save();
			World->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
		}
	}

	UpdateSettingsTab();

	return FReply::Handled();
}

/** 取消按钮单击事件 */
FReply FWirelessSignalEditorModule::OnCancelClicked()
{

	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		// 若系统正在运行
		if (World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			// 停止, 移除所有与光照构建结束绑定的事件
			World->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
		}
	}
	// 更新配置Tab
	UpdateSettingsTab();

	return FReply::Handled();
}

/** 地图变更事件 */
void FWirelessSignalEditorModule::OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (SettingsView.IsValid())
	{
		if (World->GetSubsystem<UWirelessSignalSubsystem>())
		{
			SettingsView->SetObject(World->GetSubsystem<UWirelessSignalSubsystem>()->GetSettings(), true);

			// 若加载或新建地图
			if (MapChangeType == EMapChangeType::LoadMap || MapChangeType == EMapChangeType::NewMap)
			{	
				// 将 UpdateSettingsTab 与 OnLightBuildEnded 绑定
				World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().AddRaw(this, &FWirelessSignalEditorModule::UpdateSettingsTab);
			}
			// 若销毁地图
			else if (MapChangeType == EMapChangeType::TearDownWorld)
			{
				// 移除 OnLightBuildEnded 所有绑定
				World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
			}
		}
	}
}

/** 层编辑器菜单创建事件 */
TSharedRef<FExtender> FWirelessSignalEditorModule::OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList)
{
	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension("LevelEditorLighting", EExtensionHook::First, nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FWirelessSignalEditorModule::CreateBuildMenu));

	return Extender;
}

/** 创建 Build Menu */
void FWirelessSignalEditorModule::CreateBuildMenu(FMenuBuilder& Builder)
{
	FUIAction ActionOpenWirelessSignalSettingsTab(FExecuteAction::CreateLambda([]() {
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
		LevelEditorTabManager->TryInvokeTab(WirelessSignalSettingsTabName);
	}), FCanExecuteAction());

	Builder.AddMenuEntry(LOCTEXT("WirelessSignalSettingsTitle", "Wireless Signal"),
		LOCTEXT("OpensWirelessSignalSettings", "Opens Wireless Signal settings tab."), FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.LightingScenarioIcon16x"), ActionOpenWirelessSignalSettingsTab,
		NAME_None, EUserInterfaceActionType::Button);
}

#undef LOCTEXT_NAMESPACE
