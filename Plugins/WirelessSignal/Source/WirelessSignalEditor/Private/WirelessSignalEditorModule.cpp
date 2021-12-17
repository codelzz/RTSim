// Copyright Epic Games, Inc. All Rights Reserved.

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
//#include "WirelessSignalModule.h"
#include "LevelEditorViewport.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"

extern ENGINE_API void ToggleLightmapPreview_GameThread(UWorld* InWorld);

// 静态光照系统
#define LOCTEXT_NAMESPACE "StaticLightingSystem"

// 指定实现模式为编辑器模块
IMPLEMENT_MODULE( FWirelessSignalEditorModule, WirelessSignalEditor )

// 指定设置Tab名称
FName WirelessSignalSettingsTabName = TEXT("WirelessSignalSettings");

void FWirelessSignalEditorModule::StartupModule()
{
	// 层编辑器模块事件绑定
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	LevelEditorModule.OnTabManagerChanged().AddRaw(this, &FWirelessSignalEditorModule::RegisterTabSpawner);
	LevelEditorModule.OnMapChanged().AddRaw(this, &FWirelessSignalEditorModule::OnMapChanged);
	auto BuildMenuExtender = FLevelEditorModule::FLevelEditorMenuExtender::CreateRaw(this, &FWirelessSignalEditorModule::OnExtendLevelEditorBuildMenu);
	LevelEditorModule.GetAllLevelEditorToolbarBuildMenuExtenders().Add(BuildMenuExtender);
}

void FWirelessSignalEditorModule::ShutdownModule()
{
}

void FWirelessSignalEditorModule::RegisterTabSpawner()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

	LevelEditorTabManager->RegisterTabSpawner(
		WirelessSignalSettingsTabName,
		FOnSpawnTab::CreateRaw(this, &FWirelessSignalEditorModule::SpawnSettingsTab))
		.SetDisplayName(LOCTEXT("WirelessSignalSettingsTitle", "Wireless Signal"))				// 指定Menu中Tab显示名称
		//.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.LightingScenarioIcon16x"))	// 指定Menu中Tab显示图标
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

TSharedRef<SDockTab> FWirelessSignalEditorModule::SpawnSettingsTab(const FSpawnTabArgs& Args)
{
	// 获取属性编辑器
	FPropertyEditorModule& PropPlugin = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, true, FDetailsViewArgs::HideNameArea, false, GUnrealEd);
	DetailsViewArgs.bShowActorLabel = false;

	// 创建一个DetailView作为SettingView
	SettingsView = PropPlugin.CreateDetailView(DetailsViewArgs);

	// 获取子系统配置并绑定在SettingView上
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (World->GetSubsystem<UWirelessSignalSubsystem>())
		{
			SettingsView->SetObject(World->GetSubsystem<UWirelessSignalSubsystem>()->GetSettings());
		}
	}
	// Settting DockTab 布局
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("Level.LightingScenarioIcon16x"))	// 指定table title 图标
		.Label(NSLOCTEXT("WirelessSignal", "WirelessSignalSettingsTabTitle", "Wireless Signal")) // 指定table title 名称
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				// Build 按钮控件 -------------------------------------------------------
				// 开始构建按钮(仅在子系统非运行情况下显示,绑定 OnStartClicked 事件)
				// Start Build
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
				// 保存和停止构建按钮(仅在系统运行时且在系统处于只烘焙可见区域时显示, 绑定 OnSaveAndStopClicked)
				// Save and Stop Building
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
				// 取消构建按钮(仅在系统运行时显示, 绑定OnCancelClicked)
				// Cancel Build
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
				
				// 实时模式checkbox控件 -------------------------------------------------------
				// 是否开启实时模式 checkbox
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
				// 是否开启实时模式 textblock
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
			// 系统状态说明 ----------------------------------------------------------------------
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

					// Ready, BWYS, 准备完成，光照将会基于以只烘培可见区域重构
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

void FWirelessSignalEditorModule::UpdateSettingsTab()
{
	if (SettingsView.IsValid())
	{
		SettingsView->ForceRefresh();
	}
}

bool FWirelessSignalEditorModule::IsBakeWhatYouSeeMode()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (UWirelessSignalSubsystem* LMSubsystem = World->GetSubsystem<UWirelessSignalSubsystem>())
		{
			return LMSubsystem->GetSettings()->Mode == EWirelessSignalMode::BakeWhatYouSee;
		}
	}

	return false;
}

bool FWirelessSignalEditorModule::IsRealtimeOn() 
{
	return GCurrentLevelEditingViewportClient && GCurrentLevelEditingViewportClient->IsRealtime();
}

