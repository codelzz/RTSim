// Copyright Epic Games, Inc. All Rights Reserved.

#include "Scene.h"
#include "WirelessSignal.h"
#include "ComponentRecreateRenderStateContext.h"
#include "Async/Async.h"
// #include "LightmapEncoding.h"
#include "WirelessSignalCommon.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/ScopedSlowTask.h"
// #include "LightmapPreviewVirtualTexture.h"
#include "EngineModule.h"
// #include "LightmapRenderer.h"
// #include "VolumetricLightmap.h"
// #include "GPUScene.h"
#include "RayTracingDynamicGeometryCollection.h"
//#include "Lightmass/LightmassImportanceVolume.h"
#include "Logging/MessageLog.h"
#include "Misc/ConfigCacheIni.h"
#include "LevelEditorViewport.h"
#include "Editor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
// #include "LightmapDenoising.h"
#include "Misc/FileHelper.h"
#include "Components/ReflectionCaptureComponent.h"

#define LOCTEXT_NAMESPACE "StaticLightingSystem"

// extern RENDERER_API void SetupSkyIrradianceEnvironmentMapConstantsFromSkyIrradiance(FVector4* OutSkyIrradianceEnvironmentMap, const FSHVectorRGB3 SkyIrradiance);
// extern ENGINE_API bool GCompressLightmaps;

// extern float GetTerrainExpandPatchCount(float LightMapRes, int32& X, int32& Y, int32 ComponentSize, int32 LightmapSize, int32& DesiredSize, uint32 LightingLOD);

