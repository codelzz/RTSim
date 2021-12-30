// Copyright Epic Games, Inc. All Rights Reserved.

#include "WirelessSignalSettings.h"
#include "EngineUtils.h"
#include "WirelessSignalModule.h"
#include "ComponentRecreateRenderStateContext.h"
#include "Misc/ScopedSlowTask.h"
#include "LandscapeComponent.h"
#include "WirelessSignal.h"
#include "Editor.h"
#include "LevelEditorViewport.h"

#define LOCTEXT_NAMESPACE "StaticLightingSystem"

int32 GWirelessSignalShowProgressBars = 1;
static FAutoConsoleVariableRef CVarWirelessSignalShowProgressBars(
	TEXT("r.WirelessSignal.ShowProgressBars"),
	GWirelessSignalShowProgressBars,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalOnlyBakeWhatYouSee = 0;
static FAutoConsoleVariableRef CVarWirelessSignalOnlyBakeWhatYouSee(
	TEXT("r.WirelessSignal.OnlyBakeWhatYouSee"),
	GWirelessSignalOnlyBakeWhatYouSee,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalSamplesPerTexel = 512;
static FAutoConsoleVariableRef CVarWirelessSignalSamplesPerTexel(
	TEXT("r.WirelessSignal.SamplesPerTexel"),
	GWirelessSignalSamplesPerTexel,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalShadowSamplesPerTexel = 512; // 512 samples to reach good image plane stratification. Shadow samples are 100x faster than path samples
static FAutoConsoleVariableRef CVarWirelessSignalShadowSamplesPerTexel(
	TEXT("r.WirelessSignal.ShadowSamplesPerTexel"),
	GWirelessSignalShadowSamplesPerTexel,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalUseIrradianceCaching = 0;
static FAutoConsoleVariableRef CVarWirelessSignalUseIrradianceCaching(
	TEXT("r.WirelessSignal.IrradianceCaching"),
	GWirelessSignalUseIrradianceCaching,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalIrradianceCachingQuality = 128;
static FAutoConsoleVariableRef CVarWirelessSignalIrradianceCachingQuality(
	TEXT("r.WirelessSignal.IrradianceCaching.Quality"),
	GWirelessSignalIrradianceCachingQuality,
	TEXT("\n"),
	ECVF_Default
);

float GWirelessSignalIrradianceCachingSpacing = 32.0f;
static FAutoConsoleVariableRef CVarWirelessSignalIrradianceCachingSpacing(
	TEXT("r.WirelessSignal.IrradianceCaching.Spacing"),
	GWirelessSignalIrradianceCachingSpacing,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalVisualizeIrradianceCache = 0;
static FAutoConsoleVariableRef CVarWirelessSignalVisualizeIrradianceCache(
	TEXT("r.WirelessSignal.IrradianceCaching.Visualize"),
	GWirelessSignalVisualizeIrradianceCache,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalUseFirstBounceRayGuiding = 0;
static FAutoConsoleVariableRef CVarWirelessSignalUseFirstBounceRayGuiding(
	TEXT("r.WirelessSignal.FirstBounceRayGuiding"),
	GWirelessSignalUseFirstBounceRayGuiding,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalFirstBounceRayGuidingTrialSamples = 128;
static FAutoConsoleVariableRef CVarWirelessSignalFirstBounceRayGuidingTrialSamples(
	TEXT("r.WirelessSignal.FirstBounceRayGuiding.TrialSamples"),
	GWirelessSignalFirstBounceRayGuidingTrialSamples,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalGPUTilePoolSize = 40;
static FAutoConsoleVariableRef CVarWirelessSignalGPUTilePoolSize(
	TEXT("r.WirelessSignal.System.GPUTilePoolSize"),
	GWirelessSignalGPUTilePoolSize,
	TEXT("\n"),
	ECVF_Default
);

int32 GWirelessSignalDenoiseGIOnCompletion = WITH_INTELOIDN;
#if WITH_INTELOIDN
static FAutoConsoleVariableRef CVarGWirelessSignalDenoiseOnCompletion(
	TEXT("r.WirelessSignal.DenoiseGIOnCompletion"),
	GWirelessSignalDenoiseGIOnCompletion,
	TEXT("\n"),
	ECVF_Default
);
#endif

int32 GWirelessSignalDenoiseGIDuringInteractiveBake = 0;
#if WITH_INTELOIDN
static FAutoConsoleVariableRef CVarGWirelessSignalDenoiseGIDuringInteractiveBake(
	TEXT("r.WirelessSignal.DenoiseGIDuringInteractiveBake"),
	GWirelessSignalDenoiseGIDuringInteractiveBake,
	TEXT("\n"),
	ECVF_Default
);
#endif

AWirelessSignalSettingsActor::AWirelessSignalSettingsActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("Sprite")))
{
#if WITH_EDITORONLY_DATA
	bActorLabelEditable = false;
#endif // WITH_EDITORONLY_DATA
	bIsEditorOnlyActor = true;

	Settings = ObjectInitializer.CreateDefaultSubobject<UWirelessSignalSettings>(this, TEXT("WirelessSignalSettings"));
}

void UWirelessSignalSettings::ApplyImmediateSettingsToRunningInstances()
{
	// Replicate value to any running instances
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));
		FWirelessSignal* WirelessSignal = (FWirelessSignal*)WirelessSignalModule.GetStaticLightingSystemForWorld(World);
		if (WirelessSignal)
		{
			WirelessSignal->Settings->bShowProgressBars = bShowProgressBars;
			WirelessSignal->Settings->TilePassesInSlowMode = TilePassesInSlowMode;
			WirelessSignal->Settings->TilePassesInFullSpeedMode = TilePassesInFullSpeedMode;
			WirelessSignal->Settings->bVisualizeIrradianceCache = bVisualizeIrradianceCache;
		}
	}
}

#if WITH_EDITOR
void UWirelessSignalSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged)
	{
		FName PropertyName = PropertyThatChanged->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, bUseIrradianceCaching))
		{
			if (!bUseIrradianceCaching)
			{
				bUseFirstBounceRayGuiding = false;
			}
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, bShowProgressBars))
		{
			ApplyImmediateSettingsToRunningInstances();
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, TilePassesInSlowMode))
		{
			ApplyImmediateSettingsToRunningInstances();
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, TilePassesInFullSpeedMode))
		{
			ApplyImmediateSettingsToRunningInstances();
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, bVisualizeIrradianceCache))
		{
			ApplyImmediateSettingsToRunningInstances();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UWirelessSignalSettings::CanEditChange(const FProperty* InProperty) const
{
	FName PropertyName = InProperty->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, bShowProgressBars))
	{
		return true;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, TilePassesInSlowMode))
	{
		return true;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, TilePassesInFullSpeedMode))
	{
		return true;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWirelessSignalSettings, bVisualizeIrradianceCache))
	{
		return true;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		if (World->GetSubsystem<UWirelessSignalSubsystem>()->IsRunning())
		{
			return false;
		}
	}

	return true;
}
#endif