bool FWirelessSignalEditorModule::IsRunning() 
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		return World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning();
	}
	return false;
}


/* ---OnStartClicked()---------------------------------------------------------------------
 *	开始按钮点击事件
 */
FReply FWirelessSignalEditorModule::OnStartClicked()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		// 在子系统非运行状态下执行
		if (!World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			// 若处于只烘培可见区域 且 非实时模式下 且 GCurrentLevelEditingViewportClient非空 情况下
			if (IsBakeWhatYouSeeMode() && !IsRealtimeOn() && GCurrentLevelEditingViewportClient != nullptr)
			{
				// 将模式设置为实时
				GCurrentLevelEditingViewportClient->SetRealtime(true);
			}
			// 启动子系统
			World->GetSubsystem<UWirelessSignalSubsystem>()->Launch();
			// 绑定 UpdateSettingsTab 到 OnLightBuildEnded 事件
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().AddRaw(this, &FWirelessSignalEditorModule::UpdateSettingsTab);
		}
	}
	// 更新 SettingsTab
	UpdateSettingsTab();

	return FReply::Handled();
}

/* ---OnSaveAndStopClicked()---------------------------------------------------------------------
 * 保存并结束单击事件
 */
FReply FWirelessSignalEditorModule::OnSaveAndStopClicked()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		// 若子系统处于运行状态
		if (World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			// 保存，停止，并取消所有绑定在 OnLightBuildEnded 上的事件
			World->GetSubsystem<UWirelessSignalSubsystem>()->Save();
			World->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
		}
	}
	// 更新 SettingsTab
	UpdateSettingsTab();

	return FReply::Handled();
}

/* ---OnCancelClicked()---------------------------------------------------------------------
 * 取消单击事件
 */
FReply FWirelessSignalEditorModule::OnCancelClicked()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		// 若系统正在运行
		if (World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			// 停止，并取消所有绑定在 OnLightBuildEnded 上的事件
			World->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
		}
	}
	// 更新 SettingsTab
	UpdateSettingsTab();

	return FReply::Handled();
}

/** ---OnMapChanged-------------------------------------------------------------------------
 * 地图变更事件 
 */
void FWirelessSignalEditorModule::OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	// 若 SettingsView 有效
	if (SettingsView.IsValid())
	{
		// 若子系统存在
		if (World->GetSubsystem<UWirelessSignalSubsystem>())
		{
			// 绑定子系统配置到 SettingsView
			SettingsView->SetObject(World->GetSubsystem<UWirelessSignalSubsystem>()->GetSettings(), true);
			// 若加载或新建地图
			if (MapChangeType == EMapChangeType::LoadMap || MapChangeType == EMapChangeType::NewMap)
			{
				// 将 UpdateSettingsTab 绑定在 OnLightBuildEnded 事件
				World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().AddRaw(this, &FWirelessSignalEditorModule::UpdateSettingsTab);
			}
			// 若销毁地图
			else if (MapChangeType == EMapChangeType::TearDownWorld)
			{
				// 取消所有 OnLightBuildEnded 绑定事件
				World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
			}
		}
	}
}

/** ---OnExtendLevelEditorBuildMenu-------------------------------------------------------------------------
 * 层编辑器 Build Menu 扩展事件 
 */
TSharedRef<FExtender> FWirelessSignalEditorModule::OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList)
{
	TSharedRef<FExtender> Extender(new FExtender());
	// 将插件加入 LevelEditorLighting 子目录下
	Extender->AddMenuExtension("LevelEditorLighting", EExtensionHook::First, nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FWirelessSignalEditorModule::CreateBuildMenu));

	return Extender;
}

/** ---CreateBuildMenu-------------------------------------------------------------------------
 * 创建 Build Menu
 */
void FWirelessSignalEditorModule::CreateBuildMenu(FMenuBuilder& Builder)
{
	FUIAction ActionOpenWirelessSignalSettingsTab(FExecuteAction::CreateLambda([]() {
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
		LevelEditorTabManager->TryInvokeTab(WirelessSignalSettingsTabName);
	}), FCanExecuteAction());

	Builder.AddMenuEntry(LOCTEXT("WirelessSignalSettingsTitle", "Wireless Signal"),
		LOCTEXT("OpensWirelessSignalSettings", "Opens Wireless Signal settings tab."), 
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.LightingScenarioIcon16x"), 
		ActionOpenWirelessSignalSettingsTab,
		NAME_None, EUserInterfaceActionType::Button);
}

#undef LOCTEXT_NAMESPACE
