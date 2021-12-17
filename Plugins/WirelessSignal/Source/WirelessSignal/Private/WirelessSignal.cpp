#include "WirelessSignal.h"
#include "ComponentRecreateRenderStateContext.h"
#include "EngineModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Components/SkyLightComponent.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Async/Async.h"
// #include "LightmapRenderer.h"
#include "WirelessSignalSettings.h"

#define LOCTEXT_NAMESPACE "StaticLightingSystem"

extern ENGINE_API void ToggleLightmapPreview_GameThread(UWorld* InWorld);

FWirelessSignal::FWirelessSignal(UWorld* InWorld, FWirelessSignalModule* InWirelessSignalModule, UWirelessSignalSettings* InSettings)
	: World(InWorld)
	, WirelessSignalModule(InWirelessSignalModule)
	, Settings(InSettings)
	, Scene(this)
	, LightBuildPercentage(0)
{
	// 检查是否处于游戏线程
	check(IsInGameThread());
	// 检查Settings是否有效
	check(Settings);

	// 安装游戏线程事件钩子
	InstallGameThreadEventHooks();

	// 避免 Settings 被意外销毁
	SettingsGuard = MakeUnique<FGCObjectScopeGuard>(Settings);

	// 开始 Wireless Signal 构建 "process" 通知
	FNotificationInfo Info(LOCTEXT("LightBuildMessage", "Building lighting"));
	Info.bFireAndForget = false;

	// [注意] 这里我们忽略只烘焙可见区域逻辑
	/*
	if (InSettings->Mode == EWirelessSignalMode::BakeWhatYouSee)
	{
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			LOCTEXT("Save", "Save and Stop"),
			FText::GetEmpty(),
			FSimpleDelegate::CreateLambda([InWorld, this]() {
				this->Scene.ApplyFinishedLightmapsToWorld();
				InWorld->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
				})));
	}
	*/

	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("LightBuildCancel", "Cancel"),
		LOCTEXT("LightBuildCancelToolTip", "Cancels the lighting build in progress."),
		FSimpleDelegate::CreateLambda([InWorld]() {
			InWorld->GetSubsystem<UWirelessSignalSubsystem>()->Stop();
			InWorld->GetSubsystem<UWirelessSignalSubsystem>()->OnLightBuildEnded().Broadcast();
			})));

	LightBuildNotification = FSlateNotificationManager::Get().AddNotification(Info);
	if (LightBuildNotification.IsValid())
	{
		LightBuildNotification->SetCompletionState(SNotificationItem::CS_Pending);
	}

	StartTime = FPlatformTime::Seconds();
}

void FWirelessSignal::GameThreadDestroy()
{
	check(IsInGameThread());

	UE_LOG(LogWirelessSignal, Log, TEXT("Total lighting time: %s"), *FPlatformTime::PrettyTime(FPlatformTime::Seconds() - StartTime));

	RemoveGameThreadEventHooks();

	if (!IsEngineExitRequested() && LightBuildNotification.IsValid())
	{
		FText CompletedText = LOCTEXT("LightBuildDoneMessage", "Lighting build completed");
		LightBuildNotification->SetText(CompletedText);
		LightBuildNotification->SetCompletionState(SNotificationItem::CS_Success);
		LightBuildNotification->ExpireAndFadeout();
	}

	Scene.RemoveAllComponents();
}

FWirelessSignal::~FWirelessSignal()
{
	// RenderThreadDestroy
	check(IsInRenderingThread());
}

/** 安装游戏线程事件钩子 */
void FWirelessSignal::InstallGameThreadEventHooks()
{	
	FWorldDelegates::OnPreWorldFinishDestroy.AddRaw(this, &FWirelessSignal::OnPreWorldFinishDestroy);

	FStaticLightingSystemInterface::OnPrimitiveComponentRegistered.AddRaw(this, &FWirelessSignal::OnPrimitiveComponentRegistered);
	FStaticLightingSystemInterface::OnPrimitiveComponentUnregistered.AddRaw(this, &FWirelessSignal::OnPrimitiveComponentUnregistered);
	FStaticLightingSystemInterface::OnLightComponentRegistered.AddRaw(this, &FWirelessSignal::OnLightComponentRegistered);
	FStaticLightingSystemInterface::OnLightComponentUnregistered.AddRaw(this, &FWirelessSignal::OnLightComponentUnregistered);
	FStaticLightingSystemInterface::OnStationaryLightChannelReassigned.AddRaw(this, &FWirelessSignal::OnStationaryLightChannelReassigned);
	FStaticLightingSystemInterface::OnLightmassImportanceVolumeModified.AddRaw(this, &FWirelessSignal::OnWirelessSignalImportanceVolumeModified);
	FStaticLightingSystemInterface::OnMaterialInvalidated.AddRaw(this, &FWirelessSignal::OnMaterialInvalidated);
}

