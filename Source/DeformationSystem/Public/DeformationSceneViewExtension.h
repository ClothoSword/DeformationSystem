// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

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
	// End FSceneViewExtensionBase implementation

	struct FDeformationPayload* DeformationPayload = nullptr;

	TRefCountPtr<IPooledRenderTarget> PersistentDepth0 = nullptr;
	TRefCountPtr<IPooledRenderTarget> PersistentDepth1 = nullptr;
	TRefCountPtr<IPooledRenderTarget> DepthRT = nullptr;
	TRefCountPtr<IPooledRenderTarget> DeformNormalAndHeight = nullptr;

	FVector4f InvDeviceZToWorldZTransform;

	FVector CaptureOrigin = FVector::ZeroVector;

	//FRDGTextureRef PersistentDepth0 = nullptr;
	//FRDGTextureRef PersistentDepth1 = nullptr;
};