namespace WirelessSignal
{

/********************************************************************************************************************
 *
 *											场景类 FScene
 *
 ********************************************************************************************************************/

FScene::FScene(FWirelessSignal* InWirelessSignal)
	: WirelessSignal(InWirelessSignal)
	, Settings(InWirelessSignal->Settings)
//	, Geometries(*this)
{
	/*
	StaticMeshInstances.LinkRenderStateArray(RenderState.StaticMeshInstanceRenderStates);
	InstanceGroups.LinkRenderStateArray(RenderState.InstanceGroupRenderStates);
	Landscapes.LinkRenderStateArray(RenderState.LandscapeRenderStates);

	RenderState.Settings = Settings;

	ENQUEUE_RENDER_COMMAND(RenderThreadInit)(
		[&RenderState = RenderState](FRHICommandListImmediate&) mutable
	{
		RenderState.RenderThreadInit();
	});
	*/
}

/** 将完成后的 Lightmap 应用在 World 中*/
void FScene::ApplyFinishedLightmapsToWorld()
{
	/*
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VirtualTexturedLightmaps"));
	const bool bUseVirtualTextures = (CVar->GetValueOnAnyThread() != 0) && UseVirtualTexturing(GMaxRHIFeatureLevel);

	UWorld* World = GPULightmass->World;

	{
		FScopedSlowTask SlowTask(3);
		SlowTask.MakeDialog();

		SlowTask.EnterProgressFrame(1, LOCTEXT("InvalidatingPreviousLightingStatus", "Invalidating previous lighting"));

		FGlobalComponentRecreateRenderStateContext RecreateRenderStateContext; // Implicit FlushRenderingCommands();

		ULevel* LightingScenario = World->GetActiveLightingScenario();

		// Now we can access RT scene & preview lightmap textures directly

		TSet<FGuid> BuildDataResourcesToKeep;

		for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
		{
			ULevel* Level = World->GetLevel(LevelIndex);

			if (Level)
			{
				if (!Level->bIsVisible && !Level->bIsLightingScenario)
				{
					// Do not touch invisible, normal levels
					GatherBuildDataResourcesToKeep(Level, LightingScenario, BuildDataResourcesToKeep);
				}
			}
		}

		for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
		{
			ULevel* Level = World->GetLevel(LevelIndex);

			if (Level)
			{
				// Invalidate static lighting for normal visible levels, and the current lighting scenario
				// Since the current lighting scenario can contain build data for invisible normal levels, use BuildDataResourcesToKeep
				if (Level->bIsVisible && (!Level->bIsLightingScenario || Level == LightingScenario))
				{
					Level->ReleaseRenderingResources();

					if (Level->MapBuildData)
					{
						Level->MapBuildData->InvalidateStaticLighting(World, false, &BuildDataResourcesToKeep);
					}
				}
			}
		}

		for (FDirectionalLightBuildInfo& DirectionalLight : LightScene.DirectionalLights.Elements)
		{
			UDirectionalLightComponent* Light = DirectionalLight.ComponentUObject;
			check(!DirectionalLight.bStationary || Light->PreviewShadowMapChannel != INDEX_NONE);

			ULevel* StorageLevel = LightingScenario ? LightingScenario : Light->GetOwner()->GetLevel();
			UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();
			FLightComponentMapBuildData& LightBuildData = Registry->FindOrAllocateLightBuildData(Light->LightGuid, true);
			LightBuildData.ShadowMapChannel = DirectionalLight.bStationary ? Light->PreviewShadowMapChannel : INDEX_NONE;
		}

		for (FPointLightBuildInfo& PointLight : LightScene.PointLights.Elements)
		{
			UPointLightComponent* Light = PointLight.ComponentUObject;
			check(!PointLight.bStationary || Light->PreviewShadowMapChannel != INDEX_NONE);

			ULevel* StorageLevel = LightingScenario ? LightingScenario : Light->GetOwner()->GetLevel();
			UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();
			FLightComponentMapBuildData& LightBuildData = Registry->FindOrAllocateLightBuildData(Light->LightGuid, true);
			LightBuildData.ShadowMapChannel = PointLight.bStationary ? Light->PreviewShadowMapChannel : INDEX_NONE;
		}

		for (FSpotLightBuildInfo& SpotLight : LightScene.SpotLights.Elements)
		{
			USpotLightComponent* Light = SpotLight.ComponentUObject;
			check(!SpotLight.bStationary || Light->PreviewShadowMapChannel != INDEX_NONE);

			ULevel* StorageLevel = LightingScenario ? LightingScenario : Light->GetOwner()->GetLevel();
			UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();
			FLightComponentMapBuildData& LightBuildData = Registry->FindOrAllocateLightBuildData(Light->LightGuid, true);
			LightBuildData.ShadowMapChannel = SpotLight.bStationary ? Light->PreviewShadowMapChannel : INDEX_NONE;
		}

		for (FRectLightBuildInfo& RectLight : LightScene.RectLights.Elements)
		{
			URectLightComponent* Light = RectLight.ComponentUObject;
			check(!RectLight.bStationary || Light->PreviewShadowMapChannel != INDEX_NONE);

			ULevel* StorageLevel = LightingScenario ? LightingScenario : Light->GetOwner()->GetLevel();
			UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();
			FLightComponentMapBuildData& LightBuildData = Registry->FindOrAllocateLightBuildData(Light->LightGuid, true);
			LightBuildData.ShadowMapChannel = RectLight.bStationary ? Light->PreviewShadowMapChannel : INDEX_NONE;
		}

		{
			ULevel* SubLevelStorageLevel = LightingScenario ? LightingScenario : World->PersistentLevel;
			UMapBuildDataRegistry* SubLevelRegistry = SubLevelStorageLevel->GetOrCreateMapBuildData();
			FPrecomputedVolumetricLightmapData& SubLevelData = SubLevelRegistry->AllocateLevelPrecomputedVolumetricLightmapBuildData(World->PersistentLevel->LevelBuildDataId);

			SubLevelData = *RenderState.VolumetricLightmapRenderer->GetPrecomputedVolumetricLightmapForPreview()->Data;

			ENQUEUE_RENDER_COMMAND(ReadbackVLMDataCmd)([&SubLevelData](FRHICommandListImmediate& RHICmdList) {
				SCOPED_GPU_MASK(RHICmdList, FRHIGPUMask::GPU0());
				ReadbackVolumetricLightmapDataLayerFromGPU(RHICmdList, SubLevelData.IndirectionTexture, SubLevelData.IndirectionTextureDimensions);
				ReadbackVolumetricLightmapDataLayerFromGPU(RHICmdList, SubLevelData.BrickData.AmbientVector, SubLevelData.BrickDataDimensions);
				for (int32 i = 0; i < UE_ARRAY_COUNT(SubLevelData.BrickData.SHCoefficients); i++)
				{
					ReadbackVolumetricLightmapDataLayerFromGPU(RHICmdList, SubLevelData.BrickData.SHCoefficients[i], SubLevelData.BrickDataDimensions);
				}
				ReadbackVolumetricLightmapDataLayerFromGPU(RHICmdList, SubLevelData.BrickData.DirectionalLightShadowing, SubLevelData.BrickDataDimensions);
				});
		}

		// Fill non-existing mip 0 tiles by upsampling from higher mips, if available
		if (RenderState.LightmapRenderer->bOnlyBakeWhatYouSee)
		{
			for (FLightmapRenderState& Lightmap : RenderState.LightmapRenderStates.Elements)
			{
				for (int32 TileX = 0; TileX < Lightmap.GetPaddedSizeInTiles().X; TileX++)
				{
					FTileDataLayer::Evict();

					for (int32 TileY = 0; TileY < Lightmap.GetPaddedSizeInTiles().Y; TileY++)
					{
						FTileVirtualCoordinates Coords(FIntPoint(TileX, TileY), 0);
						if (!Lightmap.DoesTileHaveValidCPUData(Coords, RenderState.LightmapRenderer->GetCurrentRevision()))
						{
							if (!Lightmap.TileStorage.Contains(Coords))
							{
								Lightmap.TileStorage.Add(Coords, FTileStorage{});
							}

							for (int32 MipLevel = 0; MipLevel <= Lightmap.GetMaxLevel(); MipLevel++)
							{
								FTileVirtualCoordinates ParentCoords(FIntPoint(TileX / (1 << MipLevel), TileY / (1 << MipLevel)), MipLevel);
								if (Lightmap.DoesTileHaveValidCPUData(ParentCoords, RenderState.LightmapRenderer->GetCurrentRevision()))
								{
									Lightmap.TileStorage[Coords].CPUTextureData[0]->Decompress();
									Lightmap.TileStorage[Coords].CPUTextureData[1]->Decompress();
									Lightmap.TileStorage[Coords].CPUTextureData[2]->Decompress();

									Lightmap.TileStorage[ParentCoords].CPUTextureData[0]->Decompress();
									Lightmap.TileStorage[ParentCoords].CPUTextureData[1]->Decompress();
									Lightmap.TileStorage[ParentCoords].CPUTextureData[2]->Decompress();

									for (int32 X = 0; X < GPreviewLightmapVirtualTileSize; X++)
									{
										for (int32 Y = 0; Y < GPreviewLightmapVirtualTileSize; Y++)
										{
											FIntPoint DstPixelPosition = FIntPoint(X, Y);
											FIntPoint SrcPixelPosition = (FIntPoint(TileX, TileY) * GPreviewLightmapVirtualTileSize + FIntPoint(X, Y)) / (1 << MipLevel);
											SrcPixelPosition.X %= GPreviewLightmapVirtualTileSize;
											SrcPixelPosition.Y %= GPreviewLightmapVirtualTileSize;

											int32 DstRowPitchInPixels = GPreviewLightmapVirtualTileSize;
											int32 SrcRowPitchInPixels = GPreviewLightmapVirtualTileSize;

											int32 SrcLinearIndex = SrcPixelPosition.Y * SrcRowPitchInPixels + SrcPixelPosition.X;
											int32 DstLinearIndex = DstPixelPosition.Y * DstRowPitchInPixels + DstPixelPosition.X;

											Lightmap.TileStorage[Coords].CPUTextureData[0]->Data[DstLinearIndex] = Lightmap.TileStorage[ParentCoords].CPUTextureData[0]->Data[SrcLinearIndex];
											Lightmap.TileStorage[Coords].CPUTextureData[1]->Data[DstLinearIndex] = Lightmap.TileStorage[ParentCoords].CPUTextureData[1]->Data[SrcLinearIndex];
											Lightmap.TileStorage[Coords].CPUTextureData[2]->Data[DstLinearIndex] = Lightmap.TileStorage[ParentCoords].CPUTextureData[2]->Data[SrcLinearIndex];
										}
									}

									break;
								}
							}
						}
					}
				}
			}
		}

		SlowTask.EnterProgressFrame(1, LOCTEXT("EncodingTexturesStaticLightingStatis", "Encoding textures"));

		{
			int32 NumLightmapsToTranscode = 0;

			for (int32 InstanceIndex = 0; InstanceIndex < StaticMeshInstances.Elements.Num(); InstanceIndex++)
			{
				for (int32 LODIndex = 0; LODIndex < StaticMeshInstances.Elements[InstanceIndex].LODLightmaps.Num(); LODIndex++)
				{
					if (StaticMeshInstances.Elements[InstanceIndex].LODLightmaps[LODIndex].IsValid())
					{
						NumLightmapsToTranscode++;
					}
				}
			}

			for (int32 InstanceGroupIndex = 0; InstanceGroupIndex < InstanceGroups.Elements.Num(); InstanceGroupIndex++)
			{
				for (int32 LODIndex = 0; LODIndex < InstanceGroups.Elements[InstanceGroupIndex].LODLightmaps.Num(); LODIndex++)
				{
					if (InstanceGroups.Elements[InstanceGroupIndex].LODLightmaps[LODIndex].IsValid())
					{
						NumLightmapsToTranscode++;
					}
				}
			}

			for (int32 LandscapeIndex = 0; LandscapeIndex < Landscapes.Elements.Num(); LandscapeIndex++)
			{
				for (int32 LODIndex = 0; LODIndex < Landscapes.Elements[LandscapeIndex].LODLightmaps.Num(); LODIndex++)
				{
					if (Landscapes.Elements[LandscapeIndex].LODLightmaps[LODIndex].IsValid())
					{
						NumLightmapsToTranscode++;
					}
				}
			}

			FDenoiserContext DenoiserContext;

			FScopedSlowTask SubSlowTask(NumLightmapsToTranscode, LOCTEXT("TranscodingLightmaps", "Transcoding lightmaps"));
			SubSlowTask.MakeDialog();

			for (int32 InstanceIndex = 0; InstanceIndex < StaticMeshInstances.Elements.Num(); InstanceIndex++)
			{
				for (int32 LODIndex = 0; LODIndex < StaticMeshInstances.Elements[InstanceIndex].LODLightmaps.Num(); LODIndex++)
				{
					if (StaticMeshInstances.Elements[InstanceIndex].LODLightmaps[LODIndex].IsValid())
					{
						if (Settings->DenoisingOptions == EGPULightmassDenoisingOptions::OnCompletion)
						{
							SubSlowTask.EnterProgressFrame(1, LOCTEXT("DenoisingAndTranscodingLightmaps", "Denoising & transcoding lightmaps"));
						}
						else
						{
							SubSlowTask.EnterProgressFrame(1, LOCTEXT("TranscodingLightmaps", "Transcoding lightmaps"));
						}

						FLightmapRenderState& Lightmap = RenderState.LightmapRenderStates.Elements[StaticMeshInstances.Elements[InstanceIndex].LODLightmaps[LODIndex].GetElementId()];

						for (auto& Tile : Lightmap.TileStorage)
						{
							Tile.Value.CPUTextureData[0]->Decompress();
							Tile.Value.CPUTextureData[1]->Decompress();
							Tile.Value.CPUTextureData[2]->Decompress();
						}

						// Transencode GI layers
						TArray<FLightSampleData> LightSampleData;
						LightSampleData.AddZeroed(Lightmap.GetSize().X * Lightmap.GetSize().Y); // LightSampleData will have different row pitch as VT is padded to tiles

						{
							int32 SrcRowPitchInPixels = GPreviewLightmapVirtualTileSize;
							int32 DstRowPitchInPixels = Lightmap.GetSize().X;

							CopyRectTiled(
								FIntPoint(0, 0),
								FIntRect(FIntPoint(0, 0), Lightmap.GetSize()),
								SrcRowPitchInPixels,
								DstRowPitchInPixels,
								[&Lightmap, &LightSampleData](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
								{
									LightSampleData[DstLinearIndex] = ConvertToLightSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[0]->Data[SrcLinearIndex], Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[1]->Data[SrcLinearIndex]);
								});
						}

#if 0
						{
							// Debug: dump color and SH as pfm
							TArray<FVector> Color;
							TArray<FVector> SH;
							Color.AddZeroed(Lightmap.GetSize().X * Lightmap.GetSize().Y);
							SH.AddZeroed(Lightmap.GetSize().X * Lightmap.GetSize().Y);

							for (int32 Y = 0; Y < Lightmap.GetSize().Y; Y++)
							{
								for (int32 X = 0; X < Lightmap.GetSize().X; X++)
								{
									Color[Y * Lightmap.GetSize().X + X][0] = LightSampleData[Y * Lightmap.GetSize().X + X].Coefficients[0][0];
									Color[Y * Lightmap.GetSize().X + X][1] = LightSampleData[Y * Lightmap.GetSize().X + X].Coefficients[0][1];
									Color[Y * Lightmap.GetSize().X + X][2] = LightSampleData[Y * Lightmap.GetSize().X + X].Coefficients[0][2];

									SH[Y * Lightmap.GetSize().X + X][0] = LightSampleData[Y * Lightmap.GetSize().X + X].Coefficients[1][0];
									SH[Y * Lightmap.GetSize().X + X][1] = LightSampleData[Y * Lightmap.GetSize().X + X].Coefficients[1][1];
									SH[Y * Lightmap.GetSize().X + X][2] = LightSampleData[Y * Lightmap.GetSize().X + X].Coefficients[1][2];
								}
							}

							{
								FFileHelper::SaveStringToFile(
									FString::Printf(TEXT("PF\n%d %d\n-1.0\n"), Lightmap.GetSize().X, Lightmap.GetSize().Y),
									*FString::Printf(TEXT("%s_Irradiance_%dspp.pfm"), *Lightmap.Name, GGPULightmassSamplesPerTexel),
									FFileHelper::EEncodingOptions::ForceAnsi
								);

								TArray<uint8> Bytes((uint8*)Color.GetData(), Color.GetTypeSize() * Color.Num());

								FFileHelper::SaveArrayToFile(Bytes, *FString::Printf(TEXT("%s_Irradiance_%dspp.pfm"), *Lightmap.Name, GGPULightmassSamplesPerTexel), &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
							}

							{
								FFileHelper::SaveStringToFile(
									FString::Printf(TEXT("PF\n%d %d\n-1.0\n"), Lightmap.GetSize().X, Lightmap.GetSize().Y),
									*FString::Printf(TEXT("%s_SH_%dspp.pfm"), *Lightmap.Name, GGPULightmassSamplesPerTexel),
									FFileHelper::EEncodingOptions::ForceAnsi
								);

								TArray<uint8> Bytes((uint8*)SH.GetData(), SH.GetTypeSize() * SH.Num());

								FFileHelper::SaveArrayToFile(Bytes, *FString::Printf(TEXT("%s_SH_%dspp.pfm"), *Lightmap.Name, GGPULightmassSamplesPerTexel), &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
							}
						}
#endif

						if (Settings->DenoisingOptions == EGPULightmassDenoisingOptions::OnCompletion)
						{
							DenoiseLightSampleData(Lightmap.GetSize(), LightSampleData, DenoiserContext);
						}

						FQuantizedLightmapData* QuantizedLightmapData = new FQuantizedLightmapData();
						QuantizedLightmapData->SizeX = Lightmap.GetSize().X;
						QuantizedLightmapData->SizeY = Lightmap.GetSize().Y;

						QuantizeLightSamples(LightSampleData, QuantizedLightmapData->Data, QuantizedLightmapData->Scale, QuantizedLightmapData->Add);

						// Add static lights to lightmap data
						{
							for (FDirectionalLightBuildInfo& DirectionalLight : LightScene.DirectionalLights.Elements)
							{
								if (!DirectionalLight.bStationary)
								{
									UDirectionalLightComponent* Light = DirectionalLight.ComponentUObject;
									QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
								}
							}

							for (FPointLightBuildInfo& PointLight : LightScene.PointLights.Elements)
							{
								if (!PointLight.bStationary)
								{
									UPointLightComponent* Light = PointLight.ComponentUObject;
									if (PointLight.AffectsBounds(StaticMeshInstances.Elements[InstanceIndex].WorldBounds))
									{
										QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
									}
								}
							}

							for (FSpotLightBuildInfo& SpotLight : LightScene.SpotLights.Elements)
							{
								if (!SpotLight.bStationary)
								{
									USpotLightComponent* Light = SpotLight.ComponentUObject;
									if (SpotLight.AffectsBounds(StaticMeshInstances.Elements[InstanceIndex].WorldBounds))
									{
										QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
									}
								}
							}

							for (FRectLightBuildInfo& RectLight : LightScene.RectLights.Elements)
							{
								if (!RectLight.bStationary)
								{
									URectLightComponent* Light = RectLight.ComponentUObject;
									if (RectLight.AffectsBounds(StaticMeshInstances.Elements[InstanceIndex].WorldBounds))
									{
										QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
									}
								}
							}
						}

						// Transencode stationary light shadow masks
						TMap<ULightComponent*, FShadowMapData2D*> ShadowMaps;
						{
							auto TransencodeShadowMap = [&Lightmap, &ShadowMaps](
								FLocalLightBuildInfo& LightBuildInfo,
								FLocalLightRenderState& Light
								)
							{
								check(Light.bStationary);
								check(Light.ShadowMapChannel != INDEX_NONE);
								FQuantizedShadowSignedDistanceFieldData2D* ShadowMap = new FQuantizedShadowSignedDistanceFieldData2D(Lightmap.GetSize().X, Lightmap.GetSize().Y);

								int32 SrcRowPitchInPixels = GPreviewLightmapVirtualTileSize;
								int32 DstRowPitchInPixels = Lightmap.GetSize().X;

								CopyRectTiled(
									FIntPoint(0, 0),
									FIntRect(FIntPoint(0, 0), Lightmap.GetSize()),
									SrcRowPitchInPixels,
									DstRowPitchInPixels,
									[&Lightmap, &ShadowMap, &Light](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
									{
										ShadowMap->GetData()[DstLinearIndex] = ConvertToShadowSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[2]->Data[SrcLinearIndex], Light.ShadowMapChannel);
									});

								ShadowMaps.Add(LightBuildInfo.GetComponentUObject(), ShadowMap);
							};

							// For all relevant lights
							// Directional lights are always relevant
							for (FDirectionalLightBuildInfo& DirectionalLight : LightScene.DirectionalLights.Elements)
							{
								if (!DirectionalLight.bStationary)
								{
									continue;
								}

								int32 ElementId = &DirectionalLight - LightScene.DirectionalLights.Elements.GetData();
								TransencodeShadowMap(DirectionalLight, RenderState.LightSceneRenderState.DirectionalLights.Elements[ElementId]);
							}

							for (FPointLightRenderStateRef& PointLight : Lightmap.RelevantPointLights)
							{
								int32 ElementId = PointLight.GetElementIdChecked();
								TransencodeShadowMap(LightScene.PointLights.Elements[ElementId], PointLight);
							}

							for (FSpotLightRenderStateRef& SpotLight : Lightmap.RelevantSpotLights)
							{
								int32 ElementId = SpotLight.GetElementIdChecked();
								TransencodeShadowMap(LightScene.SpotLights.Elements[ElementId], SpotLight);
							}

							for (FRectLightRenderStateRef& RectLight : Lightmap.RelevantRectLights)
							{
								int32 ElementId = RectLight.GetElementIdChecked();
								TransencodeShadowMap(LightScene.RectLights.Elements[ElementId], RectLight);
							}
						}

						{
							UStaticMeshComponent* StaticMeshComponent = StaticMeshInstances.Elements[InstanceIndex].ComponentUObject;
							if (StaticMeshComponent && StaticMeshComponent->GetOwner() && StaticMeshComponent->GetOwner()->GetLevel())
							{
								// Should have happened at a higher level
								check(!StaticMeshComponent->IsRenderStateCreated());
								// The rendering thread reads from LODData and IrrelevantLights, therefore
								// the component must have finished detaching from the scene on the rendering
								// thread before it is safe to continue.
								check(StaticMeshComponent->AttachmentCounter.GetValue() == 0);

								// Ensure LODData has enough entries in it, free not required.
								const bool bLODDataCountChanged = StaticMeshComponent->SetLODDataCount(LODIndex + 1, StaticMeshComponent->GetStaticMesh()->GetNumLODs());
								if (bLODDataCountChanged)
								{
									StaticMeshComponent->MarkPackageDirty();
								}

								ELightMapPaddingType PaddingType = GAllowLightmapPadding ? LMPT_NormalPadding : LMPT_NoPadding;
								const bool bHasNonZeroData = QuantizedLightmapData->HasNonZeroData();

								ULevel* StorageLevel = LightingScenario ? LightingScenario : StaticMeshComponent->GetOwner()->GetLevel();
								UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();

								FStaticMeshComponentLODInfo& ComponentLODInfo = StaticMeshComponent->LODData[LODIndex];

								// Detect duplicated MapBuildDataId
								if (Registry->GetMeshBuildDataDuringBuild(ComponentLODInfo.MapBuildDataId))
								{
									ComponentLODInfo.MapBuildDataId.Invalidate();

									if (LODIndex > 0)
									{
										// Non-zero LODs derive their MapBuildDataId from LOD0. In this case also regenerate LOD0 GUID
										StaticMeshComponent->LODData[0].MapBuildDataId.Invalidate();
										check(StaticMeshComponent->LODData[0].CreateMapBuildDataId(0));
									}
								}

								if (ComponentLODInfo.CreateMapBuildDataId(LODIndex))
								{
									StaticMeshComponent->MarkPackageDirty();
								}

								FMeshMapBuildData& MeshBuildData = Registry->AllocateMeshBuildData(ComponentLODInfo.MapBuildDataId, true);

								const bool bNeedsLightMap = true;// bHasNonZeroData;
								if (bNeedsLightMap)
								{
									// Create a light-map for the primitive.
									TMap<ULightComponent*, FShadowMapData2D*> EmptyShadowMapData;
									MeshBuildData.LightMap = FLightMap2D::AllocateLightMap(
										Registry,
										QuantizedLightmapData,
										bUseVirtualTextures ? ShadowMaps : EmptyShadowMapData,
										StaticMeshComponent->Bounds,
										PaddingType,
										LMF_Streamed
									);
								}
								else
								{
									MeshBuildData.LightMap = NULL;
									delete QuantizedLightmapData;
								}

								if (ShadowMaps.Num() > 0 && !bUseVirtualTextures)
								{
									MeshBuildData.ShadowMap = FShadowMap2D::AllocateShadowMap(
										Registry,
										ShadowMaps,
										StaticMeshComponent->Bounds,
										PaddingType,
										SMF_Streamed
									);
								}
								else
								{
									MeshBuildData.ShadowMap = NULL;
								}
							}
						}

						FTileDataLayer::Evict();
					}
				}
			}

			for (int32 InstanceGroupIndex = 0; InstanceGroupIndex < InstanceGroups.Elements.Num(); InstanceGroupIndex++)
			{
				for (int32 LODIndex = 0; LODIndex < InstanceGroups.Elements[InstanceGroupIndex].LODLightmaps.Num(); LODIndex++)
				{
					if (InstanceGroups.Elements[InstanceGroupIndex].LODLightmaps[LODIndex].IsValid())
					{
						if (Settings->DenoisingOptions == EGPULightmassDenoisingOptions::OnCompletion)
						{
							SubSlowTask.EnterProgressFrame(1, LOCTEXT("DenoisingAndTranscodingLightmaps", "Denoising & transcoding lightmaps"));
						}
						else
						{
							SubSlowTask.EnterProgressFrame(1, LOCTEXT("TranscodingLightmaps", "Transcoding lightmaps"));
						}

						FLightmapRenderState& Lightmap = RenderState.LightmapRenderStates.Elements[InstanceGroups.Elements[InstanceGroupIndex].LODLightmaps[LODIndex].GetElementId()];

						for (auto& Tile : Lightmap.TileStorage)
						{
							Tile.Value.CPUTextureData[0]->Decompress();
							Tile.Value.CPUTextureData[1]->Decompress();
							Tile.Value.CPUTextureData[2]->Decompress();
						}

						FInstanceGroup& InstanceGroup = InstanceGroups.Elements[InstanceGroupIndex];

						int32 BaseLightMapWidth = InstanceGroup.LODPerInstanceLightmapSize[LODIndex].X;
						int32 BaseLightMapHeight = InstanceGroup.LODPerInstanceLightmapSize[LODIndex].Y;

						int32 InstancesPerRow = FMath::CeilToInt(FMath::Sqrt(InstanceGroup.ComponentUObject->PerInstanceSMData.Num()));

						// Transencode GI layers
						TArray<TArray<FLightSampleData>> InstanceGroupLightSampleData;
						TArray<TUniquePtr<FQuantizedLightmapData>> InstancedSourceQuantizedData;
						TArray<TMap<ULightComponent*, TUniquePtr<FShadowMapData2D>>> InstancedShadowMapData;
						InstanceGroupLightSampleData.AddDefaulted(InstanceGroup.ComponentUObject->PerInstanceSMData.Num());
						InstancedSourceQuantizedData.AddDefaulted(InstanceGroup.ComponentUObject->PerInstanceSMData.Num());
						InstancedShadowMapData.AddDefaulted(InstanceGroup.ComponentUObject->PerInstanceSMData.Num());

						for (int32 InstanceIndex = 0; InstanceIndex < InstanceGroupLightSampleData.Num(); InstanceIndex++)
						{
							TArray<FLightSampleData>& LightSampleData = InstanceGroupLightSampleData[InstanceIndex];
							LightSampleData.AddZeroed(BaseLightMapWidth * BaseLightMapHeight);
							InstancedSourceQuantizedData[InstanceIndex] = MakeUnique<FQuantizedLightmapData>();

							int32 SrcRowPitchInPixels = GPreviewLightmapVirtualTileSize;
							int32 DstRowPitchInPixels = BaseLightMapWidth;

							int32 RenderIndex = InstanceGroup.ComponentUObject->GetRenderIndex(InstanceIndex);

							if (RenderIndex != INDEX_NONE)
							{
								FIntPoint InstanceTilePos = FIntPoint(RenderIndex % InstancesPerRow, RenderIndex / InstancesPerRow);
								FIntPoint InstanceTileMin = FIntPoint(InstanceTilePos.X * BaseLightMapWidth, InstanceTilePos.Y * BaseLightMapHeight);

								CopyRectTiled(
									InstanceTileMin,
									FIntRect(FIntPoint(0, 0), FIntPoint(BaseLightMapWidth, BaseLightMapHeight)),
									SrcRowPitchInPixels,
									DstRowPitchInPixels,
									[&Lightmap, &LightSampleData](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
									{
										LightSampleData[DstLinearIndex] = ConvertToLightSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[0]->Data[SrcLinearIndex], Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[1]->Data[SrcLinearIndex]);
									});
							}

							if (Settings->DenoisingOptions == EGPULightmassDenoisingOptions::OnCompletion)
							{
								DenoiseLightSampleData(FIntPoint(BaseLightMapWidth, BaseLightMapHeight), LightSampleData, DenoiserContext);
							}

							FQuantizedLightmapData& QuantizedLightmapData = *InstancedSourceQuantizedData[InstanceIndex];
							QuantizedLightmapData.SizeX = BaseLightMapWidth;
							QuantizedLightmapData.SizeY = BaseLightMapHeight;

							QuantizeLightSamples(LightSampleData, QuantizedLightmapData.Data, QuantizedLightmapData.Scale, QuantizedLightmapData.Add);

							// Transencode stationary light shadow masks
							TMap<ULightComponent*, TUniquePtr<FShadowMapData2D>>& ShadowMaps = InstancedShadowMapData[InstanceIndex];

							{
								// For all relevant lights
								// Directional lights are always relevant
								for (FDirectionalLightBuildInfo& DirectionalLight : LightScene.DirectionalLights.Elements)
								{
									if (!DirectionalLight.bStationary)
									{
										continue;
									}

									check(DirectionalLight.ShadowMapChannel != INDEX_NONE);
									TUniquePtr<FQuantizedShadowSignedDistanceFieldData2D> ShadowMap = MakeUnique<FQuantizedShadowSignedDistanceFieldData2D>(BaseLightMapWidth, BaseLightMapHeight);

									if (RenderIndex != INDEX_NONE)
									{
										FIntPoint InstanceTilePos = FIntPoint(RenderIndex % InstancesPerRow, RenderIndex / InstancesPerRow);
										FIntPoint InstanceTileMin = FIntPoint(InstanceTilePos.X * BaseLightMapWidth, InstanceTilePos.Y * BaseLightMapHeight);

										CopyRectTiled(
											InstanceTileMin,
											FIntRect(FIntPoint(0, 0), FIntPoint(BaseLightMapWidth, BaseLightMapHeight)),
											SrcRowPitchInPixels,
											DstRowPitchInPixels,
											[&Lightmap, &ShadowMap, &DirectionalLight](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
											{
												ShadowMap->GetData()[DstLinearIndex] = ConvertToShadowSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[2]->Data[SrcLinearIndex], DirectionalLight.ShadowMapChannel);
											});
									}

									ShadowMaps.Add(DirectionalLight.ComponentUObject, MoveTemp(ShadowMap));
								}

								for (FPointLightRenderStateRef& PointLight : Lightmap.RelevantPointLights)
								{
									check(PointLight->bStationary);
									check(PointLight->ShadowMapChannel != INDEX_NONE);
									TUniquePtr<FQuantizedShadowSignedDistanceFieldData2D> ShadowMap = MakeUnique<FQuantizedShadowSignedDistanceFieldData2D>(BaseLightMapWidth, BaseLightMapHeight);

									if (RenderIndex != INDEX_NONE)
									{
										FIntPoint InstanceTilePos = FIntPoint(RenderIndex % InstancesPerRow, RenderIndex / InstancesPerRow);
										FIntPoint InstanceTileMin = FIntPoint(InstanceTilePos.X * BaseLightMapWidth, InstanceTilePos.Y * BaseLightMapHeight);

										CopyRectTiled(
											InstanceTileMin,
											FIntRect(FIntPoint(0, 0), FIntPoint(BaseLightMapWidth, BaseLightMapHeight)),
											SrcRowPitchInPixels,
											DstRowPitchInPixels,
											[&Lightmap, &ShadowMap, &PointLight](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
											{
												ShadowMap->GetData()[DstLinearIndex] = ConvertToShadowSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[2]->Data[SrcLinearIndex], PointLight->ShadowMapChannel);
											});
									}

									ShadowMaps.Add(LightScene.PointLights.Elements[PointLight.GetElementIdChecked()].ComponentUObject, MoveTemp(ShadowMap));
								}

								for (FSpotLightRenderStateRef& SpotLight : Lightmap.RelevantSpotLights)
								{
									check(SpotLight->bStationary);
									check(SpotLight->ShadowMapChannel != INDEX_NONE);
									TUniquePtr<FQuantizedShadowSignedDistanceFieldData2D> ShadowMap = MakeUnique<FQuantizedShadowSignedDistanceFieldData2D>(BaseLightMapWidth, BaseLightMapHeight);

									if (RenderIndex != INDEX_NONE)
									{
										FIntPoint InstanceTilePos = FIntPoint(RenderIndex % InstancesPerRow, RenderIndex / InstancesPerRow);
										FIntPoint InstanceTileMin = FIntPoint(InstanceTilePos.X * BaseLightMapWidth, InstanceTilePos.Y * BaseLightMapHeight);

										CopyRectTiled(
											InstanceTileMin,
											FIntRect(FIntPoint(0, 0), FIntPoint(BaseLightMapWidth, BaseLightMapHeight)),
											SrcRowPitchInPixels,
											DstRowPitchInPixels,
											[&Lightmap, &ShadowMap, &SpotLight](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
											{
												ShadowMap->GetData()[DstLinearIndex] = ConvertToShadowSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[2]->Data[SrcLinearIndex], SpotLight->ShadowMapChannel);
											});
									}

									ShadowMaps.Add(LightScene.SpotLights.Elements[SpotLight.GetElementIdChecked()].ComponentUObject, MoveTemp(ShadowMap));
								}

								for (FRectLightRenderStateRef& RectLight : Lightmap.RelevantRectLights)
								{
									check(RectLight->bStationary);
									check(RectLight->ShadowMapChannel != INDEX_NONE);
									TUniquePtr<FQuantizedShadowSignedDistanceFieldData2D> ShadowMap = MakeUnique<FQuantizedShadowSignedDistanceFieldData2D>(BaseLightMapWidth, BaseLightMapHeight);

									if (RenderIndex != INDEX_NONE)
									{
										FIntPoint InstanceTilePos = FIntPoint(RenderIndex % InstancesPerRow, RenderIndex / InstancesPerRow);
										FIntPoint InstanceTileMin = FIntPoint(InstanceTilePos.X * BaseLightMapWidth, InstanceTilePos.Y * BaseLightMapHeight);

										CopyRectTiled(
											InstanceTileMin,
											FIntRect(FIntPoint(0, 0), FIntPoint(BaseLightMapWidth, BaseLightMapHeight)),
											SrcRowPitchInPixels,
											DstRowPitchInPixels,
											[&Lightmap, &ShadowMap, &RectLight](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
											{
												ShadowMap->GetData()[DstLinearIndex] = ConvertToShadowSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[2]->Data[SrcLinearIndex], RectLight->ShadowMapChannel);
											});
									}

									ShadowMaps.Add(LightScene.RectLights.Elements[RectLight.GetElementIdChecked()].ComponentUObject, MoveTemp(ShadowMap));
								}
							}
						}

						// Add static lights to lightmap data
						// Instanced lightmaps will eventually be merged together, so just add to the first one
						if (InstancedSourceQuantizedData.Num() > 0)
						{
							TUniquePtr<FQuantizedLightmapData>& QuantizedLightmapData = InstancedSourceQuantizedData[0];
							{
								for (FDirectionalLightBuildInfo& DirectionalLight : LightScene.DirectionalLights.Elements)
								{
									if (!DirectionalLight.bStationary)
									{
										UDirectionalLightComponent* Light = DirectionalLight.ComponentUObject;
										QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
									}
								}

								for (FPointLightBuildInfo& PointLight : LightScene.PointLights.Elements)
								{
									if (!PointLight.bStationary)
									{
										UPointLightComponent* Light = PointLight.ComponentUObject;
										if (PointLight.AffectsBounds(InstanceGroup.WorldBounds))
										{
											QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
										}
									}
								}

								for (FSpotLightBuildInfo& SpotLight : LightScene.SpotLights.Elements)
								{
									if (!SpotLight.bStationary)
									{
										USpotLightComponent* Light = SpotLight.ComponentUObject;
										if (SpotLight.AffectsBounds(InstanceGroup.WorldBounds))
										{
											QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
										}
									}
								}

								for (FRectLightBuildInfo& RectLight : LightScene.RectLights.Elements)
								{
									if (!RectLight.bStationary)
									{
										URectLightComponent* Light = RectLight.ComponentUObject;
										if (RectLight.AffectsBounds(InstanceGroup.WorldBounds))
										{
											QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
										}
									}
								}
							}
						}

						UStaticMesh* ResolvedMesh = InstanceGroup.ComponentUObject->GetStaticMesh();
						if (InstanceGroup.ComponentUObject->LODData.Num() != ResolvedMesh->GetNumLODs())
						{
							InstanceGroup.ComponentUObject->MarkPackageDirty();
						}

						// Ensure LODData has enough entries in it, free not required.
						InstanceGroup.ComponentUObject->SetLODDataCount(ResolvedMesh->GetNumLODs(), ResolvedMesh->GetNumLODs());

						ULevel* StorageLevel = LightingScenario ? LightingScenario : InstanceGroup.ComponentUObject->GetOwner()->GetLevel();
						UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();

						FStaticMeshComponentLODInfo& ComponentLODInfo = InstanceGroup.ComponentUObject->LODData[LODIndex];

						// Detect duplicated MapBuildDataId
						if (Registry->GetMeshBuildDataDuringBuild(ComponentLODInfo.MapBuildDataId))
						{
							ComponentLODInfo.MapBuildDataId.Invalidate();

							if (LODIndex > 0)
							{
								// Non-zero LODs derive their MapBuildDataId from LOD0. In this case also regenerate LOD0 GUID
								InstanceGroup.ComponentUObject->LODData[0].MapBuildDataId.Invalidate();
								check(InstanceGroup.ComponentUObject->LODData[0].CreateMapBuildDataId(0));
							}
						}

						if (ComponentLODInfo.CreateMapBuildDataId(LODIndex))
						{
							InstanceGroup.ComponentUObject->MarkPackageDirty();
						}

						FMeshMapBuildData& MeshBuildData = Registry->AllocateMeshBuildData(InstanceGroup.ComponentUObject->LODData[LODIndex].MapBuildDataId, true);

						MeshBuildData.PerInstanceLightmapData.Empty(InstancedSourceQuantizedData.Num());
						MeshBuildData.PerInstanceLightmapData.AddZeroed(InstancedSourceQuantizedData.Num());

						// Create a light-map for the primitive.
						// When using VT, shadow map data is included with lightmap allocation
						const ELightMapPaddingType PaddingType = GAllowLightmapPadding ? LMPT_NormalPadding : LMPT_NoPadding;

						TArray<TMap<ULightComponent*, TUniquePtr<FShadowMapData2D>>> EmptyShadowMapData;
						TRefCountPtr<FLightMap2D> NewLightMap = FLightMap2D::AllocateInstancedLightMap(Registry, InstanceGroup.ComponentUObject,
							MoveTemp(InstancedSourceQuantizedData),
							bUseVirtualTextures ? MoveTemp(InstancedShadowMapData) : MoveTemp(EmptyShadowMapData),
							Registry, InstanceGroup.ComponentUObject->LODData[LODIndex].MapBuildDataId, InstanceGroup.ComponentUObject->Bounds, PaddingType, LMF_Streamed);

						MeshBuildData.LightMap = NewLightMap;

						if (InstancedShadowMapData.Num() > 0 && !bUseVirtualTextures)
						{
							TRefCountPtr<FShadowMap2D> NewShadowMap = FShadowMap2D::AllocateInstancedShadowMap(Registry, InstanceGroup.ComponentUObject,
								MoveTemp(InstancedShadowMapData),
								Registry, InstanceGroup.ComponentUObject->LODData[LODIndex].MapBuildDataId, InstanceGroup.ComponentUObject->Bounds, PaddingType, SMF_Streamed);

							MeshBuildData.ShadowMap = NewShadowMap;
						}

						FTileDataLayer::Evict();
					}
				}
			}

			for (int32 LandscapeIndex = 0; LandscapeIndex < Landscapes.Elements.Num(); LandscapeIndex++)
			{
				for (int32 LODIndex = 0; LODIndex < Landscapes.Elements[LandscapeIndex].LODLightmaps.Num(); LODIndex++)
				{
					if (Landscapes.Elements[LandscapeIndex].LODLightmaps[LODIndex].IsValid())
					{
						if (Settings->DenoisingOptions == EGPULightmassDenoisingOptions::OnCompletion)
						{
							SubSlowTask.EnterProgressFrame(1, LOCTEXT("DenoisingAndTranscodingLightmaps", "Denoising & transcoding lightmaps"));
						}
						else
						{
							SubSlowTask.EnterProgressFrame(1, LOCTEXT("TranscodingLightmaps", "Transcoding lightmaps"));
						}

						FLightmapRenderState& Lightmap = RenderState.LightmapRenderStates.Elements[Landscapes.Elements[LandscapeIndex].LODLightmaps[LODIndex].GetElementId()];

						for (auto& Tile : Lightmap.TileStorage)
						{
							Tile.Value.CPUTextureData[0]->Decompress();
							Tile.Value.CPUTextureData[1]->Decompress();
							Tile.Value.CPUTextureData[2]->Decompress();
						}

						// Transencode GI layers
						TArray<FLightSampleData> LightSampleData;
						LightSampleData.AddZeroed(Lightmap.GetSize().X * Lightmap.GetSize().Y); // LightSampleData will have different row pitch as VT is padded to tiles

						{
							int32 SrcRowPitchInPixels = GPreviewLightmapVirtualTileSize;
							int32 DstRowPitchInPixels = Lightmap.GetSize().X;

							CopyRectTiled(
								FIntPoint(0, 0),
								FIntRect(FIntPoint(0, 0), Lightmap.GetSize()),
								SrcRowPitchInPixels,
								DstRowPitchInPixels,
								[&Lightmap, &LightSampleData](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
								{
									LightSampleData[DstLinearIndex] = ConvertToLightSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[0]->Data[SrcLinearIndex], Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[1]->Data[SrcLinearIndex]);
								});
						}

						if (Settings->DenoisingOptions == EGPULightmassDenoisingOptions::OnCompletion)
						{
							DenoiseLightSampleData(Lightmap.GetSize(), LightSampleData, DenoiserContext);
						}

						FQuantizedLightmapData* QuantizedLightmapData = new FQuantizedLightmapData();
						QuantizedLightmapData->SizeX = Lightmap.GetSize().X;
						QuantizedLightmapData->SizeY = Lightmap.GetSize().Y;

						QuantizeLightSamples(LightSampleData, QuantizedLightmapData->Data, QuantizedLightmapData->Scale, QuantizedLightmapData->Add);

						// Add static lights to lightmap data
						{
							for (FDirectionalLightBuildInfo& DirectionalLight : LightScene.DirectionalLights.Elements)
							{
								if (!DirectionalLight.bStationary)
								{
									UDirectionalLightComponent* Light = DirectionalLight.ComponentUObject;
									QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
								}
							}

							for (FPointLightBuildInfo& PointLight : LightScene.PointLights.Elements)
							{
								if (!PointLight.bStationary)
								{
									UPointLightComponent* Light = PointLight.ComponentUObject;
									if (PointLight.AffectsBounds(Landscapes.Elements[LandscapeIndex].WorldBounds))
									{
										QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
									}
								}
							}

							for (FSpotLightBuildInfo& SpotLight : LightScene.SpotLights.Elements)
							{
								if (!SpotLight.bStationary)
								{
									USpotLightComponent* Light = SpotLight.ComponentUObject;
									if (SpotLight.AffectsBounds(Landscapes.Elements[LandscapeIndex].WorldBounds))
									{
										QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
									}
								}
							}

							for (FRectLightBuildInfo& RectLight : LightScene.RectLights.Elements)
							{
								if (!RectLight.bStationary)
								{
									URectLightComponent* Light = RectLight.ComponentUObject;
									if (RectLight.AffectsBounds(Landscapes.Elements[LandscapeIndex].WorldBounds))
									{
										QuantizedLightmapData->LightGuids.Add(Light->LightGuid);
									}
								}
							}
						}

						// Transencode stationary light shadow masks
						TMap<ULightComponent*, FShadowMapData2D*> ShadowMaps;
						{
							auto TransencodeShadowMap = [&Lightmap, &ShadowMaps](
								FLocalLightBuildInfo& LightBuildInfo,
								FLocalLightRenderState& Light
								)
							{
								check(Light.bStationary);
								check(Light.ShadowMapChannel != INDEX_NONE);
								FQuantizedShadowSignedDistanceFieldData2D* ShadowMap = new FQuantizedShadowSignedDistanceFieldData2D(Lightmap.GetSize().X, Lightmap.GetSize().Y);

								int32 SrcRowPitchInPixels = GPreviewLightmapVirtualTileSize;
								int32 DstRowPitchInPixels = Lightmap.GetSize().X;

								CopyRectTiled(
									FIntPoint(0, 0),
									FIntRect(FIntPoint(0, 0), Lightmap.GetSize()),
									SrcRowPitchInPixels,
									DstRowPitchInPixels,
									[&Lightmap, &ShadowMap, &Light](int32 DstLinearIndex, FIntPoint SrcTilePosition, int32 SrcLinearIndex) mutable
									{
										ShadowMap->GetData()[DstLinearIndex] = ConvertToShadowSample(Lightmap.TileStorage[FTileVirtualCoordinates(SrcTilePosition, 0)].CPUTextureData[2]->Data[SrcLinearIndex], Light.ShadowMapChannel);
									});

								ShadowMaps.Add(LightBuildInfo.GetComponentUObject(), ShadowMap);
							};

							// For all relevant lights
							// Directional lights are always relevant
							for (FDirectionalLightBuildInfo& DirectionalLight : LightScene.DirectionalLights.Elements)
							{
								if (!DirectionalLight.bStationary)
								{
									continue;
								}

								int32 ElementId = &DirectionalLight - LightScene.DirectionalLights.Elements.GetData();
								TransencodeShadowMap(DirectionalLight, RenderState.LightSceneRenderState.DirectionalLights.Elements[ElementId]);
							}

							for (FPointLightRenderStateRef& PointLight : Lightmap.RelevantPointLights)
							{
								int32 ElementId = PointLight.GetElementIdChecked();
								TransencodeShadowMap(LightScene.PointLights.Elements[ElementId], PointLight);
							}

							for (FSpotLightRenderStateRef& SpotLight : Lightmap.RelevantSpotLights)
							{
								int32 ElementId = SpotLight.GetElementIdChecked();
								TransencodeShadowMap(LightScene.SpotLights.Elements[ElementId], SpotLight);
							}

							for (FRectLightRenderStateRef& RectLight : Lightmap.RelevantRectLights)
							{
								int32 ElementId = RectLight.GetElementIdChecked();
								TransencodeShadowMap(LightScene.RectLights.Elements[ElementId], RectLight);
							}
						}

						{
							ULandscapeComponent* LandscapeComponent = Landscapes.Elements[LandscapeIndex].ComponentUObject;
							ELightMapPaddingType PaddingType = LMPT_NoPadding;
							const bool bHasNonZeroData = QuantizedLightmapData->HasNonZeroData();

							ULevel* StorageLevel = LightingScenario ? LightingScenario : LandscapeComponent->GetOwner()->GetLevel();
							UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();
							FMeshMapBuildData& MeshBuildData = Registry->AllocateMeshBuildData(LandscapeComponent->MapBuildDataId, true);

							const bool bNeedsLightMap = true;// bHasNonZeroData;
							if (bNeedsLightMap)
							{
								// Create a light-map for the primitive.
								TMap<ULightComponent*, FShadowMapData2D*> EmptyShadowMapData;
								MeshBuildData.LightMap = FLightMap2D::AllocateLightMap(
									Registry,
									QuantizedLightmapData,
									bUseVirtualTextures ? ShadowMaps : EmptyShadowMapData,
									LandscapeComponent->Bounds,
									PaddingType,
									LMF_Streamed
								);
							}
							else
							{
								MeshBuildData.LightMap = NULL;
								delete QuantizedLightmapData;
							}

							if (ShadowMaps.Num() > 0 && !bUseVirtualTextures)
							{
								MeshBuildData.ShadowMap = FShadowMap2D::AllocateShadowMap(
									Registry,
									ShadowMaps,
									LandscapeComponent->Bounds,
									PaddingType,
									SMF_Streamed
								);
							}
							else
							{
								MeshBuildData.ShadowMap = NULL;
							}

							if (ALandscapeProxy* Proxy = Cast<ALandscapeProxy>(LandscapeComponent->GetOuter()))
							{
								TSet<ULandscapeComponent*> Components;
								Components.Add(LandscapeComponent);
								Proxy->FlushGrassComponents(&Components, false);
							}
						}

						FTileDataLayer::Evict();
					}
				}
			}

		}

		GCompressLightmaps = Settings->bCompressLightmaps;

		FLightMap2D::EncodeTextures(World, LightingScenario, true, true);
		FShadowMap2D::EncodeTextures(World, LightingScenario, true, true);

		SlowTask.EnterProgressFrame(1, LOCTEXT("ApplyingNewLighting", "Applying new lighting"));

		for (int32 LevelIndex = 0; LevelIndex < World->GetNumLevels(); LevelIndex++)
		{
			bool bMarkLevelDirty = false;
			ULevel* Level = World->GetLevel(LevelIndex);

			if (Level)
			{
				if (Level->bIsVisible && (!Level->bIsLightingScenario || Level == LightingScenario))
				{
					ULevel* StorageLevel = LightingScenario ? LightingScenario : Level;
					UMapBuildDataRegistry* Registry = StorageLevel->GetOrCreateMapBuildData();

					Registry->SetupLightmapResourceClusters();

					Level->InitializeRenderingResources();
				}
			}
		}
	}

	// Always turn Realtime back on after baking lighting
	if (GCurrentLevelEditingViewportClient)
	{
		GCurrentLevelEditingViewportClient->SetRealtime(true);
	}
	*/
}

/** 移除所有组件 */
void FScene::RemoveAllComponents()
{
	/*
	TArray<UStaticMeshComponent*> RegisteredStaticMeshComponents;
	TArray<UInstancedStaticMeshComponent*> RegisteredInstancedStaticMeshComponents;
	TArray<ULandscapeComponent*> RegisteredLandscapeComponents;
	RegisteredStaticMeshComponentUObjects.GetKeys(RegisteredStaticMeshComponents);
	RegisteredInstancedStaticMeshComponentUObjects.GetKeys(RegisteredInstancedStaticMeshComponents);
	RegisteredLandscapeComponentUObjects.GetKeys(RegisteredLandscapeComponents);

	for (auto Component : RegisteredStaticMeshComponents)
	{
		RemoveGeometryInstanceFromComponent(Component);
	}
	for (auto Component : RegisteredInstancedStaticMeshComponents)
	{
		RemoveGeometryInstanceFromComponent(Component);
	}
	for (auto Component : RegisteredLandscapeComponents)
	{
		RemoveGeometryInstanceFromComponent(Component);
	}

	TArray<UDirectionalLightComponent*> RegisteredDirectionalLightComponents;
	TArray<UPointLightComponent*> RegisteredPointLightComponents;
	TArray<USpotLightComponent*> RegisteredSpotLightComponents;
	TArray<URectLightComponent*> RegisteredRectLightComponents;
	LightScene.RegisteredDirectionalLightComponentUObjects.GetKeys(RegisteredDirectionalLightComponents);
	LightScene.RegisteredPointLightComponentUObjects.GetKeys(RegisteredPointLightComponents);
	LightScene.RegisteredSpotLightComponentUObjects.GetKeys(RegisteredSpotLightComponents);
	LightScene.RegisteredRectLightComponentUObjects.GetKeys(RegisteredRectLightComponents);

	for (auto Light : RegisteredDirectionalLightComponents)
	{
		RemoveLight(Light);
	}
	for (auto Light : RegisteredPointLightComponents)
	{
		RemoveLight(Light);
	}
	for (auto Light : RegisteredSpotLightComponents)
	{
		RemoveLight(Light);
	}
	for (auto Light : RegisteredRectLightComponents)
	{
		RemoveLight(Light);
	}

	if (LightScene.SkyLight.IsSet())
	{
		RemoveLight(LightScene.SkyLight->ComponentUObject);
	}
	*/
}

void FScene::BackgroundTick()
{
	/*
	int32 Percentage = FPlatformAtomics::AtomicRead(&RenderState.Percentage);

	if (GPULightmass->LightBuildNotification.IsValid())
	{
		bool bIsViewportNonRealtime = GCurrentLevelEditingViewportClient && !GCurrentLevelEditingViewportClient->IsRealtime();
		if (bIsViewportNonRealtime)
		{
			if (GPULightmass->Settings->Mode == EGPULightmassMode::FullBake)
			{
				FText Text = FText::Format(LOCTEXT("LightBuildProgressMessage", "Building lighting{0}:  {1}%"), FText(), FText::AsNumber(Percentage));
				GPULightmass->LightBuildNotification->SetText(Text);
			}
			else
			{
				FText Text = FText::Format(LOCTEXT("LightBuildProgressForCurrentViewMessage", "Building lighting for current view{0}:  {1}%"), FText(), FText::AsNumber(Percentage));
				GPULightmass->LightBuildNotification->SetText(Text);
			}
		}
		else
		{
			if (GPULightmass->Settings->Mode == EGPULightmassMode::FullBake)
			{
				FText Text = FText::Format(LOCTEXT("LightBuildProgressSlowModeMessage", "Building lighting{0}:  {1}% (slow mode)"), FText(), FText::AsNumber(Percentage));
				GPULightmass->LightBuildNotification->SetText(Text);
			}
			else
			{
				FText Text = FText::Format(LOCTEXT("LightBuildProgressForCurrentViewSlowModeMessage", "Building lighting for current view{0}:  {1}% (slow mode)"), FText(), FText::AsNumber(Percentage));
				GPULightmass->LightBuildNotification->SetText(Text);
			}
		}
	}
	GPULightmass->LightBuildPercentage = Percentage;

	if (Percentage < 100 || GPULightmass->Settings->Mode == EGPULightmassMode::BakeWhatYouSee)
	{
		if (bNeedsVoxelization)
		{
			GatherImportanceVolumes();

			ENQUEUE_RENDER_COMMAND(BackgroundTickRenderThread)([&RenderState = RenderState](FRHICommandListImmediate&) mutable {
				RenderState.VolumetricLightmapRenderer->VoxelizeScene();
				RenderState.VolumetricLightmapRenderer->FrameNumber = 0;
				RenderState.VolumetricLightmapRenderer->SamplesTaken = 0;
				});

			bNeedsVoxelization = false;
		}

		ENQUEUE_RENDER_COMMAND(BackgroundTickRenderThread)([&RenderState = RenderState](FRHICommandListImmediate&) mutable {
			RenderState.BackgroundTick();
			});
	}
	else
	{
		ApplyFinishedLightmapsToWorld();
		GPULightmass->World->GetSubsystem<UGPULightmassSubsystem>()->OnLightBuildEnded().Broadcast();
	}
	*/
}

/********************************************************************************************************************
 *										
 *											场景渲染状态类 FSceneRenderState 
 * 
 ********************************************************************************************************************/

/** 渲染线程初始化 */
void FSceneRenderState::RenderThreadInit()
{
	// 检查是否处于渲染线程
	check(IsInRenderingThread());

	/*
	LightmapRenderer = MakeUnique<FLightmapRenderer>(this);
	VolumetricLightmapRenderer = MakeUnique<FVolumetricLightmapRenderer>(this);
	IrradianceCache = MakeUnique<FIrradianceCache>(Settings->IrradianceCacheQuality, Settings->IrradianceCacheSpacing, Settings->IrradianceCacheCornerRejection);
	IrradianceCache->CurrentRevision = LightmapRenderer->GetCurrentRevision();
	*/
}

/** 渲染线程后台任务 */
void FSceneRenderState::BackgroundTick()
{
	/*
	if (IrradianceCache->CurrentRevision != LightmapRenderer->GetCurrentRevision())
	{
		IrradianceCache = MakeUnique<FIrradianceCache>(Settings->IrradianceCacheQuality, Settings->IrradianceCacheSpacing, Settings->IrradianceCacheCornerRejection);
		IrradianceCache->CurrentRevision = LightmapRenderer->GetCurrentRevision();
	}

	bool bHaveFinishedSurfaceLightmaps = false;

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(GPULightmassCountProgress);

		uint64 SamplesTaken = 0;
		uint64 TotalSamples = 0;

		// Count surface lightmap work
		if (!LightmapRenderer->bOnlyBakeWhatYouSee)
		{
			// Count work has been done
			for (FLightmapRenderState& Lightmap : LightmapRenderStates.Elements)
			{
				for (int32 Y = 0; Y < Lightmap.GetPaddedSizeInTiles().Y; Y++)
				{
					for (int32 X = 0; X < Lightmap.GetPaddedSizeInTiles().X; X++)
					{
						FTileVirtualCoordinates VirtualCoordinates(FIntPoint(X, Y), 0);

						TotalSamples += Settings->GISamples * GPreviewLightmapPhysicalTileSize * GPreviewLightmapPhysicalTileSize;
						SamplesTaken += (Lightmap.DoesTileHaveValidCPUData(VirtualCoordinates, LightmapRenderer->GetCurrentRevision()) ?
							Settings->GISamples :
							FMath::Min(Lightmap.RetrieveTileState(VirtualCoordinates).RenderPassIndex, Settings->GISamples - 1)) * GPreviewLightmapPhysicalTileSize * GPreviewLightmapPhysicalTileSize;
					}
				}
			}
		}
		else // LightmapRenderer->bOnlyBakeWhatYouSee == true
		{
			if (LightmapRenderer->RecordedTileRequests.Num() > 0)
			{
				for (FLightmapTileRequest& Tile : LightmapRenderer->RecordedTileRequests)
				{
					TotalSamples += Settings->GISamples * GPreviewLightmapPhysicalTileSize * GPreviewLightmapPhysicalTileSize;

					SamplesTaken += (Tile.RenderState->DoesTileHaveValidCPUData(Tile.VirtualCoordinates, LightmapRenderer->GetCurrentRevision()) ?
						Settings->GISamples :
						FMath::Min(Tile.RenderState->RetrieveTileState(Tile.VirtualCoordinates).RenderPassIndex, Settings->GISamples - 1)) * GPreviewLightmapPhysicalTileSize * GPreviewLightmapPhysicalTileSize;
				}
			}
			else
			{
				for (TArray<FLightmapTileRequest>& FrameRequests : LightmapRenderer->TilesVisibleLastFewFrames)
				{
					for (FLightmapTileRequest& Tile : FrameRequests)
					{
						TotalSamples += Settings->GISamples * GPreviewLightmapPhysicalTileSize * GPreviewLightmapPhysicalTileSize;

						SamplesTaken += (Tile.RenderState->DoesTileHaveValidCPUData(Tile.VirtualCoordinates, LightmapRenderer->GetCurrentRevision()) ?
							Settings->GISamples :
							FMath::Min(Tile.RenderState->RetrieveTileState(Tile.VirtualCoordinates).RenderPassIndex, Settings->GISamples - 1)) * GPreviewLightmapPhysicalTileSize * GPreviewLightmapPhysicalTileSize;
					}
				}
			}
		}

		bHaveFinishedSurfaceLightmaps = SamplesTaken == TotalSamples;

		{
			int32 NumCellsPerBrick = 5 * 5 * 5;
			SamplesTaken += VolumetricLightmapRenderer->SamplesTaken;
			TotalSamples += (uint64)VolumetricLightmapRenderer->NumTotalBricks * NumCellsPerBrick * Settings->GISamples * VolumetricLightmapRenderer->GetGISamplesMultiplier();
		}

		int32 IntPercentage = FMath::FloorToInt(SamplesTaken * 100.0 / TotalSamples);
		IntPercentage = FMath::Max(IntPercentage, 0);
		// With high number of samples (like 8192) double precision isn't enough to prevent fake 100%s
		IntPercentage = FMath::Min(IntPercentage, SamplesTaken < TotalSamples ? 99 : 100);

		FPlatformAtomics::InterlockedExchange(&Percentage, IntPercentage);
	}

	LightmapRenderer->BackgroundTick();

	// If we're in background baking mode, schedule VLM work to be after surface lightmaps
	bool bIsViewportNonRealtime = GCurrentLevelEditingViewportClient && !GCurrentLevelEditingViewportClient->IsRealtime();
	if (!bIsViewportNonRealtime || (bIsViewportNonRealtime && bHaveFinishedSurfaceLightmaps))
	{
		VolumetricLightmapRenderer->BackgroundTick();
	}
	*/
}



}

#undef LOCTEXT_NAMESPACE
