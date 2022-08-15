#include "DeformationFilter.h"
#include "DeformationSubsystem.h"
#include "DeformationSceneViewExtension.h"
#include "RenderGraphUtils.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"

class FDeformationFilterCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDeformationFilterCS);
	SHADER_USE_PARAMETER_STRUCT(FDeformationFilterCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, InteractionTextureResMinus1)
		SHADER_PARAMETER(float, RenderAreaScalar)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(float, PostDeltaTimes2)
		SHADER_PARAMETER(float, TemporalFilterFactor)
		SHADER_PARAMETER(float, SnowAddition)
		SHADER_PARAMETER(float, SnowAdditionHeightPrec)
		SHADER_PARAMETER(float, MaxSnowDepth)
		SHADER_PARAMETER(float, NonUniformExponent)
		SHADER_PARAMETER(uint32, TemporalFilterFlag)
		SHADER_PARAMETER(FVector2f, SampleOffset)
		SHADER_PARAMETER(FVector2f, InteractionCenterWS)
		SHADER_PARAMETER(FVector4f, InvDeviceZToWorldZTransform)
		SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, DepthRT)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, PrevDeformation)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, PersistanceTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, OutputTexture)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}
};

IMPLEMENT_GLOBAL_SHADER(FDeformationFilterCS, "/Plugins/DeformationSystem/Private/DeformationHeight.usf", "MainCS", SF_Compute);

void DeformationFilterPass(FRDGBuilder& GraphBuilder, const FSceneView& View, FDeformationSceneViewExtension* DeformationSceneViewExtension)
{
	FDeformationPayload* DeformationPayload = DeformationSceneViewExtension->DeformationPayload;
	FDeformationFilterCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FDeformationFilterCS::FParameters>();
	PassParameters->InteractionTextureResMinus1 = DeformationPayload->InteractionTextureResolution - 1;
	PassParameters->RenderAreaScalar = 1.0f;
	PassParameters->SnowAdditionHeightPrec = 0.0;//0.001;
	PassParameters->MaxSnowDepth = 110.f;
	PassParameters->DeltaTime = DeformationPayload->DeltaTime;
	PassParameters->PostDeltaTimes2 = DeformationPayload->PostDelta * 2.0f;
	PassParameters->TemporalFilterFactor = DeformationPayload->TemporalFilterFactor;
	PassParameters->SnowAddition = DeformationPayload->SnowAddition;
	PassParameters->TemporalFilterFlag = 0;
	//PassParameters->NonUniformExponent = -0.3;
	PassParameters->NonUniformExponent = 0.f;
	PassParameters->InteractionCenterWS = DeformationPayload->InteractionCenterWS;
	PassParameters->InvDeviceZToWorldZTransform = DeformationSceneViewExtension->InvDeviceZToWorldZTransform;

	GraphBuilder.RHICmdList.CopyTexture(DeformationPayload->DepthRT, DeformationSceneViewExtension->DepthRT->GetRenderTargetItem().TargetableTexture, {});
	FRDGTextureRef DepthRTTex = GraphBuilder.RegisterExternalTexture(DeformationSceneViewExtension->DepthRT);
	PassParameters->DepthRT = GraphBuilder.CreateSRV(DepthRTTex);

	FRDGTextureRef PersistentDepthTex0 = GraphBuilder.RegisterExternalTexture(DeformationSceneViewExtension->PersistentDepth0);
	FRDGTextureRef PersistentDepthTex1 = GraphBuilder.RegisterExternalTexture(DeformationSceneViewExtension->PersistentDepth1);

	if (DeformationPayload->ExcuteCounter % 2 == 0)
	{
		PassParameters->PrevDeformation =  GraphBuilder.CreateSRV(PersistentDepthTex0); 
		PassParameters->PersistanceTexture = GraphBuilder.CreateUAV(PersistentDepthTex1);
	}
	else
	{
		PassParameters->PrevDeformation = GraphBuilder.CreateSRV(PersistentDepthTex1);
		PassParameters->PersistanceTexture = GraphBuilder.CreateUAV(PersistentDepthTex0);
	}

	FRDGTextureRef DeformNormalAndHeightTex = GraphBuilder.RegisterExternalTexture(DeformationSceneViewExtension->DeformNormalAndHeight);
	FRDGTextureUAVRef OutputUAV = GraphBuilder.CreateUAV(DeformNormalAndHeightTex);
	PassParameters->OutputTexture = OutputUAV;

	PassParameters->SampleOffset = DeformationPayload->SampleOffset;
	PassParameters->DepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(View.FeatureLevel);
	TShaderMapRef<FDeformationFilterCS> ComputeShader(GlobalShaderMap);

	ValidateShaderParameters(ComputeShader, *PassParameters);
	ClearUnusedGraphResources(ComputeShader, PassParameters);

	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("DeformationHeightFilterCS"), ComputeShader, PassParameters, FComputeShaderUtils::GetGroupCount(FIntPoint(DeformationPayload->InteractionTextureResolution), FIntPoint(16)));

	DeformationPayload->ExcuteCounter++;
}
