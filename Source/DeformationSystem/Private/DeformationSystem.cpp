// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeformationSystem.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FDeformationSystemModule"

void FDeformationSystemModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("DeformationSystem"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugins/DeformationSystem"), PluginShaderDir);
}

void FDeformationSystemModule::ShutdownModule()
{}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDeformationSystemModule, DeformationSystem)