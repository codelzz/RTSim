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

void FWirelessSignalEditorModule::StartupModule()
{
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

	LevelEditorTabManager->RegisterTabSpawner(WirelessSignalSettingsTabName, FOnSpawnTab::CreateRaw(this, &FWirelessSignalEditorModule::SpawnSettingsTab))
		.SetDisplayName(LOCTEXT("WirelessSignalSettingsTitle", "GPU Lightmass"))
		//.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.LightingScenarioIcon16x"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

TSharedRef<SDockTab> FWirelessSignalEditorModule::SpawnSettingsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropPlugin = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, true, FDetailsViewArgs::HideNameArea, false, GUnrealEd);
	DetailsViewArgs.bShowActorLabel = false;

	SettingsView = PropPlugin.CreateDetailView(DetailsViewArgs);

	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (World->GetSubsystem<UWirelessSignalSubsystem>())
		{
			SettingsView->SetObject(World->GetSubsystem<UWirelessSignalSubsystem>()->GetSettings());
		}
	}

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("Level.LightingScenarioIcon16x"))
		.Label(NSLOCTEXT("WirelessSignal", "WirelessSignalSettingsTabTitle", "GPU Lightmass"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[

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
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.f, 4.f)
			[
				SAssignNew(Messages, STextBlock)
				.AutoWrapText(true)
				.Text_Lambda( []() -> FText
				{

					bool bIsRayTracingEnabled = IsRayTracingEnabled();
					FText RTDisabledMsg = LOCTEXT("WirelessSignalRayTracingDisabled", "GPU Lightmass requires ray tracing support which is disabled.");
					if (!bIsRayTracingEnabled)
					{
						return LOCTEXT("WirelessSignalRayTracingDisabled", "GPU Lightmass requires ray tracing support which is disabled.");
					}

					// Ready
					static FText ReadyMsg = FText(LOCTEXT("WirelessSignalReady", "GPU Lightmass is ready."));

					// Ready, BWYS
					static FText BWYSReadyMsg = FText(LOCTEXT("WirelessSignalReadyBWYS", "GPU Lightmass is ready. Lighting will rebuild continuously in Bake What You See mode until saved or canceled."));

					// Ready, BWYS+RT OFF Warning 
					static FText RtOffBWYSWarningMsg = LOCTEXT("WirelessSignalSpeedReadyRTWarning", "Building Lighting when using Bake What You See Mode will automatically enable Viewport Realtime to start building. Lighting will rebuild continuously in Bake What You See mode until saved or canceled.");

					// Building FULL + RT Off Warning
					UWorld* World = GEditor->GetEditorWorldContext().World();
					FText BuildingMsg = FText::Format(LOCTEXT("WirelessSignalBuildingLighting", "GPU Lightmass is building lighting for {0}."), FText::FromString(World->GetActiveLightingScenario() ? World->GetActiveLightingScenario()->GetOuter()->GetName() : World->GetName()));

					// Building FULL + RT ON Warning 
					static FText BuildingFullRTOnMsg = LOCTEXT("WirelessSignalBuildingFullRTOn", "GPU Lightmass runs in slow mode when the viewport is realtime to avoid freezing. Uncheck Viewport Realtime to get full speed.");

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

FReply FWirelessSignalEditorModule::OnStartClicked()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (!World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			if (IsBakeWhatYouSeeMode() && !IsRealtimeOn() && GCurrentLevelEditingViewportClient != nullptr)
			{
				GCurrentLevelEditingViewportClient->SetRealtime(true);
			}

			World->GetSubsystem<UWirelessSignalSubsystem>()->Launch();
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().AddRaw(this, &FWirelessSignalEditorModule::UpdateSettingsTab);
		}
	}

	UpdateSettingsTab();

	return FReply::Handled();
}


FReply FWirelessSignalEditorModule::OnSaveAndStopClicked()
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			World->GetSubsystem<UWirelessSignalSubsystem>()->Save();
			World->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
		}
	}

	UpdateSettingsTab();

	return FReply::Handled();
}

FReply FWirelessSignalEditorModule::OnCancelClicked()
{

	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		if (World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			World->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
			World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
		}
	}
	
	UpdateSettingsTab();

	return FReply::Handled();
}

void FWirelessSignalEditorModule::OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (SettingsView.IsValid())
	{
		if (World->GetSubsystem<UWirelessSignalSubsystem>())
		{
			SettingsView->SetObject(World->GetSubsystem<UWirelessSignalSubsystem>()->GetSettings(), true);

			if (MapChangeType == EMapChangeType::LoadMap || MapChangeType == EMapChangeType::NewMap)
			{
				World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().AddRaw(this, &FWirelessSignalEditorModule::UpdateSettingsTab);
			}
			else if (MapChangeType == EMapChangeType::TearDownWorld)
			{
				World->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().RemoveAll(this);
			}
		}
	}
}

TSharedRef<FExtender> FWirelessSignalEditorModule::OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList)
{
	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension("LevelEditorLighting", EExtensionHook::First, nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FWirelessSignalEditorModule::CreateBuildMenu));

	return Extender;
}

void FWirelessSignalEditorModule::CreateBuildMenu(FMenuBuilder& Builder)
{
	FUIAction ActionOpenWirelessSignalSettingsTab(FExecuteAction::CreateLambda([]() {
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
		LevelEditorTabManager->TryInvokeTab(WirelessSignalSettingsTabName);
	}), FCanExecuteAction());

	Builder.AddMenuEntry(LOCTEXT("WirelessSignalSettingsTitle", "GPU Lightmass"),
		LOCTEXT("OpensWirelessSignalSettings", "Opens GPU Lightmass settings tab."), FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.LightingScenarioIcon16x"), ActionOpenWirelessSignalSettingsTab,
		NAME_None, EUserInterfaceActionType::Button);
}

#undef LOCTEXT_NAMESPACE
