// Copyright Epic Games, Inc. All Rights Reserved.

#include "GeometryInterface.h"
#include "MeshBatch.h"

namespace WirelessSignal
{

TArray<FMeshBatch> FGeometryInstanceRenderStateRef::GetMeshBatchesForGBufferRendering(FTileVirtualCoordinates CoordsForCulling)
{
	return Collection.GetMeshBatchesForGBufferRendering(*this, CoordsForCulling);
}

}