void UWirelessSignalSettings::GatherSettingsFromCVars()
{
	bShowProgressBars = GWirelessSignalShowProgressBars == 1;

	if (GWirelessSignalOnlyBakeWhatYouSee == 1)
	{
		Mode = EWirelessSignalMode::BakeWhatYouSee;
	}

	GISamples = GWirelessSignalSamplesPerTexel;
	StationaryLightShadowSamples = GWirelessSignalSamplesPerTexel;
	
	bUseIrradianceCaching = GWirelessSignalUseIrradianceCaching == 1;
	IrradianceCacheQuality = GWirelessSignalIrradianceCachingQuality;
	IrradianceCacheSpacing = GWirelessSignalIrradianceCachingSpacing;
	bVisualizeIrradianceCache = GWirelessSignalVisualizeIrradianceCache == 1;

	bUseFirstBounceRayGuiding = GWirelessSignalUseFirstBounceRayGuiding == 1;
	FirstBounceRayGuidingTrialSamples = GWirelessSignalFirstBounceRayGuidingTrialSamples;

	if (GWirelessSignalDenoiseGIOnCompletion == 1)
	{
		DenoisingOptions = EWirelessSignalDenoisingOptions::OnCompletion;
	}

	if (GWirelessSignalDenoiseGIDuringInteractiveBake == 1)
	{
		// Override
		DenoisingOptions = EWirelessSignalDenoisingOptions::DuringInteractivePreview;
	}

	LightmapTilePoolSize = GWirelessSignalGPUTilePoolSize;
}

void UWirelessSignalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorld* World = Cast<UWorld>(GetOuter());
	
	if (!World) return;

	AWirelessSignalSettingsActor* SettingsActor = GetSettingsActor();

	if (SettingsActor == nullptr)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnInfo.Name = AWirelessSignalSettingsActor::StaticClass()->GetFName();
		SpawnInfo.bHideFromSceneOutliner = true;
		SettingsActor = World->SpawnActor<AWirelessSignalSettingsActor>(AWirelessSignalSettingsActor::StaticClass(), SpawnInfo);
	}

	if (SettingsActor == nullptr)
	{
		UE_LOG(LogWirelessSignal, Warning, TEXT("Failed to spawn settings actor in World: $s"), *World->GetName());
	}
}

