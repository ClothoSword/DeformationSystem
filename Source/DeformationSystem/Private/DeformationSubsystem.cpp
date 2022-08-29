// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeformationSubsystem.h"
#include "DeformationSceneViewExtension.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "GameFramework/Character.h"

static float GDeformationTickThreshold = 0.05f;
static FAutoConsoleVariableRef CVarDeformationTickThreshold(
	TEXT("DeformationSystem.DeformationTickThreshold"),
	GDeformationTickThreshold,
	TEXT("Defromation subssystem may execute, when timer is greater than the threshold")
);

static TAutoConsoleVariable<int32> CVarDeformationSystem(
	TEXT("DeformationSystem.Enable"),
	1,
	TEXT("0: Disable\n")
	TEXT("1: Enable (default)"),
	ECVF_Default
);

bool UDeformationSubsystem::IsTickable() const
{
	return !!CVarDeformationSystem.GetValueOnGameThread();
}

void UDeformationSubsystem::Tick(float DeltaTime)
{
	FVector CharacterLocation = FVector::ZeroVector;
	FVector SceneCaptureLocation = FVector::ZeroVector;

	if (!CharacterPawn)
	{
		CharacterPawn = Cast<APawn>(UGameplayStatics::GetPlayerCharacter(CachedWorld, 0));
		if (!CharacterPawn)
		{
			CharacterPawn = UGameplayStatics::GetPlayerPawn(CachedWorld, 0);
			if (!CharacterPawn)
				return;
		}

		CharacterLocation = CharacterPawn->GetActorLocation();
		DeformationActor = GetWorld()->SpawnActor<AActor>();
		DepthCaptureComponent = NewObject<USceneCaptureComponent2D>(DeformationActor, TEXT("DepthSceneCapture"));
		DeformationActor->SetActorTransform(FTransform());
		DeformationActor->SetRootComponent(DepthCaptureComponent);
		DeformationActor->SetActorLocation(FVector(CharacterLocation.X, CharacterLocation.Y, CharacterLocation.Z - 5000));
		DeformationActor->SetActorRotation(FRotator::MakeFromEuler(FVector(0.0f, 90.0f, 90.0f)));

		DepthCaptureComponent->ShowFlags.SetLensFlares(false);
		DepthCaptureComponent->ShowFlags.SetOnScreenDebug(false);
		DepthCaptureComponent->ShowFlags.SetEyeAdaptation(false);
		DepthCaptureComponent->ShowFlags.SetColorGrading(false);
		DepthCaptureComponent->ShowFlags.SetCameraImperfections(false);
		DepthCaptureComponent->ShowFlags.SetDepthOfField(false);
		DepthCaptureComponent->ShowFlags.SetVignette(false);
		DepthCaptureComponent->ShowFlags.SetGrain(false);
		DepthCaptureComponent->ShowFlags.SetSeparateTranslucency(false);
		DepthCaptureComponent->ShowFlags.SetScreenPercentage(false);
		DepthCaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
		DepthCaptureComponent->ShowFlags.SetTemporalAA(false);
		DepthCaptureComponent->ShowFlags.SetAmbientOcclusion(false);
		DepthCaptureComponent->ShowFlags.SetIndirectLightingCache(false);
		DepthCaptureComponent->ShowFlags.SetLightShafts(false);
		DepthCaptureComponent->ShowFlags.SetHighResScreenshotMask(false);
		DepthCaptureComponent->ShowFlags.SetHMDDistortion(false);
		DepthCaptureComponent->ShowFlags.SetStereoRendering(false);
		DepthCaptureComponent->ShowFlags.SetDistanceFieldAO(false);
		DepthCaptureComponent->ShowFlags.SetVolumetricFog(false);
		DepthCaptureComponent->ShowFlags.SetVolumetricLightmap(false);
		DepthCaptureComponent->ShowFlags.SetLumenGlobalIllumination(false);
		DepthCaptureComponent->ShowFlags.SetLumenReflections(false);
		DepthCaptureComponent->ShowFlags.SetLighting(false);
		DepthCaptureComponent->ShowFlags.SetDynamicShadows(false);
		DepthCaptureComponent->ShowFlags.SetVirtualShadowMapCaching(false);
		DepthCaptureComponent->ShowFlags.SetShaderPrint(false);
		DepthCaptureComponent->ShowFlags.SetTranslucency(false);
		DepthCaptureComponent->ShowFlags.SetBloom(false);
		DepthCaptureComponent->ShowFlags.SetTonemapper(false);
		DepthCaptureComponent->ShowFlags.SetAntiAliasing(false);
		DepthCaptureComponent->ShowFlags.SetFog(false);
		DepthCaptureComponent->ShowFlags.SetAtmosphere(false);

		DepthCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		DepthCaptureComponent->OrthoWidth = SceneCaptureSizeWS;
		DepthCaptureComponent->TextureTarget = DepthRT;
		DepthCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		DepthCaptureComponent->bCaptureEveryFrame = false;
		DepthCaptureComponent->bCaptureOnMovement = false;
		DepthCaptureComponent->bAlwaysPersistRenderingState = true;
		if (SceneCaptureMaterial)
		{
			DepthCaptureComponent->PostProcessSettings.WeightedBlendables.Array.SetNumZeroed(1);
			DepthCaptureComponent->PostProcessSettings.WeightedBlendables.Array[0].Weight = 1.0f;
			DepthCaptureComponent->PostProcessSettings.WeightedBlendables.Array[0].Object = SceneCaptureMaterial;
		}

		return;
	}

	TickTimer += DeltaTime;

	if (!DeformationSceneViewExtension->bActiveThisFrame && TickTimer > GDeformationTickThreshold)
	{
		TickTimer = 0;
		CharacterLocation = CharacterPawn->GetActorLocation();
		SceneCaptureLocation = DeformationActor->GetActorLocation();
		FVector2f CurrentLocation = FVector2f(CharacterLocation.X, CharacterLocation.Y);
		SampleOffset = FVector2f(FMath::Floor(CurrentLocation.X / PostDeltaWS) + 0.5, FMath::Floor(CurrentLocation.Y / PostDeltaWS) + 0.5) * PostDeltaWS - FVector2f(SceneCaptureLocation.X, SceneCaptureLocation.Y);
		DeformationActor->AddActorWorldOffset(FVector(SampleOffset.X, SampleOffset.Y, 0.0f));
		SceneCaptureLocation = DeformationActor->GetActorLocation();

		if (DeformationMPC)
		{
			UKismetMaterialLibrary::SetVectorParameterValue(CachedWorld, DeformationMPC, TEXT("SceneCaptureLocation"), FLinearColor(SceneCaptureLocation));
			UKismetMaterialLibrary::SetScalarParameterValue(CachedWorld, DeformationMPC, TEXT("SceneCaptureSizeWS"), SceneCaptureSizeWS);
			ExtrudedHeight = UKismetMaterialLibrary::GetScalarParameterValue(CachedWorld, DeformationMPC, TEXT("ExtrudedHeight"));
		}

		DeformationSceneViewExtension->DeformationPayload->ExtrudedHeight = ExtrudedHeight;
		DeformationSceneViewExtension->DeformationPayload->SampleOffset = SampleOffset;
		DeformationSceneViewExtension->DeformationPayload->DeltaTime = DeltaTime;
		DeformationSceneViewExtension->DeformationPayload->InteractionCenterWS = CurrentLocation;
		DepthCaptureComponent->CaptureScene();
		DeformationSceneViewExtension->bActiveThisFrame = true;
	}
}

