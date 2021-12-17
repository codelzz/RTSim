// Copyright Epic Games, Inc. All Rights Reserved.
// Modified from UE4 source code

#pragma once

#include "CoreMinimal.h"

// 虚拟瓦片大小
static const int32 GPreviewLightmapVirtualTileSize = 64;
// 瓦片边缘宽度
static const int32 GPreviewLightmapTileBorderSize = 2;
// 物理瓦片大小
static const int32 GPreviewLightmapPhysicalTileSize = GPreviewLightmapVirtualTileSize + 2 * GPreviewLightmapTileBorderSize;
// 最大次层数
static const int32 GPreviewLightmapMipmapMaxLevel = 14;

// 瓦片虚拟坐标 结构体
struct FTileVirtualCoordinates
{
	// 坐标值(int32 x, int32 y)
	FIntPoint Position{ EForceInit::ForceInitToZero };
	// 次级层
	int32 MipLevel = -1;

	FTileVirtualCoordinates() = default;

	// 利用虚拟地址实例化结构体
	FTileVirtualCoordinates(uint32 vAddress, uint8 vLevel) : 
		Position((int32)FMath::ReverseMortonCode2(vAddress), (int32)FMath::ReverseMortonCode2(vAddress >> 1)), 
		MipLevel(vLevel) {}
	// 利用位置实例化结构体
	FTileVirtualCoordinates(FIntPoint InPosition, uint8 vLevel) : 
		Position(InPosition), 
		MipLevel(vLevel) {}
	// 将位置转化为虚拟地址
	uint32 GetVirtualAddress() const { return FMath::MortonCode2(Position.X) | (FMath::MortonCode2(Position.Y) << 1); }
	
	// == 操作符逻辑 
	bool operator==(const FTileVirtualCoordinates& Rhs) const
	{
		return Position == Rhs.Position && MipLevel == Rhs.MipLevel;
	}
};

// 计算Hash
FORCEINLINE uint32 GetTypeHash(const FTileVirtualCoordinates& Tile)
{
	return HashCombine(GetTypeHash(Tile.Position), GetTypeHash(Tile.MipLevel));
}