/** 启动 */
void UWirelessSignalSubsystem::Launch()
{
	// 获取 World 指针
	UWorld* World = Cast<UWorld>(GetOuter());
	if (!World) return;

	// 加载无线信号模块
	FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));

	// 若 Module 不存在当前 World 对应的的静态光照系统
	if (!WirelessSignalModule.GetStaticLightingSystemForWorld(World))
	{
		UWirelessSignalSettings* SettingsCopy = DuplicateObject(GetSettings(), GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UWirelessSignalSettings::StaticClass()));

		FScopedSlowTask SlowTask(1);
		SlowTask.MakeDialog();
		SlowTask.EnterProgressFrame(1, LOCTEXT("StartingStaticLightingSystem", "Starting static lighting system"));

		{
			FGlobalComponentRecreateRenderStateContext RecreateRenderStateContext; // Implicit FlushRenderingCommands();

			FlushRenderingCommands(); // Flush again to execute commands generated by DestroyRenderState_Concurrent()
			
			// 分配一个静态光照系统
			IStaticLightingSystem* StaticLightingSystem = WirelessSignalModule.AllocateStaticLightingSystemForWorldWithSettings(World, SettingsCopy);

			// 若成功分配
			if (StaticLightingSystem)
			{
				UE_LOG(LogTemp, Log, TEXT("Static lighting system is created for world %s."), *World->GetPathName(World->GetOuter()));
				
				// 重新分配静态光通道
				ULightComponent::ReassignStationaryLightChannels(World, false, NULL);
#if WITH_EDITOR
				if (!GIsEditor)
				{
					if (GEngine)
					{
						GEngine->OnPostEditorTick().AddStatic(&FStaticLightingSystemInterface::GameTick);
					}
				}
#endif // WITH_EDITOR

				int32 NumPrimitiveComponents = 0;
				int32 NumLightComponents = 0;
				
				// 遍历系统基元组件
				for (UPrimitiveComponent* Component : TObjectRange<UPrimitiveComponent>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
				{
					if (Component->HasValidSettingsForStaticLighting(false))
					{
						NumPrimitiveComponents++;
					}
				}
				
				// 遍历系统光源组件
				for (ULightComponentBase* Component : TObjectRange<ULightComponentBase>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
				{
					if (Component->bAffectsWorld && Component->HasStaticShadowing())
					{
						NumLightComponents++;
					}
				}

				FScopedSlowTask SubSlowTask(NumPrimitiveComponents + NumLightComponents, LOCTEXT("RegisteringComponentsWithStaticLightingSystem", "Registering components with static lighting system"));
				SubSlowTask.MakeDialog();

				for (UPrimitiveComponent* Component : TObjectRange<UPrimitiveComponent>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
				{
					if (Component->HasValidSettingsForStaticLighting(false))
					{
						FStaticLightingSystemInterface::OnPrimitiveComponentRegistered.Broadcast(Component);

						SubSlowTask.EnterProgressFrame(1, LOCTEXT("RegisteringComponentsWithStaticLightingSystem", "Registering components with static lighting system"));
					}
				}

				for (ULightComponentBase* Component : TObjectRange<ULightComponentBase>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
				{
					if (Component->bAffectsWorld && Component->HasStaticShadowing())
					{
						FStaticLightingSystemInterface::OnLightComponentRegistered.Broadcast(Component);

						SubSlowTask.EnterProgressFrame(1, LOCTEXT("RegisteringComponentsWithStaticLightingSystem", "Registering components with static lighting system"));
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Tried to create static lighting system for world %s, but failed"), *World->GetPathName(World->GetOuter()));
			}
		}

		FlushRenderingCommands(); // Flush commands generated by ~FGlobalComponentRecreateRenderStateContext();
	}
}

void UWirelessSignalSubsystem::Stop()
{
	UWorld* World = Cast<UWorld>(GetOuter());

	if (!World) return;

	FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));

	if (WirelessSignalModule.GetStaticLightingSystemForWorld(World))
	{
		FScopedSlowTask SlowTask(1);
		SlowTask.MakeDialog();
		SlowTask.EnterProgressFrame(1, LOCTEXT("RemovingStaticLightingSystem", "Removing static lighting system"));

		{
			FGlobalComponentRecreateRenderStateContext RecreateRenderStateContext; // Implicit FlushRenderingCommands();

			FlushRenderingCommands(); // Flush again to execute commands generated by DestroyRenderState_Concurrent()

			int32 NumPrimitiveComponents = 0;
			int32 NumLightComponents = 0;

			for (UPrimitiveComponent* Component : TObjectRange<UPrimitiveComponent>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
			{
				NumPrimitiveComponents++;
			}

			for (ULightComponentBase* Component : TObjectRange<ULightComponentBase>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
			{
				NumLightComponents++;
			}

			FScopedSlowTask SubSlowTask(NumPrimitiveComponents + NumLightComponents, LOCTEXT("UnregisteringComponentsWithStaticLightingSystem", "Unregistering components with static lighting system"));

			// Unregister all landscapes first to prevent grass picking up landscape lightmaps
			for (ULandscapeComponent* Component : TObjectRange<ULandscapeComponent>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
			{
				FStaticLightingSystemInterface::OnPrimitiveComponentUnregistered.Broadcast(Component);
			}

			for (UPrimitiveComponent* Component : TObjectRange<UPrimitiveComponent>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
			{
				FStaticLightingSystemInterface::OnPrimitiveComponentUnregistered.Broadcast(Component);

				SubSlowTask.EnterProgressFrame(1, LOCTEXT("UnregisteringComponentsWithStaticLightingSystem", "Unregistering components with static lighting system"));
			}

			for (ULightComponentBase* Component : TObjectRange<ULightComponentBase>(RF_ClassDefaultObject | RF_ArchetypeObject, true, EInternalObjectFlags::PendingKill))
			{
				FStaticLightingSystemInterface::OnLightComponentUnregistered.Broadcast(Component);

				SubSlowTask.EnterProgressFrame(1, LOCTEXT("UnregisteringComponentsWithStaticLightingSystem", "Unregistering components with static lighting system"));
			}

			WirelessSignalModule.RemoveStaticLightingSystemForWorld(World);

			UE_LOG(LogTemp, Log, TEXT("Static lighting system is removed for world %s."), *World->GetPathName(World->GetOuter()));
		}

		FlushRenderingCommands(); // Flush commands generated by ~FGlobalComponentRecreateRenderStateContext();
	}

	// Always turn Realtime on after building lighting
	SetRealtime(true);
}

bool UWirelessSignalSubsystem::IsRunning()
{
	UWorld* World = Cast<UWorld>(GetOuter());

	if (!World) return false;

	FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));

	return WirelessSignalModule.GetStaticLightingSystemForWorld(World) != nullptr;
}

AWirelessSignalSettingsActor* UWirelessSignalSubsystem::GetSettingsActor()
{
	UWorld* World = Cast<UWorld>(GetOuter());

	if (!World) return nullptr;

	AWirelessSignalSettingsActor* SettingsActor = nullptr;

	for (TActorIterator<AWirelessSignalSettingsActor> It(World, AWirelessSignalSettingsActor::StaticClass(), EActorIteratorFlags::SkipPendingKill); It; ++It)
	{
		SettingsActor = *It;
		break;
	}

	return SettingsActor;
}

UWirelessSignalSettings* UWirelessSignalSubsystem::GetSettings()
{
	return GetSettingsActor() ? GetSettingsActor()->Settings : nullptr;
}

void UWirelessSignalSubsystem::StartRecordingVisibleTiles()
{
	// Replicate value to any running instances
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));
		FWirelessSignal* WirelessSignal = (FWirelessSignal*)WirelessSignalModule.GetStaticLightingSystemForWorld(World);
		if (WirelessSignal)
		{
			WirelessSignal->StartRecordingVisibleTiles();
		}
	}
}

void UWirelessSignalSubsystem::EndRecordingVisibleTiles()
{
	// Replicate value to any running instances
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));
		FWirelessSignal* WirelessSignal = (FWirelessSignal*)WirelessSignalModule.GetStaticLightingSystemForWorld(World);
		if (WirelessSignal)
		{
			WirelessSignal->EndRecordingVisibleTiles();
		}
	}
}

