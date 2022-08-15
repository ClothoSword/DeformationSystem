// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeformationSubsystem.h"
#include "DeformationSceneViewExtension.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UDeformationSubsystem::Tick(float DeltaTime)
{
	FVector CharacterLocation = FVector::ZeroVector;
	if (CachedWorld != GetWorld())
	{
		Character = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
		if (!Character || !Character->GetMesh())
			return;

		CachedWorld = GetWorld();
		Character->GetMesh()->bRenderCustomDepth = true;
		CharacterLocation = Character->GetActorLocation();
		DeformationActor = GetWorld()->SpawnActor<AActor>();
		DepthCaptureComponent = NewObject<USceneCaptureComponent2D>(DeformationActor, TEXT("DepthSceneCapture"));
		DeformationActor->SetActorTransform(FTransform());
		DeformationActor->SetRootComponent(DepthCaptureComponent);
		DeformationActor->SetActorLocation(FVector(CharacterLocation.X, CharacterLocation.Y, CharacterLocation.Z - 500));

		DeformationSceneViewExtension->CaptureOrigin = DeformationActor->GetActorLocation();

		DepthCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		DepthCaptureComponent->OrthoWidth = SceneCaptureSizeWS;
		DepthCaptureComponent->TextureTarget = DepthRT;
		DepthCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		DepthCaptureComponent->bCaptureEveryFrame = false;
		DepthCaptureComponent->bCaptureOnMovement = false;
		DepthCaptureComponent->bAlwaysPersistRenderingState = true;
		DepthCaptureComponent->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, 90.0f, 90.0f)));
		if (SceneCaptureMaterial)
		{
			DepthCaptureComponent->PostProcessSettings.WeightedBlendables.Array.SetNumZeroed(1);
			DepthCaptureComponent->PostProcessSettings.WeightedBlendables.Array[0].Weight = 1.0f;
			DepthCaptureComponent->PostProcessSettings.WeightedBlendables.Array[0].Object = SceneCaptureMaterial;
		}
		
		DepthCaptureComponent->RegisterComponent();

		DepthCaptureComponent->CaptureScene();

		return;
	}
	CharacterLocation = Character->GetActorLocation();
	FVector2f CurrentLocation = FVector2f(CharacterLocation.X, CharacterLocation.Y);
	SampleOffset = FVector2f(FMath::Floor(CurrentLocation.X / PostDeltaWS), FMath::Floor(CurrentLocation.Y / PostDeltaWS)) - CurrentLocation;
	SampleOffset = FVector2f::ZeroVector;

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

	DeformNormalHeightTexture = NewObject<UTextureRenderTarget2D>(this, TEXT("DeformNormalHeightTexture"));
	DeformNormalHeightTexture->RenderTargetFormat = RTF_RGB10A2;
	DeformNormalHeightTexture->bCanCreateUAV = true;
	DeformNormalHeightTexture->InitAutoFormat(DepthRTResolution, DepthRTResolution);
	DeformNormalHeightTexture->UpdateResourceImmediate(true);

	const FString DeformationSystem = TEXT("DeformationSystem");
	const FString PluginPath = FPaths::Combine(FPaths::ProjectPluginsDir(), DeformationSystem);

	SceneCaptureMaterial = LoadObject<UMaterial>(NULL, TEXT("/DeformationSystem/M_SceneCapturePP"), NULL, LOAD_None, NULL);
	DeformationMPC = LoadObject<UMaterialParameterCollection>(NULL, TEXT("/DeformationSystem/MPC_Deformation"), NULL, LOAD_None, NULL);

	DeformationSceneViewExtension = FSceneViewExtensions::NewExtension<FDeformationSceneViewExtension>(World);

	if(!DeformationSceneViewExtension->DeformationPayload)
		DeformationSceneViewExtension->DeformationPayload = new FDeformationPayload;

	ENQUEUE_RENDER_COMMAND(EndCaptureCommand)([&](FRHICommandListImmediate& RHICommandList)
	{
		DeformationSceneViewExtension->DeformationPayload->InteractionTextureResolution = DepthRTResolution;
		DeformationSceneViewExtension->DeformationPayload->PostDelta = PostDeltaWS;
		DeformationSceneViewExtension->DeformationPayload->TemporalFilterFactor = 2.0;
		DeformationSceneViewExtension->DeformationPayload->SnowAddition = 0.0;
		DeformationSceneViewExtension->DeformationPayload->DepthRT = DepthRT->GetRenderTargetResource()->GetRenderTargetTexture();
		DeformationSceneViewExtension->DeformationPayload->DeformNormalAndHeight = DeformNormalHeightTexture->GetRenderTargetResource()->GetRenderTargetTexture();
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
	struct FConstructorStatics
	{
	};
	static FConstructorStatics ConstructorStatics;

	SceneCaptureSizeWS = 4096;
	DepthRTResolution = 1024;
	PostDeltaWS = (float)SceneCaptureSizeWS / DepthRTResolution;
	SampleOffset = FVector2f::Zero();
	CachedWorld = nullptr;
}