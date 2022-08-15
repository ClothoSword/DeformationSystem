// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RendererInterface.h"
#include "RenderGraphResources.h"

void DeformationFilterPass(FRDGBuilder& GraphBuilder, const FSceneView& View, class FDeformationSceneViewExtension* DeformationSceneViewExtension);