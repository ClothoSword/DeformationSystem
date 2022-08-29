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

void FDeformationRenderResources::InitRHI()
{
	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	FRDGBuilder GraphBuilder(RHICmdList);

	if (!PersistentDepth0)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(RenderTargetResolution), PF_R8, FClearValueBinding::Black, TexCreate_None, TexCreate_UAV | TexCreate_ShaderResource | TexCreate_RenderTargetable, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, PersistentDepth0, TEXT("PersistentDepth0"));
	}
	if (!PersistentDepth1)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(RenderTargetResolution), PF_R8, FClearValueBinding::Black, TexCreate_None, TexCreate_UAV | TexCreate_ShaderResource | TexCreate_RenderTargetable, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, PersistentDepth1, TEXT("PersistentDepth1"));
	}

	if (!DeformNormalAndDepth)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(RenderTargetResolution), PF_A2B10G10R10, FClearValueBinding(FLinearColor(0.5, 0.5, 0, 1.0)), TexCreate_RenderTargetable, TexCreate_UAV, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, DeformNormalAndDepth, TEXT("DeformNormalAndDepth"));
	}

	if (!DepthRT)
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(RenderTargetResolution), PF_G16R16F, FClearValueBinding::None, TexCreate_None, TexCreate_ShaderResource, false);
		GRenderTargetPool.FindFreeElement(GraphBuilder.RHICmdList, Desc, DepthRT, TEXT("DepthRT"));
	}

	GraphBuilder.Execute();
}

void FDeformationRenderResources::ReleaseRHI()
{
	PersistentDepth0.SafeRelease();
	PersistentDepth1.SafeRelease();
	DeformNormalAndDepth.SafeRelease();
	DepthRT.SafeRelease();
}


bool FDeformationSceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext&) const
{
	return bActiveThisFrame;
}

void FDeformationSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	if (bActiveThisFrame)
	{
		if (bClearRT)
		{
			FRHIRenderPassInfo RPInfo(GDeformationRenderResources.PersistentDepth0->GetRHI(), ERenderTargetActions::Clear_Store);
			GraphBuilder.RHICmdList.BeginRenderPass(RPInfo, TEXT("Clear PersistentDepth0"));
			GraphBuilder.RHICmdList.EndRenderPass();

			RPInfo = FRHIRenderPassInfo(GDeformationRenderResources.PersistentDepth1->GetRHI(), ERenderTargetActions::Clear_Store);
			GraphBuilder.RHICmdList.BeginRenderPass(RPInfo, TEXT("Clear PersistentDepth1"));
			GraphBuilder.RHICmdList.EndRenderPass();

			bClearRT = false;
			bActiveThisFrame = false;
			return;
		}

		if (!InView.bIsSceneCapture)
			DeformationFilterPass(GraphBuilder, InView, this);
	}
}