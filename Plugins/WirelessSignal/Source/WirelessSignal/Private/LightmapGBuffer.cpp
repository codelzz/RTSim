// Copyright Epic Games, Inc. All Rights Reserved.

#include "LightmapGBuffer.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FLightmapGBufferParams, "LightmapGBufferParams");

IMPLEMENT_MATERIAL_SHADER_TYPE(, FLightmapGBufferVS, TEXT("/Plugin/WirelessSignal/Private/LightmapGBuffer.usf"), TEXT("LightmapGBufferVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FLightmapGBufferPS, TEXT("/Plugin/WirelessSignal/Private/LightmapGBuffer.usf"), TEXT("LightmapGBufferPS"), SF_Pixel);
