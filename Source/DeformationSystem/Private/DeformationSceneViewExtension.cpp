// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeformationSceneViewExtension.h"
#include "RenderGraphBuilder.h"
#include "RHIDefinitions.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"

#include "DeformationFilter.h"
#include "DeformationSubsystem.h"

FDeformationSceneViewExtension::FDeformationSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld)
	: FWorldSceneViewExtension(AutoReg, InWorld)
{
}

FDeformationSceneViewExtension::~FDeformationSceneViewExtension()
{
}

TRefCountPtr<IPooledRenderTarget> CreateRenderTarget(FRDGBuilder& GraphBuilder, const FTexture2DRHIRef RenderTarget)
{
	const FTexture2DRHIRef& Texture = RenderTarget;
	ensure(Texture.IsValid());

	FSceneRenderTargetItem Item;
	Item.TargetableTexture = Texture;
	Item.ShaderResourceTexture = Texture;

	FPooledRenderTargetDesc Desc;

	if (Texture)
	{
		Desc.Extent = Texture->GetSizeXY();
		Desc.Format = Texture->GetFormat();
	}
	else
	{
		Desc.Extent = RenderTarget->GetSizeXY();
	}

	Desc.NumMips = 1;
	Desc.DebugName = *RenderTarget->GetName().ToString();
	Desc.Flags = TexCreate_UAV | TexCreate_ShaderResource;

	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	GRenderTargetPool.CreateUntrackedElement(Desc, PooledRenderTarget, Item);

	return PooledRenderTarget;
}

void FDeformationSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	if (!PersistentDepth0)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(DeformationPayload->InteractionTextureResolution), PF_R8, FClearValueBinding::Black, TexCreate_None, TexCreate_UAV | TexCreate_ShaderResource | TexCreate_RenderTargetable, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, PersistentDepth0, TEXT("PersistentDepth0"));

		GraphBuilder.RHICmdList.Transition(FRHITransitionInfo(PersistentDepth0->GetRHI(), ERHIAccess::Unknown, ERHIAccess::RTV));
		ClearRenderTarget(GraphBuilder.RHICmdList, PersistentDepth0->GetRHI());
		GraphBuilder.RHICmdList.Transition(FRHITransitionInfo(PersistentDepth0->GetRHI(), ERHIAccess::RTV, ERHIAccess::SRVMask));
	}
	if (!PersistentDepth1)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(DeformationPayload->InteractionTextureResolution), PF_R8, FClearValueBinding::Black, TexCreate_None, TexCreate_UAV | TexCreate_ShaderResource | TexCreate_RenderTargetable, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, PersistentDepth1, TEXT("PersistentDepth1"));

		GraphBuilder.RHICmdList.Transition(FRHITransitionInfo(PersistentDepth1->GetRHI(), ERHIAccess::Unknown, ERHIAccess::RTV));
		ClearRenderTarget(GraphBuilder.RHICmdList, PersistentDepth1->GetRHI());
		GraphBuilder.RHICmdList.Transition(FRHITransitionInfo(PersistentDepth1->GetRHI(), ERHIAccess::RTV, ERHIAccess::SRVMask));
	}

	if (!DeformNormalAndDepth)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(DeformationPayload->InteractionTextureResolution), PF_A2B10G10R10, FClearValueBinding(FLinearColor(0.5, 0.5, 0, 1.0)), TexCreate_None, TexCreate_UAV, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, DeformNormalAndDepth, TEXT("DeformNormalAndDepth"));
	}

	if (!DepthRT)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(DeformationPayload->InteractionTextureResolution), PF_G16R16F, FClearValueBinding::None, TexCreate_None, TexCreate_ShaderResource, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, DepthRT, TEXT("DepthRT"));
	}

	if (!InView.bIsSceneCapture)
	{
		DeformationFilterPass(GraphBuilder, InView, this);
	}
	else
	{
		if (FVector::Distance(InView.CullingOrigin, CaptureOrigin) < 0.01)
			InvDeviceZToWorldZTransform = InView.InvDeviceZToWorldZTransform;
	}
}