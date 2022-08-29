// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class FDeformationRenderResources : public FRenderResource
{
public:
	FDeformationRenderResources()
		: RenderTargetResolution(1024)
	{}

	FDeformationRenderResources(uint32 InRenderTargetResolution)
		: RenderTargetResolution(InRenderTargetResolution)
	{}

	//Begin FRenderResource Interface.
	virtual void InitRHI() override;
	virtual void ReleaseRHI() override;
	//End FRenderResource Interface.

	TRefCountPtr<IPooledRenderTarget> PersistentDepth0 = nullptr;
	TRefCountPtr<IPooledRenderTarget> PersistentDepth1 = nullptr;
	TRefCountPtr<IPooledRenderTarget> DepthRT = nullptr;
	TRefCountPtr<IPooledRenderTarget> DeformNormalAndDepth = nullptr;

private:
	uint32 RenderTargetResolution;
};

static TGlobalResource<FDeformationRenderResources> GDeformationRenderResources;

class FDeformationSceneViewExtension : public FWorldSceneViewExtension
{
public:
	FDeformationSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld);
	~FDeformationSceneViewExtension();

	// FSceneViewExtensionBase implementation : 
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;

	bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext&) const;
	// End FSceneViewExtensionBase implementation

	struct FDeformationPayload* DeformationPayload = nullptr;

	bool bActiveThisFrame = false;
	bool bClearRT = false;

	FVector CaptureOrigin = FVector::ZeroVector;
};
