// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/EngineTypes.h"
#include "DeformationSubsystem.generated.h"

class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UMaterialParameterCollection;
class UMaterialInterface;
class ACharacter;
class FDeformationSceneViewExtension;

struct FDeformationPayload 
{
	uint16 InteractionTextureResolution;
	FVector2f SampleOffset;
	FVector2f InteractionCenterWS;

	float PostDelta;
	float TemporalFilterFactor;
	float SnowAddition;
	float DeltaTime;
	float ExtrudedHeight;

	FTexture2DRHIRef DepthRT;
	FTexture2DRHIRef DeformNormalAndDepth;

	FRDGTextureRef PersistentDepth0;
	FRDGTextureRef PersistentDepth1;

	uint32 ExcuteCounter = 0;
};

UCLASS(BlueprintType, Transient)
class DEFORMATIONSYSTEM_API UDeformationSubsystem final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UDeformationSubsystem();

	// FTickableGameObject implementation Begin
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject implementation End

	// UWorldSubsystem implementation Begin
	/** Override to support water subsystems in editor preview worlds */
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	// UWorldSubsystem implementation End

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// USubsystem implementation End

public:
	UPROPERTY()
	USceneCaptureComponent2D* DepthCaptureComponent;
	UPROPERTY()
	UTextureRenderTarget2D* DepthRT;
	UPROPERTY()
	UTextureRenderTarget2D* DeformNormalDepthTexture;
	UPROPERTY()
	UMaterialParameterCollection* DeformationMPC;
	UPROPERTY()
	UMaterialInterface* SceneCaptureMaterial;

private:
	uint16 DepthRTResolution;
	uint16 SceneCaptureSizeWS;
	float PostDeltaWS;
	FVector2f SampleOffset;
	float ExtrudedHeight;

	ACharacter* Character;
	AActor* DeformationActor;

	TSharedPtr<FDeformationSceneViewExtension, ESPMode::ThreadSafe> DeformationSceneViewExtension;
	UWorld* CachedWorld;
};