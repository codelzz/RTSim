// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Ԥ�� Lightmap ������Ƭ�ߴ�
static const int32 GPreviewLightmapVirtualTileSize = 64;
// Ԥ�� Lightmap ��Ƭ��Ե�ߴ�
static const int32 GPreviewLightmapTileBorderSize = 2;
// Ԥ�� Lightmap ������Ƭ��С
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

	// ���������ַʵ����
	FTileVirtualCoordinates(uint32 vAddress, uint8 vLevel) : Position((int32)FMath::ReverseMortonCode2(vAddress), (int32)FMath::ReverseMortonCode2(vAddress >> 1)), MipLevel(vLevel) {}
	// ��������ʵ����
	FTileVirtualCoordinates(FIntPoint InPosition, uint8 vLevel) : Position(InPosition), MipLevel(vLevel) {}
	// ������ת��Ϊ�����ַ
	uint32 GetVirtualAddress() const { return FMath::MortonCode2(Position.X) | (FMath::MortonCode2(Position.Y) << 1); }

	// == �������߼� 
	bool operator==(const FTileVirtualCoordinates& Rhs) const
	{
		return Position == Rhs.Position && MipLevel == Rhs.MipLevel;
	}
};

FORCEINLINE uint32 GetTypeHash(const FTileVirtualCoordinates& Tile)
{
	// ���� Hash
	return HashCombine(GetTypeHash(Tile.Position), GetTypeHash(Tile.MipLevel));
}
