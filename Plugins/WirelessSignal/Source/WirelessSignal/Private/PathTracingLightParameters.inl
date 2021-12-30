// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RayTracingDefinitions.h"
#include "PathTracingDefinitions.h"

RENDERER_API FRDGTexture* PrepareIESAtlas(const TMap<FTexture*, int>& InIESLightProfilesMap, FRDGBuilder& GraphBuilder);

RENDERER_API void PrepareLightGrid(FRDGBuilder& GraphBuilder, FPathTracingLightGrid* LightGridParameters, const FPathTracingLight* Lights, uint32 NumLights, uint32 NumInfiniteLights, FRDGBufferSRV* LightsSRV);

template<typename PassParameterType>
void SetupPathTracingLightParameters(
	const WirelessSignal::FLightSceneRenderState& LightScene,
	FRDGBuilder& GraphBuilder,
	PassParameterType* PassParameters)
{
	TArray<FPathTracingLight> Lights;

	// sky light is out of scope 
	{
		PassParameters->SkylightTexture = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
		PassParameters->SkylightTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->SkylightPdf = GraphBuilder.RegisterExternalTexture(GSystemTextures.BlackDummy);
		PassParameters->SkylightInvResolution = 0;
		PassParameters->SkylightMipCount = 0;
	}


	for (auto Light : LightScene.DirectionalLights.Elements)
	{
		FPathTracingLight& DestLight = Lights.AddDefaulted_GetRef();

		DestLight.Normal = -Light.Direction;
		DestLight.Color = FVector(Light.Color);
		DestLight.Dimensions = FVector(
			FMath::Sin(0.5f * FMath::DegreesToRadians(Light.LightSourceAngle)),
			FMath::Sin(0.5f * FMath::DegreesToRadians(Light.LightSourceSoftAngle)),
			0.0f);
		DestLight.Attenuation = 1.0;
		DestLight.IESTextureSlice = -1;

		DestLight.Flags = PATHTRACER_FLAG_TRANSMISSION_MASK;
		DestLight.Flags |= PATHTRACER_FLAG_LIGHTING_CHANNEL_MASK;
		DestLight.Flags |= PATHTRACER_FLAG_CAST_SHADOW_MASK;
		DestLight.Flags |= Light.bStationary ? PATHTRACER_FLAG_STATIONARY_MASK : 0;
		DestLight.Flags |= PATHTRACING_LIGHT_DIRECTIONAL;
	}

	uint32 NumInfiniteLights = Lights.Num();

	TMap<FTexture*, int> IESLightProfilesMap;

	for (auto Light : LightScene.PointLights.Elements)
	{
		FPathTracingLight& DestLight = Lights.AddDefaulted_GetRef();

		DestLight.Position = Light.Position;
		DestLight.Color = FVector(Light.Color);
		DestLight.Normal = FVector(1, 0, 0);
		DestLight.dPdu = FVector::CrossProduct(Light.Tangent, Light.Direction);
		DestLight.dPdv = Light.Tangent;

		DestLight.Dimensions = FVector(Light.SourceRadius, Light.SourceSoftRadius, Light.SourceLength);
		DestLight.Attenuation = 1.0f / Light.AttenuationRadius;
		DestLight.FalloffExponent = Light.FalloffExponent;

		if (Light.IESTexture)
		{
			DestLight.IESTextureSlice = IESLightProfilesMap.FindOrAdd(Light.IESTexture, IESLightProfilesMap.Num());
		}
		else
		{
			DestLight.IESTextureSlice = INDEX_NONE;
		}

		DestLight.Flags = PATHTRACER_FLAG_TRANSMISSION_MASK;
		DestLight.Flags |= PATHTRACER_FLAG_LIGHTING_CHANNEL_MASK;
		DestLight.Flags |= PATHTRACER_FLAG_CAST_SHADOW_MASK;
		DestLight.Flags |= Light.IsInverseSquared ? 0 : PATHTRACER_FLAG_NON_INVERSE_SQUARE_FALLOFF_MASK;
		DestLight.Flags |= Light.bStationary ? PATHTRACER_FLAG_STATIONARY_MASK : 0;
		DestLight.Flags |= PATHTRACING_LIGHT_POINT;

		float Radius = Light.AttenuationRadius;
		FVector Center = DestLight.Position;
		// simple sphere of influence
		DestLight.BoundMin = Center - FVector(Radius, Radius, Radius);
		DestLight.BoundMax = Center + FVector(Radius, Radius, Radius);
	}

	PassParameters->SceneLightCount = Lights.Num();
	{
		// Upload the buffer of lights to the GPU
		// need at least one since zero-sized buffers are not allowed
		if (Lights.Num() == 0)
		{
			Lights.AddDefaulted();
		}
		PassParameters->SceneLights = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(CreateStructuredBuffer(GraphBuilder, TEXT("PathTracer.LightsBuffer"), sizeof(FPathTracingLight), Lights.Num(), Lights.GetData(), sizeof(FPathTracingLight) * Lights.Num())));
	}

	if (IESLightProfilesMap.Num() > 0)
	{
		PassParameters->IESTexture = PrepareIESAtlas(IESLightProfilesMap, GraphBuilder);
	}
	else
	{
		PassParameters->IESTexture = GraphBuilder.RegisterExternalTexture(GSystemTextures.WhiteDummy);
	}

	PrepareLightGrid(GraphBuilder, &PassParameters->LightGridParameters, Lights.GetData(), PassParameters->SceneLightCount, NumInfiniteLights, PassParameters->SceneLights);
}