/** 移除游戏线程事件钩子*/
void FWirelessSignal::RemoveGameThreadEventHooks()
{
	FWorldDelegates::OnPreWorldFinishDestroy.RemoveAll(this);

	FStaticLightingSystemInterface::OnPrimitiveComponentRegistered.RemoveAll(this);
	FStaticLightingSystemInterface::OnPrimitiveComponentUnregistered.RemoveAll(this);
	FStaticLightingSystemInterface::OnLightComponentRegistered.RemoveAll(this);
	FStaticLightingSystemInterface::OnLightComponentUnregistered.RemoveAll(this);
	FStaticLightingSystemInterface::OnStationaryLightChannelReassigned.RemoveAll(this);
	FStaticLightingSystemInterface::OnLightmassImportanceVolumeModified.RemoveAll(this);
	FStaticLightingSystemInterface::OnMaterialInvalidated.RemoveAll(this);
}

/** 基元组件注册 */
void FWirelessSignal::OnPrimitiveComponentRegistered(UPrimitiveComponent* InComponent)
{
	if (InComponent->GetWorld() != World) return;

	if (!InComponent->IsRegistered()) return;
	if (!InComponent->IsVisible()) return;

	check(InComponent->HasValidSettingsForStaticLighting(false));
	
	// 根据不同的几何组件类型 LandscapeComponent/UInstancedStaticMeshComponent/UStaticMeshComponent 
	// 将几何组件加入场景
	/*
	if (ULandscapeComponent* LandscapeComponent = Cast<ULandscapeComponent>(InComponent))
	{
		Scene.AddGeometryInstanceFromComponent(LandscapeComponent);
	}
	else if (UInstancedStaticMeshComponent* InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(InComponent))
	{
		Scene.AddGeometryInstanceFromComponent(InstancedStaticMeshComponent);
	}
	else if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(InComponent))
	{
		Scene.AddGeometryInstanceFromComponent(StaticMeshComponent);
	}
	*/
}

/** 基元组件注销 */
void FWirelessSignal::OnPrimitiveComponentUnregistered(UPrimitiveComponent* InComponent)
{
	if (InComponent->GetWorld() != World) return;
	
	// 根据不同的几何组件类型 LandscapeComponent/UInstancedStaticMeshComponent/UStaticMeshComponent
	// 将几何组件从场景中移除
	/*
	if (ULandscapeComponent* LandscapeComponent = Cast<ULandscapeComponent>(InComponent))
	{
		Scene.RemoveGeometryInstanceFromComponent(LandscapeComponent);
	}
	if (UInstancedStaticMeshComponent* InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(InComponent))
	{
		Scene.RemoveGeometryInstanceFromComponent(InstancedStaticMeshComponent);
	}
	else if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(InComponent))
	{
		Scene.RemoveGeometryInstanceFromComponent(StaticMeshComponent);
	}
	*/
}

/** 光源组件注册 */
void FWirelessSignal::OnLightComponentRegistered(ULightComponentBase* InComponent)
{
	if (InComponent->GetWorld() != World) return;

	if (!InComponent->IsRegistered()) return;
	if (!InComponent->IsVisible()) return;

	// 根据不同光源组件类型 DirectionalLight/RectLight/SpotLight/PointLight/SkyLight
	// 将光源组件加入场景
	/*
	if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(InComponent))
	{
		Scene.AddLight(DirectionalLight);
	}
	else if (URectLightComponent* RectLight = Cast<URectLightComponent>(InComponent))
	{
		Scene.AddLight(RectLight);
	}
	else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(InComponent))
	{
		Scene.AddLight(SpotLight);
	}
	else if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(InComponent))
	{
		Scene.AddLight(PointLight);
	}
	else if (USkyLightComponent* SkyLight = Cast<USkyLightComponent>(InComponent))
	{
		Scene.AddLight(SkyLight);
	}
	*/
}

/** 光源组件注销 */
void FWirelessSignal::OnLightComponentUnregistered(ULightComponentBase* InComponent)
{
	if (InComponent->GetWorld() != World) return;

	UE_LOG(LogWirelessSignal, Display, TEXT("LightComponent %s on actor %s is being unregistered with Wireless Signal"), *InComponent->GetName(), *InComponent->GetOwner()->GetName());
	
	// 根据不同光源组件类型 DirectionalLight/RectLight/SpotLight/PointLight/SkyLight
	// 将光源从场景中移除
	/*
	if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(InComponent))
	{
		Scene.RemoveLight(DirectionalLight);
	}
	else if (URectLightComponent* RectLight = Cast<URectLightComponent>(InComponent))
	{
		Scene.RemoveLight(RectLight);
	}
	else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(InComponent))
	{
		Scene.RemoveLight(SpotLight);
	}
	else if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(InComponent))
	{
		Scene.RemoveLight(PointLight);
	}
	else if (USkyLightComponent* SkyLight = Cast<USkyLightComponent>(InComponent))
	{
		Scene.RemoveLight(SkyLight);
	}
	*/
}

