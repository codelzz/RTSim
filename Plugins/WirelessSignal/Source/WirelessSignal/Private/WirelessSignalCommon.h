// Copyright Epic Games, Inc. All Rights Reserved.
// Modified from UE4 source code

#pragma once

#include "CoreMinimal.h"

// ������Ƭ��С
static const int32 GPreviewLightmapVirtualTileSize = 64;
// ��Ƭ��Ե���
static const int32 GPreviewLightmapTileBorderSize = 2;
// ������Ƭ��С
static const int32 GPreviewLightmapPhysicalTileSize = GPreviewLightmapVirtualTileSize + 2 * GPreviewLightmapTileBorderSize;
// ���β���
static const int32 GPreviewLightmapMipmapMaxLevel = 14;

// ��Ƭ�������� �ṹ��
struct FTileVirtualCoordinates
{
	// ����ֵ(int32 x, int32 y)
	FIntPoint Position{ EForceInit::ForceInitToZero };
	// �μ���
	int32 MipLevel = -1;

	FTileVirtualCoordinates() = default;

	// ���������ַʵ�����ṹ��
	FTileVirtualCoordinates(uint32 vAddress, uint8 vLevel) : 
		Position((int32)FMath::ReverseMortonCode2(vAddress), (int32)FMath::ReverseMortonCode2(vAddress >> 1)), 
		MipLevel(vLevel) {}
	// ����λ��ʵ�����ṹ��
	FTileVirtualCoordinates(FIntPoint InPosition, uint8 vLevel) : 
		Position(InPosition), 
		MipLevel(vLevel) {}
	// ��λ��ת��Ϊ�����ַ
	uint32 GetVirtualAddress() const { return FMath::MortonCode2(Position.X) | (FMath::MortonCode2(Position.Y) << 1); }
	
	// == �������߼� 
	bool operator==(const FTileVirtualCoordinates& Rhs) const
	{
		return Position == Rhs.Position && MipLevel == Rhs.MipLevel;
	}
};

// ����Hash
FORCEINLINE uint32 GetTypeHash(const FTileVirtualCoordinates& Tile)
{
	return HashCombine(GetTypeHash(Tile.Position), GetTypeHash(Tile.MipLevel));
}