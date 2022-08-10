// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeformationSceneViewExtension.h"
#include "RenderGraphBuilder.h"
#include "RHIDefinitions.h"

#include "DeformationFilter.h"
#include "DeformationSubsystem.h"

FDeformationSceneViewExtension::FDeformationSceneViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld)
	: FWorldSceneViewExtension(AutoReg, InWorld)
{
}

FDeformationSceneViewExtension::~FDeformationSceneViewExtension()
{
}

void FDeformationSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	if (!InView.bIsSceneCapture)
	{
		DeformationFilterPass(GraphBuilder, InView, DeformationPayload);
	}
}