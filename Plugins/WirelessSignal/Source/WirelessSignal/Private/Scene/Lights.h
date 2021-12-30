// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EntityArray.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "GeometryInterface.h"
#include "Engine/MapBuildDataRegistry.h"

struct FLightShaderConstants
{
	FVector Position;
	float InvRadius;
	FVector Color;
	float FalloffExponent;
	FVector   Direction;
	float     SpecularScale;
	FVector   Tangent;
	float     SourceRadius;
	FVector2D   SpotAngles;
	float      SoftSourceRadius;
	float      SourceLength;
	float      RectLightBarnCosAngle;
	float      RectLightBarnLength;

	FLightShaderConstants() = default;

	FLightShaderConstants(const FLightShaderParameters& LightShaderParameters)
	{
		Position = LightShaderParameters.Position;
		InvRadius = LightShaderParameters.InvRadius;
		Color = LightShaderParameters.Color;
		FalloffExponent = LightShaderParameters.FalloffExponent;
		Direction = LightShaderParameters.Direction;
		SpecularScale = LightShaderParameters.SpecularScale;
		Tangent = LightShaderParameters.Tangent;
		SourceRadius = LightShaderParameters.SourceRadius;
		SpotAngles = LightShaderParameters.SpotAngles;
		SoftSourceRadius = LightShaderParameters.SoftSourceRadius;
		SourceLength = LightShaderParameters.SourceLength;
		RectLightBarnCosAngle = LightShaderParameters.RectLightBarnCosAngle;
		RectLightBarnLength = LightShaderParameters.RectLightBarnLength;
	}
};

namespace WirelessSignal
{

/**
 * BuildInfo store extra game thread data for WirelessSignal's internal usage beyond the component
 * RenderState is WirelessSignal's equivalent for scene proxies
 */

struct FLightSceneRenderState;

struct FLocalLightBuildInfo
{
	FLocalLightBuildInfo() = default;
	virtual ~FLocalLightBuildInfo() {}
	FLocalLightBuildInfo(FLocalLightBuildInfo&& In) = default; // Default move constructor is killed by the custom destructor

	bool bStationary = false;
	int ShadowMapChannel = INDEX_NONE;

	TUniquePtr<FLightComponentMapBuildData> LightComponentMapBuildData;

	virtual bool AffectsBounds(const FBoxSphereBounds& InBounds) const = 0;
	virtual ULightComponent* GetComponentUObject() const = 0;
};

struct FLocalLightRenderState
{
	virtual ~FLocalLightRenderState() {}

	bool bStationary = false;
	int ShadowMapChannel = INDEX_NONE;

	virtual FLightShaderParameters GetLightShaderParameters() const = 0;
};

class FLightArrayBase;

class FLightBuildInfoRef : public FGenericEntityRef
{
public:
	FLightBuildInfoRef(
		FLightArrayBase& LightArray,
		TArray<TSet<RefAddr>>& Refs,
		TSparseArray<int32>& RefAllocator,
		int32 ElementId)
		: FGenericEntityRef(ElementId, Refs, RefAllocator)
		, LightArray(LightArray)
	{}

	void RemoveFromAray();
	FLocalLightBuildInfo& Resolve();

private:
	FLightArrayBase& LightArray;

	template<typename T>
	friend class TLightArray;
};

class FLightRenderStateArrayBase;

class FLightRenderStateRef : public FGenericEntityRef
{
public:
	FLightRenderStateRef(
		FLightRenderStateArrayBase& LightArray,
		TArray<TSet<RefAddr>>& Refs,
		TSparseArray<int32>& RefAllocator,
		int32 ElementId)
		: FGenericEntityRef(ElementId, Refs, RefAllocator)
		, LightRenderStateArray(LightArray)
	{}

	FLightRenderStateRef(FLightRenderStateArrayBase& LightArray, const FGenericEntityRef& InRef)
		: FGenericEntityRef(InRef)
		, LightRenderStateArray(LightArray)
	{}

	FLocalLightRenderState& Resolve();

private:
	FLightRenderStateArrayBase& LightRenderStateArray;

	template<typename T>
	friend class TLightRenderStateArray;
};

struct FDirectionalLightBuildInfo : public FLocalLightBuildInfo
{
	FDirectionalLightBuildInfo(UDirectionalLightComponent* DirectionalLightComponent);

	UDirectionalLightComponent* ComponentUObject = nullptr;

	virtual ULightComponent* GetComponentUObject() const override { return ComponentUObject; }
	virtual bool AffectsBounds(const FBoxSphereBounds& InBounds) const override { return true; }
};

using FDirectionalLightRef = TEntityArray<FDirectionalLightBuildInfo>::EntityRefType;

struct FPointLightBuildInfo : public FLocalLightBuildInfo
{
	FPointLightBuildInfo(UPointLightComponent* ComponentUObject);