TStatId UDeformationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDeformationSubsystem, STATGROUP_Tickables);
}

bool UDeformationSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::Editor || WorldType == EWorldType::PIE;
}

void UDeformationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();
	check(World != nullptr);

	DeformationSceneViewExtension = FSceneViewExtensions::NewExtension<FDeformationSceneViewExtension>(World);
	CachedWorld = World;

	if (!DeformationSceneViewExtension->DeformationPayload)
		DeformationSceneViewExtension->DeformationPayload = new FDeformationPayload;

	DepthRT = NewObject<UTextureRenderTarget2D>(this, TEXT("DepthRT"));
	DepthRT->RenderTargetFormat = RTF_RG16f;
	DepthRT->InitAutoFormat(DepthRTResolution, DepthRTResolution);
	DepthRT->UpdateResourceImmediate(true);

	UKismetRenderingLibrary::ClearRenderTarget2D(CachedWorld, DeformNormalDepthTexture, FLinearColor(0.5, 0.5, 0, 1));

	ENQUEUE_RENDER_COMMAND(EndCaptureCommand)([&](FRHICommandListImmediate& RHICommandList)
	{
		DeformationSceneViewExtension->DeformationPayload->InteractionTextureResolution = DepthRTResolution;
		DeformationSceneViewExtension->DeformationPayload->PostDelta = PostDeltaWS;
		DeformationSceneViewExtension->DeformationPayload->TemporalFilterFactor = 2.0;
		DeformationSceneViewExtension->DeformationPayload->SnowAddition = 0.0;
		DeformationSceneViewExtension->DeformationPayload->ExtrudedHeight = ExtrudedHeight;
		DeformationSceneViewExtension->DeformationPayload->DepthRT = DepthRT->GetRenderTargetResource()->GetRenderTargetTexture();
		DeformationSceneViewExtension->DeformationPayload->DeformNormalAndDepth = DeformNormalDepthTexture ? DeformNormalDepthTexture->GetRenderTargetResource()->GetRenderTargetTexture() : nullptr;
		DeformationSceneViewExtension->bActiveThisFrame = true;
		DeformationSceneViewExtension->bClearRT = true;
	});
}

void UDeformationSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (DeformationSceneViewExtension->DeformationPayload)
		delete DeformationSceneViewExtension->DeformationPayload;
}

UDeformationSubsystem::UDeformationSubsystem()
{
	SceneCaptureSizeWS = 4096;
	DepthRTResolution = 1024;
	ExtrudedHeight = 0.0f;
	PostDeltaWS = (float)SceneCaptureSizeWS / DepthRTResolution;
	SampleOffset = FVector2f::Zero();
	CachedWorld = nullptr;
	TickTimer = 0.0f;

	SceneCaptureMaterial = LoadObject<UMaterial>(NULL, TEXT("/DeformationSystem/M_SceneCapturePP"), NULL, LOAD_None, NULL);
	DeformationMPC = LoadObject<UMaterialParameterCollection>(NULL, TEXT("/DeformationSystem/MPC_Deformation"), NULL, LOAD_None, NULL);
	DeformNormalDepthTexture = LoadObject<UTextureRenderTarget2D>(NULL, TEXT("/DeformationSystem/RT_NormalAndDepth"), NULL, LOAD_None, NULL);
	if (DeformNormalDepthTexture)
	{
		DeformNormalDepthTexture->RenderTargetFormat = RTF_RGB10A2;
		DeformNormalDepthTexture->bCanCreateUAV = true;
		DeformNormalDepthTexture->InitAutoFormat(DepthRTResolution, DepthRTResolution);
		DeformNormalDepthTexture->UpdateResourceImmediate(false);
	}
}