/** 静态光通道重分配 */
void FWirelessSignal::OnStationaryLightChannelReassigned(ULightComponentBase* InComponent, int32 NewShadowMapChannel)
{
	if (InComponent->GetWorld() != World) return;

	// 根据不同光源组件类型 DirectionalLight/RectLight/SpotLight/PointLight/SkyLight
	// 重新将光源加入场景
	/*
	if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(InComponent))
	{
		if (Scene.HasLight(DirectionalLight))
		{
			Scene.RemoveLight(DirectionalLight);
			Scene.AddLight(DirectionalLight);
		}
	}
	else if (URectLightComponent* RectLight = Cast<URectLightComponent>(InComponent))
	{
		if (Scene.HasLight(RectLight))
		{
			Scene.RemoveLight(RectLight);
			Scene.AddLight(RectLight);
		}
	}
	else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(InComponent))
	{
		if (Scene.HasLight(SpotLight))
		{
			Scene.RemoveLight(SpotLight);
			Scene.AddLight(SpotLight);
		}
	}
	else if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(InComponent))
	{
		if (Scene.HasLight(PointLight))
		{
			Scene.RemoveLight(PointLight);
			Scene.AddLight(PointLight);
		}
	}
	*/
}

/** 预世界完成销毁 */
void FWirelessSignal::OnPreWorldFinishDestroy(UWorld* InWorld)
{
	UE_LOG(LogWirelessSignal, Display, TEXT("World %s is being destroyed"), *World->GetName());

	if (InWorld != World) return;

	UE_LOG(LogWirelessSignal, Display, TEXT("Removing all delegates (including this one)"), *World->GetName());

	// 调用 FWirelessSignal 解构器
	WirelessSignalModule->RemoveStaticLightingSystemForWorld(World);
}

void FWirelessSignal::EditorTick()
{
	Scene.BackgroundTick();
}

const FMeshMapBuildData* FWirelessSignal::GetPrimitiveMeshMapBuildData(const UPrimitiveComponent* Component, int32 LODIndex)
{
	if (Component->GetWorld() != World) return nullptr;

	// return Scene.GetComponentLightmapData(Component, LODIndex);
	return nullptr;
}

const FLightComponentMapBuildData* FWirelessSignal::GetLightComponentMapBuildData(const ULightComponent* Component)
{
	if (Component->GetWorld() != World) return nullptr;

	// return Scene.GetComponentLightmapData(Component);
	return nullptr;
}

const FPrecomputedVolumetricLightmap* FWirelessSignal::GetPrecomputedVolumetricLightmap()
{
	check(IsInRenderingThread() || IsInParallelRenderingThread());

	// return Scene.RenderState.VolumetricLightmapRenderer->GetPrecomputedVolumetricLightmapForPreview();
	return nullptr;
}

void FWirelessSignal::OnWirelessSignalImportanceVolumeModified()
{
	Scene.bNeedsVoxelization = true;
}

void FWirelessSignal::OnMaterialInvalidated(FMaterialRenderProxy* Material)
{
	/*
	if (Scene.RenderState.CachedRayTracingScene.IsValid())
	{
		Scene.RenderState.CachedRayTracingScene.Reset();
		UE_LOG(LogWirelessSignal, Log, TEXT("Cached ray tracing scene is invalidated due to material changes"));
	}
	*/
}

/*
void FWirelessSignal::StartRecordingVisibleTiles()
{
	ENQUEUE_RENDER_COMMAND(BackgroundTickRenderThread)([&RenderState = Scene.RenderState](FRHICommandListImmediate&) mutable {
		RenderState.LightmapRenderer->RecordedTileRequests.Empty();
		RenderState.LightmapRenderer->bIsRecordingTileRequests = true;
	});
}
void FWirelessSignal::EndRecordingVisibleTiles()
{
	ENQUEUE_RENDER_COMMAND(BackgroundTickRenderThread)([&RenderState = Scene.RenderState](FRHICommandListImmediate&) mutable {
		RenderState.LightmapRenderer->bIsRecordingTileRequests = false;
		RenderState.LightmapRenderer->DeduplicateRecordedTileRequests();
	});
}
*/

#undef LOCTEXT_NAMESPACE