	UPointLightComponent* ComponentUObject = nullptr;

	FVector Position;
	float AttenuationRadius;

	virtual ULightComponent* GetComponentUObject() const override { return ComponentUObject; }
	virtual bool AffectsBounds(const FBoxSphereBounds& InBounds) const override;
};

using FPointLightRef = TEntityArray<FPointLightBuildInfo>::EntityRefType;

struct FDirectionalLightRenderState : public FLocalLightRenderState
{
	FDirectionalLightRenderState(UDirectionalLightComponent* DirectionalLightComponent);

	FVector Direction;
	FLinearColor Color;
	float LightSourceAngle;
	float LightSourceSoftAngle;

	virtual FLightShaderParameters GetLightShaderParameters() const override;
};

using FDirectionalLightRenderStateRef = TEntityArray<FDirectionalLightRenderState>::EntityRefType;

struct FPointLightRenderState : public FLocalLightRenderState
{
	FPointLightRenderState(UPointLightComponent* PointLightComponent);

	FVector Position;
	FVector Direction;
	FVector Tangent;
	FLinearColor Color;
	float AttenuationRadius;
	float SourceRadius;
	float SourceSoftRadius;
	float SourceLength;
	float FalloffExponent;
	bool IsInverseSquared;
	FTexture* IESTexture;

	virtual FLightShaderParameters GetLightShaderParameters() const override;
};

using FPointLightRenderStateRef = TEntityArray<FPointLightRenderState>::EntityRefType;

class FLightArrayBase
{
public:
	virtual ~FLightArrayBase() {}
	virtual void Remove(const FLightBuildInfoRef& Light) = 0;
	virtual FLocalLightBuildInfo& ResolveAsLocalLightBuildInfo(const FLightBuildInfoRef& Light) = 0;
};

template<typename T>
class TLightArray : public FLightArrayBase, public TEntityArray<T>
{
public:
	virtual void Remove(const FLightBuildInfoRef& Light) override
	{
		check(&Light.LightArray == (FLightArrayBase*)this);

		this->RemoveAt(Light.GetElementIdChecked());
	}

	virtual FLocalLightBuildInfo& ResolveAsLocalLightBuildInfo(const FLightBuildInfoRef& Light) override
	{
		check(&Light.LightArray == (FLightArrayBase*)this);

		return this->Elements[Light.GetElementIdChecked()];
	}
};

class FLightRenderStateArrayBase
{
public:
	virtual ~FLightRenderStateArrayBase() {}
	virtual FLocalLightRenderState& ResolveAsLocalLightRenderState(const FLightRenderStateRef& Light) = 0;
};

template<typename T>
class TLightRenderStateArray : public FLightRenderStateArrayBase, public TEntityArray<T>
{
public:
	virtual FLocalLightRenderState& ResolveAsLocalLightRenderState(const FLightRenderStateRef& Light) override
	{
		check(&Light.LightRenderStateArray == (FLightRenderStateArrayBase*)this);

		return this->Elements[Light.GetElementIdChecked()];
	}
};

struct FLightScene
{
	TLightArray<FDirectionalLightBuildInfo> DirectionalLights;
	TLightArray<FPointLightBuildInfo> PointLights;

	TMap<UDirectionalLightComponent*, FDirectionalLightRef> RegisteredDirectionalLightComponentUObjects;
	TMap<UPointLightComponent*, FPointLightRef> RegisteredPointLightComponentUObjects;
};

struct FLightSceneRenderState
{
	TLightRenderStateArray<FDirectionalLightRenderState> DirectionalLights;
	TLightRenderStateArray<FPointLightRenderState> PointLights;
};

}

static uint32 GetTypeHash(const WirelessSignal::FDirectionalLightRenderState& O)
{
	return HashCombine(GetTypeHash(O.ShadowMapChannel), HashCombine(GetTypeHash(O.LightSourceAngle), HashCombine(GetTypeHash(O.Color), HashCombine(GetTypeHash(O.Direction), GetTypeHash(O.bStationary)))));
}

static uint32 GetTypeHash(const WirelessSignal::FDirectionalLightRenderStateRef& Ref)
{
	return GetTypeHash(Ref.GetReference_Unsafe());
}

static uint32 GetTypeHash(const WirelessSignal::FPointLightRenderState& O)
{
	return HashCombine(GetTypeHash(O.AttenuationRadius), HashCombine(GetTypeHash(O.ShadowMapChannel), HashCombine(GetTypeHash(O.SourceRadius), HashCombine(GetTypeHash(O.Color), HashCombine(GetTypeHash(O.Position), GetTypeHash(O.bStationary))))));
}

static uint32 GetTypeHash(const WirelessSignal::FPointLightRenderStateRef& Ref)
{
	return GetTypeHash(Ref.GetReference_Unsafe());
}

