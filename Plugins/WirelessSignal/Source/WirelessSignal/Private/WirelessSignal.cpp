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
	// ����Ƿ�����Ϸ�߳�
	check(IsInGameThread());
	// ���Settings�Ƿ���Ч
	check(Settings);

	// ��װ��Ϸ�߳��¼�����
	InstallGameThreadEventHooks();

	// ���� Settings ����������
	SettingsGuard = MakeUnique<FGCObjectScopeGuard>(Settings);

	// ��ʼ Wireless Signal ���� "process" ֪ͨ
	FNotificationInfo Info(LOCTEXT("LightBuildMessage", "Building lighting"));
	Info.bFireAndForget = false;

	// [ע��] �������Ǻ���ֻ�決�ɼ������߼�
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

/** ��װ��Ϸ�߳��¼����� */
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

/** �Ƴ���Ϸ�߳��¼�����*/
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

/** ��Ԫ���ע�� */
void FWirelessSignal::OnPrimitiveComponentRegistered(UPrimitiveComponent* InComponent)
{
	if (InComponent->GetWorld() != World) return;

	if (!InComponent->IsRegistered()) return;
	if (!InComponent->IsVisible()) return;

	check(InComponent->HasValidSettingsForStaticLighting(false));
	
	// ���ݲ�ͬ�ļ���������� LandscapeComponent/UInstancedStaticMeshComponent/UStaticMeshComponent 
	// ������������볡��
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

/** ��Ԫ���ע�� */
void FWirelessSignal::OnPrimitiveComponentUnregistered(UPrimitiveComponent* InComponent)
{
	if (InComponent->GetWorld() != World) return;
	
	// ���ݲ�ͬ�ļ���������� LandscapeComponent/UInstancedStaticMeshComponent/UStaticMeshComponent
	// ����������ӳ������Ƴ�
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

/** ��Դ���ע�� */
void FWirelessSignal::OnLightComponentRegistered(ULightComponentBase* InComponent)
{
	if (InComponent->GetWorld() != World) return;

	if (!InComponent->IsRegistered()) return;
	if (!InComponent->IsVisible()) return;

	// ���ݲ�ͬ��Դ������� DirectionalLight/RectLight/SpotLight/PointLight/SkyLight
	// ����Դ������볡��
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

/** ��Դ���ע�� */
void FWirelessSignal::OnLightComponentUnregistered(ULightComponentBase* InComponent)
{
	if (InComponent->GetWorld() != World) return;

	UE_LOG(LogWirelessSignal, Display, TEXT("LightComponent %s on actor %s is being unregistered with Wireless Signal"), *InComponent->GetName(), *InComponent->GetOwner()->GetName());
	
	// ���ݲ�ͬ��Դ������� DirectionalLight/RectLight/SpotLight/PointLight/SkyLight
	// ����Դ�ӳ������Ƴ�
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

/** ��̬��ͨ���ط��� */
void FWirelessSignal::OnStationaryLightChannelReassigned(ULightComponentBase* InComponent, int32 NewShadowMapChannel)
{
	if (InComponent->GetWorld() != World) return;

	// ���ݲ�ͬ��Դ������� DirectionalLight/RectLight/SpotLight/PointLight/SkyLight
	// ���½���Դ���볡��
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

/** Ԥ����������� */
void FWirelessSignal::OnPreWorldFinishDestroy(UWorld* InWorld)
{
	UE_LOG(LogWirelessSignal, Display, TEXT("World %s is being destroyed"), *World->GetName());

	if (InWorld != World) return;

	UE_LOG(LogWirelessSignal, Display, TEXT("Removing all delegates (including this one)"), *World->GetName());

	// ���� FWirelessSignal �⹹��
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