int32 UWirelessSignalSubsystem::GetPercentage()
{
	UWorld* World = Cast<UWorld>(GetOuter());

	if (!World) return 0;

	FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));

	if (WirelessSignalModule.StaticLightingSystems.Find(World) != nullptr)
	{
		FWirelessSignal* WirelessSignal = WirelessSignalModule.StaticLightingSystems[World];
		return WirelessSignal->LightBuildPercentage;
	}

	return 0;
}

void UWirelessSignalSubsystem::SetRealtime(bool bInRealtime)
{
	if (GCurrentLevelEditingViewportClient != nullptr)
	{
		GCurrentLevelEditingViewportClient->SetRealtime(bInRealtime);
	}
	else
	{
		UE_LOG(LogWirelessSignal, Warning, TEXT("CurrentLevelEditingViewportClient is NULL!"));
	}
}

void UWirelessSignalSubsystem::Save()
{
	UWorld* World = Cast<UWorld>(GetOuter());

	if (!World) return;

	FWirelessSignalModule& WirelessSignalModule = FModuleManager::LoadModuleChecked<FWirelessSignalModule>(TEXT("WirelessSignal"));

	if (WirelessSignalModule.StaticLightingSystems.Find(World) != nullptr)
	{
		FWirelessSignal* WirelessSignal = WirelessSignalModule.StaticLightingSystems[World];
		WirelessSignal->Scene.ApplyFinishedLightmapsToWorld();;
	}
}

#undef LOCTEXT_NAMESPACE
