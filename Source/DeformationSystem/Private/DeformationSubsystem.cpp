// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeformationSubsystem.h"
#include "DeformationSceneViewExtension.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

void UDeformationSubsystem::Tick(float DeltaTime)
{
	FVector PawnLocation = FVector::ZeroVector;
	if (!PlayerPawn)
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		PawnLocation = PlayerPawn->GetActorLocation();
		DepthCaptureComponent->GetOwner()->SetActorLocation(FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - 500));
		DepthCaptureComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
		DepthCaptureComponent->CaptureScene();

		return;
	}
	PawnLocation = PlayerPawn->GetActorLocation();
	FVector2f CurrentLocation = FVector2f(PawnLocation.X, PawnLocation.Y);
	SampleOffset = FVector2f(FMath::Floor(CurrentLocation.X / PostDeltaWS), FMath::Floor(CurrentLocation.Y / PostDeltaWS)) - CurrentLocation;

	DeformationSceneViewExtension->DeformationPayload->SampleOffset = SampleOffset;
	DeformationSceneViewExtension->DeformationPayload->DeltaTime = DeltaTime;
	DeformationSceneViewExtension->DeformationPayload->InteractionCenterWS = CurrentLocation;

	DepthCaptureComponent->GetOwner()->AddActorWorldOffset(FVector(SampleOffset.X, SampleOffset.Y, 0.0f));
	DepthCaptureComponent->CaptureScene();
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

	DepthRT = NewObject<UTextureRenderTarget2D>(this, TEXT("DepthRT"));
	DepthRT->RenderTargetFormat = RTF_RG16f;
	DepthRT->InitAutoFormat(DepthRTResolution, DepthRTResolution);
	DepthRT->UpdateResourceImmediate(true);

	PersistentDepth0 = NewObject<UTextureRenderTarget2D>(this, TEXT("PersistentDepth0"));
	PersistentDepth0->RenderTargetFormat = RTF_R16f;
	PersistentDepth0->InitAutoFormat(DepthRTResolution, DepthRTResolution);
	PersistentDepth0->UpdateResourceImmediate(true);

	PersistentDepth1 = NewObject<UTextureRenderTarget2D>(this, TEXT("PersistentDepth1"));
	PersistentDepth1->RenderTargetFormat = RTF_R16f;
	PersistentDepth1->InitAutoFormat(DepthRTResolution, DepthRTResolution);
	PersistentDepth1->UpdateResourceImmediate(true);

	DeformNormalHeightTexture = NewObject<UTextureRenderTarget2D>(this, TEXT("DeformNormalHeightTexture"));
	DeformNormalHeightTexture->RenderTargetFormat = RTF_RGB10A2;
	DeformNormalHeightTexture->InitAutoFormat(DepthRTResolution, DepthRTResolution);
	DeformNormalHeightTexture->UpdateResourceImmediate(true);

	AActor* TempActor = NewObject<AActor>(World->GetCurrentLevel());
	TempActor->SetActorTransform(FTransform());

	DepthCaptureComponent = NewObject<USceneCaptureComponent2D>(TempActor, TEXT("DepthSceneCapture"));
	DepthCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	DepthCaptureComponent->OrthoWidth = SceneCaptureSizeWS;
	DepthCaptureComponent->TextureTarget = DepthRT;
	DepthCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	DepthCaptureComponent->bCaptureEveryFrame = false;
	DepthCaptureComponent->bCaptureOnMovement = false;

	DeformationSceneViewExtension = FSceneViewExtensions::NewExtension<FDeformationSceneViewExtension>(World);

	if(!DeformationSceneViewExtension->DeformationPayload)
		DeformationSceneViewExtension->DeformationPayload = new FDeformationPayload;

	DeformationSceneViewExtension->DeformationPayload->InteractionTextureResolution = DepthRTResolution;
	DeformationSceneViewExtension->DeformationPayload->PostDelta = PostDeltaWS;
	DeformationSceneViewExtension->DeformationPayload->TemporalFilterFactor = 2.0;
	DeformationSceneViewExtension->DeformationPayload->SnowAddition = 0.0;
	DeformationSceneViewExtension->DeformationPayload->DepthRTSRV = DepthRT->GetRenderTargetResource()->GetRenderTargetTexture();
	DeformationSceneViewExtension->DeformationPayload->PersistentDepthSRV = PersistentDepth0->GetRenderTargetResource()->GetRenderTargetTexture();
	DeformationSceneViewExtension->DeformationPayload->PersistentDepthUAV = PersistentDepth1->GetRenderTargetResource()->GetRenderTargetUAV();
	DeformationSceneViewExtension->DeformationPayload->DeformUAV = DeformNormalHeightTexture->GetRenderTargetResource()->GetRenderTargetUAV();
}

void UDeformationSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (DeformationSceneViewExtension->DeformationPayload)
		delete DeformationSceneViewExtension->DeformationPayload;
}

UDeformationSubsystem::UDeformationSubsystem()
{
	struct FConstructorStatics
	{
	};
	static FConstructorStatics ConstructorStatics;

	SceneCaptureSizeWS = 4096;
	DepthRTResolution = 1024;
	PostDeltaWS = (float)SceneCaptureSizeWS / DepthRTResolution;
	SampleOffset = FVector2f::Zero();
}